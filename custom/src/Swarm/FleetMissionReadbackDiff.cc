#include "FleetMissionReadbackDiff.h"

#include "FleetMissionPayloadBuilder.h"
#include "FleetMissionHeadingEncoding.h"
#include "QGCMAVLink.h"

#include <QMap>
#include <QMetaType>
#include <QStringList>

#include <cmath>

namespace {
constexpr int MaximumItems = 2048;
constexpr int MaximumRows = 64;
constexpr double MaximumExactInteger = 9007199254740991.0;

const QStringList& fields()
{
    static const QStringList value{QStringLiteral("sequenceNumber"), QStringLiteral("command"), QStringLiteral("frame"),
        QStringLiteral("autoContinue"), QStringLiteral("isCurrentItem"), QStringLiteral("param1"), QStringLiteral("param2"),
        QStringLiteral("param3"), QStringLiteral("param4"), QStringLiteral("param5"), QStringLiteral("param6"),
        QStringLiteral("param7"), QStringLiteral("yawMode"), QStringLiteral("latitudeE7"), QStringLiteral("longitudeE7"),
        QStringLiteral("param5Integer"), QStringLiteral("param6Integer")};
    return value;
}

QString valueType(const QVariant& value)
{
    switch (value.type()) {
    case QVariant::Invalid: return QStringLiteral("invalid");
    case QVariant::Bool: return QStringLiteral("bool");
    case QVariant::Int: return QStringLiteral("int");
    case QVariant::UInt: return QStringLiteral("uint");
    case QVariant::LongLong: return QStringLiteral("int64");
    case QVariant::ULongLong: return QStringLiteral("uint64");
    case QVariant::Double: return QStringLiteral("double");
    case QVariant::String: return QStringLiteral("string");
    case QVariant::Map: return QStringLiteral("map");
    case QVariant::List: return QStringLiteral("list");
    default: return QStringLiteral("other");
    }
}

struct Scalar {
    QString state = QStringLiteral("missing");
    QString type = QStringLiteral("invalid");
    QVariant value;
    bool comparable = true;
};

Scalar scalar(const QVariantMap& item, const QString& field)
{
    Scalar result;
    if (!item.contains(field)) return result;
    const QVariant value = item.value(field);
    result.type = valueType(value);
    if (!value.isValid()) { result.state = QStringLiteral("null"); return result; }
    if (value.isNull()) { result.state = QStringLiteral("typed-null"); return result; }
    result.state = QStringLiteral("present");
    switch (value.type()) {
    case QVariant::Bool: result.value = value.toBool(); return result;
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double: {
        const double number = value.toDouble();
        if (std::isfinite(number) && std::abs(number) <= MaximumExactInteger) {
            // Preserve type separately; safe JSON numeric value is exact for
            // every accepted integer and finite for decoded floating points.
            result.value = number; return result;
        }
        result.state = QStringLiteral("invalid-number"); result.comparable = false; return result;
    }
    case QVariant::String: {
        const QString text = value.toString();
        if (field == QStringLiteral("yawMode") && (text == QStringLiteral("system-default") || text == QStringLiteral("absolute-degrees")
            || text == QStringLiteral("not-applicable") || text == QStringLiteral("unsupported-command"))) {
            result.value = text; return result;
        }
        break;
    }
    default: break;
    }
    result.state = QStringLiteral("unsupported-value"); result.comparable = false;
    return result;
}

QString canonicalError(const QVariantMap& canonical)
{
    const QString value = canonical.value(QStringLiteral("errorCode")).toString();
    const QStringList allowed{QStringLiteral("wire_resource_limit"), QStringLiteral("invalid_wire_item"),
        QStringLiteral("unsupported_wire_command"), QStringLiteral("invalid_wire_parameter"),
        QStringLiteral("unsupported_wire_parameter"), QStringLiteral("invalid_wire_coordinate")};
    return allowed.contains(value) ? value : QStringLiteral("diagnostic-canonical-unavailable");
}

QVariantMap normalize(const QVariantList& items, bool observed)
{
    if (items.size() > MaximumItems) return {{QStringLiteral("ok"), false}, {QStringLiteral("errorCode"), QStringLiteral("wire_resource_limit")}};
    for (int index = 0; index < items.size(); ++index) {
        if (items.at(index).type() != QVariant::Map || items.at(index).isNull() || items.at(index).toMap().size() > 16)
            return {{QStringLiteral("ok"), false}, {QStringLiteral("errorCode"), QStringLiteral("invalid_wire_item")}, {QStringLiteral("pointIndex"), index}};
        const QVariantMap item = items.at(index).toMap();
        for (auto it = item.constBegin(); it != item.constEnd(); ++it) {
            // Do not let typed-null QVariant coercion turn unknown into 0/false.
            // Explicit untyped null yaw remains the Builder's supported value.
            if (it.value().isValid() && it.value().isNull()) return {{QStringLiteral("ok"), false},
                {QStringLiteral("errorCode"), QStringLiteral("invalid_wire_item")}, {QStringLiteral("pointIndex"), index}};
        }
    }
    return FleetMissionPayloadBuilder::canonicalWireItems(items, observed);
}

bool canonicalOk(const QVariantMap& value)
{
    return value.value(QStringLiteral("ok")).type() == QVariant::Bool && value.value(QStringLiteral("ok")).toBool();
}

int errorIndex(const QVariantMap& value)
{
    return value.value(QStringLiteral("pointIndex")).type() == QVariant::Int
        && value.value(QStringLiteral("pointIndex")).toInt() >= 0 && value.value(QStringLiteral("pointIndex")).toInt() < MaximumItems
        ? value.value(QStringLiteral("pointIndex")).toInt() : -1;
}

QVariantMap base(int expected, int observed)
{
    return {{QStringLiteral("format"), QStringLiteral("merivus-mission-readback-diff")}, {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("sourceRepresentation"), QStringLiteral("ordinary-decoded-mission-items-not-raw-int-proof")},
        {QStringLiteral("state"), QStringLiteral("unknown")}, {QStringLiteral("comparisonBasis"), QStringLiteral("none")},
        {QStringLiteral("reason"), QStringLiteral("input-item-limit")},
        {QStringLiteral("expectedItemCount"), expected}, {QStringLiteral("observedItemCount"), observed},
        {QStringLiteral("expectedCanonicalValid"), false}, {QStringLiteral("observedCanonicalValid"), false},
        {QStringLiteral("expectedContentHash"), QStringLiteral("")}, {QStringLiteral("observedContentHash"), QStringLiteral("")},
        {QStringLiteral("expectedErrorCode"), QStringLiteral("")}, {QStringLiteral("observedErrorCode"), QStringLiteral("")},
        {QStringLiteral("expectedErrorItemIndex"), -1}, {QStringLiteral("observedErrorItemIndex"), -1},
        {QStringLiteral("comparedItemCount"), 0}, {QStringLiteral("uncomparedItemCount"), qMax(expected, observed)},
        {QStringLiteral("differenceCount"), 0}, {QStringLiteral("differenceCountComplete"), false},
        {QStringLiteral("displayedDifferenceCount"), 0}, {QStringLiteral("rowsTruncated"), false}, {QStringLiteral("rows"), QVariantList{}},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("currentOnboardContentsProven"), false}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

bool navHeading(const QVariant& value)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap row = value.toMap();
    const Scalar command = scalar(row, QStringLiteral("command"));
    if (!command.comparable || command.state != QStringLiteral("present")
        || command.value.toDouble() != MAV_CMD_NAV_WAYPOINT) return false;
    return row.value(QStringLiteral("yawMode")) == QVariant(QStringLiteral("absolute-degrees"))
        || (row.value(QStringLiteral("param4")).isValid() && !row.value(QStringLiteral("param4")).isNull());
}

bool hasHeading(const QVariantList& items)
{
    for (const QVariant& item : items) if (navHeading(item)) return true;
    return false;
}

QVariant finiteScalar(const QVariantMap& row, const QString& key)
{
    const QVariant original = row.value(key);
    // Every actual float32 is exactly representable as a double, including
    // illegal finite heading values larger than the generic integer-safe
    // display range. Retain them for diagnosis; this grants no canonical
    // acceptance. Integer carriers keep the legacy exact-number bound below.
    if (original.type() == QVariant::Double && !original.isNull() && std::isfinite(original.toDouble()))
        return original;
    const Scalar value = scalar(row, key);
    return value.state == QStringLiteral("present") && value.comparable
            && (value.type == QStringLiteral("int") || value.type == QStringLiteral("uint")
                || value.type == QStringLiteral("int64") || value.type == QStringLiteral("uint64")
                || value.type == QStringLiteral("double"))
        ? value.value : QVariant();
}

QVariantMap headingObservations(const QVariantList& expected, const QVariantList& observed)
{
    QVariantList rows;
    int count = 0;
    for (int index = 0; index < qMax(expected.size(), observed.size()); ++index) {
        const QVariant left = index < expected.size() ? expected.at(index) : QVariant();
        const QVariant right = index < observed.size() ? observed.at(index) : QVariant();
        if (!navHeading(left) && !navHeading(right)) continue;
        ++count;
        if (rows.size() >= MaximumRows) continue;
        const QVariantMap source = left.toMap(), actual = right.toMap();
        const QVariant actualValue = finiteScalar(actual, QStringLiteral("param4"));
        QString actualState = !actual.contains(QStringLiteral("param4")) ? QStringLiteral("missing")
            : !actual.value(QStringLiteral("param4")).isValid()
                || actual.value(QStringLiteral("param4")).userType() == QMetaType::Nullptr ? QStringLiteral("system-default")
            : actualValue.isValid() ? QStringLiteral("finite") : QStringLiteral("invalid");
        QVariant expectedCanonical, canonical;
        QString expectedError, error;
        const bool expectedKnown = navHeading(left)
            && FleetMissionHeadingEncoding::canonicalParam4(MAV_CMD_NAV_WAYPOINT, source.value(QStringLiteral("param4")),
                source.value(QStringLiteral("yawMode")), false, expectedCanonical, expectedError)
            && expectedCanonical.isValid();
        const bool canonicalKnown = navHeading(right)
            && FleetMissionHeadingEncoding::canonicalParam4(MAV_CMD_NAV_WAYPOINT, actual.value(QStringLiteral("param4")),
                actual.value(QStringLiteral("yawMode")), true, canonical, error) && canonical.isValid();
        // Preserve a malformed finite expectation for diagnostics as well;
        // only expectedKnown can credit it as a legal encoded tick.
        const QVariant expectedValue = navHeading(left) ? finiteScalar(source, QStringLiteral("param4")) : QVariant();
        if (!canonicalKnown && error.isEmpty()) error = actualState == QStringLiteral("missing")
            ? QStringLiteral("heading-observation-missing") : actualState == QStringLiteral("system-default")
                ? QStringLiteral("heading-observation-system-default") : QStringLiteral("unsupported_wire_parameter");
        if (canonicalKnown && !expectedKnown) error = QStringLiteral("heading-expected-unavailable");
        if (canonicalKnown && expectedKnown && expectedCanonical != canonical) error = QStringLiteral("heading-semantic-different");
        rows.append(QVariantMap{{QStringLiteral("sequenceNumber"), index},
            {QStringLiteral("expectedEncoded"), expectedValue.isValid() ? expectedValue : QVariant::fromValue(nullptr)},
            {QStringLiteral("actualRaw"), actualValue.isValid() ? actualValue : QVariant::fromValue(nullptr)},
            {QStringLiteral("actualState"), actualState},
            {QStringLiteral("canonical"), canonicalKnown ? canonical : QVariant::fromValue(nullptr)},
            {QStringLiteral("canonicalKnown"), canonicalKnown},
            {QStringLiteral("absoluteErrorBoundDegrees"), FleetMissionHeadingEncoding::MaximumReadbackErrorDegrees},
            {QStringLiteral("error"), error.isEmpty() ? QStringLiteral("") : error},
            {QStringLiteral("semanticMatch"), expectedKnown && canonicalKnown && expectedCanonical == canonical}});
    }
    return {{QStringLiteral("totalCount"), count}, {QStringLiteral("displayedCount"), rows.size()},
        {QStringLiteral("truncated"), count > rows.size()}, {QStringLiteral("complete"), true}, {QStringLiteral("rows"), rows}};
}

bool headingObservationsKnown(const QVariant& value)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap map = value.toMap();
    const QStringList keys{QStringLiteral("totalCount"), QStringLiteral("displayedCount"), QStringLiteral("truncated"),
        QStringLiteral("complete"), QStringLiteral("rows")};
    if (map.size() != keys.size()) return false;
    for (const QString& key : keys) if (!map.contains(key)) return false;
    const QVariant count = map.value(QStringLiteral("totalCount")), displayed = map.value(QStringLiteral("displayedCount"));
    if (count.type() != QVariant::Int || count.isNull() || count.toInt() < 1 || count.toInt() > MaximumItems
        || displayed.type() != QVariant::Int || displayed.isNull() || displayed.toInt() != qMin(count.toInt(), MaximumRows)
        || map.value(QStringLiteral("truncated")).type() != QVariant::Bool || map.value(QStringLiteral("truncated")).isNull()
        || map.value(QStringLiteral("truncated")).toBool() != (count.toInt() > displayed.toInt())
        || map.value(QStringLiteral("complete")).type() != QVariant::Bool || map.value(QStringLiteral("complete")).isNull()
        || !map.value(QStringLiteral("complete")).toBool()
        || map.value(QStringLiteral("rows")).type() != QVariant::List || map.value(QStringLiteral("rows")).isNull()
        || map.value(QStringLiteral("rows")).toList().size() != displayed.toInt()) return false;
    const QStringList rowKeys{QStringLiteral("sequenceNumber"), QStringLiteral("expectedEncoded"), QStringLiteral("actualRaw"),
        QStringLiteral("actualState"), QStringLiteral("canonical"), QStringLiteral("canonicalKnown"),
        QStringLiteral("absoluteErrorBoundDegrees"), QStringLiteral("error"), QStringLiteral("semanticMatch")};
    const QStringList states{QStringLiteral("finite"), QStringLiteral("system-default"), QStringLiteral("missing"), QStringLiteral("invalid")};
    const QStringList errors{QStringLiteral(""), QStringLiteral("unsupported_wire_parameter"), QStringLiteral("heading-observation-missing"),
        QStringLiteral("heading-observation-system-default"), QStringLiteral("heading-expected-unavailable"), QStringLiteral("heading-semantic-different")};
    int previous = -1;
    for (const QVariant& item : map.value(QStringLiteral("rows")).toList()) {
        if (item.type() != QVariant::Map || item.isNull()) return false;
        const QVariantMap row = item.toMap();
        if (row.size() != rowKeys.size()) return false;
        for (const QString& key : rowKeys) if (!row.contains(key)) return false;
        const QVariant sequence = row.value(QStringLiteral("sequenceNumber"));
        if (sequence.type() != QVariant::Int || sequence.isNull() || sequence.toInt() <= previous || sequence.toInt() >= MaximumItems
            || row.value(QStringLiteral("actualState")).type() != QVariant::String || row.value(QStringLiteral("actualState")).isNull()
            || !states.contains(row.value(QStringLiteral("actualState")).toString())
            || row.value(QStringLiteral("error")).type() != QVariant::String || row.value(QStringLiteral("error")).isNull()
            || !errors.contains(row.value(QStringLiteral("error")).toString())
            || row.value(QStringLiteral("canonicalKnown")).type() != QVariant::Bool || row.value(QStringLiteral("canonicalKnown")).isNull()
            || row.value(QStringLiteral("semanticMatch")).type() != QVariant::Bool || row.value(QStringLiteral("semanticMatch")).isNull()
            || finiteScalar(row, QStringLiteral("absoluteErrorBoundDegrees")) != QVariant(FleetMissionHeadingEncoding::MaximumReadbackErrorDegrees)) return false;
        previous = sequence.toInt();
        for (const QString& key : {QStringLiteral("expectedEncoded"), QStringLiteral("actualRaw"), QStringLiteral("canonical")}) {
            const QVariant field = row.value(key);
            if (field.isValid() && field.userType() != QMetaType::Nullptr && !finiteScalar(row, key).isValid()) return false;
        }
        const QVariant actual = finiteScalar(row, QStringLiteral("actualRaw")), canonical = finiteScalar(row, QStringLiteral("canonical"));
        if ((row.value(QStringLiteral("actualState")) == QVariant(QStringLiteral("finite"))) != actual.isValid()
            || row.value(QStringLiteral("canonicalKnown")).toBool() != canonical.isValid()) return false;
        QVariant checked;
        QString error;
        if (canonical.isValid() && (!actual.isValid()
            || !FleetMissionHeadingEncoding::canonicalParam4(MAV_CMD_NAV_WAYPOINT, actual, QStringLiteral("absolute-degrees"), true, checked, error)
            || checked != canonical)) return false;
        if (row.value(QStringLiteral("semanticMatch")).toBool()
            && (!canonical.isValid() || !FleetMissionHeadingEncoding::canonicalParam4(MAV_CMD_NAV_WAYPOINT,
                row.value(QStringLiteral("expectedEncoded")), QStringLiteral("absolute-degrees"), false, checked, error)
                || checked != canonical || !row.value(QStringLiteral("error")).toString().isEmpty())) return false;
    }
    return true;
}

void appendDifference(QVariantList* rows, int* count, int index, const QString& field, const QString& kind,
                      const Scalar& expected, const Scalar& observed)
{
    ++(*count);
    if (rows->size() >= MaximumRows) return;
    rows->append(QVariantMap{{QStringLiteral("itemIndex"), index}, {QStringLiteral("field"), field},
        {QStringLiteral("differenceKind"), kind}, {QStringLiteral("expectedState"), expected.state},
        {QStringLiteral("observedState"), observed.state}, {QStringLiteral("expectedType"), expected.type},
        {QStringLiteral("observedType"), observed.type}, {QStringLiteral("expectedValue"), expected.value},
        {QStringLiteral("observedValue"), observed.value}});
}

bool digest(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty()) return true;
    if (text.size() != 64) return false;
    for (const QChar ch : text) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}
} // namespace

QVariantMap FleetMissionReadbackDiff::compare(const QVariantList& expectedRawItems, const QVariantList& observedDecodedItems)
{
    const int expectedCount = expectedRawItems.size(), observedCount = observedDecodedItems.size();
    QVariantMap result = base(expectedCount, observedCount);
    if (expectedCount > MaximumItems || observedCount > MaximumItems) return result;
    const bool explicitHeading = hasHeading(expectedRawItems) || hasHeading(observedDecodedItems);
    if (explicitHeading) {
        result.insert(QStringLiteral("schemaVersion"), 2);
        // Even a canonical match retains the actual, unmodified float32 yaw.
        // Complete means all bounded candidate indices were counted, not that
        // every raw value is valid or that all detail rows fit the display.
        result.insert(QStringLiteral("headingObservations"), headingObservations(expectedRawItems, observedDecodedItems));
    }
    const QVariantMap expectedCanonical = normalize(expectedRawItems, false), observedCanonical = normalize(observedDecodedItems, true);
    const bool expectedValid = canonicalOk(expectedCanonical), observedValid = canonicalOk(observedCanonical);
    result.insert(QStringLiteral("expectedCanonicalValid"), expectedValid);
    result.insert(QStringLiteral("observedCanonicalValid"), observedValid);
    result.insert(QStringLiteral("expectedContentHash"), expectedValid ? expectedCanonical.value(QStringLiteral("contentHash")) : QVariant(QStringLiteral("")));
    result.insert(QStringLiteral("observedContentHash"), observedValid ? observedCanonical.value(QStringLiteral("contentHash")) : QVariant(QStringLiteral("")));
    result.insert(QStringLiteral("expectedErrorCode"), expectedValid ? QStringLiteral("") : canonicalError(expectedCanonical));
    result.insert(QStringLiteral("observedErrorCode"), observedValid ? QStringLiteral("") : canonicalError(observedCanonical));
    result.insert(QStringLiteral("expectedErrorItemIndex"), expectedValid ? -1 : errorIndex(expectedCanonical));
    result.insert(QStringLiteral("observedErrorItemIndex"), observedValid ? -1 : errorIndex(observedCanonical));
    const bool canonical = expectedValid && observedValid;
    const QVariantList expected = canonical ? expectedCanonical.value(QStringLiteral("items")).toList() : expectedRawItems;
    const QVariantList observed = canonical ? observedCanonical.value(QStringLiteral("items")).toList() : observedDecodedItems;
    result.insert(QStringLiteral("comparisonBasis"), canonical
        ? FleetMissionHeadingEncoding::comparisonBasis(explicitHeading) : QStringLiteral("raw-fields-only"));
    const bool matched = canonical && expected == observed
        && expectedCanonical.value(QStringLiteral("contentHash")) == observedCanonical.value(QStringLiteral("contentHash"));
    result.insert(QStringLiteral("state"), matched ? QStringLiteral("matched") : canonical || (expectedValid && observedCount == 0)
        ? QStringLiteral("different") : QStringLiteral("unknown"));
    result.insert(QStringLiteral("reason"), matched ? QStringLiteral("canonical-match") : canonical ? QStringLiteral("canonical-different")
        : !expectedValid ? QStringLiteral("expected-canonical-invalid") : observedCount == 0 ? QStringLiteral("observed-empty") : QStringLiteral("observed-canonical-invalid"));
    QVariantList rows;
    int differences = 0, compared = 0, uncompared = 0;
    if (expectedCount != observedCount) {
        Scalar left, right;
        left.state = right.state = QStringLiteral("present"); left.type = right.type = QStringLiteral("int");
        left.value = expectedCount; right.value = observedCount;
        appendDifference(&rows, &differences, -1, QStringLiteral("itemCount"), QStringLiteral("count"), left, right);
    }
    for (int index = 0; index < qMax(expectedCount, observedCount); ++index) {
        if (index >= expected.size() || index >= observed.size()) {
            Scalar left, right;
            left.state = index >= expected.size() ? QStringLiteral("item-missing") : QStringLiteral("present");
            right.state = index >= observed.size() ? QStringLiteral("item-missing") : QStringLiteral("present");
            left.type = index >= expected.size() ? QStringLiteral("invalid") : valueType(expected.at(index));
            right.type = index >= observed.size() ? QStringLiteral("invalid") : valueType(observed.at(index));
            appendDifference(&rows, &differences, index, QStringLiteral("item"), QStringLiteral("item-presence"), left, right);
            ++compared; continue;
        }
        const QVariant leftItem = expected.at(index), rightItem = observed.at(index);
        if (leftItem.type() != QVariant::Map || leftItem.isNull() || leftItem.toMap().size() > 17
            || rightItem.type() != QVariant::Map || rightItem.isNull() || rightItem.toMap().size() > 17) {
            Scalar left, right;
            left.state = leftItem.type() == QVariant::Map && !leftItem.isNull() && leftItem.toMap().size() <= 17 ? QStringLiteral("present") : QStringLiteral("item-invalid");
            right.state = rightItem.type() == QVariant::Map && !rightItem.isNull() && rightItem.toMap().size() <= 17 ? QStringLiteral("present") : QStringLiteral("item-invalid");
            left.type = valueType(leftItem); right.type = valueType(rightItem);
            if (left.state != right.state || left.type != right.type)
                appendDifference(&rows, &differences, index, QStringLiteral("item"), QStringLiteral("state"), left, right);
            ++uncompared; continue;
        }
        const QVariantMap left = leftItem.toMap(), right = rightItem.toMap();
        bool complete = true;
        for (auto it = left.constBegin(); it != left.constEnd(); ++it) if (!fields().contains(it.key())) complete = false;
        for (auto it = right.constBegin(); it != right.constEnd(); ++it) if (!fields().contains(it.key())) complete = false;
        for (const QString& field : fields()) {
            const Scalar first = scalar(left, field), second = scalar(right, field);
            complete = complete && first.comparable && second.comparable;
            QString kind;
            if (first.state != second.state) kind = QStringLiteral("state");
            else if (first.type != second.type) kind = QStringLiteral("type");
            else if (first.comparable && second.comparable && first.value != second.value) kind = QStringLiteral("value");
            if (!kind.isEmpty()) appendDifference(&rows, &differences, index, field, kind, first, second);
        }
        if (complete) ++compared; else ++uncompared;
    }
    result.insert(QStringLiteral("comparedItemCount"), compared); result.insert(QStringLiteral("uncomparedItemCount"), uncompared);
    result.insert(QStringLiteral("differenceCount"), differences);
    result.insert(QStringLiteral("differenceCountComplete"), canonical && uncompared == 0);
    result.insert(QStringLiteral("displayedDifferenceCount"), rows.size()); result.insert(QStringLiteral("rowsTruncated"), differences > rows.size());
    result.insert(QStringLiteral("rows"), rows);
    return result;
}

QVariantMap FleetMissionReadbackDiff::summary(const QVariantMap& diagnostic)
{
    // Legacy history remains scalar-only. V2 adds only the independently
    // validated <=64 heading observations, retaining actual raw yaw values;
    // arbitrary difference rows/nesting are still never copied into history.
    const bool heading = diagnostic.value(QStringLiteral("schemaVersion")).type() == QVariant::Int
        && !diagnostic.value(QStringLiteral("schemaVersion")).isNull()
        && diagnostic.value(QStringLiteral("schemaVersion")).toInt() == 2;
    if (diagnostic.size() != (heading ? 29 : 28)
        || diagnostic.contains(QStringLiteral("headingObservations")) != heading
        || (heading && !headingObservationsKnown(diagnostic.value(QStringLiteral("headingObservations"))))
        || diagnostic.value(QStringLiteral("format")).type() != QVariant::String
        || diagnostic.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-mission-readback-diff")
        || diagnostic.value(QStringLiteral("sourceRepresentation")).type() != QVariant::String
        || diagnostic.value(QStringLiteral("sourceRepresentation")).toString() != QStringLiteral("ordinary-decoded-mission-items-not-raw-int-proof")
        || diagnostic.value(QStringLiteral("rows")).type() != QVariant::List || diagnostic.value(QStringLiteral("rows")).isNull()
        || diagnostic.value(QStringLiteral("rows")).toList().size() > MaximumRows) return {};
    QVariantMap result = base(0, 0);
    result.remove(QStringLiteral("rows"));
    if (heading) result.insert(QStringLiteral("headingObservations"), diagnostic.value(QStringLiteral("headingObservations")));
    const QStringList intKeys{QStringLiteral("schemaVersion"), QStringLiteral("expectedItemCount"), QStringLiteral("observedItemCount"),
        QStringLiteral("expectedErrorItemIndex"), QStringLiteral("observedErrorItemIndex"), QStringLiteral("comparedItemCount"),
        QStringLiteral("uncomparedItemCount"), QStringLiteral("differenceCount"), QStringLiteral("displayedDifferenceCount")};
    const QStringList boolKeys{QStringLiteral("expectedCanonicalValid"), QStringLiteral("observedCanonicalValid"),
        QStringLiteral("differenceCountComplete"), QStringLiteral("rowsTruncated"), QStringLiteral("historicalOnly"),
        QStringLiteral("snapshotOnly"), QStringLiteral("currentOnboardContentsProven"), QStringLiteral("startupPermitGranted"), QStringLiteral("flightCommandReleased")};
    const QStringList textKeys{QStringLiteral("state"), QStringLiteral("comparisonBasis"), QStringLiteral("reason"),
        QStringLiteral("expectedErrorCode"), QStringLiteral("observedErrorCode")};
    const QMap<QString, QStringList> textValues{
        {QStringLiteral("state"), {QStringLiteral("matched"), QStringLiteral("different"), QStringLiteral("unknown")}},
        {QStringLiteral("comparisonBasis"), {QStringLiteral("canonical-v2"), QStringLiteral("canonical-heading-tick-v3"), QStringLiteral("raw-fields-only"), QStringLiteral("none")}},
        {QStringLiteral("reason"), {QStringLiteral("input-item-limit"), QStringLiteral("canonical-match"), QStringLiteral("canonical-different"),
            QStringLiteral("expected-canonical-invalid"), QStringLiteral("observed-empty"), QStringLiteral("observed-canonical-invalid")}};
    const QStringList errors{QStringLiteral(""), QStringLiteral("wire_resource_limit"), QStringLiteral("invalid_wire_item"),
        QStringLiteral("unsupported_wire_command"), QStringLiteral("invalid_wire_parameter"), QStringLiteral("unsupported_wire_parameter"),
        QStringLiteral("invalid_wire_coordinate"), QStringLiteral("diagnostic-canonical-unavailable")};
    for (const QString& key : intKeys) {
        const QVariant value = diagnostic.value(key);
        const bool index = key == QStringLiteral("expectedErrorItemIndex") || key == QStringLiteral("observedErrorItemIndex");
        if (value.type() != QVariant::Int || value.isNull() || value.toInt() < (index ? -1 : 0)
            || (index && value.toInt() >= MaximumItems)) return {};
        result.insert(key, value);
    }
    for (const QString& key : boolKeys) {
        const QVariant value = diagnostic.value(key);
        if (value.type() != QVariant::Bool || value.isNull()) return {};
        if ((key == QStringLiteral("historicalOnly") || key == QStringLiteral("snapshotOnly")) && !value.toBool()) return {};
        if ((key == QStringLiteral("currentOnboardContentsProven") || key == QStringLiteral("startupPermitGranted")
            || key == QStringLiteral("flightCommandReleased")) && value.toBool()) return {};
        result.insert(key, value);
    }
    for (const QString& key : textKeys) {
        const QVariant value = diagnostic.value(key);
        const QStringList allowed = textValues.contains(key) ? textValues.value(key) : errors;
        if (value.type() != QVariant::String || value.isNull() || !allowed.contains(value.toString())) return {};
        result.insert(key, value);
    }
    for (const QString& key : {QStringLiteral("expectedContentHash"), QStringLiteral("observedContentHash")}) {
        if (!digest(diagnostic.value(key))) return {};
        result.insert(key, diagnostic.value(key));
    }
    const int expected = result.value(QStringLiteral("expectedItemCount")).toInt();
    const int observed = result.value(QStringLiteral("observedItemCount")).toInt();
    const int compared = result.value(QStringLiteral("comparedItemCount")).toInt();
    const int uncompared = result.value(QStringLiteral("uncomparedItemCount")).toInt();
    const int differences = result.value(QStringLiteral("differenceCount")).toInt();
    const int displayed = result.value(QStringLiteral("displayedDifferenceCount")).toInt();
    if (result.value(QStringLiteral("schemaVersion")).toInt() != (heading ? 2 : 1) || compared > MaximumItems
        || static_cast<qint64>(compared) + uncompared != qMax(expected, observed)
        || differences > 1 + MaximumItems * fields().size() || displayed > MaximumRows || displayed > differences
        || displayed != diagnostic.value(QStringLiteral("rows")).toList().size()
        || result.value(QStringLiteral("rowsTruncated")).toBool() != (differences > displayed)) return {};
    for (const QString& side : {QStringLiteral("expected"), QStringLiteral("observed")}) {
        const bool valid = result.value(side + QStringLiteral("CanonicalValid")).toBool();
        if (result.value(side + QStringLiteral("ContentHash")).toString().isEmpty() == valid) return {};
    }
    const QString basis = result.value(QStringLiteral("comparisonBasis")).toString();
    const bool canonical = basis == FleetMissionHeadingEncoding::comparisonBasis(heading);
    if ((basis == QStringLiteral("canonical-v2") || basis == QStringLiteral("canonical-heading-tick-v3")) && !canonical) return {};
    if (result.value(QStringLiteral("differenceCountComplete")).toBool() != (canonical && uncompared == 0)
        || (canonical && (!result.value(QStringLiteral("expectedCanonicalValid")).toBool()
            || !result.value(QStringLiteral("observedCanonicalValid")).toBool()))
        || (result.value(QStringLiteral("state")).toString() == QStringLiteral("matched") && (!canonical || differences != 0))) return {};
    return result;
}
