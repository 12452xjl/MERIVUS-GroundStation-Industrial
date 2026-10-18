#include "FleetMissionClearanceInput.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QVector>

#include <cmath>
#include <initializer_list>

namespace {
constexpr qint64 MaximumBytes = 4LL * 1024 * 1024;
constexpr int MaximumDepth = 16;
constexpr int MaximumNodes = 131072;
constexpr double MaximumExactInteger = 9007199254740991.0;

bool actualMap(const QVariant& value)
{
    return value.userType() == QMetaType::QVariantMap && !value.isNull();
}

bool actualList(const QVariant& value)
{
    return value.userType() == QMetaType::QVariantList && !value.isNull();
}

bool exact(const QVariantMap& value, std::initializer_list<const char*> names)
{
    if (value.size() != int(names.size())) return false;
    for (const char* name : names) if (!value.contains(QString::fromLatin1(name))) return false;
    return true;
}

bool boundedTree(const QVariant& value, int depth, int* nodes, qint64* textBytes)
{
    if (depth > MaximumDepth || ++*nodes > MaximumNodes || !value.isValid()) return false;
    // QString's ordinary empty value is compatible with native JSON text;
    // field validators below still require their declared nonempty strings.
    if (value.userType() == QMetaType::QString) {
        const QString string = value.toString();
        if (string.size() > MaximumBytes) return false;
        const QByteArray utf8 = string.toUtf8();
        if (QString::fromUtf8(utf8) != string || utf8.size() > MaximumBytes - *textBytes) return false;
        *textBytes += utf8.size();
        return true;
    }
    // No null field exists in this schema. Reject typed nulls and opaque
    // pointers before QVariant conversion can silently manufacture defaults.
    if (value.isNull()) return false;
    switch (value.userType()) {
    case QMetaType::QVariantMap: {
        const QVariantMap map = value.toMap();
        if (map.size() > MaximumNodes - *nodes) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > MaximumBytes) return false;
            const QByteArray key = it.key().toUtf8();
            if (QString::fromUtf8(key) != it.key() || key.size() > MaximumBytes - *textBytes) return false;
            *textBytes += key.size();
            if (!boundedTree(it.value(), depth + 1, nodes, textBytes)) return false;
        }
        return true;
    }
    case QMetaType::QVariantList: {
        const QVariantList list = value.toList();
        if (list.size() > MaximumNodes - *nodes) return false;
        for (const QVariant& child : list) if (!boundedTree(child, depth + 1, nodes, textBytes)) return false;
        return true;
    }
    case QMetaType::Bool: return true;
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Double: {
        const double number = value.toDouble();
        return std::isfinite(number) && std::abs(number) <= MaximumExactInteger;
    }
    default: return false;
    }
}

bool bounded(const QVariant& value)
{
    int nodes = 0;
    qint64 textBytes = 0;
    return boundedTree(value, 0, &nodes, &textBytes)
        && QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact).size() <= MaximumBytes;
}

bool integer(const QVariant& value, qint64 minimum, qint64 maximum, qint64* result)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.userType()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < double(minimum) || number > double(maximum)
        || std::floor(number) != number) return false;
    *result = qint64(number);
    return true;
}

bool text(const QVariant& value, int maximum = 128)
{
    if (value.userType() != QMetaType::QString || value.isNull()) return false;
    const QString string = value.toString();
    if (string.isEmpty() || string.size() > maximum || string != string.trimmed()
        || QString::fromUtf8(string.toUtf8()) != string) return false;
    for (QChar ch : string) if (ch.isNull() || ch.category() == QChar::Other_Control) return false;
    return true;
}

bool constant(const QVariant& value, const char* expected)
{
    return text(value) && value.toString() == QString::fromLatin1(expected);
}

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (QChar ch : value.toString()) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

bool revision(const QVariant& value)
{
    if (!text(value, 20)) return false;
    const QString string = value.toString();
    for (QChar ch : string) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool accepted = false;
    const quint64 number = string.toULongLong(&accepted);
    return accepted && number != 0 && QString::number(number) == string;
}

bool utc(const QVariant& value, QDateTime* date)
{
    if (!text(value, 24) || value.toString().size() != 24 || !value.toString().endsWith(QLatin1Char('Z'))) return false;
    const QDateTime parsed = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
    if (!parsed.isValid() || parsed.toUTC().toString(Qt::ISODateWithMs) != value.toString()) return false;
    *date = parsed;
    return true;
}

QVariantMap rejected(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason},
        {QStringLiteral("input"), QVariantMap()}, {QStringLiteral("fingerprint"), QStringLiteral("")}};
}

bool localFrame(const QVariant& input, QVariantMap* result)
{
    if (!actualMap(input)) return false;
    const QVariantMap frame = input.toMap();
    if (!exact(frame, {"modelName", "originLatitudeE7", "originLongitudeE7", "northNanometresPerLatitudeE7",
            "eastNanometresPerLongitudeE7", "maximumLatitudeDeltaE7", "maximumLongitudeDeltaE7"})
        || !constant(frame.value(QStringLiteral("modelName")), "engineer-declared-local-affine-geodetic-chart")) return false;
    qint64 latitude = 0, longitude = 0, northScale = 0, eastScale = 0, latitudeDelta = 0, longitudeDelta = 0;
    if (!integer(frame.value(QStringLiteral("originLatitudeE7")), -800000000, 800000000, &latitude)
        || !integer(frame.value(QStringLiteral("originLongitudeE7")), -1800000000, 1800000000, &longitude)
        || !integer(frame.value(QStringLiteral("northNanometresPerLatitudeE7")), 1, 200000000, &northScale)
        || !integer(frame.value(QStringLiteral("eastNanometresPerLongitudeE7")), 1, 200000000, &eastScale)
        || !integer(frame.value(QStringLiteral("maximumLatitudeDeltaE7")), 1, 2000000, &latitudeDelta)
        || !integer(frame.value(QStringLiteral("maximumLongitudeDeltaE7")), 1, 2000000, &longitudeDelta)
        || latitude - latitudeDelta < -800000000 || latitude + latitudeDelta > 800000000
        || longitude - longitudeDelta < -1800000000 || longitude + longitudeDelta > 1800000000) return false;
    *result = {{QStringLiteral("modelName"), frame.value(QStringLiteral("modelName"))},
        {QStringLiteral("originLatitudeE7"), double(latitude)}, {QStringLiteral("originLongitudeE7"), double(longitude)},
        {QStringLiteral("northNanometresPerLatitudeE7"), double(northScale)},
        {QStringLiteral("eastNanometresPerLongitudeE7"), double(eastScale)},
        {QStringLiteral("maximumLatitudeDeltaE7"), double(latitudeDelta)},
        {QStringLiteral("maximumLongitudeDeltaE7"), double(longitudeDelta)}};
    return true;
}

struct Point { qint64 east; qint64 north; };

bool polygon(const QVariant& input, QVariantList* result)
{
    if (!actualList(input)) return false;
    const QVariantList values = input.toList();
    if (values.size() < 3 || values.size() > 16) return false;
    QVector<Point> points;
    QVariantList normalized;
    for (const QVariant& value : values) {
        if (!actualMap(value) || !exact(value.toMap(), {"eastMm", "northMm"})) return false;
        const QVariantMap point = value.toMap();
        Point parsed{};
        if (!integer(point.value(QStringLiteral("eastMm")), -50000000, 50000000, &parsed.east)
            || !integer(point.value(QStringLiteral("northMm")), -50000000, 50000000, &parsed.north)) return false;
        for (const Point& other : points) if (parsed.east == other.east && parsed.north == other.north) return false;
        points.append(parsed);
        normalized.append(QVariantMap{{QStringLiteral("eastMm"), double(parsed.east)}, {QStringLiteral("northMm"), double(parsed.north)}});
    }
    // Every other vertex must be strictly to the left of every oriented edge.
    // Differences <=1e8, products <=1e16, subtraction <=2e16: exact qint64.
    // This rejects clockwise, concave, collinear and self-crossing input.
    for (int i = 0; i < points.size(); ++i) {
        const int next = (i + 1) % points.size();
        const Point& a = points.at(i);
        const Point& b = points.at(next);
        for (int j = 0; j < points.size(); ++j) {
            if (j == i || j == next) continue;
            const Point& c = points.at(j);
            const qint64 cross = (b.east - a.east) * (c.north - a.north)
                - (b.north - a.north) * (c.east - a.east);
            if (cross <= 0) return false;
        }
    }
    *result = normalized;
    return true;
}

bool volumes(const QVariant& input, int minimum, int maximum, bool terrain,
    QSet<QString>* identifiers, QVariantList* result)
{
    if (!actualList(input)) return false;
    const QVariantList values = input.toList();
    if (values.size() < minimum || values.size() > maximum || identifiers->size() + values.size() > 1024) return false;
    QVariantList normalized;
    const QString idKey = terrain ? QStringLiteral("cellId") : QStringLiteral("volumeId");
    for (const QVariant& value : values) {
        if (!actualMap(value)) return false;
        const QVariantMap row = value.toMap();
        if (!(terrain ? exact(row, {"cellId", "polygon", "maximumTerrainAmslMm"})
            : exact(row, {"volumeId", "polygon", "minimumAmslMm", "maximumAmslMm"}))
            || !text(row.value(idKey)) || identifiers->contains(row.value(idKey).toString())) return false;
        QVariantList points;
        if (!polygon(row.value(QStringLiteral("polygon")), &points)) return false;
        QVariantMap output{{idKey, row.value(idKey)}, {QStringLiteral("polygon"), points}};
        qint64 lower = 0, upper = 0;
        if (terrain) {
            if (!integer(row.value(QStringLiteral("maximumTerrainAmslMm")), -1000000, 20000000, &upper)) return false;
            output.insert(QStringLiteral("maximumTerrainAmslMm"), double(upper));
        } else {
            if (!integer(row.value(QStringLiteral("minimumAmslMm")), -1000000, 20000000, &lower)
                || !integer(row.value(QStringLiteral("maximumAmslMm")), -1000000, 20000000, &upper) || lower >= upper) return false;
            output.insert(QStringLiteral("minimumAmslMm"), double(lower));
            output.insert(QStringLiteral("maximumAmslMm"), double(upper));
        }
        identifiers->insert(row.value(idKey).toString());
        normalized.append(output);
    }
    *result = normalized;
    return true;
}

bool scene(const QVariant& input, QVariantMap* result)
{
    if (!actualMap(input)) return false;
    const QVariantMap value = input.toMap();
    if (!exact(value, {"sceneId", "revision", "sourceLabel", "sourceDocumentHash", "acquiredAtUtc", "validUntilUtc",
            "horizontalDatum", "verticalDatumId", "localFrame", "coverageVolumes", "obstacleVolumes", "terrainCells", "corridorVolumes"})
        || !text(value.value(QStringLiteral("sceneId"))) || !revision(value.value(QStringLiteral("revision")))
        || !text(value.value(QStringLiteral("sourceLabel"))) || !digest(value.value(QStringLiteral("sourceDocumentHash")))
        || !constant(value.value(QStringLiteral("horizontalDatum")), "wgs84") || !text(value.value(QStringLiteral("verticalDatumId")))) return false;
    QDateTime acquired, until;
    if (!utc(value.value(QStringLiteral("acquiredAtUtc")), &acquired) || !utc(value.value(QStringLiteral("validUntilUtc")), &until)
        || until <= acquired) return false;
    QVariantMap frame;
    QVariantList coverage, obstacles, terrain, corridors;
    QSet<QString> identifiers;
    if (!localFrame(value.value(QStringLiteral("localFrame")), &frame)
        || !volumes(value.value(QStringLiteral("coverageVolumes")), 1, 128, false, &identifiers, &coverage)
        || !volumes(value.value(QStringLiteral("obstacleVolumes")), 0, 512, false, &identifiers, &obstacles)
        || !volumes(value.value(QStringLiteral("terrainCells")), 1, 256, true, &identifiers, &terrain)
        || !volumes(value.value(QStringLiteral("corridorVolumes")), 1, 128, false, &identifiers, &corridors)) return false;
    *result = value;
    result->insert(QStringLiteral("localFrame"), frame);
    result->insert(QStringLiteral("coverageVolumes"), coverage);
    result->insert(QStringLiteral("obstacleVolumes"), obstacles);
    result->insert(QStringLiteral("terrainCells"), terrain);
    result->insert(QStringLiteral("corridorVolumes"), corridors);
    return true;
}

bool vehicles(const QVariant& input, qint64 version, QVariantList* result)
{
    if (!actualList(input)) return false;
    const QVariantList values = input.toList();
    if (values.isEmpty() || values.size() > 32) return false;
    QSet<int> ids;
    QVariantList normalized;
    for (const QVariant& value : values) {
        if (!actualMap(value)) return false;
        const QVariantMap row = value.toMap();
        const bool fieldsMatch = version == 1
            ? exact(row, {"vehicleId", "configurationFingerprint", "bodyHorizontalRadiusMm", "bodyVerticalHalfExtentMm",
                "horizontalTrackingErrorMm", "verticalTrackingErrorMm"})
            : exact(row, {"vehicleId", "configurationFingerprint", "bodyHorizontalRadiusMm", "bodyVerticalHalfExtentMm",
                "horizontalTrackingErrorMm", "verticalTrackingErrorMm", "supportHorizontalRadiusMm",
                "supportBelowReferenceMm", "supportAboveReferenceMm"});
        if (!fieldsMatch || !digest(row.value(QStringLiteral("configurationFingerprint")))) return false;
        qint64 id = 0;
        if (!integer(row.value(QStringLiteral("vehicleId")), 1, 255, &id) || ids.contains(int(id))) return false;
        ids.insert(int(id));
        QVariantMap output{{QStringLiteral("vehicleId"), double(id)},
            {QStringLiteral("configurationFingerprint"), row.value(QStringLiteral("configurationFingerprint"))}};
        for (const QString& key : {QStringLiteral("bodyHorizontalRadiusMm"), QStringLiteral("bodyVerticalHalfExtentMm"),
                QStringLiteral("horizontalTrackingErrorMm"), QStringLiteral("verticalTrackingErrorMm")}) {
            qint64 amount = 0;
            const bool body = key == QStringLiteral("bodyHorizontalRadiusMm") || key == QStringLiteral("bodyVerticalHalfExtentMm");
            if (!integer(row.value(key), body ? 1 : 0, 100000, &amount)) return false;
            output.insert(key, double(amount));
        }
        if (version == 2) {
            for (const QString& key : {QStringLiteral("supportHorizontalRadiusMm"), QStringLiteral("supportBelowReferenceMm"),
                    QStringLiteral("supportAboveReferenceMm")}) {
                qint64 amount = 0;
                if (!integer(row.value(key), key == QStringLiteral("supportAboveReferenceMm") ? 0 : 1, 100000, &amount)) return false;
                output.insert(key, double(amount));
            }
        }
        normalized.append(output);
    }
    *result = normalized;
    return true;
}

bool contacts(const QVariant& input, const QVariantMap& normalizedScene,
    const QVariantList& normalizedVehicles, QVariantList* result)
{
    if (!actualList(input)) return false;
    const QVariantList values = input.toList();
    if (values.size() > 64) return false;
    QSet<int> vehicleIds;
    for (const QVariant& value : normalizedVehicles)
        vehicleIds.insert(value.toMap().value(QStringLiteral("vehicleId")).toInt());
    // These are already completely normalized scene cells, not a second
    // source or a guessed ground elevation. Equal maxima remain declarations.
    QMap<QString, qint64> terrainHeights;
    for (const QVariant& value : normalizedScene.value(QStringLiteral("terrainCells")).toList()) {
        const QVariantMap cell = value.toMap();
        terrainHeights.insert(cell.value(QStringLiteral("cellId")).toString(),
            cell.value(QStringLiteral("maximumTerrainAmslMm")).toLongLong());
    }
    QSet<QString> contactIds, vehicleKinds;
    QVariantList normalized;
    for (const QVariant& value : values) {
        if (!actualMap(value)) return false;
        const QVariantMap row = value.toMap();
        if (!exact(row, {"contactId", "vehicleId", "kind", "payloadHash", "missionSequenceNumber", "surfaceModel",
                "referencePoint", "padPolygon", "terrainCellIds", "wireGroundToleranceMm",
                "referenceHeightToleranceMm", "maximumSupportPenetrationMm"})
            || !text(row.value(QStringLiteral("contactId")))
            || contactIds.contains(row.value(QStringLiteral("contactId")).toString())
            || !digest(row.value(QStringLiteral("payloadHash")))
            || !constant(row.value(QStringLiteral("surfaceModel")), "engineer-declared-flat-contact-pad")
            || (!constant(row.value(QStringLiteral("kind")), "takeoff")
                && !constant(row.value(QStringLiteral("kind")), "landing"))) return false;
        qint64 id = 0, sequence = 0;
        if (!integer(row.value(QStringLiteral("vehicleId")), 1, 255, &id) || !vehicleIds.contains(int(id))
            || !integer(row.value(QStringLiteral("missionSequenceNumber")), 0, 2047, &sequence)) return false;
        const QString pair = QString::number(id) + QLatin1Char('/') + row.value(QStringLiteral("kind")).toString();
        if (vehicleKinds.contains(pair)) return false;
        if (!actualMap(row.value(QStringLiteral("referencePoint")))) return false;
        const QVariantMap reference = row.value(QStringLiteral("referencePoint")).toMap();
        qint64 latitude = 0, longitude = 0, ground = 0;
        if (!exact(reference, {"latitudeE7", "longitudeE7", "groundAmslMm"})
            || !integer(reference.value(QStringLiteral("latitudeE7")), -800000000, 800000000, &latitude)
            || !integer(reference.value(QStringLiteral("longitudeE7")), -1800000000, 1800000000, &longitude)
            || !integer(reference.value(QStringLiteral("groundAmslMm")), -1000000, 20000000, &ground)) return false;
        QVariantList pad;
        if (!polygon(row.value(QStringLiteral("padPolygon")), &pad)
            || !actualList(row.value(QStringLiteral("terrainCellIds")))) return false;
        const QVariantList terrainIds = row.value(QStringLiteral("terrainCellIds")).toList();
        if (terrainIds.isEmpty() || terrainIds.size() > 256) return false;
        QSet<QString> usedTerrain;
        QVariantList normalizedTerrainIds;
        for (const QVariant& terrainId : terrainIds) {
            if (!text(terrainId)) return false;
            const QString key = terrainId.toString();
            if (usedTerrain.contains(key) || !terrainHeights.contains(key) || terrainHeights.value(key) != ground) return false;
            usedTerrain.insert(key);
            normalizedTerrainIds.append(key);
        }
        QVariantMap output{{QStringLiteral("contactId"), row.value(QStringLiteral("contactId"))},
            {QStringLiteral("vehicleId"), double(id)}, {QStringLiteral("kind"), row.value(QStringLiteral("kind"))},
            {QStringLiteral("payloadHash"), row.value(QStringLiteral("payloadHash"))},
            {QStringLiteral("missionSequenceNumber"), double(sequence)},
            {QStringLiteral("surfaceModel"), row.value(QStringLiteral("surfaceModel"))},
            {QStringLiteral("referencePoint"), QVariantMap{{QStringLiteral("latitudeE7"), double(latitude)},
                {QStringLiteral("longitudeE7"), double(longitude)}, {QStringLiteral("groundAmslMm"), double(ground)}}},
            {QStringLiteral("padPolygon"), pad}, {QStringLiteral("terrainCellIds"), normalizedTerrainIds}};
        for (const QString& key : {QStringLiteral("wireGroundToleranceMm"), QStringLiteral("referenceHeightToleranceMm"),
                QStringLiteral("maximumSupportPenetrationMm")}) {
            qint64 amount = 0;
            if (!integer(row.value(key), 0, 100000, &amount)) return false;
            output.insert(key, double(amount));
        }
        contactIds.insert(row.value(QStringLiteral("contactId")).toString());
        vehicleKinds.insert(pair);
        normalized.append(output);
    }
    *result = normalized;
    return true;
}

bool policy(const QVariant& input, QVariantMap* result)
{
    if (!actualMap(input)) return false;
    const QVariantMap value = input.toMap();
    if (!exact(value, {"horizontalSafetyMarginMm", "verticalSafetyMarginMm", "horizontalSceneErrorMm", "verticalSceneErrorMm",
            "horizontalProjectionErrorMm", "verticalDatumAlignmentErrorMm", "datumAlignmentDeclared", "contactHandling", "coverageRequirement"})
        || value.value(QStringLiteral("datumAlignmentDeclared")).userType() != QMetaType::Bool
        || value.value(QStringLiteral("datumAlignmentDeclared")).isNull() || !value.value(QStringLiteral("datumAlignmentDeclared")).toBool()
        || !constant(value.value(QStringLiteral("contactHandling")), "require-explicit-contact-context")
        || !constant(value.value(QStringLiteral("coverageRequirement")), "entire-swept-envelope")) return false;
    QVariantMap normalized = value;
    for (const QString& key : {QStringLiteral("horizontalSafetyMarginMm"), QStringLiteral("verticalSafetyMarginMm"),
            QStringLiteral("horizontalSceneErrorMm"), QStringLiteral("verticalSceneErrorMm"),
            QStringLiteral("horizontalProjectionErrorMm"), QStringLiteral("verticalDatumAlignmentErrorMm")}) {
        qint64 amount = 0;
        if (!integer(value.value(key), 0, 100000, &amount)) return false;
        normalized.insert(key, double(amount));
    }
    *result = normalized;
    return true;
}

bool membersMatch(const QVariant& input, const QVariantList& declarations)
{
    if (!actualList(input)) return false;
    const QVariantList members = input.toList();
    if (members.isEmpty() || members.size() > 32 || members.size() != declarations.size()) return false;
    QSet<int> ids;
    for (int index = 0; index < members.size(); ++index) {
        if (!actualMap(members.at(index))) return false;
        qint64 id = 0;
        if (!integer(members.at(index).toMap().value(QStringLiteral("vehicleId")), 1, 255, &id)
            || ids.contains(int(id)) || double(id) != declarations.at(index).toMap().value(QStringLiteral("vehicleId")).toDouble()) return false;
        ids.insert(int(id));
    }
    return true;
}
} // namespace

QVariantMap FleetMissionClearanceInput::normalize(const QVariant& source)
{
    if (!actualMap(source) || !bounded(source)) return rejected(QStringLiteral("clearance-input-type-value-or-budget-invalid"));
    const QVariantMap input = source.toMap();
    qint64 version = 0;
    if (!integer(input.value(QStringLiteral("schemaVersion")), 1, 2, &version)
        || !(version == 1 ? exact(input, {"schemaVersion", "scene", "vehicles", "policy"})
            : exact(input, {"schemaVersion", "scene", "vehicles", "policy", "contacts"})))
        return rejected(QStringLiteral("clearance-input-schema-invalid"));
    QVariantMap normalizedScene, normalizedPolicy;
    QVariantList normalizedVehicles;
    if (!scene(input.value(QStringLiteral("scene")), &normalizedScene)) return rejected(QStringLiteral("clearance-input-scene-invalid"));
    if (!vehicles(input.value(QStringLiteral("vehicles")), version, &normalizedVehicles)) return rejected(QStringLiteral("clearance-input-vehicles-invalid"));
    if (!policy(input.value(QStringLiteral("policy")), &normalizedPolicy)) return rejected(QStringLiteral("clearance-input-policy-invalid"));
    QVariantMap normalized{{QStringLiteral("schemaVersion"), double(version)}, {QStringLiteral("scene"), normalizedScene},
        {QStringLiteral("vehicles"), normalizedVehicles}, {QStringLiteral("policy"), normalizedPolicy}};
    if (version == 2) {
        QVariantList normalizedContacts;
        if (!contacts(input.value(QStringLiteral("contacts")), normalizedScene, normalizedVehicles, &normalizedContacts))
            return rejected(QStringLiteral("clearance-input-contacts-invalid"));
        normalized.insert(QStringLiteral("contacts"), normalizedContacts);
    }
    if (!bounded(normalized)) return rejected(QStringLiteral("clearance-input-normalized-budget-invalid"));
    const QByteArray bytes = QJsonDocument::fromVariant(normalized).toJson(QJsonDocument::Compact);
    return {{QStringLiteral("ok"), true}, {QStringLiteral("reason"), QStringLiteral("")}, {QStringLiteral("input"), normalized},
        {QStringLiteral("fingerprint"), QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())}};
}

QVariantMap FleetMissionClearanceInput::inspectForPlan(const QVariantMap& plan)
{
    QVariantMap result{{QStringLiteral("provided"), false}, {QStringLiteral("inputValid"), true},
        {QStringLiteral("reason"), QStringLiteral("")}, {QStringLiteral("input"), QVariantMap()},
        {QStringLiteral("fingerprint"), QStringLiteral("")}, {QStringLiteral("memberIdsMatch"), false}, {QStringLiteral("snapshotOnly"), true}};
    if (!plan.contains(QStringLiteral("constraints"))) return result;
    const QVariant constraintsValue = plan.value(QStringLiteral("constraints"));
    if (!actualMap(constraintsValue)) {
        result.insert(QStringLiteral("inputValid"), false);
        result.insert(QStringLiteral("reason"), QStringLiteral("clearance-plan-constraints-invalid"));
        return result;
    }
    const QVariantMap constraints = constraintsValue.toMap();
    if (!constraints.contains(QStringLiteral("clearanceInput"))) return result;
    result.insert(QStringLiteral("provided"), true);
    const QVariantMap checked = normalize(constraints.value(QStringLiteral("clearanceInput")));
    if (!checked.value(QStringLiteral("ok")).toBool()) {
        result.insert(QStringLiteral("inputValid"), false);
        result.insert(QStringLiteral("reason"), checked.value(QStringLiteral("reason")));
        return result;
    }
    const QVariantMap input = checked.value(QStringLiteral("input")).toMap();
    result.insert(QStringLiteral("input"), input);
    result.insert(QStringLiteral("fingerprint"), checked.value(QStringLiteral("fingerprint")));
    const bool matched = membersMatch(plan.value(QStringLiteral("members")), input.value(QStringLiteral("vehicles")).toList());
    result.insert(QStringLiteral("memberIdsMatch"), matched);
    result.insert(QStringLiteral("inputValid"), matched);
    if (!matched) result.insert(QStringLiteral("reason"), QStringLiteral("clearance-plan-active-member-ids-or-order-mismatch"));
    return result;
}
