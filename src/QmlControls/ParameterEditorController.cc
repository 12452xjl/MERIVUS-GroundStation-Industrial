/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ParameterEditorController.h"
#include "QGCApplication.h"
#include "ParameterManager.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "MultiVehicleManager.h"
#include "VehicleLinkManager.h"
#include "OfflineParameterFile.h"
#include "ComponentInformationManager.h"
#include "CompInfoParam.h"

#include <QStandardPaths>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
#include <QThread>
#include <QUuid>
#include <atomic>
#include <cstring>
#include <functional>
#include <utility>

namespace {
bool exactValue(const QVariant& a, const QVariant& b)
{
    if (a.userType() != b.userType() || a.isNull() != b.isNull()) return false;
    if (a.userType() == QMetaType::Float) {
        const float x = a.value<float>(), y = b.value<float>();
        return std::memcmp(&x, &y, sizeof(x)) == 0;
    }
    if (a.userType() == QMetaType::Double) {
        const double x = a.toDouble(), y = b.toDouble();
        return std::memcmp(&x, &y, sizeof(x)) == 0;
    }
    if (a.type() == QVariant::List) {
        const auto x = a.toList(), y = b.toList();
        if (x.size() != y.size()) return false;
        for (int i = 0; i < x.size(); ++i) if (!exactValue(x.at(i), y.at(i))) return false;
        return true;
    }
    if (a.type() == QVariant::Map) {
        const auto x = a.toMap(), y = b.toMap();
        if (x.keys() != y.keys()) return false;
        for (auto it = x.constBegin(); it != x.constEnd(); ++it) if (!exactValue(it.value(), y.value(it.key()))) return false;
        return true;
    }
    return a == b;
}

QVariantMap metadataValues(const QPointer<FactMetaData>& metadata)
{
    if (!metadata) return {};
    return {{QStringLiteral("name"), metadata->name()}, {QStringLiteral("type"), int(metadata->type())},
        {QStringLiteral("min"), metadata->rawMin()}, {QStringLiteral("max"), metadata->rawMax()},
        {QStringLiteral("readOnly"), metadata->readOnly()}, {QStringLiteral("writeOnly"), metadata->writeOnly()},
        {QStringLiteral("control"), metadata->hasControl()}, {QStringLiteral("volatile"), metadata->volatileValue()},
        {QStringLiteral("reboot"), metadata->vehicleRebootRequired()}, {QStringLiteral("qgcReboot"), metadata->qgcRebootRequired()},
        {QStringLiteral("units"), metadata->rawUnits()}, {QStringLiteral("cookedUnits"), metadata->cookedUnits()},
        {QStringLiteral("decimals"), metadata->decimalPlaces()}, {QStringLiteral("increment"), metadata->rawIncrement()},
        {QStringLiteral("enumStrings"), metadata->enumStrings()}, {QStringLiteral("enumValues"), metadata->enumValues()},
        {QStringLiteral("bitStrings"), metadata->bitmaskStrings()}, {QStringLiteral("bitValues"), metadata->bitmaskValues()}};
}

QPointer<FactMetaData> actualMetadata(const QPointer<Fact>& fact)
{
    if (!fact) return {};
    const QObject* address = fact->metaData();
    for (QObject* child : fact->children()) if (child == address) return qobject_cast<FactMetaData*>(child);
    return {};
}

QVector<int> versionNumbers(const QPointer<Vehicle>& vehicle)
{
    if (!vehicle) return {};
    return {vehicle->firmwareMajorVersion(), vehicle->firmwareMinorVersion(), vehicle->firmwarePatchVersion(),
        vehicle->firmwareVersionType(), vehicle->firmwareCustomMajorVersion(), vehicle->firmwareCustomMinorVersion(),
        vehicle->firmwareCustomPatchVersion()};
}

bool readImportBytes(const QString& path, const std::function<bool()>& current, QByteArray& bytes)
{
    const QFileInfo before(path);
    if (!current() || !before.isFile() || before.isSymLink() || before.size() < 1
        || before.size() > OfflineParameterFile::MaximumBytes) return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || !current()) return false;
    QByteArray candidate;
    while (!file.atEnd()) {
        if (!current()) return false;
        const QByteArray block = file.read(64 * 1024);
        if (block.isEmpty() || file.error() != QFileDevice::NoError
            || candidate.size() + block.size() > OfflineParameterFile::MaximumBytes) return false;
        candidate.append(block);
    }
    const QFileInfo after(path);
    const bool complete = current() && file.pos() == before.size() && candidate.size() == before.size()
        && file.error() == QFileDevice::NoError && after.isFile() && !after.isSymLink()
        && after.size() == before.size() && after.canonicalFilePath() == before.canonicalFilePath()
        && after.lastModified() == before.lastModified();
    file.close();
    if (!complete || file.isOpen() || file.error() != QFileDevice::NoError || !current()) return false;
    bytes = candidate;
    return true;
}
}

struct ParameterEditorController::FilePreview {
    struct Entry {
        OfflineParameterFile::Row row;
        QPointer<Fact> fact;
        QPointer<FactMetaData> metadata;
        QVariant baseline;
        QVariantMap metadataBaseline;
        FactMetaData::Translator rawTranslator = nullptr;
        FactMetaData::Translator cookedTranslator = nullptr;
        QPointer<ParameterEditorDiff> display;
        bool missing = true;
    };
    QString token;
    QPointer<LinkInterface> primary;
    WeakLinkInterfacePtr primaryOwner;
    std::function<bool()> sourceCurrent;
    std::shared_ptr<std::atomic_bool> changed = std::make_shared<std::atomic_bool>(false);
    QList<QMetaObject::Connection> connections;
    QMap<int, QPair<QPointer<CompInfoParam>, quint64>> providers;
    QVector<Entry> entries;
    bool retired = false;
    bool consumed = false;
    ~FilePreview() { for (const auto& connection : connections) QObject::disconnect(connection); }
};

std::shared_ptr<ParameterEditorController::FilePreview> ParameterEditorController::_captureFilePreview()
{
    const QPointer<ParameterEditorController> self(this);
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return {};
    const QPointer<MultiVehicleManager> fleet(qgcApp()->toolbox()->multiVehicleManager());
    if (!fleet) return {};
    const QPointer<Vehicle> vehicle(fleet->activeVehicle() ? fleet->activeVehicle() : fleet->offlineEditingVehicle());
    if (!vehicle || vehicle.data() != _vehicle) return {};
    const QPointer<ParameterManager> manager(_parameterMgr);
    if (!manager || manager->parent() != vehicle || vehicle->parameterManager() != manager) return {};
    QPointer<VehicleLinkManager> links;
    const QObject* linksAddress = vehicle->vehicleLinkManager();
    for (QObject* child : vehicle->children()) if (child == linksAddress) links = qobject_cast<VehicleLinkManager*>(child);
    const bool offline = vehicle->isOfflineEditingVehicle();
    if (!offline && !links) return {};
    const WeakLinkInterfacePtr primaryOwner = links ? links->primaryLink() : WeakLinkInterfacePtr();
    auto primaryLease = primaryOwner.lock();
    const QPointer<LinkInterface> primary(primaryLease.get());
    primaryLease.reset();
    if (!self || !vehicle || !manager || !fleet) return {};
    if (!offline && !primary) return {};
    const int id = vehicle->id(), component = vehicle->defaultComponentId();
    const auto firmware = vehicle->firmwareType();
    const auto type = vehicle->vehicleType();
    const void* plugin = vehicle->firmwarePlugin(); // Address comparison only; never dereferenced here.
    QPointer<ComponentInformationManager> metadataManager;
    const QObject* metadataManagerAddress = vehicle->compInfoManager();
    for (QObject* child : vehicle->children()) if (child == metadataManagerAddress) metadataManager = qobject_cast<ComponentInformationManager*>(child);
    if (!metadataManager) return {};
    const QString epoch = vehicle->autopilotVersionObservationSourceEpochMemory();
    const QString managerEpoch = manager->_parameterObservationEpoch;
    const auto versions = versionNumbers(vehicle);
    const auto preview = std::make_shared<FilePreview>();
    preview->primary = primary;
    preview->primaryOwner = primaryOwner;
    const auto changed = preview->changed;
    preview->sourceCurrent = [self, fleet, vehicle, manager, metadataManager, links, primary, primaryOwner,
                              offline, id, component, firmware, type, plugin, epoch, managerEpoch, versions, changed]() {
        if (!self || !fleet || !vehicle || !manager || !metadataManager || changed->load(std::memory_order_relaxed)
            || self->_vehicle != vehicle || self->_parameterMgr != manager || manager->parent() != vehicle
            || manager->_vehicle != vehicle || manager->_parameterObservationEpoch != managerEpoch
            || vehicle->parameterManager() != manager || vehicle->thread() != self->thread() || manager->thread() != self->thread()
            || metadataManager->parent() != vehicle || metadataManager->thread() != self->thread() || vehicle->compInfoManager() != metadataManager
            || (fleet->activeVehicle() ? fleet->activeVehicle() : fleet->offlineEditingVehicle()) != vehicle
            || vehicle->id() != id || vehicle->defaultComponentId() != component || vehicle->firmwareType() != firmware
            || vehicle->vehicleType() != type || vehicle->firmwarePlugin() != plugin || vehicle->isOfflineEditingVehicle() != offline
            || vehicle->autopilotVersionObservationSourceEpochMemory() != epoch || versionNumbers(vehicle) != versions
            || !vehicle->componentInformationCallbackAllowed()) return false;
        if (offline) return true;
        if (!links || !primary || vehicle->vehicleLinkManager() != links || links->thread() != self->thread()
            || links->communicationLost()) return false;
        // UDP/Mock links legitimately live on their own worker thread. Keep
        // the exact QObject and weak ownership, not an incorrect GUI-affinity requirement.
        const auto now = links->primaryLink();
        return !now.expired() && !now.owner_before(primaryOwner) && !primaryOwner.owner_before(now);
    };
    const auto watch = [&](const QMetaObject::Connection& connection) {
        preview->connections.append(connection);
        return connection && self && preview->sourceCurrent();
    };
    const auto retire = [changed]() { changed->store(true, std::memory_order_relaxed); };
    if (!preview->sourceCurrent()
        || !watch(connect(fleet.data(), &MultiVehicleManager::activeVehicleChanged, self.data(), retire, Qt::DirectConnection))
        || !watch(connect(vehicle.data(), &Vehicle::firmwareVersionChanged, self.data(), retire, Qt::DirectConnection))
        || !watch(connect(vehicle.data(), &Vehicle::firmwareCustomVersionChanged, self.data(), retire, Qt::DirectConnection))) return {};
    if (links && (!watch(connect(links.data(), &VehicleLinkManager::primaryLinkChanged, self.data(), retire, Qt::DirectConnection))
        || !watch(connect(links.data(), &VehicleLinkManager::communicationLostChanged, self.data(), retire, Qt::DirectConnection)))) return {};
    return self && preview->sourceCurrent() ? preview : std::shared_ptr<FilePreview>();
}

bool ParameterEditorController::_filePreviewCurrent(const std::shared_ptr<FilePreview>& preview) const
{
    if (!preview || preview->retired || !preview->sourceCurrent || !preview->sourceCurrent()) return false;
    for (auto it = preview->providers.constBegin(); it != preview->providers.constEnd(); ++it) {
        const auto provider = it.value().first;
        if (!provider || provider->thread() != thread() || provider->vehicle != _vehicle || provider->compId != it.key()
            || it.value().second == 0
            || provider->parent() != _vehicle->compInfoManager() || provider->metadataRevisionMemory() != it.value().second) return false;
    }
    return true;
}

bool ParameterEditorController::_diffFailure(const QString& error)
{
    _diffError = error;
    emit diffErrorChanged(); // No access after a potentially destructive notification.
    return false;
}

bool ParameterEditorController::_clearDiffPrivate()
{
    const QPointer<ParameterEditorController> self(this);
    if (_filePreview) _filePreview->retired = true;
    const auto old = std::move(_filePreview);
    _diffToken.clear(); _diffVehicleId = -1; _diffOtherVehicle = false; _diffMultipleComponents = false;
    _diffList.clearAndDeleteContents();
    if (!self) return false;
    emit diffTokenChanged();
    if (!self) return false;
    emit diffOtherVehicleChanged(false);
    if (!self) return false;
    emit diffMultipleComponentsChanged(false);
    return bool(self);
}

ParameterEditorController::ParameterEditorController(void)
    : _parameterMgr(_vehicle->parameterManager())
{
    _buildLists();

    connect(this, &ParameterEditorController::currentCategoryChanged,   this, &ParameterEditorController::_currentCategoryChanged);
    connect(this, &ParameterEditorController::currentGroupChanged,      this, &ParameterEditorController::_currentGroupChanged);
    connect(this, &ParameterEditorController::searchTextChanged,        this, &ParameterEditorController::_searchTextChanged);
    connect(this, &ParameterEditorController::showModifiedOnlyChanged,  this, &ParameterEditorController::_searchTextChanged);

    connect(_parameterMgr, &ParameterManager::factAdded, this, &ParameterEditorController::_factAdded);
    connect(_parameterMgr, &ParameterManager::parameterImportChanged, this, &ParameterEditorController::parameterImportChanged);

    ParameterEditorCategory* category = _categories.count() ? _categories.value<ParameterEditorCategory*>(0) : nullptr;
    setCurrentCategory(category);
}

ParameterEditorController::~ParameterEditorController()
{

}

void ParameterEditorController::_buildListsForComponent(int compId)
{
    for (const QString& factName: _parameterMgr->parameterNames(compId)) {
        Fact* fact = _parameterMgr->getParameter(compId, factName);

        ParameterEditorCategory* category = nullptr;
        if (_mapCategoryName2Category.contains(fact->category())) {
            category = _mapCategoryName2Category[fact->category()];
        } else {
            category        = new ParameterEditorCategory(this);
            category->name  = fact->category();
            _mapCategoryName2Category[fact->category()] = category;
            _categories.append(category);
        }

        ParameterEditorGroup* group = nullptr;
        if (category->mapGroupName2Group.contains(fact->group())) {
            group = category->mapGroupName2Group[fact->group()];
        } else {
            group               = new ParameterEditorGroup(this);
            group->componentId  = compId;
            group->name         = fact->group();
            category->mapGroupName2Group[fact->group()] = group;
            category->groups.append(group);
        }

        group->facts.append(fact);
    }
}

void ParameterEditorController::_buildLists(void)
{
    // Autopilot component should always be first list
    _buildListsForComponent(MAV_COMP_ID_AUTOPILOT1);

    // "Standard" category should always be first
    for (int i=0; i<_categories.count(); i++) {
        ParameterEditorCategory* category = _categories.value<ParameterEditorCategory*>(i);
        if (category->name == "Standard" && i != 0) {
            _categories.removeAt(i);
            _categories.insert(0, category);
            break;
        }
    }

    // Default category should always be last
    for (int i=0; i<_categories.count(); i++) {
        ParameterEditorCategory* category = _categories.value<ParameterEditorCategory*>(i);
        if (category->name == FactMetaData::kDefaultCategory) {
            if (i != _categories.count() - 1) {
                _categories.removeAt(i);
                _categories.append(category);
            }
            break;
        }
    }

    // Now add other random components
    for (int compId: _parameterMgr->componentIds()) {
        if (compId != MAV_COMP_ID_AUTOPILOT1) {
            _buildListsForComponent(compId);
        }
    }

    // Default group should always be last
    for (int i=0; i<_categories.count(); i++) {
        ParameterEditorCategory* category = _categories.value<ParameterEditorCategory*>(i);
        for (int j=0; j<category->groups.count(); j++) {
            ParameterEditorGroup* group = category->groups.value<ParameterEditorGroup*>(j);
            if (group->name == FactMetaData::kDefaultGroup) {
                if (j != _categories.count() - 1) {
                    category->groups.removeAt(j);
                    category->groups.append(category);
                }
                break;
            }
        }
    }
}

void ParameterEditorController::_factAdded(int compId, Fact* fact)
{
    bool                        inserted = false;
    ParameterEditorCategory*    category = nullptr;

    if (_mapCategoryName2Category.contains(fact->category())) {
        category = _mapCategoryName2Category[fact->category()];
    } else {
        category        = new ParameterEditorCategory(this);
        category->name  = fact->category();
        _mapCategoryName2Category[fact->category()] = category;

        // Insert in sorted order
        inserted = false;
        for (int i=0; i<_categories.count(); i++) {
            if (_categories.value<ParameterEditorCategory*>(i)->name > category->name) {
                _categories.insert(i, category);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            _categories.append(category);
        }
    }

    ParameterEditorGroup* group = nullptr;
    if (category->mapGroupName2Group.contains(fact->group())) {
        group = category->mapGroupName2Group[fact->group()];
    } else {
        group               = new ParameterEditorGroup(this);
        group->componentId  = compId;
        group->name         = fact->group();
        category->mapGroupName2Group[fact->group()] = group;

        // Insert in sorted order
        QmlObjectListModel& groups = category->groups;
        inserted = false;
        for (int i=0; i<groups.count(); i++) {
            if (groups.value<ParameterEditorGroup*>(i)->name > group->name) {
                groups.insert(i, group);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            groups.append(group);
        }
    }

    // Insert in sorted order
    QmlObjectListModel& facts = group->facts;
    inserted = false;
    for (int i=0; i<facts.count(); i++) {
        if (facts.value<Fact*>(i)->name() > fact->name()) {
            facts.insert(i, fact);
            inserted = true;
            break;
        }
    }
    if (!inserted) {
        facts.append(fact);
    }
}

QStringList ParameterEditorController::searchParameters(const QString& searchText, bool searchInName, bool searchInDescriptions)
{
    QStringList list;

    for(const QString &paramName: _parameterMgr->parameterNames(_vehicle->defaultComponentId())) {
        if (searchText.isEmpty()) {
            list += paramName;
        } else {
            Fact* fact = _parameterMgr->getParameter(_vehicle->defaultComponentId(), paramName);

            if (searchInName && fact->name().contains(searchText, Qt::CaseInsensitive)) {
                list += paramName;
            } else if (searchInDescriptions && (fact->shortDescription().contains(searchText, Qt::CaseInsensitive) || fact->longDescription().contains(searchText, Qt::CaseInsensitive))) {
                list += paramName;
            }
        }
    }
    list.sort();

    return list;
}

void ParameterEditorController::saveToFile(const QString& filename)
{
    if (!filename.isEmpty()) {
        QString parameterFilename = filename;
        if (!QFileInfo(filename).fileName().contains(".")) {
            parameterFilename += QString(".%1").arg(AppSettings::parameterFileExtension);
        }

        QFile file(parameterFilename);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qgcApp()->showAppMessage(tr("Unable to create file: %1").arg(parameterFilename));
            return;
        }

        QTextStream stream(&file);
        _parameterMgr->writeParametersToStream(stream);
        file.close();
    }
}

void ParameterEditorController::clearDiff(const QString& expectedToken)
{
    if (QThread::currentThread() != thread() || expectedToken.isEmpty() || expectedToken != _diffToken) return;
    const QPointer<ParameterEditorController> self(this);
    const bool alreadyBusy = _diffBusy;
    _diffBusy = true;
    const auto release = qScopeGuard([self, alreadyBusy]() { if (self) self->_diffBusy = alreadyBusy; });
    // Exact retirement remains allowed during our own callbacks. It cannot
    // grant a nested build/apply or clear a later dialog's different token.
    _clearDiffPrivate();
}

bool ParameterEditorController::sendDiff(const QString& expectedToken)
{
    if (QThread::currentThread() != thread() || _diffBusy) return false;
    if (expectedToken.isEmpty() || expectedToken != _diffToken) return false; // Do not change a later dialog's diagnostic.
    const QPointer<ParameterEditorController> self(this);
    const auto preview = _filePreview;
    const QPointer<ParameterManager> manager(_parameterMgr);
    _diffBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_diffBusy = false; });
    const auto fail = [self](const QString& reason) {
        if (!self) return false;
        return self->_diffFailure(reason);
    };
    const auto current = [self, preview, manager, expectedToken]() {
        return self && manager && !expectedToken.isEmpty() && self->_filePreview == preview
            && self->_diffToken == expectedToken && preview && preview->token == expectedToken
            && self->_parameterMgr == manager && self->_filePreviewCurrent(preview);
    };
    if (!current() || preview->consumed || _diffOtherVehicle || _diffVehicleId != _vehicle->id())
        return fail(tr("This parameter preview is no longer applicable. Reload the file."));
    if (!writeAllowed() || !current()) return fail(tr("Parameter writing is currently blocked."));

    QVector<int> selected;
    if (_diffList.count() != preview->entries.size()) return fail(tr("The preview rows changed."));
    for (int i = 0; i < preview->entries.size(); ++i) {
        const auto& entry = preview->entries.at(i);
        if (!entry.display || _diffList.get(i) != entry.display) return fail(tr("The preview rows changed."));
        if (entry.display->load) selected.append(i); // Only this selection flag is read from public rows.
    }
    const auto rowCurrent = [manager](const FilePreview::Entry& entry, const QVariant& value) {
        if (!manager) return false;
        const Fact* mapped = manager->_mapCompId2FactMap.value(entry.row.componentId).value(entry.row.name, nullptr);
        if (entry.missing) return !mapped; // A previously missing parameter must still be missing before sending it.
        return entry.fact && mapped == entry.fact.data() && entry.fact->parent() == manager && entry.fact->thread() == manager->thread()
            && entry.fact->componentId() == entry.row.componentId && entry.fact->name() == entry.row.name
            && entry.fact->type() == entry.row.factType && entry.metadata && actualMetadata(entry.fact) == entry.metadata
            && entry.metadata->thread() == manager->thread() && exactValue(entry.fact->rawValue(), value)
            && exactValue(metadataValues(entry.metadata), entry.metadataBaseline)
            && entry.metadata->rawTranslator() == entry.rawTranslator && entry.metadata->cookedTranslator() == entry.cookedTranslator;
    };
    // No row can be sent until all selected native candidates pass together.
    for (int index : selected) {
        const auto& entry = preview->entries.at(index);
        if (entry.row.mavType == MAV_PARAM_TYPE_UINT64 || entry.row.mavType == MAV_PARAM_TYPE_INT64
            || entry.row.mavType == MAV_PARAM_TYPE_REAL64)
            return fail(tr("A selected row uses a numeric type unsupported by the current PARAM_SET sender."));
        if (!current() || !rowCurrent(entry, entry.baseline)) return fail(tr("A selected parameter or its metadata changed. Reload the file."));
        if (entry.metadata && entry.metadata->readOnly()) return fail(tr("A selected parameter is read-only."));
    }
    if (!current() || !writeAllowed() || !current()) return fail(tr("The parameter source changed before applying the selection."));
    const auto admissionFence = [self, current, rowCurrent, preview, selected]() {
        if (!current() || !self->writeAllowed() || !current()) return false;
        for (int index : selected) {
            const auto& entry = preview->entries.at(index);
            if (!rowCurrent(entry, entry.baseline)) return false;
        }
        return current();
    };
    if (!admissionFence()) return fail(tr("The original preview changed before transaction admission."));
    if (selected.isEmpty()) {
        preview->consumed = true; // No transaction, no write, and no remote acknowledgement.
        _diffError.clear();
        emit diffErrorChanged();
        return bool(self) && admissionFence();
    }
    QVector<OfflineParameterFile::Row> rows;
    rows.reserve(selected.size());
    for (int index : selected) rows.append(preview->entries.at(index).row);
    // PM captures its own native source and values while this exact preview
    // remains applicable. An accepted run outlives normal dialog destruction;
    // clearDiff must not cancel it or change already-entered write facts.
    const QVariantMap receipt = manager->beginParameterImport(rows, preview->primary.data(), preview->primaryOwner, admissionFence);
    if (!self) return false;
    if (!receipt.value(QStringLiteral("accepted")).toBool())
        return fail(tr("The import transaction was not accepted: %1").arg(receipt.value(QStringLiteral("reason")).toString()));
    preview->consumed = true;
    if (_filePreview == preview && _diffToken == expectedToken) {
        _diffError.clear();
        emit diffErrorChanged();
    }
    return bool(self); // Admission only; native state records writes, matching reports and terminal failures.
}

QVariantMap ParameterEditorController::parameterImportState(void) const
{
    const QPointer<ParameterManager> manager(_parameterMgr);
    if (QThread::currentThread() != thread() || !manager || manager->thread() != thread()
        || manager->parent() != _vehicle) return {};
    return manager->parameterImportState();
}

bool ParameterEditorController::cancelParameterImport(const QString& runId)
{
    const QPointer<ParameterManager> manager(_parameterMgr);
    if (QThread::currentThread() != thread() || runId.isEmpty() || !manager
        || manager->thread() != thread() || manager->parent() != _vehicle) return false;
    // Stopping this original run is permitted after loss of the write gate.
    // The native manager checks the exact id; no switch to the active vehicle.
    return manager->cancelParameterImport(runId);
}

bool ParameterEditorController::buildDiffFromFile(const QString& filename)
{
    if (QThread::currentThread() != thread() || _diffBusy) return false;
    const QString path = filename; // The caller's property may change during model notifications.
    const QPointer<ParameterEditorController> self(this);
    _diffBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_diffBusy = false; });
    // Freeze the actual source BEFORE clearing a previous model can notify.
    const auto preview = _captureFilePreview();
    if (!self) return false;
    if (!_clearDiffPrivate() || !self) return false;
    const auto current = [self, preview]() { return self && self->_filePreviewCurrent(preview); };
    const auto fail = [self](const QString& error) { return self ? self->_diffFailure(error) : false; };
    if (!current()) return fail(tr("The parameter source is unavailable or changed while loading."));
    QByteArray bytes;
    OfflineParameterFile::Document document;
    QString error;
    if (!readImportBytes(path, current, bytes)) return fail(tr("Unable to read a complete, stable parameter file (maximum 4 MiB)."));
    if (!OfflineParameterFile::decode(bytes, document, error)) return fail(tr("Parameter file rejected: %1").arg(error));
    if (!current()) return fail(tr("The parameter source changed while parsing."));
    const QPointer<ParameterManager> manager(_parameterMgr);
    const bool otherVehicle = document.sourceSystemId != _vehicle->id();
    int firstComponent = -1;
    bool multipleComponents = false;
    QList<QPointer<ParameterEditorDiff>> pendingRows;
    bool published = false;
    const auto cleanup = qScopeGuard([&pendingRows, &published]() {
        if (!published) for (const auto& row : pendingRows) if (row) row->deleteLater();
    });
    QHash<const Fact*, QPointer<Fact>> actualFacts;
    for (QObject* child : manager->children()) if (auto* fact = qobject_cast<Fact*>(child)) actualFacts.insert(fact, fact);
    QPointer<ComponentInformationManager> metadataManager;
    const QObject* managerAddress = _vehicle->compInfoManager();
    for (QObject* child : _vehicle->children()) if (child == managerAddress) metadataManager = qobject_cast<ComponentInformationManager*>(child);
    for (const auto& row : document.rows) {
        if (!current()) return fail(tr("The parameter source or metadata directory changed."));
        if (firstComponent == -1) firstComponent = row.componentId;
        else if (firstComponent != row.componentId) multipleComponents = true;
        FilePreview::Entry entry;
        entry.row = row;
        const Fact* mapped = manager->_mapCompId2FactMap.value(row.componentId).value(row.name, nullptr);
        entry.missing = !mapped;
        entry.fact = actualFacts.value(mapped);
        if (mapped && !entry.fact) return fail(tr("A parameter Fact is no longer owned by this manager."));
        QString fileText = row.value.toString(), vehicleText, units;
        if (entry.fact) {
            if (!preview->providers.contains(row.componentId)) {
                if (!metadataManager) return fail(tr("The parameter metadata directory is unavailable."));
                const QObject* address = metadataManager->compInfoParam(static_cast<uint8_t>(row.componentId));
                if (!current() || !metadataManager) return fail(tr("The parameter metadata source changed."));
                QPointer<CompInfoParam> provider;
                for (QObject* child : metadataManager->children()) if (child == address) provider = qobject_cast<CompInfoParam*>(child);
                if (!provider || provider->metadataRevisionMemory() == 0) return fail(tr("The parameter metadata provider is unavailable."));
                preview->providers.insert(row.componentId, qMakePair(provider, provider->metadataRevisionMemory()));
            }
            if (!current() || !entry.fact || manager->_mapCompId2FactMap.value(row.componentId).value(row.name) != entry.fact)
                return fail(tr("The original Fact was retired while obtaining metadata."));
            entry.metadata = actualMetadata(entry.fact);
            if (entry.fact->thread() != thread() || entry.fact->parent() != manager || entry.fact->type() != row.factType
                || entry.fact->name() != row.name || entry.fact->componentId() != row.componentId
                || !entry.metadata || entry.metadata->thread() != thread() || entry.metadata->type() != row.factType)
                return fail(tr("A file type does not match its actual vehicle Fact, or metadata is unavailable."));
            entry.baseline = entry.fact->rawValue();
            entry.metadataBaseline = metadataValues(entry.metadata);
            entry.rawTranslator = entry.metadata->rawTranslator(); entry.cookedTranslator = entry.metadata->cookedTranslator();
            QVariant validated;
            if (!entry.metadata->convertAndValidateRaw(row.value, false, validated, error)
                || !OfflineParameterFile::valueMatches(row, validated))
                return fail(tr("A file value is outside the current parameter metadata domain."));
            // This temporary Fact owns a real metadata copy. No live metadata
            // reboot flag or pointer is lent to a preview object.
            QPointer<Fact> displayFact(new Fact(row.componentId, row.name, row.factType));
            const auto clearTemporary = qScopeGuard([displayFact]() { if (displayFact) delete displayFact.data(); });
            if (!current() || !displayFact || !entry.fact || !entry.metadata || actualMetadata(entry.fact) != entry.metadata)
                return fail(tr("The source changed while creating a temporary Fact."));
            QPointer<FactMetaData> copy(new FactMetaData(*entry.metadata));
            const auto clearCopy = qScopeGuard([copy]() { if (copy && !copy->parent()) delete copy.data(); });
            if (!current() || !displayFact || !copy) return fail(tr("The source changed while preparing display values."));
            copy->setParent(displayFact.data());
            if (!current() || !displayFact || !copy) return fail(tr("The temporary metadata was retired."));
            displayFact->setMetaData(copy.data());
            if (!current() || !displayFact || !copy) return fail(tr("The temporary metadata was retired."));
            displayFact->_containerSetRawValue(row.value);
            if (!current() || !displayFact || !copy || !OfflineParameterFile::valueMatches(row, displayFact->rawValue()))
                return fail(tr("A preview value could not be installed exactly."));
            fileText = displayFact->enumOrValueString();
            if (!current() || !entry.fact || !entry.metadata || actualMetadata(entry.fact) != entry.metadata)
                return fail(tr("The actual Fact changed while formatting the preview."));
            // enumOrValueString can add an unknown enum entry. Format the
            // baseline through the same owned copy, never through the live Fact.
            if (!displayFact || !copy) return fail(tr("The temporary preview was retired."));
            displayFact->_containerSetRawValue(entry.baseline);
            if (!current() || !displayFact || !copy) return fail(tr("The temporary baseline was retired."));
            vehicleText = displayFact->enumOrValueString();
            if (!current() || !displayFact || !copy) return fail(tr("The temporary baseline was retired."));
            units = copy->cookedUnits();
            if (!current() || !entry.fact || !entry.metadata
                || !exactValue(entry.fact->rawValue(), entry.baseline)
                || !exactValue(metadataValues(entry.metadata), entry.metadataBaseline)) return fail(tr("The preview baseline changed."));
            if (entry.metadata->readOnly() || OfflineParameterFile::valueMatches(row, entry.baseline)) continue;
        }
        QPointer<ParameterEditorDiff> display(new ParameterEditorDiff(nullptr));
        pendingRows.append(display);
        if (!current() || !display) return fail(tr("The preview was retired while creating rows."));
        display->componentId = row.componentId; display->name = row.name; display->valueType = row.factType;
        display->fileValue = fileText; display->fileValueVar = row.value; display->vehicleValue = vehicleText;
        display->noVehicleValue = entry.missing; display->units = units;
        display->setParent(self.data());
        if (!current() || !display) return fail(tr("The preview was retired while parenting rows."));
        entry.display = display;
        preview->entries.append(entry);
    }
    if (!current()) return fail(tr("The source changed before publishing the complete preview."));
    // Recheck all baselines after every candidate has been prepared.
    for (const auto& entry : preview->entries) {
        const Fact* mapped = manager->_mapCompId2FactMap.value(entry.row.componentId).value(entry.row.name, nullptr);
        if ((entry.missing && mapped) || (!entry.missing && (!entry.fact || mapped != entry.fact || !entry.metadata
            || actualMetadata(entry.fact) != entry.metadata || !exactValue(entry.fact->rawValue(), entry.baseline)
            || !exactValue(metadataValues(entry.metadata), entry.metadataBaseline)))) return fail(tr("A preview baseline changed before publication."));
    }
    preview->token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _filePreview = preview; _diffToken = preview->token; _diffVehicleId = document.sourceSystemId;
    _diffOtherVehicle = otherVehicle; _diffMultipleComponents = multipleComponents; _diffError.clear();
    QObjectList objects;
    for (const auto& row : pendingRows) objects.append(row.data());
    published = true;
    _diffList.swapObjectList(objects); // One complete native model, never a valid prefix of a malformed document.
    const auto installed = [&]() { return current() && self->_filePreview == preview && self->_diffToken == preview->token; };
    if (!installed()) return fail(tr("The preview was retired during publication."));
    emit diffTokenChanged();
    if (!installed()) return fail(tr("The preview was retired during notification."));
    emit diffOtherVehicleChanged(otherVehicle);
    if (!installed()) return fail(tr("The preview was retired during notification."));
    emit diffMultipleComponentsChanged(multipleComponents);
    if (!installed()) return fail(tr("The preview was retired during notification."));
    emit diffErrorChanged();
    return installed();
}

void ParameterEditorController::refresh(void)
{
    _parameterMgr->refreshAllParameters();
}

bool ParameterEditorController::resetAllToDefaults(void)
{
    if (!writeAllowed()) {
        qWarning() << "Parameter reset blocked:" << writeBlockReason();
        return false;
    }
    _parameterMgr->resetAllParametersToDefaults();
    refresh();
    return true;
}

bool ParameterEditorController::resetAllToVehicleConfiguration(void)
{
    if (!writeAllowed()) {
        qWarning() << "Parameter configuration reset blocked:" << writeBlockReason();
        return false;
    }
    _parameterMgr->resetAllToVehicleConfiguration();
    refresh();
    return true;
}

bool ParameterEditorController::writeAllowed(void) const
{
    return _writeBlockReason().isEmpty();
}

QString ParameterEditorController::writeBlockReason(void) const
{
    return _writeBlockReason();
}

QString ParameterEditorController::_writeBlockReason(void) const
{
    const QPointer<ParameterManager> manager(_parameterMgr);
    const QPointer<Vehicle> vehicle(manager ? qobject_cast<Vehicle*>(manager->parent()) : nullptr);
    if (!vehicle || vehicle.data() != _vehicle || vehicle->parameterManager() != manager
        || !qgcApp() || !qgcApp()->toolbox()) {
        return tr("No active vehicle.");
    }
    Vehicle* activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (!activeVehicle || activeVehicle != vehicle) {
        return tr("The selected vehicle changed. Reopen the parameter editor before writing.");
    }
    if (vehicle->isOfflineEditingVehicle() || !vehicle->isInitialConnectComplete()) {
        return tr("Vehicle connection is not initialized.");
    }
    QPointer<VehicleLinkManager> links;
    const QObject* linksAddress = vehicle->vehicleLinkManager();
    for (QObject* child : vehicle->children()) if (child == linksAddress) links = qobject_cast<VehicleLinkManager*>(child);
    if (!links) {
        return tr("Vehicle link state is unknown.");
    }
    if (links->communicationLost() || links->primaryLink().expired()) {
        return tr("Vehicle link is lost.");
    }
    if (vehicle->armed() || vehicle->flying()) {
        return tr("Parameter maintenance is allowed only while disarmed and on the ground.");
    }
    return QString();
}

bool ParameterEditorController::_shouldShow(Fact* fact) const
{
    if (!_showModifiedOnly) {
        return true;
    }

    return fact->defaultValueAvailable() && !fact->valueEqualsDefault();
}

void ParameterEditorController::_searchTextChanged(void)
{
    QObjectList newParameterList;

    QStringList rgSearchStrings = _searchText.split(' ', Qt::SkipEmptyParts);

    if (rgSearchStrings.isEmpty() && !_showModifiedOnly) {
        ParameterEditorCategory* category = _categories.count() ? _categories.value<ParameterEditorCategory*>(0) : nullptr;
        setCurrentCategory(category);
        _searchParameters.clear();
    } else {
        _searchParameters.beginReset();
        _searchParameters.clear();

        for (const QString &paraName: _parameterMgr->parameterNames(_vehicle->defaultComponentId())) {
            Fact* fact = _parameterMgr->getParameter(_vehicle->defaultComponentId(), paraName);
            bool matched = _shouldShow(fact);
            // All of the search items must match in order for the parameter to be added to the list
            if (matched) {
                for (const auto& searchItem : rgSearchStrings) {
                    QRegularExpression re = QRegularExpression(searchItem, QRegularExpression::CaseInsensitiveOption);
                    if (re.isValid()) {
                        if (!fact->name().contains(re) &&
                                !fact->shortDescription().contains(re) &&
                                !fact->longDescription().contains(re)) {
                            matched = false;
                        }
                    } else {
                        if (!fact->name().contains(searchItem, Qt::CaseInsensitive) &&
                                !fact->shortDescription().contains(searchItem, Qt::CaseInsensitive) &&
                                !fact->longDescription().contains(searchItem, Qt::CaseInsensitive)) {
                            matched = false;
                        }
                    }
                }
            }
            if (matched) {
                _searchParameters.append(fact);
            }
        }

        _searchParameters.endReset();

        if (_parameters != &_searchParameters) {
            _parameters = &_searchParameters;
            emit parametersChanged();

            _currentCategory    = nullptr;
            _currentGroup       = nullptr;
        }
    }
}

void ParameterEditorController::_currentCategoryChanged(void)
{
    ParameterEditorGroup* group = nullptr;
    if (_currentCategory) {
        // Select first group when category changes
        group = _currentCategory->groups.value<ParameterEditorGroup*>(0);
    } else {
        group = nullptr;
    }
    setCurrentGroup(group);
}

void ParameterEditorController::_currentGroupChanged(void)
{
    _parameters = _currentGroup ? &_currentGroup->facts : nullptr;
    emit parametersChanged();
}

void ParameterEditorController::setCurrentCategory(QObject* currentCategory)
{
    ParameterEditorCategory* category = qobject_cast<ParameterEditorCategory*>(currentCategory);
    if (category != _currentCategory) {
        _currentCategory = category;
        emit currentCategoryChanged();
    }
}

void ParameterEditorController::setCurrentGroup(QObject* currentGroup)
{
    ParameterEditorGroup* group = qobject_cast<ParameterEditorGroup*>(currentGroup);
    if (group != _currentGroup) {
        _currentGroup = group;
        emit currentGroupChanged();
    }
}
