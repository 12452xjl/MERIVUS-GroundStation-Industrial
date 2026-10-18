/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CompInfoParam.h"
#include "ComponentParameterMetadata.h"
#include "ComponentInformationManager.h"
#include "Fact.h"
#include "FactMetaData.h"
#include "ParameterManager.h"
#include "FirmwarePlugin.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QRegExp>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSet>
#include <QSettings>
#include <QThread>
#include <limits>

QGC_LOGGING_CATEGORY(CompInfoParamLog, "CompInfoParamLog")

const char* CompInfoParam::_cachedMetaDataFilePrefix    = "ParameterFactMetaData";
const char* CompInfoParam::_indexedNameTag              = "{n}";

struct CompInfoParamCatalog
{
    struct Indexed { QRegularExpression expression; QPointer<FactMetaData> metadata; };
    QPointer<QObject> owner = new QObject;
    QPointer<Vehicle> vehicle;
    QPointer<ComponentInformationManager> manager;
    QPointer<FirmwarePlugin> plugin;
    QPointer<VehicleLinkManager> links;
    QPointer<LinkInterface> primary;
    WeakLinkInterfacePtr primaryOwner;
    MAV_AUTOPILOT firmware = MAV_AUTOPILOT_GENERIC;
    MAV_TYPE vehicleType = MAV_TYPE_GENERIC;
    int vehicleId = 0;
    int defaultComponentId = 0;
    QString sourceEpoch;
    bool offline = false;
    bool jsonInstalled = false;
    bool opaqueAttempted = false;
    QPointer<QObject> opaque;
    QMap<QString, QPointer<FactMetaData>> direct;
    QVector<Indexed> indexed;
    QMap<QPair<QString, int>, QPointer<FactMetaData>> resolved;
    QSet<QPair<QString, int>> attempted;
    ~CompInfoParamCatalog() { delete owner.data(); }
};

namespace {
constexpr qint64 maximumJsonBytes = 32LL * 1024 * 1024;
// Bound positive AND negative lookups, including APM's allocating XML lookup.
constexpr int maximumResolvedNames = 65536;

template<class T> QPointer<T> ownedChild(QObject* owner, const QObject* address)
{
    if (!owner || !address) return {};
    for (QObject* child : owner->children()) {
        if (child == address) return qobject_cast<T*>(child);
    }
    return {};
}
}

CompInfoParam::CompInfoParam(uint8_t compId, Vehicle* vehicle, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_PARAMETER, compId, vehicle, parent)
    , _vehicle(vehicle)
{
    // VehicleLinkManager/version observations do not yet exist in Vehicle's
    // constructor. Bind a source only at a real load/lookup, never here.
}

CompInfoParam::~CompInfoParam()
{
    _destroying = true;
    _catalog.reset();
}

void CompInfoParam::resetMetadata()
{
    if (QThread::currentThread() != thread() || _destroying) return;
    // Detach before any QObject deletion; an in-flight call owns its local
    // shared catalogue until its virtual/factory stack has unwound.
    const auto retired = std::move(_catalog);
    if (_serial == std::numeric_limits<quint64>::max()) _serialExhausted = true;
    else ++_serial;
}

quint64 CompInfoParam::metadataRevisionMemory() const
{
    return QThread::currentThread() == thread() && !_destroying && !_serialExhausted ? _serial : 0;
}

bool CompInfoParam::_catalogCurrent(const std::shared_ptr<CompInfoParamCatalog>& catalog, quint64 serial) const
{
    if (QThread::currentThread() != thread()) return false;
    const auto original = catalog;
    const QPointer<CompInfoParam> self(const_cast<CompInfoParam*>(this));
    const auto memoryCurrent = [this, &original, serial]() {
        if (_destroying || _serialExhausted || _serial != serial || !original || _catalog != original
            || !original->owner || !_vehicle || original->vehicle != _vehicle || !original->manager
            || parent() != original->manager || _vehicle->compInfoManager() != original->manager
            || !original->plugin || _vehicle->firmwarePlugin() != original->plugin
            || !_vehicle->componentInformationCallbackAllowed()
            || _vehicle->firmwareType() != original->firmware || _vehicle->vehicleType() != original->vehicleType
            || _vehicle->id() != original->vehicleId || _vehicle->isOfflineEditingVehicle() != original->offline
            || _vehicle->autopilotVersionObservationSourceEpochMemory() != original->sourceEpoch) return false;
        if (original->offline) return true; // Explicitly local XML/generic editing, not wire provenance.
        if (!original->links || !original->primary || _vehicle->vehicleLinkManager() != original->links
            || _vehicle->defaultComponentId() != original->defaultComponentId) return false;
        const auto current = original->links->primaryLink();
        return !current.expired() && !current.owner_before(original->primaryOwner) && !original->primaryOwner.owner_before(current);
    };
    if (!memoryCurrent()) return false;
    if (original->offline) return true;
    auto primary = original->links->primaryLink().lock();
    const bool same = primary && primary.get() == original->primary.data();
    primary.reset();
    return self && same && memoryCurrent();
}

std::shared_ptr<CompInfoParamCatalog> CompInfoParam::_catalogForRequest()
{
    const QPointer<CompInfoParam> self(this);
    if (_catalog && _catalogCurrent(_catalog, _serial)) return _catalog;
    if (!self) return {};
    if (_catalog) {
        const quint64 before = _serial;
        resetMetadata();
        if (!self || _serialExhausted || before == std::numeric_limits<quint64>::max() || _serial != before + 1) return {};
    }
    if (!self || _destroying || _serialExhausted || !_vehicle || !_vehicle->componentInformationCallbackAllowed()) return {};
    const auto catalog = std::make_shared<CompInfoParamCatalog>();
    catalog->vehicle = _vehicle;
    catalog->manager = qobject_cast<ComponentInformationManager*>(parent());
    if (!catalog->manager || _vehicle->compInfoManager() != catalog->manager) return {};
    catalog->plugin = _vehicle->firmwarePlugin();
    catalog->firmware = _vehicle->firmwareType();
    catalog->vehicleType = _vehicle->vehicleType();
    catalog->vehicleId = _vehicle->id();
    catalog->defaultComponentId = _vehicle->defaultComponentId();
    catalog->sourceEpoch = _vehicle->autopilotVersionObservationSourceEpochMemory();
    catalog->offline = _vehicle->isOfflineEditingVehicle();
    const quint64 serial = _serial;
    if (!catalog->offline) {
        catalog->links = _vehicle->vehicleLinkManager();
        if (!catalog->links) return {};
        catalog->primaryOwner = catalog->links->primaryLink();
        auto primary = catalog->primaryOwner.lock();
        if (!primary) return {};
        catalog->primary = primary.get();
        _catalog = catalog;
        primary.reset();
    } else _catalog = catalog;
    return self && _catalogCurrent(catalog, serial) ? catalog : nullptr;
}

void CompInfoParam::setJson(const QString& fileName)
{
    if (QThread::currentThread() != thread() || _destroying) return;
    const QString path = fileName;
    const QPointer<CompInfoParam> self(this);
    if (_busy) { resetMetadata(); return; }
    _busy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_busy = false; });
    const quint64 beforeReset = _serial;
    resetMetadata(); // Empty, malformed and superseded loads all retire old JSON first.
    if (!self || _serialExhausted || beforeReset == std::numeric_limits<quint64>::max() || _serial != beforeReset + 1) return;
    const auto original = _catalogForRequest();
    if (!self || !original || path.isEmpty()) return;
    const quint64 serial = _serial;
    const QFileInfo before(path);
    if (!before.isAbsolute() || !before.isFile() || before.isSymLink() || before.size() < 1
        || before.size() > maximumJsonBytes || before.canonicalFilePath().isEmpty()) {
        qCWarning(CompInfoParamLog) << "Metadata json file rejected: compid:" << compId << "path-or-size-invalid";
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() != before.size()) {
        qCWarning(CompInfoParamLog) << "Metadata json file open failed: compid:" << compId;
        return;
    }
    QByteArray bytes;
    bytes.reserve(static_cast<int>(before.size()));
    while (!file.atEnd()) {
        const QByteArray chunk = file.read(64 * 1024);
        if (file.error() != QFile::NoError || chunk.isEmpty() || chunk.size() > maximumJsonBytes - bytes.size()) return;
        bytes.append(chunk);
        if (!self || !_catalogCurrent(original, serial)) return;
    }
    const bool readOk = file.error() == QFile::NoError && bytes.size() == before.size() && file.size() == before.size();
    file.close();
    const QFileInfo after(path);
    if (!readOk || file.error() != QFile::NoError || !after.isFile() || after.isSymLink()
        || after.size() != before.size() || after.lastModified() != before.lastModified()
        || after.canonicalFilePath() != before.canonicalFilePath()
        || !self || !_catalogCurrent(original, serial)) return;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    QVector<ComponentParameterMetadata::Definition> definitions;
    QString error;
    if (parseError.error != QJsonParseError::NoError || !ComponentParameterMetadata::parse(document, definitions, error)) {
        qCWarning(CompInfoParamLog) << "Metadata json validation failed: compid:" << compId << error;
        return;
    }
    // Independent complete candidate. No successful prefix touches the live maps.
    const auto candidate = std::make_shared<CompInfoParamCatalog>();
    candidate->vehicle = original->vehicle; candidate->manager = original->manager;
    candidate->plugin = original->plugin; candidate->links = original->links;
    candidate->primary = original->primary; candidate->primaryOwner = original->primaryOwner;
    candidate->firmware = original->firmware; candidate->vehicleType = original->vehicleType;
    candidate->vehicleId = original->vehicleId; candidate->defaultComponentId = original->defaultComponentId;
    candidate->sourceEpoch = original->sourceEpoch; candidate->offline = original->offline;
    for (const auto& definition : definitions) {
        if (!self || !_catalogCurrent(original, serial)) return;
        FactMetaData* returned = ComponentParameterMetadata::create(definition, candidate->owner);
        if (!self || !_catalogCurrent(original, serial) || !candidate->owner) return;
        const auto metadata = ownedChild<FactMetaData>(candidate->owner, returned);
        if (!metadata || metadata->name() != definition.name || metadata->type() != definition.type) return;
        const int tag = definition.name.indexOf(QLatin1String(_indexedNameTag));
        if (tag < 0) candidate->direct.insert(definition.name, metadata);
        else {
            const QString expression = QStringLiteral("\\A") + QRegularExpression::escape(definition.name.left(tag))
                + QStringLiteral("([0-9]+)") + QRegularExpression::escape(definition.name.mid(tag + 3)) + QStringLiteral("\\z");
            const QRegularExpression regex(expression);
            if (!regex.isValid()) return;
            candidate->indexed.append({regex, metadata});
        }
    }
    if (!self || !_catalogCurrent(original, serial)) return;
    candidate->jsonInstalled = true;
    _catalog = candidate;
}

bool CompInfoParam::_prepareOpaque(const std::shared_ptr<CompInfoParamCatalog>& catalog, quint64 serial)
{
    if (!_catalogCurrent(catalog, serial)) return false;
    if (catalog->opaqueAttempted) return catalog->opaque;
    catalog->opaqueAttempted = true;
    if (compId != MAV_COMP_ID_AUTOPILOT1) return false;
    const QPointer<CompInfoParam> self(this);
    const auto current = [self, catalog, serial]() { return self && self->_catalogCurrent(catalog, serial); };
    int major = -1, minor = -1;
    const QString path = _parameterMetaDataFile(catalog->vehicle, catalog->firmware, major, minor, current);
    if (!current() || path.isEmpty()) return false;
    // FirmwarePlugin's native factory contract transfers one valid parentless
    // object. A foreign/parented object is neither adopted nor deleted here.
    QObject* returned = catalog->plugin->_loadParameterMetaData(path);
    QPointer<QObject> opaque(returned);
    const bool ownsReturned = opaque && !opaque->parent() && opaque->thread() == QThread::currentThread();
    if (!current()) { if (ownsReturned) delete opaque.data(); return false; }
    if (!ownsReturned) return false;
    opaque->setParent(catalog->owner);
    if (!current() || !opaque || opaque->parent() != catalog->owner) return false;
    catalog->opaque = opaque;
    return true;
}

FactMetaData* CompInfoParam::_templateForName(const std::shared_ptr<CompInfoParamCatalog>& catalog, quint64 serial,
                                           const QString& name, FactMetaData::ValueType_t type)
{
    if (!_catalogCurrent(catalog, serial) || name.isEmpty() || name.size() > 16) return nullptr;
    const QPair<QString, int> key(name, static_cast<int>(type));
    if (catalog->attempted.contains(key)) return catalog->resolved.value(key).data();
    if (catalog->attempted.size() >= maximumResolvedNames) return nullptr;
    catalog->attempted.insert(key);
    const QPointer<CompInfoParam> self(this);
    QPointer<FactMetaData> source;
    QString index;
    if (catalog->jsonInstalled) {
        source = catalog->direct.value(name);
        if (!source) {
            for (const auto& entry : catalog->indexed) {
                const auto match = entry.expression.match(name);
                if (!match.hasMatch()) continue;
                if (source) return nullptr; // Ambiguity is unknown, never last-wins.
                source = entry.metadata; index = match.captured(1);
            }
        }
    } else {
        if (!_prepareOpaque(catalog, serial) || !self || !_catalogCurrent(catalog, serial)) return nullptr;
        const QPointer<QObject> opaque = catalog->opaque;
        FactMetaData* returned = catalog->plugin->_getMetaDataForFact(opaque, name, type, catalog->vehicleType);
        if (!self || !_catalogCurrent(catalog, serial) || !opaque || catalog->opaque != opaque) return nullptr;
        // Do not turn an external virtual's possibly stale raw return into a
        // QPointer. First locate it among this still-live opaque's real children.
        source = ownedChild<FactMetaData>(opaque, returned);
    }
    if (!source || source->type() != type || (index.isEmpty() && source->name() != name)) return nullptr;
    FactMetaData* returned = new FactMetaData(*source, catalog->owner);
    if (!self || !_catalogCurrent(catalog, serial)) return nullptr;
    const auto copy = ownedChild<FactMetaData>(catalog->owner, returned);
    if (!copy) return nullptr;
    copy->setName(name);
    if (!index.isEmpty()) {
        QString shortText = source->shortDescription(), longText = source->longDescription();
        shortText.replace(QLatin1String(_indexedNameTag), index);
        longText.replace(QLatin1String(_indexedNameTag), index);
        copy->setShortDescription(shortText); copy->setLongDescription(longText);
    }
    if (!self || !_catalogCurrent(catalog, serial) || !copy) return nullptr;
    catalog->resolved.insert(key, copy);
    return copy;
}

QPointer<FactMetaData> CompInfoParam::createMetaDataForFact(Fact* actualFact)
{
    if (QThread::currentThread() != thread() || _busy || _destroying || _serialExhausted || !actualFact) return nullptr;
    const QPointer<CompInfoParam> self(this);
    const QPointer<Fact> fact(actualFact);
    const QPointer<ParameterManager> manager(qobject_cast<ParameterManager*>(fact->parent()));
    if (!manager || manager->vehicle() != _vehicle || manager->parent() != _vehicle
        || fact->thread() != thread() || fact->componentId() != compId) return nullptr;
    const QString name = fact->name(); const auto valueType = fact->type();
    _busy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_busy = false; });
    const auto catalog = _catalogForRequest();
    if (!self || !catalog) return nullptr;
    const quint64 serial = _serial;
    const QPointer<FactMetaData> source(_templateForName(catalog, serial, name, valueType));
    const auto current = [self, fact, manager, catalog, serial, name, valueType]() {
        return self && fact && manager && self->_catalogCurrent(catalog, serial)
            && fact->parent() == manager && manager->vehicle() == catalog->vehicle && manager->parent() == catalog->vehicle
            && fact->componentId() == self->compId && fact->name() == name && fact->type() == valueType;
    };
    if (!current()) return nullptr;
    const QPointer<FactMetaData> copy(source ? new FactMetaData(*source) : new FactMetaData(valueType));
    if (!current()) { delete copy.data(); return nullptr; }
    if (!source) {
        copy->setName(name);
        const int separator = name.indexOf(QLatin1Char('_'));
        if (separator > 0) copy->setGroup(name.left(separator));
        if (compId != MAV_COMP_ID_AUTOPILOT1) copy->setCategory(tr("Component %1").arg(compId));
    }
    if (!current() || !copy) { delete copy.data(); return nullptr; }
    copy->setParent(fact);
    return current() && copy && copy->parent() == fact ? copy : QPointer<FactMetaData>();
}

bool CompInfoParam::parameterIsVolatile(const QString& name, FactMetaData::ValueType_t type, bool& value)
{
    if (QThread::currentThread() != thread() || _busy || _destroying || _serialExhausted) return false;
    const QString actualName = name;
    const QPointer<CompInfoParam> self(this);
    _busy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_busy = false; });
    const auto catalog = _catalogForRequest();
    if (!self || !catalog) return false;
    const quint64 serial = _serial;
    const QPointer<FactMetaData> metadata(_templateForName(catalog, serial, actualName, type));
    if (!self || !_catalogCurrent(catalog, serial) || !metadata) return false;
    value = metadata->volatileValue();
    return true;
}

FirmwarePlugin* CompInfoParam::_anyVehicleTypeFirmwarePlugin(MAV_AUTOPILOT firmwareType)
{
    if (!qgcApp() || !qgcApp()->toolbox()) return nullptr;
    const QPointer<FirmwarePluginManager> pluginMgr(qgcApp()->toolbox()->firmwarePluginManager());
    if (!pluginMgr) return nullptr;
    const auto supported = pluginMgr->supportedVehicleClasses(QGCMAVLink::firmwareClass(firmwareType));
    if (!pluginMgr || supported.isEmpty()) return nullptr;
    return pluginMgr->firmwarePluginForAutopilot(firmwareType, QGCMAVLink::vehicleClassToMavType(supported.first()));
}

QString CompInfoParam::_parameterMetaDataFile(Vehicle* vehicle, MAV_AUTOPILOT firmwareType, int& majorVersion, int& minorVersion,
                                               const std::function<bool()>& current)
{
    bool            cacheHit            = false;
    int             wantedMajorVersion  = 1;
    if (current && !current()) return {};
    const QPointer<Vehicle> actualVehicle(vehicle);
    const QPointer<FirmwarePlugin> fwPlugin(_anyVehicleTypeFirmwarePlugin(firmwareType));
    const auto valid = [&]() { return fwPlugin && (!vehicle || actualVehicle) && (!current || current()); };
    if (!valid()) return {};

    if (firmwareType != MAV_AUTOPILOT_PX4) {
        const QString file = fwPlugin->_internalParameterMetaDataFile(actualVehicle);
        return valid() ? file : QString();
    } else {
        // Only PX4 support the old style cached metadata
        QSettings   settings;
        QDir        cacheDir = QFileInfo(settings.fileName()).dir();

        // First look for a direct cache hit
        int cacheMinorVersion = -1, cacheMajorVersion = -1;
        QFile cacheFile(cacheDir.filePath(QString("%1.%2.%3.xml").arg(_cachedMetaDataFilePrefix).arg(firmwareType).arg(wantedMajorVersion)));
        if (cacheFile.exists()) {
            fwPlugin->_getParameterMetaDataVersionInfo(cacheFile.fileName(), cacheMajorVersion, cacheMinorVersion);
            if (!valid()) return {};
            if (wantedMajorVersion == cacheMajorVersion) {
                cacheHit = true;
            } else {
                qWarning() << "Parameter meta data cache corruption:" << cacheFile.fileName()
                           << "major version does not match file name" << "actual:expected" << cacheMajorVersion << wantedMajorVersion;
                if (!valid()) return {};
            }
        }

        if (!cacheHit) {
            // No direct hit, look for lower param set version
            QString wildcard = QString("%1.%2.*.xml").arg(_cachedMetaDataFilePrefix).arg(firmwareType);
            QStringList cacheHits = cacheDir.entryList(QStringList(wildcard), QDir::Files, QDir::Name);

            // Find the highest major version number which is below the vehicles major version number
            int cacheHitIndex = -1;
            cacheMajorVersion = -1;
            QRegExp regExp(QString("%1\\.%2\\.(\\d*)\\.xml").arg(_cachedMetaDataFilePrefix).arg(firmwareType));
            for (int i=0; i< cacheHits.count(); i++) {
                if (regExp.exactMatch(cacheHits[i]) && regExp.captureCount() == 1) {
                    int majorVersion = regExp.capturedTexts()[1].toInt();
                    if (majorVersion > cacheMajorVersion && majorVersion < wantedMajorVersion) {
                        cacheMajorVersion = majorVersion;
                        cacheHitIndex = i;
                    }
                }
            }

            if (cacheHitIndex != -1) {
                // We have a cache hit on a lower major version, read minor version as well
                int majorVersion = -1;
                cacheFile.setFileName(cacheDir.filePath(cacheHits[cacheHitIndex]));
                fwPlugin->_getParameterMetaDataVersionInfo(cacheFile.fileName(), majorVersion, cacheMinorVersion);
                if (!valid()) return {};
                if (majorVersion != cacheMajorVersion) {
                    qWarning() << "Parameter meta data cache corruption:" << cacheFile.fileName()
                               << "major version does not match file name" << "actual:expected" << majorVersion << cacheMajorVersion;
                    if (!valid()) return {};
                    cacheHit = false;
                } else {
                    cacheHit = true;
                }
            }
        }

        int internalMinorVersion = -1, internalMajorVersion = -1;
        QString internalMetaDataFile = fwPlugin->_internalParameterMetaDataFile(actualVehicle);
        if (!valid()) return {};
        fwPlugin->_getParameterMetaDataVersionInfo(internalMetaDataFile, internalMajorVersion, internalMinorVersion);
        if (!valid()) return {};
        if (cacheHit) {
            // Cache hit is available, we need to check if internal meta data is a better match, if so use internal version
            if (internalMajorVersion == wantedMajorVersion) {
                if (cacheMajorVersion == wantedMajorVersion) {
                    // Both internal and cache are direct hit on major version, Use higher minor version number
                    cacheHit = cacheMinorVersion > internalMinorVersion;
                } else {
                    // Direct internal hit, but not direct hit in cache, use internal
                    cacheHit = false;
                }
            } else {
                if (cacheMajorVersion == wantedMajorVersion) {
                    // Direct hit on cache, no direct hit on internal, use cache
                    cacheHit = true;
                } else {
                    // No direct hit anywhere, use internal
                    cacheHit = false;
                }
            }
        }

        QString metaDataFile;
        if (cacheHit && !qgcApp()->runningUnitTests()) {
            majorVersion = cacheMajorVersion;
            minorVersion = cacheMinorVersion;
            metaDataFile = cacheFile.fileName();
        } else {
            majorVersion = internalMajorVersion;
            minorVersion = internalMinorVersion;
            metaDataFile = internalMetaDataFile;
        }
        return metaDataFile;
    }
}
void CompInfoParam::_cachePX4MetaDataFile(const QString& metaDataFile)
{
    const QString path = metaDataFile;
    const QPointer<FirmwarePlugin> plugin(_anyVehicleTypeFirmwarePlugin(MAV_AUTOPILOT_PX4));
    if (!plugin) return;

    int newMajorVersion = -1, newMinorVersion = -1;
    plugin->_getParameterMetaDataVersionInfo(path, newMajorVersion, newMinorVersion);
    if (!plugin) return;
    if (newMajorVersion != 1) {
        newMajorVersion = 1;
        qgcApp()->showAppMessage(tr("Internal Error: Parameter MetaData major must be 1"));
    }
    if (!plugin) return;

    // Find the cache hit closest to this new file
    int cacheMajorVersion = -1, cacheMinorVersion = -1;
    QString cacheHit = _parameterMetaDataFile(nullptr, MAV_AUTOPILOT_PX4, cacheMajorVersion, cacheMinorVersion);
    if (!plugin) return;

    bool cacheNewFile = false;
    if (cacheHit.isEmpty()) {
        // No cache hits, store the new file
        cacheNewFile = true;
    } else if (cacheMajorVersion == newMajorVersion) {
        // Direct hit on major version in cache:
        //      Cache new file if newer/equal minor version. We cache if equal to allow flashing test builds with new parameter metadata.
        //      Also delete older cache file.
        if (newMinorVersion >= cacheMinorVersion) {
            cacheNewFile = true;
            QFile::remove(cacheHit);
        }
    } else {
        // Indirect hit in cache, store new file
        cacheNewFile = true;
    }

    if (cacheNewFile) {
        // Cached files are stored in settings location. Copy from current file to cache naming.

        QSettings settings;
        QDir cacheDir = QFileInfo(settings.fileName()).dir();
        QFile cacheFile(cacheDir.filePath(QString("%1.%2.%3.xml").arg(_cachedMetaDataFilePrefix).arg(MAV_AUTOPILOT_PX4).arg(newMajorVersion)));
        qCDebug(CompInfoParamLog) << "ParameterManager::cacheMetaDataFile caching file:" << cacheFile.fileName();
        QFile newFile(path);
        newFile.copy(cacheFile.fileName());
    }
}
