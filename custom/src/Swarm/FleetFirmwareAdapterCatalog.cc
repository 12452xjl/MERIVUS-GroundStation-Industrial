#include "FleetFirmwareAdapterCatalog.h"
#include "FleetMissionHeadingEncoding.h"

#include "QGCMAVLink.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QPointer>
#include <QSaveFile>
#include <QSet>
#include <QThread>
#include <QUuid>
#include <cmath>
#include <limits>

namespace {
constexpr int MaxEntries = 64;
constexpr int MaxDatumRelations = 32;
constexpr int MaxNodes = 8192;
constexpr int MaxDepth = 8;
constexpr qint64 MaxBytes = 1024 * 1024;

QString hash(const QVariant& value) {
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}
bool actualTrue(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool actualFalse(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
bool unsignedNumber(const QVariant& value, quint64 maximum, quint64* output) {
    const auto type = value.type();
    if (type != QVariant::Int && type != QVariant::UInt && type != QVariant::LongLong
        && type != QVariant::ULongLong && type != QVariant::Double) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < 0 || number > static_cast<double>(maximum) || std::floor(number) != number) return false;
    *output = static_cast<quint64>(number);
    return true;
}
bool decimal(const QVariant& value, quint64* output, bool positive = false) {
    if (value.type() != QVariant::String) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || (text.size() > 1 && text[0] == QLatin1Char('0'))) return false;
    for (QChar character : text) if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    bool accepted = false;
    const quint64 number = text.toULongLong(&accepted);
    if (!accepted || (positive && number == 0)) return false;
    *output = number;
    return true;
}
bool text(const QVariant& value, int maximum, QString* output = nullptr) {
    if (value.type() != QVariant::String) return false;
    const QString candidate = value.toString();
    if (candidate.isEmpty() || candidate.size() > maximum || candidate != candidate.trimmed()
        || QString::fromUtf8(candidate.toUtf8()) != candidate) return false;
    for (QChar character : candidate) if (character.isNull() || character.category() == QChar::Other_Control) return false;
    if (output) *output = candidate;
    return true;
}
bool hex(const QVariant& value, int length, bool nonzero = false) {
    if (value.type() != QVariant::String || value.toString().size() != length) return false;
    bool any = false;
    for (QChar character : value.toString()) {
        if (!(character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            && !(character >= QLatin1Char('a') && character <= QLatin1Char('f'))) return false;
        any = any || character != QLatin1Char('0');
    }
    return !nonzero || any;
}
bool uuid(const QVariant& value) {
    if (value.type() != QVariant::String || value.toString().size() != 36) return false;
    const QUuid id(value.toString());
    return !id.isNull() && id.toString(QUuid::WithoutBraces) == value.toString();
}
bool exactKeys(const QVariantMap& value, const QSet<QString>& keys) {
    if (value.size() != keys.size()) return false;
    for (auto it = value.constBegin(); it != value.constEnd(); ++it) if (!keys.contains(it.key())) return false;
    return true;
}
QString reversedWire(const QString& wire) {
    QString output;
    for (int index = 14; index >= 0; index -= 2) output.append(wire.mid(index, 2));
    return output;
}
quint32 vendorVersion(const QString& wire) {
    quint32 value = 0;
    for (int index = 0; index < 3; ++index) value |= wire.mid(index * 2, 2).toUInt(nullptr, 16) << (8 * index);
    return value;
}
bool boundedTree(const QVariant& value, int depth, int* nodes, qint64* bytes) {
    if (depth > MaxDepth || ++*nodes > MaxNodes) return false;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 64 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *bytes += it.key().toUtf8().size();
            if (*bytes > MaxBytes || !boundedTree(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        const QVariantList list = value.toList();
        if (list.size() > MaxNodes) return false;
        for (const QVariant& child : list) if (!boundedTree(child, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.type() == QVariant::String) {
        if (value.toString().size() > 4096 || QString::fromUtf8(value.toString().toUtf8()) != value.toString()) return false;
        *bytes += value.toString().toUtf8().size();
        return *bytes <= MaxBytes;
    }
    if (value.type() == QVariant::Bool || !value.isValid() || value.isNull()) return true;
    const auto type = value.type();
    if (type != QVariant::Int && type != QVariant::UInt && type != QVariant::LongLong
        && type != QVariant::ULongLong && type != QVariant::Double) return false;
    return std::isfinite(value.toDouble()) && std::abs(value.toDouble()) <= 9007199254740991.0;
}
bool bounded(const QVariant& value) {
    int nodes = 0; qint64 bytes = 0;
    return boundedTree(value, 0, &nodes, &bytes)
        && QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact).size() <= MaxBytes;
}

// Reject excessive nesting and duplicate object keys BEFORE the full parser
// can discard an earlier declaration. String values are never instructions.
bool jsonEnvelope(const QString& json) {
    struct Scope { QChar type; QSet<QString> keys; };
    QList<Scope> scopes;
    int tokens = 0;
    for (int index = 0; index < json.size(); ++index) {
        const QChar character = json[index];
        if (character == QLatin1Char('"')) {
            const int first = index;
            bool closed = false;
            while (++index < json.size()) {
                if (json[index] == QLatin1Char('\\')) { ++index; continue; }
                if (json[index] == QLatin1Char('"')) { closed = true; break; }
            }
            if (!closed || ++tokens > MaxNodes) return false;
            int next = index + 1;
            while (next < json.size() && json[next].isSpace()) ++next;
            if (next < json.size() && json[next] == QLatin1Char(':')) {
                if (scopes.isEmpty() || scopes.last().type != QLatin1Char('{') || index - first > 4096) return false;
                QJsonParseError error;
                const auto literal = QJsonDocument::fromJson((QStringLiteral("[") + json.mid(first, index - first + 1) + QStringLiteral("]")).toUtf8(), &error);
                if (error.error != QJsonParseError::NoError || !literal.isArray() || literal.array().size() != 1 || !literal.array().first().isString()) return false;
                const QString key = literal.array().first().toString();
                if (key.size() > 64 || scopes.last().keys.contains(key)) return false;
                scopes.last().keys.insert(key);
            }
        } else if (character == QLatin1Char('{') || character == QLatin1Char('[')) {
            if (scopes.size() >= MaxDepth || ++tokens > MaxNodes) return false;
            Scope scope; scope.type = character; scopes.append(scope);
        } else if (character == QLatin1Char('}') || character == QLatin1Char(']')) {
            if (scopes.isEmpty() || scopes.last().type != (character == QLatin1Char('}') ? QLatin1Char('{') : QLatin1Char('['))) return false;
            scopes.removeLast();
        } else if (!character.isSpace() && character != QLatin1Char(',') && character != QLatin1Char(':')) {
            // Count primitive values as well as strings/containers before
            // allocating the parsed tree. The JSON parser still validates
            // literal/number grammar; this pass only enforces the budget.
            if (++tokens > MaxNodes) return false;
            while (index + 1 < json.size()) {
                const QChar next = json[index + 1];
                if (next.isSpace() || next == QLatin1Char(',') || next == QLatin1Char(':') || next == QLatin1Char('}')
                    || next == QLatin1Char(']') || next == QLatin1Char('{') || next == QLatin1Char('[') || next == QLatin1Char('"')) break;
                ++index;
            }
        }
    }
    return scopes.isEmpty();
}

QVariantMap status(bool ok, const QString& error = QString()) {
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("errorCode"), error},
        {QStringLiteral("firmwareAdapterCertified"), false}, {QStringLiteral("signatureVerified"), false},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("executionPermitted"), false},
        {QStringLiteral("startPermitted"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executionVerified"), false},
        {QStringLiteral("sourceCommitProven"), false}, {QStringLiteral("hardwareIdentityCertified"), false}};
}

bool fixedSemantics(const QVariant& value) {
    if (value.type() != QVariant::Map) return false;
    const QVariantMap candidate = value.toMap();
    const QVariant schema = candidate.value(QStringLiteral("wireSchema"));
    if (schema.type() != QVariant::String || schema.isNull()
        || (schema.toString() != FleetMissionHeadingEncoding::wireSchema(false)
            && schema.toString() != FleetMissionHeadingEncoding::wireSchema(true))) return false;
    const QVariantMap required = FleetFirmwareAdapterCatalog::supportedSemantics(
        schema.toString() == FleetMissionHeadingEncoding::wireSchema(true));
    if (candidate.size() != required.size()) return false;
    for (auto it = required.constBegin(); it != required.constEnd(); ++it) {
        const QVariant actual = candidate.value(it.key());
        if (it.value().type() == QVariant::Bool) {
            if (!actualFalse(actual)) return false;
        } else if (it.value().type() == QVariant::String) {
            if (actual.type() != QVariant::String || actual.toString() != it.value().toString()) return false;
        } else if (it.value().type() == QVariant::List) {
            if (actual.type() != QVariant::List) return false;
            const QVariantList commands = actual.toList(), expected = it.value().toList();
            if (commands.size() != expected.size()) return false;
            for (int index = 0; index < commands.size(); ++index) {
                quint64 command = 0;
                if (!unsignedNumber(commands[index], 65535, &command) || command != expected[index].toUInt()) return false;
            }
        } else {
            quint64 number = 0;
            if (!unsignedNumber(actual, 255, &number) || number != it.value().toUInt()) return false;
        }
    }
    return true;
}

bool engineeringReview(const QVariant& value, QVariantMap* output, QString* problem) {
    if (value.type() != QVariant::Map || value.isNull()) {
        *problem = QStringLiteral("engineering-review-invalid"); return false;
    }
    const QVariantMap review = value.toMap();
    if (!exactKeys(review, {QStringLiteral("reviewId"), QStringLiteral("reviewerId"), QStringLiteral("reviewedAtUtc"), QStringLiteral("evidenceDigest")})
        || !text(review.value(QStringLiteral("reviewId")), 128) || !text(review.value(QStringLiteral("reviewerId")), 128)
        || !hex(review.value(QStringLiteral("evidenceDigest")), 64, true)
        || review.value(QStringLiteral("reviewedAtUtc")).type() != QVariant::String) {
        *problem = QStringLiteral("engineering-review-invalid"); return false;
    }
    const QString time = review.value(QStringLiteral("reviewedAtUtc")).toString();
    const QDateTime parsed = QDateTime::fromString(time, Qt::ISODateWithMs);
    if (time.size() != 24 || !parsed.isValid() || parsed.timeSpec() != Qt::UTC
        || parsed.toUTC().toString(Qt::ISODateWithMs) != time) {
        *problem = QStringLiteral("engineering-review-utc-invalid"); return false;
    }
    *output = review;
    return true;
}

QVariantMap datumSelector(const QVariantMap& value) {
    QVariantMap selector;
    for (const QString& key : {QStringLiteral("sceneId"), QStringLiteral("sceneRevision"),
         QStringLiteral("sceneSourceDocumentHash"), QStringLiteral("sceneVerticalDatumId"),
         QStringLiteral("configurationFingerprint"), QStringLiteral("globalOriginReferenceHash")})
        selector.insert(key, value.value(key));
    return selector;
}

bool datumQueryKnown(const QVariantMap& value) {
    quint64 revision = 0;
    return bounded(value) && exactKeys(value, {QStringLiteral("sceneId"), QStringLiteral("sceneRevision"),
        QStringLiteral("sceneSourceDocumentHash"), QStringLiteral("sceneVerticalDatumId"),
        QStringLiteral("configurationFingerprint"), QStringLiteral("globalOriginReferenceHash")})
        && text(value.value(QStringLiteral("sceneId")), 128)
        && decimal(value.value(QStringLiteral("sceneRevision")), &revision, true)
        && text(value.value(QStringLiteral("sceneVerticalDatumId")), 128)
        && hex(value.value(QStringLiteral("sceneSourceDocumentHash")), 64)
        && hex(value.value(QStringLiteral("configurationFingerprint")), 64)
        && hex(value.value(QStringLiteral("globalOriginReferenceHash")), 64);
}

bool normalizeDatumRelations(const QVariant& source, QVariantList* output, QString* problem) {
    const auto reject = [&](const QString& code) { *problem = code; return false; };
    if (source.type() != QVariant::List || source.isNull()) return reject(QStringLiteral("datum-relations-list-invalid"));
    const QVariantList rows = source.toList();
    if (rows.isEmpty() || rows.size() > MaxDatumRelations) return reject(QStringLiteral("datum-relations-count-invalid"));
    QVariantList normalized;
    QSet<QString> ids;
    QSet<QByteArray> selectors;
    for (const QVariant& row : rows) {
        if (row.type() != QVariant::Map || row.isNull()) return reject(QStringLiteral("datum-relation-shape-invalid"));
        const QVariantMap relation = row.toMap();
        if (!exactKeys(relation, {QStringLiteral("relationId"), QStringLiteral("sceneId"), QStringLiteral("sceneRevision"),
            QStringLiteral("sceneSourceDocumentHash"), QStringLiteral("sceneVerticalDatumId"), QStringLiteral("configurationFingerprint"),
            QStringLiteral("globalOriginReferenceHash"), QStringLiteral("reportedAltitudeSource"), QStringLiteral("reportedAltitudeDatumId"),
            QStringLiteral("offsetMm"), QStringLiteral("relationModel"), QStringLiteral("engineeringReview")})
            || !text(relation.value(QStringLiteral("relationId")), 128)
            || !datumQueryKnown(datumSelector(relation))) return reject(QStringLiteral("datum-relation-shape-invalid"));
        quint64 offset = 0;
        const QVariant offsetValue = relation.value(QStringLiteral("offsetMm"));
        if (!offsetValue.isValid() || offsetValue.isNull() || !unsignedNumber(offsetValue, 0, &offset)
            || relation.value(QStringLiteral("reportedAltitudeSource")).type() != QVariant::String
            || relation.value(QStringLiteral("reportedAltitudeSource")).toString() != QStringLiteral("global-position-int-alt")
            || !text(relation.value(QStringLiteral("reportedAltitudeDatumId")), 128)
            || relation.value(QStringLiteral("reportedAltitudeDatumId")) != relation.value(QStringLiteral("sceneVerticalDatumId"))
            || relation.value(QStringLiteral("relationModel")).type() != QVariant::String
            || relation.value(QStringLiteral("relationModel")).toString() != QStringLiteral("engineer-declared-same-datum-zero-offset"))
            return reject(QStringLiteral("datum-relation-not-explicit-same-datum-zero-offset"));
        QVariantMap review;
        if (!engineeringReview(relation.value(QStringLiteral("engineeringReview")), &review, problem)) return false;
        const QString id = relation.value(QStringLiteral("relationId")).toString();
        const QByteArray selector = QJsonDocument::fromVariant(datumSelector(relation)).toJson(QJsonDocument::Compact);
        if (ids.contains(id) || selectors.contains(selector)) return reject(QStringLiteral("duplicate-datum-relation-id-or-selector"));
        ids.insert(id); selectors.insert(selector);
        QVariantMap item = relation;
        item.insert(QStringLiteral("offsetMm"), 0);
        item.insert(QStringLiteral("engineeringReview"), review);
        normalized.append(item);
    }
    *output = normalized;
    return true;
}

bool normalizeDescriptor(const QVariantMap& source, QVariantMap* output, QString* problem) {
    const auto reject = [&](const QString& code) { *problem = code; return false; };
    quint64 schema = 0;
    if (!bounded(source) || !unsignedNumber(source.value(QStringLiteral("schemaVersion")), 2, &schema) || schema < 1)
        return reject(QStringLiteral("descriptor-shape-invalid"));
    QSet<QString> keys{QStringLiteral("schemaVersion"), QStringLiteral("descriptorId"),
        QStringLiteral("adapterId"), QStringLiteral("firmware"), QStringLiteral("sourceCommit"),
        QStringLiteral("sourceRepository"), QStringLiteral("engineeringReview"), QStringLiteral("semantics")};
    if (schema == 2) keys.insert(QStringLiteral("datumRelations"));
    if (!exactKeys(source, keys)) return reject(QStringLiteral("descriptor-shape-invalid"));
    QString id, repository;
    if (!text(source.value(QStringLiteral("descriptorId")), 64, &id)
        || source.value(QStringLiteral("adapterId")).type() != QVariant::String
        || source.value(QStringLiteral("adapterId")).toString() != QStringLiteral("px4-multirotor-mission-v1")
        || !hex(source.value(QStringLiteral("sourceCommit")), 40, true)
        || !text(source.value(QStringLiteral("sourceRepository")), 512, &repository)) return reject(QStringLiteral("descriptor-identity-invalid"));
    if (source.value(QStringLiteral("firmware")).type() != QVariant::Map) return reject(QStringLiteral("firmware-tuple-invalid"));
    const QVariantMap firmware = source.value(QStringLiteral("firmware")).toMap();
    if (!exactKeys(firmware, {QStringLiteral("autopilot"), QStringLiteral("flightSoftwareVersion"),
        QStringLiteral("flightCustomVersionWireHex"), QStringLiteral("boardVersion"), QStringLiteral("vendorId"),
        QStringLiteral("productId"), QStringLiteral("customVersionEncoding"), QStringLiteral("vendorVersion")}))
        return reject(QStringLiteral("firmware-tuple-invalid"));
    QVariantMap normalizedFirmware;
    for (const QString& key : {QStringLiteral("autopilot"), QStringLiteral("flightSoftwareVersion"),
                              QStringLiteral("boardVersion"), QStringLiteral("vendorId"), QStringLiteral("productId"), QStringLiteral("vendorVersion")}) {
        const quint64 maximum = key == QStringLiteral("autopilot") ? 255 : key == QStringLiteral("vendorVersion") ? 16777215
            : key == QStringLiteral("vendorId") || key == QStringLiteral("productId") ? 65535 : 4294967295ULL;
        quint64 number = 0;
        if (!unsignedNumber(firmware.value(key), maximum, &number)
            || (key != QStringLiteral("vendorVersion") && number == 0)
            || (key == QStringLiteral("autopilot") && number != MAV_AUTOPILOT_PX4)) return reject(QStringLiteral("firmware-tuple-number-invalid"));
        normalizedFirmware.insert(key, static_cast<uint>(number));
    }
    if (!hex(firmware.value(QStringLiteral("flightCustomVersionWireHex")), 16, true)
        || firmware.value(QStringLiteral("customVersionEncoding")).type() != QVariant::String
        || firmware.value(QStringLiteral("customVersionEncoding")).toString() != QStringLiteral("px4-git5-vendor3-le"))
        return reject(QStringLiteral("custom-version-layout-unsupported"));
    const QString wire = firmware.value(QStringLiteral("flightCustomVersionWireHex")).toString();
    const QString display = reversedWire(wire);
    if (display.left(10) == QString(10, QLatin1Char('0')) || source.value(QStringLiteral("sourceCommit")).toString().left(10) != display.left(10)
        || vendorVersion(wire) != normalizedFirmware.value(QStringLiteral("vendorVersion")).toUInt())
        return reject(QStringLiteral("git-prefix-or-vendor-version-mismatch"));
    normalizedFirmware.insert(QStringLiteral("flightCustomVersionWireHex"), wire);
    normalizedFirmware.insert(QStringLiteral("customVersionEncoding"), QStringLiteral("px4-git5-vendor3-le"));
    QVariantMap review;
    if (!engineeringReview(source.value(QStringLiteral("engineeringReview")), &review, problem)) return false;
    if (!fixedSemantics(source.value(QStringLiteral("semantics"))))
        return reject(QStringLiteral("adapter-semantics-not-fixed-contract"));
    QVariantList relations;
    if (schema == 2 && !normalizeDatumRelations(source.value(QStringLiteral("datumRelations")), &relations, problem)) return false;
    *output = {{QStringLiteral("schemaVersion"), static_cast<int>(schema)}, {QStringLiteral("descriptorId"), id},
        {QStringLiteral("adapterId"), QStringLiteral("px4-multirotor-mission-v1")}, {QStringLiteral("firmware"), normalizedFirmware},
        {QStringLiteral("sourceCommit"), source.value(QStringLiteral("sourceCommit"))}, {QStringLiteral("sourceRepository"), repository},
        {QStringLiteral("engineeringReview"), review}, {QStringLiteral("semantics"), FleetFirmwareAdapterCatalog::supportedSemantics(
            source.value(QStringLiteral("semantics")).toMap().value(QStringLiteral("wireSchema")).toString()
                == FleetMissionHeadingEncoding::wireSchema(true))}};
    if (schema == 2) output->insert(QStringLiteral("datumRelations"), relations);
    return true;
}

struct NormalizedArchive {
    quint64 revision = 0;
    QMap<QString, QVariantMap> descriptors;
    QByteArray originalBytes;
    QByteArray normalizedBytes;
};

// Both preview and import stage this same bounded, whole-replacement archive.
// This function has no catalog, QObject, file or approval side effects.
bool stageArchive(const QString& json, quint64 currentRevision, NormalizedArchive* output, QString* problem) {
    const auto reject = [&](const QString& code) { *problem = code; return false; };
    if (json.size() > MaxBytes) return reject(QStringLiteral("catalog-json-envelope-invalid"));
    const QByteArray bytes = json.toUtf8();
    if (bytes.size() > MaxBytes || QString::fromUtf8(bytes) != json || !jsonEnvelope(json))
        return reject(QStringLiteral("catalog-json-envelope-invalid"));
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) return reject(QStringLiteral("catalog-json-invalid"));
    const QVariantMap root = document.toVariant().toMap();
    quint64 schema = 0, importedRevision = 0;
    if (!bounded(root) || !exactKeys(root, {QStringLiteral("schemaVersion"), QStringLiteral("kind"), QStringLiteral("catalogRevision"), QStringLiteral("descriptors")})
        || !unsignedNumber(root.value(QStringLiteral("schemaVersion")), 1, &schema) || schema != 1
        || root.value(QStringLiteral("kind")).type() != QVariant::String
        || root.value(QStringLiteral("kind")).toString() != QStringLiteral("local-firmware-adapter-declarations")
        || !decimal(root.value(QStringLiteral("catalogRevision")), &importedRevision)
        || root.value(QStringLiteral("descriptors")).type() != QVariant::List
        || root.value(QStringLiteral("descriptors")).toList().size() > MaxEntries)
        return reject(QStringLiteral("catalog-root-contract-invalid"));
    if (qMax(currentRevision, importedRevision) == std::numeric_limits<quint64>::max())
        return reject(QStringLiteral("catalog-revision-exhausted"));
    NormalizedArchive staged;
    staged.revision = importedRevision;
    staged.originalBytes = bytes;
    QSet<QString> firmwareTuples;
    for (const QVariant& value : root.value(QStringLiteral("descriptors")).toList()) {
        QVariantMap descriptor;
        QString descriptorProblem;
        if (value.type() != QVariant::Map || !normalizeDescriptor(value.toMap(), &descriptor, &descriptorProblem))
            return reject(descriptorProblem.isEmpty() ? QStringLiteral("descriptor-shape-invalid") : descriptorProblem);
        const QString id = descriptor.value(QStringLiteral("descriptorId")).toString();
        const QString firmwareTuple = hash(descriptor.value(QStringLiteral("firmware")));
        if (staged.descriptors.contains(id) || firmwareTuples.contains(firmwareTuple))
            return reject(QStringLiteral("duplicate-descriptor-id-or-firmware-tuple"));
        firmwareTuples.insert(firmwareTuple);
        staged.descriptors.insert(id, descriptor);
    }
    QVariantList descriptors;
    for (const QVariantMap& descriptor : staged.descriptors) descriptors.append(descriptor);
    const QVariantMap normalizedRoot{{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("kind"), QStringLiteral("local-firmware-adapter-declarations")},
        {QStringLiteral("catalogRevision"), QString::number(importedRevision)}, {QStringLiteral("descriptors"), descriptors}};
    staged.normalizedBytes = QJsonDocument::fromVariant(normalizedRoot).toJson(QJsonDocument::Compact);
    if (staged.normalizedBytes.size() > MaxBytes) return reject(QStringLiteral("catalog-json-envelope-invalid"));
    *output = staged;
    return true;
}

QString bytesHash(const QByteArray& bytes) {
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}
}

FleetFirmwareAdapterCatalog::FleetFirmwareAdapterCatalog(QObject* parent) : QObject(parent),
    _catalogEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces)) { _refreshFingerprint(); }

void FleetFirmwareAdapterCatalog::_refreshFingerprint() {
    QVariantList entries;
    for (auto it = _entries.constBegin(); it != _entries.constEnd(); ++it)
        entries.append(QVariantMap{{QStringLiteral("descriptor"), it.value().descriptor},
            {QStringLiteral("descriptorHash"), it.value().descriptorHash}, {QStringLiteral("restoredFromArchive"), it.value().restored},
            {QStringLiteral("commandApprovalRestored"), false}, {QStringLiteral("firmwareAdapterCertified"), false}});
    _catalogFingerprint = hash(QVariantMap{{QStringLiteral("catalogEpoch"), _catalogEpoch},
        {QStringLiteral("revision"), QString::number(_revision)}, {QStringLiteral("entries"), entries}});
}

QVariantMap FleetFirmwareAdapterCatalog::datumBindingMemory() const {
    if (QThread::currentThread() != thread()) return {};
    return {{QStringLiteral("catalogEpoch"), _catalogEpoch}, {QStringLiteral("revision"), QString::number(_revision)},
        {QStringLiteral("catalogFingerprint"), _catalogFingerprint}};
}

QVariantMap FleetFirmwareAdapterCatalog::supportedSemantics(bool headingTicks) {
    QVariantMap result{{QStringLiteral("wireSchema"), FleetMissionHeadingEncoding::wireSchema(headingTicks)},
        {QStringLiteral("commands"), QVariantList{static_cast<int>(MAV_CMD_NAV_WAYPOINT), static_cast<int>(MAV_CMD_NAV_LOITER_UNLIM),
            static_cast<int>(MAV_CMD_NAV_LAND), static_cast<int>(MAV_CMD_NAV_TAKEOFF), static_cast<int>(MAV_CMD_DO_CHANGE_SPEED)}},
        {QStringLiteral("relativeDwell"), QStringLiteral("waypoint-after-arrival")},
        {QStringLiteral("horizontalSpeedType"), 1}, {QStringLiteral("verticalPolicy"), QStringLiteral("vehicle-parameters")},
        {QStringLiteral("defaultYawMode"), QStringLiteral("system-default")},
        {QStringLiteral("takeoffEndpointPolicy"), QStringLiteral("same-xyz-as-first-outbound")},
        {QStringLiteral("landingEndpointPolicy"), QStringLiteral("same-xy-ground-below-approach")},
        {QStringLiteral("absoluteTiming"), false}, {QStringLiteral("synchronizedStart"), false},
        {QStringLiteral("rtl"), false}, {QStringLiteral("clearanceCertified"), false},
        {QStringLiteral("takeoffPathCertified"), false}, {QStringLiteral("landingClearanceCertified"), false}};
    if (headingTicks) {
        result.insert(QStringLiteral("waypointYawMode"), QStringLiteral("absolute-degrees"));
        result.insert(QStringLiteral("waypointYawEncoding"), QStringLiteral("binary-degree-tick-1-over-256"));
        result.insert(QStringLiteral("waypointYawAttainmentCertified"), false);
    }
    return result;
}

bool FleetFirmwareAdapterCatalog::acceptsWireSchema(const QVariantMap& declaration, const QString& schema) const {
    if (QThread::currentThread() != thread() || !actualTrue(declaration.value(QStringLiteral("ok")))
        || !actualTrue(declaration.value(QStringLiteral("matched")))
        || declaration.value(QStringLiteral("catalogEpoch")) != QVariant(_catalogEpoch)
        || declaration.value(QStringLiteral("revision")) != QVariant(QString::number(_revision))
        || declaration.value(QStringLiteral("catalogFingerprint")) != QVariant(_catalogFingerprint)
        || declaration.value(QStringLiteral("descriptorId")).type() != QVariant::String
        || declaration.value(QStringLiteral("descriptorHash")).type() != QVariant::String) return false;
    const auto found = _entries.constFind(declaration.value(QStringLiteral("descriptorId")).toString());
    if (found == _entries.constEnd() || declaration.value(QStringLiteral("descriptorHash")).toString() != found->descriptorHash) return false;
    const QString declared = found->descriptor.value(QStringLiteral("semantics")).toMap().value(QStringLiteral("wireSchema")).toString();
    // A newly reviewed heading declaration includes the unchanged v2 subset.
    // Existing v2 declarations are never promoted to the new heading domain.
    return (schema == FleetMissionHeadingEncoding::wireSchema(false)
            && (declared == schema || declared == FleetMissionHeadingEncoding::wireSchema(true)))
        || (schema == FleetMissionHeadingEncoding::wireSchema(true) && declared == schema);
}

QVariantMap FleetFirmwareAdapterCatalog::snapshot() const {
    if (QThread::currentThread() != thread()) return status(false, QStringLiteral("catalog-owner-thread-required"));
    QVariantMap result = status(true);
    QVariantList entries;
    for (auto it = _entries.constBegin(); it != _entries.constEnd(); ++it)
        entries.append(QVariantMap{{QStringLiteral("descriptor"), it.value().descriptor},
            {QStringLiteral("descriptorHash"), it.value().descriptorHash}, {QStringLiteral("restoredFromArchive"), it.value().restored},
            {QStringLiteral("commandApprovalRestored"), false}, {QStringLiteral("firmwareAdapterCertified"), false}});
    result.insert(QStringLiteral("catalogEpoch"), _catalogEpoch);
    result.insert(QStringLiteral("revision"), QString::number(_revision));
    result.insert(QStringLiteral("entryCount"), _entries.size());
    result.insert(QStringLiteral("entries"), entries);
    result.insert(QStringLiteral("catalogFingerprint"), _catalogFingerprint);
    return result;
}

QVariantMap FleetFirmwareAdapterCatalog::registerReviewedDescriptor(const QVariantMap& descriptor, const QString& expectedRevision) {
    quint64 expected = 0;
    if (QThread::currentThread() != thread()) return status(false, QStringLiteral("catalog-owner-thread-required"));
    if (!decimal(expectedRevision, &expected) || expected != _revision) return status(false, QStringLiteral("catalog-revision-conflict"));
    if (_revision == std::numeric_limits<quint64>::max()) return status(false, QStringLiteral("catalog-revision-exhausted"));
    QVariantMap normalized; QString problem;
    if (!normalizeDescriptor(descriptor, &normalized, &problem)) return status(false, problem);
    const QString id = normalized.value(QStringLiteral("descriptorId")).toString();
    if (_entries.contains(id)) return status(false, QStringLiteral("duplicate-descriptor-id"));
    if (_entries.size() >= MaxEntries) return status(false, QStringLiteral("catalog-entry-limit"));
    for (const Entry& entry : _entries)
        if (entry.descriptor.value(QStringLiteral("firmware")) == normalized.value(QStringLiteral("firmware")))
            return status(false, QStringLiteral("duplicate-firmware-tuple"));
    // Schema 2 can carry several relations per descriptor. Keep the existing
    // whole-archive limits, not merely a separate budget for every new entry.
    QVariantList candidateDescriptors;
    for (const Entry& entry : _entries) candidateDescriptors.append(entry.descriptor);
    candidateDescriptors.append(normalized);
    if (!bounded(QVariantMap{{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("kind"), QStringLiteral("local-firmware-adapter-declarations")},
        {QStringLiteral("catalogRevision"), QString::number(_revision + 1)},
        {QStringLiteral("descriptors"), candidateDescriptors}})) return status(false, QStringLiteral("catalog-json-envelope-invalid"));
    Entry entry; entry.descriptor = normalized; entry.descriptorHash = hash(normalized);
    _entries.insert(id, entry); ++_revision;
    _refreshFingerprint();
    const quint64 revision = _revision; const QString epoch = _catalogEpoch;
    QVariantMap receipt = status(true);
    receipt.insert(QStringLiteral("descriptorId"), id); receipt.insert(QStringLiteral("descriptorHash"), entry.descriptorHash);
    receipt.insert(QStringLiteral("revision"), QString::number(revision)); receipt.insert(QStringLiteral("catalogEpoch"), epoch);
    QPointer<FleetFirmwareAdapterCatalog> self(this); emit changed();
    receipt.insert(QStringLiteral("stillCurrent"), self && _revision == revision && _catalogEpoch == epoch);
    return receipt;
}

QVariantMap FleetFirmwareAdapterCatalog::revokeDescriptor(const QString& descriptorId, const QString& expectedRevision) {
    quint64 expected = 0;
    if (QThread::currentThread() != thread()) return status(false, QStringLiteral("catalog-owner-thread-required"));
    if (!decimal(expectedRevision, &expected) || expected != _revision) return status(false, QStringLiteral("catalog-revision-conflict"));
    if (_revision == std::numeric_limits<quint64>::max()) return status(false, QStringLiteral("catalog-revision-exhausted"));
    if (!text(descriptorId, 64) || !_entries.contains(descriptorId)) return status(false, QStringLiteral("descriptor-not-found"));
    _entries.remove(descriptorId); ++_revision;
    _refreshFingerprint();
    const quint64 revision = _revision; const QString epoch = _catalogEpoch;
    QVariantMap receipt = status(true);
    receipt.insert(QStringLiteral("descriptorId"), descriptorId); receipt.insert(QStringLiteral("revision"), QString::number(revision));
    receipt.insert(QStringLiteral("catalogEpoch"), epoch);
    QPointer<FleetFirmwareAdapterCatalog> self(this); emit changed();
    receipt.insert(QStringLiteral("stillCurrent"), self && _revision == revision && _catalogEpoch == epoch);
    return receipt;
}

QVariantMap FleetFirmwareAdapterCatalog::matchObservation(const QVariantMap& observation) const {
    QVariantMap result = status(true);
    result.insert(QStringLiteral("matched"), false);
    result.insert(QStringLiteral("source"), QStringLiteral("local-engineering-declaration"));
    result.insert(QStringLiteral("descriptorId"), QString());
    result.insert(QStringLiteral("descriptorHash"), QString());
    result.insert(QStringLiteral("commandApprovalRestored"), false);
    const auto finish = [&](const QString& problem, bool ok) {
        QVariantMap finished = result;
        finished.insert(QStringLiteral("ok"), ok);
        finished.insert(QStringLiteral("errorCode"), problem);
        finished.insert(QStringLiteral("reasons"), problem.isEmpty() ? QVariantList() : QVariantList{problem});
        finished.insert(QStringLiteral("fingerprint"), hash(finished));
        return finished;
    };
    if (QThread::currentThread() != thread()) return finish(QStringLiteral("catalog-owner-thread-required"), false);
    const QVariantMap catalog = snapshot();
    for (const QString& key : {QStringLiteral("catalogEpoch"), QStringLiteral("revision"), QStringLiteral("catalogFingerprint")})
        result.insert(key, catalog.value(key));
    // This is the fixed native Vehicle getter schema, not cached initial-
    // connect metadata and not an authorization supplied by a QML caller.
    if (!bounded(observation) || !exactKeys(observation, {QStringLiteral("schemaVersion"), QStringLiteral("vehicleId"),
        QStringLiteral("componentId"), QStringLiteral("observed"), QStringLiteral("available"), QStringLiteral("fresh"),
        QStringLiteral("ageMs"), QStringLiteral("source"), QStringLiteral("sourceEpoch"), QStringLiteral("contentRevision"),
        QStringLiteral("receiptSequence"), QStringLiteral("contentHash"), QStringLiteral("flightSoftwareVersion"), QStringLiteral("middlewareSoftwareVersion"),
        QStringLiteral("osSoftwareVersion"), QStringLiteral("flightCustomVersionWireHex"), QStringLiteral("middlewareCustomVersionWireHex"),
        QStringLiteral("osCustomVersionWireHex"), QStringLiteral("px4CustomVersionDisplayHex"), QStringLiteral("boardVersion"),
        QStringLiteral("vendorId"), QStringLiteral("productId"), QStringLiteral("capabilitiesDecimal"), QStringLiteral("uidDecimal"),
        QStringLiteral("uidKnown"), QStringLiteral("uid2Hex"), QStringLiteral("uid2Known"), QStringLiteral("firmwareType"),
        QStringLiteral("vehicleType"), QStringLiteral("signatureVerified"), QStringLiteral("authenticated"),
        QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")}))
        return finish(QStringLiteral("native-version-observation-shape-invalid"), false);
    quint64 schema = 0, vehicleId = 0, componentId = 0, firmwareType = 0, vehicleType = 0;
    if (!unsignedNumber(observation.value(QStringLiteral("schemaVersion")), 1, &schema) || schema != 1
        || !unsignedNumber(observation.value(QStringLiteral("vehicleId")), 255, &vehicleId) || vehicleId == 0
        || !unsignedNumber(observation.value(QStringLiteral("componentId")), 255, &componentId) || componentId == 0
        || !unsignedNumber(observation.value(QStringLiteral("firmwareType")), 255, &firmwareType) || firmwareType != MAV_AUTOPILOT_PX4
        || !unsignedNumber(observation.value(QStringLiteral("vehicleType")), 255, &vehicleType)
        || !QGCMAVLink::isMultiRotor(static_cast<MAV_TYPE>(vehicleType)))
        return finish(QStringLiteral("native-px4-multirotor-identity-invalid"), false);
    quint64 age = 0, revision = 0, receipt = 0;
    if (!actualTrue(observation.value(QStringLiteral("observed"))) || !actualTrue(observation.value(QStringLiteral("available")))
        || !actualTrue(observation.value(QStringLiteral("fresh")))
        || observation.value(QStringLiteral("source")).type() != QVariant::String
        || observation.value(QStringLiteral("source")).toString() != QStringLiteral("live-autopilot-version")
        || !unsignedNumber(observation.value(QStringLiteral("ageMs")), 60000, &age)
        || !uuid(observation.value(QStringLiteral("sourceEpoch")))
        || !decimal(observation.value(QStringLiteral("contentRevision")), &revision, true)
        || !decimal(observation.value(QStringLiteral("receiptSequence")), &receipt, true)
        || !hex(observation.value(QStringLiteral("contentHash")), 64, true))
        return finish(QStringLiteral("native-version-observation-unavailable-or-stale"), false);
    for (const QString& flag : {QStringLiteral("signatureVerified"), QStringLiteral("authenticated"),
                               QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        if (!actualFalse(observation.value(flag))) return finish(QStringLiteral("native-observation-certification-claim-invalid"), false);
    QVariantMap payload;
    for (const QString& key : {QStringLiteral("flightSoftwareVersion"), QStringLiteral("middlewareSoftwareVersion"),
        QStringLiteral("osSoftwareVersion"), QStringLiteral("boardVersion"), QStringLiteral("vendorId"), QStringLiteral("productId")}) {
        quint64 value = 0;
        const quint64 maximum = key == QStringLiteral("vendorId") || key == QStringLiteral("productId") ? 65535 : 4294967295ULL;
        if (!unsignedNumber(observation.value(key), maximum, &value)
            || (key != QStringLiteral("middlewareSoftwareVersion") && key != QStringLiteral("osSoftwareVersion") && value == 0))
            return finish(QStringLiteral("native-firmware-tuple-number-invalid"), false);
        payload.insert(key, static_cast<uint>(value));
    }
    for (const QString& key : {QStringLiteral("flightCustomVersionWireHex"), QStringLiteral("middlewareCustomVersionWireHex"),
                               QStringLiteral("osCustomVersionWireHex"), QStringLiteral("uid2Hex")}) {
        if (!hex(observation.value(key), key == QStringLiteral("uid2Hex") ? 36 : 16, key == QStringLiteral("flightCustomVersionWireHex")))
            return finish(QStringLiteral("native-version-wire-bytes-invalid"), false);
        payload.insert(key, observation.value(key));
    }
    const QString wire = payload.value(QStringLiteral("flightCustomVersionWireHex")).toString();
    if (!hex(observation.value(QStringLiteral("px4CustomVersionDisplayHex")), 16, true)
        || observation.value(QStringLiteral("px4CustomVersionDisplayHex")).toString() != reversedWire(wire))
        return finish(QStringLiteral("native-custom-version-display-mismatch"), false);
    quint64 capabilities = 0, uid = 0;
    if (!decimal(observation.value(QStringLiteral("capabilitiesDecimal")), &capabilities)
        || !(capabilities & static_cast<quint64>(MAV_PROTOCOL_CAPABILITY_MISSION_INT))
        || !decimal(observation.value(QStringLiteral("uidDecimal")), &uid))
        return finish(QStringLiteral("native-version-capability-or-uid-invalid"), false);
    payload.insert(QStringLiteral("capabilitiesDecimal"), observation.value(QStringLiteral("capabilitiesDecimal")));
    payload.insert(QStringLiteral("uidDecimal"), observation.value(QStringLiteral("uidDecimal")));
    const bool uidKnown = uid != 0;
    const bool uid2Known = payload.value(QStringLiteral("uid2Hex")).toString() != QString(36, QLatin1Char('0'));
    if (observation.value(QStringLiteral("uidKnown")).type() != QVariant::Bool
        || observation.value(QStringLiteral("uid2Known")).type() != QVariant::Bool
        || observation.value(QStringLiteral("uidKnown")).toBool() != uidKnown
        || observation.value(QStringLiteral("uid2Known")).toBool() != uid2Known || (!uidKnown && !uid2Known))
        return finish(QStringLiteral("native-observed-identifier-unknown-or-inconsistent"), false);
    // All 12 original payload fields are exposed by Vehicle. Recompute the
    // agreed JSON identity; a digest remains a source-content check, not a
    // signature or full firmware/binary identity proof.
    const QString contentHash = QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(QJsonObject::fromVariantMap(payload)).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
    if (contentHash != observation.value(QStringLiteral("contentHash")).toString())
        return finish(QStringLiteral("native-version-content-hash-mismatch"), false);
    const QVariantMap observedFirmware{{QStringLiteral("autopilot"), static_cast<uint>(firmwareType)},
        {QStringLiteral("flightSoftwareVersion"), payload.value(QStringLiteral("flightSoftwareVersion"))},
        {QStringLiteral("flightCustomVersionWireHex"), wire}, {QStringLiteral("boardVersion"), payload.value(QStringLiteral("boardVersion"))},
        {QStringLiteral("vendorId"), payload.value(QStringLiteral("vendorId"))}, {QStringLiteral("productId"), payload.value(QStringLiteral("productId"))}};
    result.insert(QStringLiteral("observationBinding"), QVariantMap{{QStringLiteral("vehicleId"), static_cast<uint>(vehicleId)},
        {QStringLiteral("componentId"), static_cast<uint>(componentId)}, {QStringLiteral("firmwareType"), static_cast<uint>(firmwareType)},
        {QStringLiteral("vehicleType"), static_cast<uint>(vehicleType)}, {QStringLiteral("source"), QStringLiteral("live-autopilot-version")},
        {QStringLiteral("sourceEpoch"), observation.value(QStringLiteral("sourceEpoch"))},
        {QStringLiteral("contentRevision"), observation.value(QStringLiteral("contentRevision"))}, {QStringLiteral("contentHash"), contentHash},
        {QStringLiteral("uidKnown"), uidKnown}, {QStringLiteral("uid2Known"), uid2Known}, {QStringLiteral("missionIntReported"), true},
        {QStringLiteral("observedFirmware"), observedFirmware}});
    for (const Entry& entry : _entries) {
        const QVariantMap firmware = entry.descriptor.value(QStringLiteral("firmware")).toMap();
        bool same = true;
        for (auto it = observedFirmware.constBegin(); it != observedFirmware.constEnd(); ++it)
            same = same && firmware.value(it.key()) == it.value();
        if (!same || firmware.value(QStringLiteral("customVersionEncoding")).toString() != QStringLiteral("px4-git5-vendor3-le")
            || firmware.value(QStringLiteral("vendorVersion")).toUInt() != vendorVersion(wire)) continue;
        result.insert(QStringLiteral("matched"), true);
        for (const QString& key : {QStringLiteral("descriptorId"), QStringLiteral("adapterId"), QStringLiteral("sourceCommit"),
                                   QStringLiteral("sourceRepository"), QStringLiteral("engineeringReview")}) result.insert(key, entry.descriptor.value(key));
        result.insert(QStringLiteral("declarationReview"), entry.descriptor.value(QStringLiteral("engineeringReview")));
        result.insert(QStringLiteral("descriptorHash"), entry.descriptorHash);
        result.insert(QStringLiteral("restoredFromArchive"), entry.restored);
        result.insert(QStringLiteral("customVersionEncoding"), QStringLiteral("px4-git5-vendor3-le"));
        result.insert(QStringLiteral("encodedGitPrefix"), reversedWire(wire).left(10));
        result.insert(QStringLiteral("vendorVersion"), vendorVersion(wire));
        result.insert(QStringLiteral("matchMeaning"), QStringLiteral("local-declaration-correspondence-only"));
        return finish(QString(), true);
    }
    return finish(QStringLiteral("no-exact-local-adapter-declaration"), true);
}

QVariantMap FleetFirmwareAdapterCatalog::matchDatumRelation(const QVariantMap& observation, const QVariantMap& query) const {
    const auto reject = [](const QString& reason) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason}, {QStringLiteral("evidence"), QVariantMap()}};
    };
    if (QThread::currentThread() != thread()) return reject(QStringLiteral("catalog-owner-thread-required"));
    // Retain the original raw-version matching contract; a query cannot bypass
    // actual payload shape, freshness, capabilities or local firmware tuple.
    const QVariantMap firmwareBinding = matchObservation(observation);
    if (!actualTrue(firmwareBinding.value(QStringLiteral("ok"))) || !actualTrue(firmwareBinding.value(QStringLiteral("matched")))) {
        const QString reason = firmwareBinding.value(QStringLiteral("errorCode")).toString();
        return reject(reason.isEmpty() ? QStringLiteral("native-version-observation-unmatched") : reason);
    }
    if (!datumQueryKnown(query)) return reject(QStringLiteral("datum-relation-query-invalid"));
    const auto entry = _entries.constFind(firmwareBinding.value(QStringLiteral("descriptorId")).toString());
    if (entry == _entries.constEnd() || entry.value().descriptorHash != firmwareBinding.value(QStringLiteral("descriptorHash")).toString())
        return reject(QStringLiteral("datum-relation-catalog-binding-changed"));
    const QVariantMap descriptor = entry.value().descriptor;
    if (descriptor.value(QStringLiteral("schemaVersion")).toInt() != 2)
        return reject(QStringLiteral("datum-relation-not-declared"));
    QVariantMap selected;
    for (const QVariant& value : descriptor.value(QStringLiteral("datumRelations")).toList()) {
        const QVariantMap relation = value.toMap();
        if (datumSelector(relation) != query) continue;
        if (!selected.isEmpty()) return reject(QStringLiteral("ambiguous-local-datum-relation"));
        selected = relation;
    }
    if (selected.isEmpty()) return reject(QStringLiteral("no-exact-local-datum-relation"));
    const QVariantMap evidence{{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("source"), QStringLiteral("native-local-declared-datum-correspondence")},
        {QStringLiteral("firmwareBinding"), firmwareBinding}, {QStringLiteral("relation"), selected},
        {QStringLiteral("relationFingerprint"), hash(selected)}, {QStringLiteral("declaredCorrespondenceMatches"), true},
        {QStringLiteral("physicalDatumAlignmentCertified"), false}, {QStringLiteral("firmwareAdapterCertified"), false},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("flightCommandReleased"), false}};
    return {{QStringLiteral("ok"), true}, {QStringLiteral("reason"), QStringLiteral("")}, {QStringLiteral("evidence"), evidence}};
}

QString FleetFirmwareAdapterCatalog::exportJson() const {
    if (QThread::currentThread() != thread()) return QString();
    QVariantList descriptors;
    for (const Entry& entry : _entries) descriptors.append(entry.descriptor);
    const QVariantMap root{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("kind"), QStringLiteral("local-firmware-adapter-declarations")},
        {QStringLiteral("catalogRevision"), QString::number(_revision)}, {QStringLiteral("descriptors"), descriptors}};
    return QString::fromUtf8(QJsonDocument::fromVariant(root).toJson(QJsonDocument::Compact));
}

QVariantMap FleetFirmwareAdapterCatalog::previewImport(const QString& json, const QString& expectedRevision) const {
    QVariantMap result = status(false);
    for (const QString& key : {QStringLiteral("reason"), QStringLiteral("catalogEpoch"), QStringLiteral("revision"),
        QStringLiteral("catalogFingerprint"), QStringLiteral("archiveRevision"), QStringLiteral("normalizedJson"),
        QStringLiteral("normalizedFingerprint"), QStringLiteral("inputJsonSha256")}) result.insert(key, QString());
    result.insert(QStringLiteral("descriptors"), QVariantList());
    for (const QString& key : {QStringLiteral("addedDescriptorIds"), QStringLiteral("removedDescriptorIds"),
        QStringLiteral("changedDescriptorIds"), QStringLiteral("unchangedDescriptorIds")}) result.insert(key, QStringList());
    for (const QString& key : {QStringLiteral("addedCount"), QStringLiteral("removedCount"), QStringLiteral("changedCount"),
        QStringLiteral("unchangedCount"), QStringLiteral("descriptorCount"), QStringLiteral("previousEntryCount")}) result.insert(key, 0);
    result.insert(QStringLiteral("wholeReplacement"), true);
    result.insert(QStringLiteral("previewOnly"), true);
    result.insert(QStringLiteral("mutationCommitted"), false);
    const auto reject = [&](const QString& code) {
        result.insert(QStringLiteral("reason"), code);
        result.insert(QStringLiteral("errorCode"), code);
        return result;
    };
    if (QThread::currentThread() != thread()) return reject(QStringLiteral("catalog-owner-thread-required"));
    const QVariantMap current = snapshot();
    for (const QString& key : {QStringLiteral("catalogEpoch"), QStringLiteral("revision"), QStringLiteral("catalogFingerprint")})
        result.insert(key, current.value(key));
    result.insert(QStringLiteral("previousEntryCount"), _entries.size());
    quint64 expected = 0;
    if (!decimal(expectedRevision, &expected) || expected != _revision) return reject(QStringLiteral("catalog-revision-conflict"));
    NormalizedArchive staged;
    QString problem;
    if (!stageArchive(json, _revision, &staged, &problem)) return reject(problem);
    QStringList added, removed, changed, unchanged;
    QVariantList descriptors;
    for (auto it = staged.descriptors.constBegin(); it != staged.descriptors.constEnd(); ++it) {
        descriptors.append(it.value());
        const auto previous = _entries.constFind(it.key());
        if (previous == _entries.constEnd()) added.append(it.key());
        else if (previous.value().descriptorHash == hash(it.value()) && previous.value().descriptor == it.value()) unchanged.append(it.key());
        else changed.append(it.key());
    }
    for (auto it = _entries.constBegin(); it != _entries.constEnd(); ++it)
        if (!staged.descriptors.contains(it.key())) removed.append(it.key());
    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("archiveRevision"), QString::number(staged.revision));
    result.insert(QStringLiteral("descriptors"), descriptors);
    result.insert(QStringLiteral("normalizedJson"), QString::fromUtf8(staged.normalizedBytes));
    result.insert(QStringLiteral("normalizedFingerprint"), bytesHash(staged.normalizedBytes));
    result.insert(QStringLiteral("inputJsonSha256"), bytesHash(staged.originalBytes));
    result.insert(QStringLiteral("addedDescriptorIds"), added);
    result.insert(QStringLiteral("removedDescriptorIds"), removed);
    result.insert(QStringLiteral("changedDescriptorIds"), changed);
    result.insert(QStringLiteral("unchangedDescriptorIds"), unchanged);
    result.insert(QStringLiteral("addedCount"), added.size());
    result.insert(QStringLiteral("removedCount"), removed.size());
    result.insert(QStringLiteral("changedCount"), changed.size());
    result.insert(QStringLiteral("unchangedCount"), unchanged.size());
    result.insert(QStringLiteral("descriptorCount"), descriptors.size());
    return result;
}

QVariantMap FleetFirmwareAdapterCatalog::importJson(const QString& json, const QString& expectedRevision) {
    return _importJson(json, expectedRevision, std::function<bool()>(), false);
}

QVariantMap FleetFirmwareAdapterCatalog::importJson(const QString& json, const QString& expectedRevision,
                                                   const std::function<bool()>& finalCommitFence) {
    return _importJson(json, expectedRevision, finalCommitFence, true);
}

QVariantMap FleetFirmwareAdapterCatalog::_importJson(const QString& json, const QString& expectedRevision,
                                                    const std::function<bool()>& finalCommitFence, bool requireFence) {
    const auto reject = [&](const QString& code) {
        QVariantMap result = status(false, code);
        if (requireFence) {
            result.insert(QStringLiteral("reason"), code);
            result.insert(QStringLiteral("mutationCommitted"), false);
            result.insert(QStringLiteral("wholeReplacement"), true);
        }
        return result;
    };
    quint64 expected = 0;
    if (QThread::currentThread() != thread()) return reject(QStringLiteral("catalog-owner-thread-required"));
    if (!decimal(expectedRevision, &expected) || expected != _revision) return reject(QStringLiteral("catalog-revision-conflict"));
    if (requireFence && !finalCommitFence) return reject(QStringLiteral("catalog-final-commit-fence-required"));
    const quint64 originalRevision = _revision;
    const QString originalEpoch = _catalogEpoch;
    NormalizedArchive archive;
    QString problem;
    if (!stageArchive(json, originalRevision, &archive, &problem)) return reject(problem);
    QMap<QString, Entry> staged;
    for (auto it = archive.descriptors.constBegin(); it != archive.descriptors.constEnd(); ++it) {
        Entry entry;
        entry.descriptor = it.value(); entry.descriptorHash = hash(it.value()); entry.restored = true;
        staged.insert(it.key(), entry);
    }
    // The last callback is before the irreversible whole replacement. It may
    // destroy this catalog or recursively commit another revision; neither
    // case permits this invocation to overwrite the newer actual catalog.
    const QString nextEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString normalizedFingerprint = bytesHash(archive.normalizedBytes);
    const QString inputJsonSha256 = bytesHash(archive.originalBytes);
    QPointer<FleetFirmwareAdapterCatalog> self(this);
    const std::function<bool()> ownedFence = finalCommitFence;
    if (!self) return reject(QStringLiteral("catalog-source-destroyed-before-commit"));
    if (QThread::currentThread() != self->thread()) return reject(QStringLiteral("catalog-owner-thread-required"));
    if (self->_revision != originalRevision || self->_catalogEpoch != originalEpoch)
        return reject(QStringLiteral("catalog-changed-before-commit"));
    if (requireFence) {
        const bool allowed = ownedFence();
        if (!self) return reject(QStringLiteral("catalog-source-destroyed-before-commit"));
        if (QThread::currentThread() != self->thread()) return reject(QStringLiteral("catalog-owner-thread-required"));
        if (self->_revision != originalRevision || self->_catalogEpoch != originalEpoch)
            return reject(QStringLiteral("catalog-changed-before-commit"));
        if (!allowed) return reject(QStringLiteral("catalog-final-commit-fence-rejected"));
    }
    _entries = staged; _revision = qMax(originalRevision, archive.revision) + 1;
    _catalogEpoch = nextEpoch;
    _refreshFingerprint();
    const quint64 revision = _revision; const QString epoch = _catalogEpoch;
    QVariantMap receipt = status(true); receipt.insert(QStringLiteral("revision"), QString::number(revision)); receipt.insert(QStringLiteral("catalogEpoch"), epoch);
    if (requireFence) {
        receipt.insert(QStringLiteral("reason"), QString());
        receipt.insert(QStringLiteral("mutationCommitted"), true);
        receipt.insert(QStringLiteral("wholeReplacement"), true);
        receipt.insert(QStringLiteral("archiveRevision"), QString::number(archive.revision));
        receipt.insert(QStringLiteral("normalizedFingerprint"), normalizedFingerprint);
        receipt.insert(QStringLiteral("inputJsonSha256"), inputJsonSha256);
    }
    emit changed();
    // This is only a post-notification observation. A committed replacement
    // remains a true local fact even when changed() retires or deletes it.
    receipt.insert(QStringLiteral("stillCurrent"), self && QThread::currentThread() == self->thread()
        && self->_revision == revision && self->_catalogEpoch == epoch);
    return receipt;
}

bool FleetFirmwareAdapterCatalog::saveToFile(const QString& filePath) const {
    if (QThread::currentThread() != thread() || !text(filePath, 4096)) return false;
    const QByteArray bytes = exportJson().toUtf8();
    if (bytes.isEmpty() || bytes.size() > MaxBytes) return false;
    QSaveFile file(filePath); // Direct-write fallback remains disabled.
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
}

QVariantMap FleetFirmwareAdapterCatalog::loadFromFile(const QString& filePath, const QString& expectedRevision) {
    if (QThread::currentThread() != thread() || !text(filePath, 4096)) return status(false, QStringLiteral("catalog-file-path-invalid"));
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return status(false, QStringLiteral("catalog-file-unavailable"));
    const qint64 size = file.size();
    if (size < 0 || size > MaxBytes) return status(false, QStringLiteral("catalog-file-byte-limit"));
    const QByteArray bytes = file.read(MaxBytes + 1);
    if (bytes.size() != size || file.size() != size || bytes.size() > MaxBytes) return status(false, QStringLiteral("catalog-file-changed-or-truncated"));
    const QString json = QString::fromUtf8(bytes);
    if (json.toUtf8() != bytes) return status(false, QStringLiteral("catalog-file-utf8-invalid"));
    return importJson(json, expectedRevision);
}
