#include "FleetEventArchiveCodec.h"
#include "FleetMissionModeExitObservation.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QUuid>

#include <cmath>
#include <limits>

namespace {
constexpr int MaximumDepth = 16;
constexpr int MaximumNodes = 50000;
constexpr qint64 MaximumBytes = 512LL * 1024LL;
constexpr qint64 MaximumExactInteger = 9007199254740991LL;
constexpr qint64 FutureToleranceMs = 300000;

bool exactString(const QVariant& value, const QString& expected)
{
    return !value.isNull() && value.userType() == QMetaType::QString && value.toString() == expected;
}

bool exactBool(const QVariant& value, bool expected)
{
    return !value.isNull() && value.userType() == QMetaType::Bool && value.toBool() == expected;
}

bool digest(const QString& text)
{
    if (text.size() != 64) return false;
    for (QChar character : text) {
        if (!(character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            && !(character >= QLatin1Char('a') && character <= QLatin1Char('f'))) return false;
    }
    return true;
}

bool uuid(const QString& text)
{
    if (text.size() != 36) return false;
    const QUuid parsed(text);
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == text;
}

bool decimal(const QString& text, quint64 maximum)
{
    if (text.isEmpty() || text.size() > 20 || text.at(0) == QLatin1Char('0')) return false;
    for (QChar digit : text) {
        if (digit < QLatin1Char('0') || digit > QLatin1Char('9')) return false;
    }
    bool ok = false;
    const quint64 value = text.toULongLong(&ok);
    return ok && value > 0 && value <= maximum && QString::number(value) == text;
}

bool scalarBudget(qint64 amount, qint64* used)
{
    if (!used || amount < 0 || amount > MaximumBytes - *used) return false;
    *used += amount;
    return true;
}

bool unicodeBudget(const QString& text, qint64* used)
{
    // Count UTF-8 bytes without allocating an unbounded conversion first.
    // Lone UTF-16 surrogates would be replaced during conversion, so they
    // cannot enter a canonical hash as if the original string were retained.
    if (!used || text.size() > MaximumBytes - *used) return false;
    qint64 bytes = 0;
    for (int index = 0; index < text.size(); ++index) {
        const ushort code = text.at(index).unicode();
        if (code >= 0xd800 && code <= 0xdbff) {
            if (index + 1 >= text.size()) return false;
            const ushort next = text.at(++index).unicode();
            if (next < 0xdc00 || next > 0xdfff) return false;
            bytes += 4;
        } else {
            if (code >= 0xdc00 && code <= 0xdfff) return false;
            bytes += code < 0x80 ? 1 : code < 0x800 ? 2 : 3;
        }
        if (bytes > MaximumBytes - *used) return false;
    }
    return scalarBudget(bytes, used);
}

bool number(const QVariant& value, double* candidate)
{
    // A typed-null numeric QVariant may convert to zero. Unknown must not
    // become a known timestamp, command result or numeric diagnostic field.
    if (!value.isValid() || value.isNull()) return false;
    switch (value.userType()) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Double:
    case QMetaType::Float:
        break;
    case QMetaType::LongLong:
        if (value.toLongLong() < -MaximumExactInteger
            || value.toLongLong() > MaximumExactInteger) return false;
        break;
    case QMetaType::ULongLong:
        if (value.toULongLong() > static_cast<quint64>(MaximumExactInteger)) return false;
        break;
    default:
        return false;
    }
    const double actual = value.toDouble();
    if (!std::isfinite(actual)
        || (std::floor(actual) == actual && std::abs(actual) > static_cast<double>(MaximumExactInteger))) return false;
    if (candidate) *candidate = actual;
    return true;
}

bool integer(const QVariant& value, qint64 minimum, qint64 maximum, qint64* result = nullptr)
{
    double actual = 0;
    if (!number(value, &actual) || std::floor(actual) != actual
        || actual < static_cast<double>(minimum) || actual > static_cast<double>(maximum)) return false;
    if (result) *result = static_cast<qint64>(actual);
    return true;
}

bool boundedJsonValue(const QVariant& value, int depth, int* nodes, qint64* scalarBytes)
{
    if (!nodes || !scalarBytes || depth > MaximumDepth || ++(*nodes) > MaximumNodes) return false;
    // Only an untyped invalid QVariant represents JSON null. Reject ALL
    // valid-but-typed-null values, even otherwise supported scalar/container
    // types, before Qt can turn unknown into zero/false/empty on conversion.
    if (!value.isValid()) return scalarBudget(4, scalarBytes);
    if (value.isNull()) return false;
    switch (value.userType()) {
    case QMetaType::QVariantMap: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (!unicodeBudget(it.key(), scalarBytes)
                || !boundedJsonValue(it.value(), depth + 1, nodes, scalarBytes)) return false;
        }
        return true;
    }
    case QMetaType::QVariantList: {
        const QVariantList list = value.toList();
        for (const QVariant& item : list) {
            if (!boundedJsonValue(item, depth + 1, nodes, scalarBytes)) return false;
        }
        return true;
    }
    case QMetaType::QString:
        return unicodeBudget(value.toString(), scalarBytes);
    case QMetaType::Bool:
        return scalarBudget(1, scalarBytes);
    default:
        return number(value, nullptr) && scalarBudget(8, scalarBytes);
    }
}

bool canonical(const QVariantMap& map, QByteArray* bytes)
{
    if (!bytes) return false;
    int nodes = 0;
    qint64 scalarBytes = 0;
    if (!boundedJsonValue(map, 0, &nodes, &scalarBytes)) return false;
    const QByteArray encoded = QJsonDocument(QJsonObject::fromVariantMap(map)).toJson(QJsonDocument::Compact);
    if (encoded.isEmpty() || encoded.size() > MaximumBytes) return false;
    *bytes = encoded;
    return true;
}

QString hashBytes(const QByteArray& bytes)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

QString eventCategory(const QString& originalType)
{
    const QStringList categories {QStringLiteral("command"), QStringLiteral("transaction"),
        QStringLiteral("task"), QStringLiteral("formation"), QStringLiteral("fleet"),
        QStringLiteral("intent"), QStringLiteral("maintenance")};
    for (const QString& category : categories) {
        if (originalType.startsWith(category)) return category;
    }
    return QStringLiteral("other");
}

bool targets(const QVariant& value)
{
    if (value.isNull() || value.userType() != QMetaType::QVariantList) return false;
    const QVariantList list = value.toList();
    if (list.size() > 32) return false;
    QSet<qint64> seen;
    for (const QVariant& item : list) {
        qint64 id = 0;
        if (!integer(item, 1, 255, &id) || seen.contains(id)) return false;
        seen.insert(id);
    }
    return true;
}

QStringList hashedIdentifiers()
{
    return {QStringLiteral("taskId"), QStringLiteral("planId"), QStringLiteral("recordId"),
        QStringLiteral("deploymentId"), QStringLiteral("startupId"), QStringLiteral("transactionId"),
        QStringLiteral("reviewId"), QStringLiteral("operationId")};
}

bool engineeringOperation(const QString& operation)
{
    return operation == QStringLiteral("approve") || operation == QStringLiteral("revoke")
        || operation == QStringLiteral("approve-baseline") || operation == QStringLiteral("revoke-baseline")
        || operation == QStringLiteral("approve-performance");
}

QString engineeringEventType(const QString& operation)
{
    if (operation == QStringLiteral("approve-performance")) return QStringLiteral("fleet.performance.declaration.result");
    return operation == QStringLiteral("approve-baseline") || operation == QStringLiteral("revoke-baseline")
        ? QStringLiteral("fleet.configuration.review.result") : QStringLiteral("fleet.capability.review.result");
}

QString triState(const QVariant& value)
{
    return value.isNull() || value.userType() != QMetaType::Bool ? QStringLiteral("unknown")
        : value.toBool() ? QStringLiteral("yes") : QStringLiteral("no");
}

bool triStateValid(const QVariant& value)
{
    return exactString(value, QStringLiteral("yes")) || exactString(value, QStringLiteral("no"))
        || exactString(value, QStringLiteral("unknown"));
}

QString observedDigest(const QVariant& value)
{
    return !value.isNull() && value.userType() == QMetaType::QString && digest(value.toString())
        ? value.toString() : QStringLiteral("");
}

bool optionalDigest(const QVariant& value)
{
    return !value.isNull() && value.userType() == QMetaType::QString
        && (value.toString().isEmpty() || digest(value.toString()));
}

QVariantMap actualMap(const QVariant& value)
{
    return !value.isNull() && value.userType() == QMetaType::QVariantMap ? value.toMap() : QVariantMap();
}

bool engineeringSummary(const QVariant& value, qint64 expectedVehicleId,
                        const QString& expectedTypeHash)
{
    if (value.isNull() || value.userType() != QMetaType::QVariantMap) return false;
    const QVariantMap summary = value.toMap();
    qint64 vehicleId = 0, count = -1, displayed = -1;
    if (summary.size() != 21 || !integer(summary.value(QStringLiteral("schemaVersion")), 1, 1)
        || !exactString(summary.value(QStringLiteral("source")), QStringLiteral("local-engineering-review-summary"))
        || !integer(summary.value(QStringLiteral("vehicleId")), 1, 255, &vehicleId)
        || vehicleId != expectedVehicleId || summary.value(QStringLiteral("operation")).isNull()
        || summary.value(QStringLiteral("operation")).userType() != QMetaType::QString
        || !engineeringOperation(summary.value(QStringLiteral("operation")).toString())
        || expectedTypeHash != hashBytes(engineeringEventType(summary.value(QStringLiteral("operation")).toString()).toUtf8())
        || !integer(summary.value(QStringLiteral("reportedChangeCount")), -1, 4096, &count)
        || !integer(summary.value(QStringLiteral("displayedChangeCount")), -1, 128, &displayed)
        || !integer(summary.value(QStringLiteral("capabilityCount")), -1, 128)
        || !exactBool(summary.value(QStringLiteral("historicalOnly")), true)
        || !exactBool(summary.value(QStringLiteral("restoresAuthorization")), false)
        || !exactBool(summary.value(QStringLiteral("hardwareIdentityAuthenticated")), false)
        || !exactBool(summary.value(QStringLiteral("performanceCertified")), false)
        || !exactBool(summary.value(QStringLiteral("flightCommandReleased")), false)) return false;
    for (const QString& key : {QStringLiteral("mutationCommitted"), QStringLiteral("nativeOk"),
         QStringLiteral("stillCurrent"), QStringLiteral("comparisonKnown"), QStringLiteral("comparisonTruncated")})
        if (!triStateValid(summary.value(key))) return false;
    for (const QString& key : {QStringLiteral("beforeTypedFingerprint"), QStringLiteral("afterTypedFingerprint"),
         QStringLiteral("baselineTypedFingerprint"), QStringLiteral("reviewIdHash")})
        if (!optionalDigest(summary.value(key))) return false;
    if (exactString(summary.value(QStringLiteral("comparisonKnown")), QStringLiteral("yes"))) {
        if (count < 0 || displayed < 0 || displayed > count
            || exactString(summary.value(QStringLiteral("comparisonTruncated")), QStringLiteral("unknown"))) return false;
    } else if (count != -1 || displayed != -1) return false;
    // Registry acceptance/currentness never upgrades an uncommitted result.
    if ((exactString(summary.value(QStringLiteral("nativeOk")), QStringLiteral("yes"))
         || exactString(summary.value(QStringLiteral("stillCurrent")), QStringLiteral("yes")))
        && !exactString(summary.value(QStringLiteral("mutationCommitted")), QStringLiteral("yes"))) return false;
    return true;
}

struct ComparisonSummary {
    QString before = QStringLiteral("");
    QString after = QStringLiteral("");
    QString baseline = QStringLiteral("");
    QString known = QStringLiteral("unknown");
    QString truncated = QStringLiteral("unknown");
    int count = -1;
    int displayed = -1;
};

ComparisonSummary comparisonSummary(const QVariantMap& wrapper, int vehicleId)
{
    ComparisonSummary summary;
    qint64 actualId = 0, count = 0, displayed = 0;
    if (wrapper.size() != 16 || !integer(wrapper.value(QStringLiteral("schemaVersion")), 1, 1)
        || !integer(wrapper.value(QStringLiteral("systemId")), 1, 255, &actualId) || actualId != vehicleId) return summary;
    const QVariantMap report = actualMap(wrapper.value(QStringLiteral("comparison")));
    if (report.size() != 13 || !integer(report.value(QStringLiteral("schemaVersion")), 1, 1)
        || report.value(QStringLiteral("comparisonKnown")).isNull()
        || report.value(QStringLiteral("comparisonKnown")).userType() != QMetaType::Bool
        || report.value(QStringLiteral("truncated")).isNull() || report.value(QStringLiteral("truncated")).userType() != QMetaType::Bool
        || report.value(QStringLiteral("beforeKnown")).isNull() || report.value(QStringLiteral("beforeKnown")).userType() != QMetaType::Bool
        || report.value(QStringLiteral("afterKnown")).isNull() || report.value(QStringLiteral("afterKnown")).userType() != QMetaType::Bool)
        return summary;
    const QString before = observedDigest(report.value(QStringLiteral("beforeTypedFingerprint")));
    const QString after = observedDigest(report.value(QStringLiteral("afterTypedFingerprint")));
    if ((report.value(QStringLiteral("beforeKnown")).toBool() && before.isEmpty())
        || (report.value(QStringLiteral("afterKnown")).toBool() && after.isEmpty())) return summary;
    summary.before = report.value(QStringLiteral("beforeKnown")).toBool() ? before : QStringLiteral("");
    summary.after = report.value(QStringLiteral("afterKnown")).toBool() ? after : QStringLiteral("");
    summary.known = triState(report.value(QStringLiteral("comparisonKnown")));
    summary.truncated = triState(report.value(QStringLiteral("truncated")));
    if (exactString(wrapper.value(QStringLiteral("basis")), QStringLiteral("local-approved-baseline"))) summary.baseline = summary.before;
    if (summary.known == QStringLiteral("yes")) {
        const QVariant changes = report.value(QStringLiteral("changes"));
        const bool truncated = report.value(QStringLiteral("truncated")).toBool();
        if (!report.value(QStringLiteral("beforeKnown")).toBool() || !report.value(QStringLiteral("afterKnown")).toBool()
            || !integer(report.value(QStringLiteral("changeCount")), 0, 4096, &count)
            || !integer(report.value(QStringLiteral("reportedChangeCount")), 0, 128, &displayed)
            || displayed > count || changes.isNull() || changes.userType() != QMetaType::QVariantList
            || changes.toList().size() != displayed || !exactBool(report.value(QStringLiteral("complete")), !truncated)
            || !exactString(report.value(QStringLiteral("comparisonState")), truncated ? QStringLiteral("truncated")
                : count == 0 ? QStringLiteral("same") : QStringLiteral("different"))) return ComparisonSummary();
        summary.count = static_cast<int>(count); summary.displayed = static_cast<int>(displayed);
    } else if (!exactString(report.value(QStringLiteral("comparisonState")), QStringLiteral("unknown"))
               && !exactString(report.value(QStringLiteral("comparisonState")), QStringLiteral("invalid"))) return ComparisonSummary();
    return summary;
}

int effectiveCapabilityCount(const QVariant& value)
{
    if (value.isNull() || (value.userType() != QMetaType::QStringList && value.userType() != QMetaType::QVariantList)) return -1;
    QVariantList list;
    if (value.userType() == QMetaType::QStringList) {
        const QStringList strings = value.toStringList();
        if (strings.size() > 128) return -1;
        for (const QString& string : strings) list.append(string);
    } else list = value.toList();
    if (list.size() > 128) return -1;
    QString previous;
    for (const QVariant& item : list) {
        if (item.isNull() || item.userType() != QMetaType::QString) return -1;
        const QString text = item.toString();
        if (text.isEmpty() || text.size() > 128 || text.trimmed().toLower() != text
            || (!previous.isEmpty() && previous >= text)) return -1;
        previous = text;
    }
    return list.size();
}

bool runKeys(const QVariantMap& value, const QStringList& keys)
{
    if (value.size() != keys.size()) return false;
    for (const QString& key : keys)
        if (!value.contains(key) || value.value(key).isNull()) return false;
    return true;
}

bool runText(const QVariant& value)
{
    return !value.isNull() && value.userType() == QMetaType::QString;
}

bool runInteger(const QVariant& value, qint64 minimum, qint64 maximum, qint64* output = nullptr)
{
    // JSON decoding may yield an exact Double or LongLong. Do not widen the
    // native contract to strings, bools, Float or typed-null placeholders.
    switch (value.userType()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Double:
        return integer(value, minimum, maximum, output);
    default:
        return false;
    }
}

bool runCounter(const QVariant& value, bool allowZero)
{
    return runText(value) && ((allowZero && value.toString() == QStringLiteral("0"))
        || decimal(value.toString(), std::numeric_limits<quint64>::max()));
}

bool runFixedCode(const QVariant& value)
{
    if (!runText(value)) return false;
    const QString text = value.toString();
    if (text.isEmpty()) return true;
    if (text.size() > 128 || text.startsWith(QLatin1Char('-')) || text.endsWith(QLatin1Char('-'))) return false;
    bool previousHyphen = false;
    for (const QChar character : text) {
        const bool hyphen = character == QLatin1Char('-');
        if ((hyphen && previousHyphen) || (!hyphen
            && !(character >= QLatin1Char('a') && character <= QLatin1Char('z'))
            && !(character >= QLatin1Char('0') && character <= QLatin1Char('9')))) return false;
        previousHyphen = hyphen;
    }
    return true;
}

bool runSummary(const QVariant& value, bool* isGap = nullptr, int expectedSchema = 0)
{
    if (value.isNull() || value.userType() != QMetaType::QVariantMap) return false;
    const QVariantMap summary = value.toMap();
    qint64 schema = 0;
    if (!runInteger(summary.value(QStringLiteral("schemaVersion")), 1, 2, &schema)
        || (expectedSchema && schema != expectedSchema)) return false;
    QStringList fields{QStringLiteral("schemaVersion"), QStringLiteral("transitionKind"),
            QStringLiteral("transitionSourceHash"), QStringLiteral("transitionSequence"), QStringLiteral("capturedAtUtcMs"),
            QStringLiteral("observationRevision"), QStringLiteral("revisionExhausted"), QStringLiteral("runIdHash"),
            QStringLiteral("outcomeCandidate"), QStringLiteral("modeQueueBegan"), QStringLiteral("modeQueueCommitted"),
            QStringLiteral("continuityUnknown"), QStringLiteral("anomalyMask"), QStringLiteral("reasonCode"),
            QStringLiteral("sourceRetirementCode"), QStringLiteral("observerClosed"), QStringLiteral("ownerDestroyed"),
            QStringLiteral("reviewRequired"), QStringLiteral("droppedTransitionCount"), QStringLiteral("snapshotOnly"),
            QStringLiteral("historyComplete"), QStringLiteral("remoteMissionInstanceVerified"), QStringLiteral("missionCompleted"),
            QStringLiteral("resultVerified"), QStringLiteral("executionVerified"), QStringLiteral("executionPermit"),
            QStringLiteral("flightCommandReleased"), QStringLiteral("permissionsRestored")};
    if (schema == 2) fields.append(QStringLiteral("missionModeExitObservation"));
    if (!runKeys(summary, fields)
        || !runText(summary.value(QStringLiteral("transitionKind")))
        || !runText(summary.value(QStringLiteral("transitionSourceHash")))
        || !digest(summary.value(QStringLiteral("transitionSourceHash")).toString())
        || !runCounter(summary.value(QStringLiteral("transitionSequence")), false)
        || !runInteger(summary.value(QStringLiteral("capturedAtUtcMs")), 1, MaximumExactInteger)
        || !runCounter(summary.value(QStringLiteral("observationRevision")), true)
        || !runText(summary.value(QStringLiteral("runIdHash")))
        || !runText(summary.value(QStringLiteral("outcomeCandidate")))
        || !runInteger(summary.value(QStringLiteral("anomalyMask")), 0, 255)
        || !runFixedCode(summary.value(QStringLiteral("reasonCode")))
        || !runFixedCode(summary.value(QStringLiteral("sourceRetirementCode")))
        || !runCounter(summary.value(QStringLiteral("droppedTransitionCount")), true)) return false;
    for (const QString& key : {QStringLiteral("revisionExhausted"), QStringLiteral("modeQueueBegan"),
            QStringLiteral("modeQueueCommitted"), QStringLiteral("continuityUnknown"), QStringLiteral("observerClosed"),
            QStringLiteral("ownerDestroyed"), QStringLiteral("reviewRequired")})
        if (summary.value(key).userType() != QMetaType::Bool) return false;
    if (!exactBool(summary.value(QStringLiteral("snapshotOnly")), true)) return false;
    for (const QString& key : {QStringLiteral("historyComplete"), QStringLiteral("remoteMissionInstanceVerified"),
            QStringLiteral("missionCompleted"), QStringLiteral("resultVerified"), QStringLiteral("executionVerified"),
            QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased"), QStringLiteral("permissionsRestored")})
        if (!exactBool(summary.value(key), false)) return false;
    const QString kind = summary.value(QStringLiteral("transitionKind")).toString();
    const bool gap = kind == QStringLiteral("capture-gap");
    // The old global gap has no per-aircraft reception pair and remains v1.
    if (gap && schema != 1) return false;
    if (!gap && kind != QStringLiteral("registered") && kind != QStringLiteral("changed")
        && kind != QStringLiteral("closed") && kind != QStringLiteral("owner-destroyed")) return false;
    if (gap) {
        if (!exactString(summary.value(QStringLiteral("runIdHash")), QStringLiteral(""))
            || !exactString(summary.value(QStringLiteral("observationRevision")), QStringLiteral("0"))
            || !exactString(summary.value(QStringLiteral("outcomeCandidate")), QStringLiteral("not-applicable"))
            || !exactBool(summary.value(QStringLiteral("continuityUnknown")), true)
            || !exactBool(summary.value(QStringLiteral("reviewRequired")), true)
            || !exactBool(summary.value(QStringLiteral("revisionExhausted")), false)
            || !exactBool(summary.value(QStringLiteral("modeQueueBegan")), false)
            || !exactBool(summary.value(QStringLiteral("modeQueueCommitted")), false)
            || !exactBool(summary.value(QStringLiteral("observerClosed")), false)
            || !exactBool(summary.value(QStringLiteral("ownerDestroyed")), false)
            || !runInteger(summary.value(QStringLiteral("anomalyMask")), 0, 0)
            || !exactString(summary.value(QStringLiteral("reasonCode")), QStringLiteral("capture-gap"))
            || !exactString(summary.value(QStringLiteral("sourceRetirementCode")), QStringLiteral(""))
            || !runCounter(summary.value(QStringLiteral("droppedTransitionCount")), false)) return false;
    } else {
        const QString candidate = summary.value(QStringLiteral("outcomeCandidate")).toString();
        if (!digest(summary.value(QStringLiteral("runIdHash")).toString())
            || !runCounter(summary.value(QStringLiteral("observationRevision")), false)
            || (candidate != QStringLiteral("no-terminal-candidate-observed")
                && candidate != QStringLiteral("terminal-item-observed-not-completion")
                && candidate != QStringLiteral("indefinite-terminal-hold-observed")
                && candidate != QStringLiteral("landed-after-terminal-observed-not-completion"))
            || (summary.value(QStringLiteral("modeQueueCommitted")).toBool()
                && !summary.value(QStringLiteral("modeQueueBegan")).toBool())
            || (summary.value(QStringLiteral("revisionExhausted")).toBool()
                && summary.value(QStringLiteral("observationRevision")).toString()
                    != QString::number(std::numeric_limits<quint64>::max()))
            || (kind == QStringLiteral("closed") && !summary.value(QStringLiteral("observerClosed")).toBool())
            || (kind == QStringLiteral("owner-destroyed")
                && (!summary.value(QStringLiteral("ownerDestroyed")).toBool()
                    || !summary.value(QStringLiteral("observerClosed")).toBool()))) return false;
        const qint64 mask = static_cast<qint64>(summary.value(QStringLiteral("anomalyMask")).toDouble());
        if (schema == 2) {
            const QVariant raw = summary.value(QStringLiteral("missionModeExitObservation"));
            if (raw.userType() != QMetaType::QVariantMap || raw.isNull()) return false;
            const QVariantMap exit = raw.toMap();
            if (!FleetMissionModeExitObservation::valid(exit)
                || exit.value(QStringLiteral("observed")).toBool() != ((mask & 4) != 0)
                || (exit.value(QStringLiteral("observed")).toBool()
                    && !summary.value(QStringLiteral("modeQueueCommitted")).toBool())) return false;
        }
        const bool review = summary.value(QStringLiteral("continuityUnknown")).toBool()
            || !summary.value(QStringLiteral("reasonCode")).toString().isEmpty()
            || (mask & ~qint64(64)) != 0 || summary.value(QStringLiteral("ownerDestroyed")).toBool();
        if (summary.value(QStringLiteral("reviewRequired")).toBool() != review
            || (summary.value(QStringLiteral("ownerDestroyed")).toBool()
                && !summary.value(QStringLiteral("observerClosed")).toBool())) return false;
    }
    if (isGap) *isGap = gap;
    return true;
}

bool runAssociation(const QVariantMap& payload, bool gap, bool hashed)
{
    for (const QString& rawKey : {QStringLiteral("taskId"), QStringLiteral("startupId"), QStringLiteral("deploymentId")}) {
        const QVariant value = payload.value(hashed ? rawKey + QStringLiteral("Hash") : rawKey);
        if (!runText(value) || (gap ? !value.toString().isEmpty()
            : hashed ? !digest(value.toString()) : !uuid(value.toString()))) return false;
    }
    const QVariant ids = payload.value(QStringLiteral("targetVehicleIds"));
    qint64 vehicle = 0;
    if (!targets(ids) || !runInteger(payload.value(QStringLiteral("vehicleId")), gap ? 0 : 1, gap ? 0 : 255, &vehicle)) return false;
    if (gap) return ids.toList().isEmpty();
    bool found = false;
    for (const QVariant& id : ids.toList()) {
        if (!runInteger(id, 1, 255)) return false;
        found = found || id.toDouble() == vehicle;
    }
    return found;
}

bool projectedRunPayload(const QVariantMap& payload)
{
    bool gap = false;
    const bool versionFour = exactString(payload.value(QStringLiteral("payloadProjection")),
        QStringLiteral("fixed-diagnostic-summary-v4"));
    if (!runKeys(payload, {QStringLiteral("originalTypeHash"), QStringLiteral("originalSourceHash"),
            QStringLiteral("originalPayloadHash"), QStringLiteral("payloadProjection"), QStringLiteral("rawPayloadIncluded"),
            QStringLiteral("omittedFieldCount"), QStringLiteral("taskIdHash"), QStringLiteral("startupIdHash"),
            QStringLiteral("deploymentIdHash"), QStringLiteral("targetVehicleIds"), QStringLiteral("vehicleId"),
            QStringLiteral("runObservation")})
        || (!versionFour && !exactString(payload.value(QStringLiteral("payloadProjection")), QStringLiteral("fixed-diagnostic-summary-v3")))
        || !exactString(payload.value(QStringLiteral("originalTypeHash")), hashBytes(QByteArrayLiteral("task.observation.transition")))
        || !exactString(payload.value(QStringLiteral("originalSourceHash")), hashBytes(QByteArrayLiteral("SwarmMissionOrchestrator")))
        || !runText(payload.value(QStringLiteral("originalPayloadHash")))
        || !digest(payload.value(QStringLiteral("originalPayloadHash")).toString())
        || !exactBool(payload.value(QStringLiteral("rawPayloadIncluded")), false)
        || !runInteger(payload.value(QStringLiteral("omittedFieldCount")), 0, 0)
        || !runSummary(payload.value(QStringLiteral("runObservation")), &gap, versionFour ? 2 : 1)) return false;
    return runAssociation(payload, gap, true);
}

QVariantMap projectPayload(const QVariantMap& payload, const QString& originalType,
                           const QString& originalSource, const QByteArray& originalPayloadJson)
{
    QVariantMap summary {
        {QStringLiteral("originalTypeHash"), hashBytes(originalType.toUtf8())},
        {QStringLiteral("originalSourceHash"), hashBytes(originalSource.toUtf8())},
        {QStringLiteral("originalPayloadHash"), hashBytes(originalPayloadJson)},
        {QStringLiteral("payloadProjection"), payload.contains(QStringLiteral("runObservation"))
            ? (runInteger(payload.value(QStringLiteral("runObservation")).toMap().value(QStringLiteral("schemaVersion")), 2, 2)
                ? QStringLiteral("fixed-diagnostic-summary-v4") : QStringLiteral("fixed-diagnostic-summary-v3"))
            : payload.contains(QStringLiteral("engineeringReview"))
                ? QStringLiteral("fixed-diagnostic-summary-v2") : QStringLiteral("fixed-diagnostic-summary-v1")},
        {QStringLiteral("rawPayloadIncluded"), false}
    };
    if (payload.contains(QStringLiteral("runObservation"))) {
        // The exact six-field ingress was validated before projection. Empty
        // gap identities remain explicit empty hashes, never hash("").
        for (const QString& key : {QStringLiteral("taskId"), QStringLiteral("startupId"), QStringLiteral("deploymentId")}) {
            const QString identity = payload.value(key).toString();
            summary.insert(key + QStringLiteral("Hash"), identity.isEmpty() ? QStringLiteral("") : hashBytes(identity.toUtf8()));
        }
        summary.insert(QStringLiteral("targetVehicleIds"), payload.value(QStringLiteral("targetVehicleIds")));
        summary.insert(QStringLiteral("vehicleId"), payload.value(QStringLiteral("vehicleId")));
        summary.insert(QStringLiteral("runObservation"), payload.value(QStringLiteral("runObservation")));
        summary.insert(QStringLiteral("omittedFieldCount"), 0);
        return summary;
    }
    int includedFieldCount = 0;
    const QStringList numericKeys {QStringLiteral("vehicleId"), QStringLiteral("componentId"),
        QStringLiteral("command"), QStringLiteral("result")};
    for (const QString& key : numericKeys) {
        const qint64 minimum = key == QStringLiteral("vehicleId") || key == QStringLiteral("componentId") ? 1 : 0;
        const qint64 maximum = key == QStringLiteral("command") ? 65535 : 255;
        qint64 actual = 0;
        if (payload.contains(key) && integer(payload.value(key), minimum, maximum, &actual)) {
            summary.insert(key, actual);
            ++includedFieldCount;
        }
    }
    const QString targetKey = QStringLiteral("targetVehicleIds");
    if (payload.contains(targetKey) && targets(payload.value(targetKey))) {
        summary.insert(targetKey, payload.value(targetKey));
        ++includedFieldCount;
    }
    const QStringList booleanKeys {QStringLiteral("queued"), QStringLiteral("uncertain")};
    for (const QString& key : booleanKeys) {
        if (!payload.value(key).isNull() && payload.value(key).userType() == QMetaType::Bool) {
            summary.insert(key, payload.value(key));
            ++includedFieldCount;
        }
    }
    for (const QString& key : hashedIdentifiers()) {
        const QVariant value = payload.value(key);
        if (!value.isNull() && value.userType() == QMetaType::QString
            && !value.toString().isEmpty() && value.toString().size() <= 256) {
            summary.insert(key + QStringLiteral("Hash"), hashBytes(value.toString().toUtf8()));
            ++includedFieldCount;
        }
    }
    if (payload.contains(QStringLiteral("engineeringReview"))) {
        summary.insert(QStringLiteral("engineeringReview"), payload.value(QStringLiteral("engineeringReview")));
        ++includedFieldCount;
    }
    summary.insert(QStringLiteral("omittedFieldCount"), payload.size() - includedFieldCount);
    return summary;
}

bool projectedPayload(const QVariant& value)
{
    if (value.isNull() || value.userType() != QMetaType::QVariantMap) return false;
    const QVariantMap payload = value.toMap();
    if (exactString(payload.value(QStringLiteral("payloadProjection")), QStringLiteral("fixed-diagnostic-summary-v3"))
        || exactString(payload.value(QStringLiteral("payloadProjection")), QStringLiteral("fixed-diagnostic-summary-v4")))
        return projectedRunPayload(payload);
    const bool versionTwo = exactString(payload.value(QStringLiteral("payloadProjection")), QStringLiteral("fixed-diagnostic-summary-v2"));
    if (payload.size() < 6 || payload.size() > (versionTwo ? 22 : 21)
        || (!versionTwo && !exactString(payload.value(QStringLiteral("payloadProjection")), QStringLiteral("fixed-diagnostic-summary-v1")))
        || !exactBool(payload.value(QStringLiteral("rawPayloadIncluded")), false)
        || !integer(payload.value(QStringLiteral("omittedFieldCount")), 0, MaximumNodes)) return false;
    const QStringList hashKeys {QStringLiteral("originalTypeHash"), QStringLiteral("originalSourceHash"),
        QStringLiteral("originalPayloadHash")};
    for (const QString& key : hashKeys) {
        if (payload.value(key).userType() != QMetaType::QString || !digest(payload.value(key).toString())) return false;
    }
    QStringList allowed {QStringLiteral("payloadProjection"), QStringLiteral("rawPayloadIncluded"),
        QStringLiteral("omittedFieldCount"), QStringLiteral("vehicleId"), QStringLiteral("componentId"),
        QStringLiteral("targetVehicleIds"), QStringLiteral("command"), QStringLiteral("result"),
        QStringLiteral("queued"), QStringLiteral("uncertain")};
    allowed.append(hashKeys);
    if (versionTwo) allowed.append(QStringLiteral("engineeringReview"));
    for (const QString& key : hashedIdentifiers()) allowed.append(key + QStringLiteral("Hash"));
    for (auto it = payload.constBegin(); it != payload.constEnd(); ++it) {
        if (it.value().isNull() || !allowed.contains(it.key())) return false;
        if (it.key() == QStringLiteral("vehicleId") || it.key() == QStringLiteral("componentId")) {
            if (!integer(it.value(), 1, 255)) return false;
        } else if (it.key() == QStringLiteral("command")) {
            if (!integer(it.value(), 0, 65535)) return false;
        } else if (it.key() == QStringLiteral("result")) {
            if (!integer(it.value(), 0, 255)) return false;
        } else if (it.key() == QStringLiteral("targetVehicleIds")) {
            if (!targets(it.value())) return false;
        } else if (it.key() == QStringLiteral("queued") || it.key() == QStringLiteral("uncertain")) {
            if (it.value().userType() != QMetaType::Bool) return false;
        } else if (it.key() == QStringLiteral("engineeringReview")) {
            qint64 vehicleId = 0;
            if (!integer(payload.value(QStringLiteral("vehicleId")), 1, 255, &vehicleId)
                || !engineeringSummary(it.value(), vehicleId, payload.value(QStringLiteral("originalTypeHash")).toString())) return false;
        } else if (it.key().endsWith(QStringLiteral("Hash"))) {
            if (it.value().userType() != QMetaType::QString || !digest(it.value().toString())) return false;
        }
    }
    return !versionTwo || payload.contains(QStringLiteral("engineeringReview"));
}

bool projectedEnvelope(const QVariantMap& envelope)
{
    for (auto it = envelope.constBegin(); it != envelope.constEnd(); ++it) {
        if (it.value().isNull()) return false;
    }
    if (envelope.size() != 18
        || !exactString(envelope.value(QStringLiteral("format")), QStringLiteral("merivus.fleet.event-archive"))
        || !integer(envelope.value(QStringLiteral("schemaVersion")), 1, 1)
        || envelope.value(QStringLiteral("runId")).userType() != QMetaType::QString
        || !uuid(envelope.value(QStringLiteral("runId")).toString())
        || envelope.value(QStringLiteral("sequence")).userType() != QMetaType::QString
        || !decimal(envelope.value(QStringLiteral("sequence")).toString(), static_cast<quint64>(std::numeric_limits<qint64>::max()))
        || envelope.value(QStringLiteral("sourceEventEpoch")).userType() != QMetaType::QString
        || !uuid(envelope.value(QStringLiteral("sourceEventEpoch")).toString())
        || envelope.value(QStringLiteral("sourceEventSequence")).userType() != QMetaType::QString
        || !decimal(envelope.value(QStringLiteral("sourceEventSequence")).toString(), std::numeric_limits<quint64>::max())
        || envelope.value(QStringLiteral("clockDiscontinuityObserved")).userType() != QMetaType::Bool
        || !exactString(envelope.value(QStringLiteral("eventProvenance")), QStringLiteral("accepted-native-blackbox-summary-not-aircraft-authentication"))
        || !exactBool(envelope.value(QStringLiteral("historicalOnly")), true)
        || !exactBool(envelope.value(QStringLiteral("authenticated")), false)
        || !exactBool(envelope.value(QStringLiteral("signatureVerified")), false)
        || !exactBool(envelope.value(QStringLiteral("executionPermit")), false)
        || !exactBool(envelope.value(QStringLiteral("flightCommandReleased")), false)
        || !exactBool(envelope.value(QStringLiteral("completionVerified")), false)
        || !exactBool(envelope.value(QStringLiteral("restoresAuthorization")), false)
        || envelope.value(QStringLiteral("event")).userType() != QMetaType::QVariantMap) return false;
    qint64 receivedUtc = 0;
    if (!integer(envelope.value(QStringLiteral("receivedAtUtcMs")), 1, MaximumExactInteger, &receivedUtc)
        || !integer(envelope.value(QStringLiteral("receivedAtMonotonicMs")), 0, MaximumExactInteger)) return false;
    const QVariantMap event = envelope.value(QStringLiteral("event")).toMap();
    for (auto it = event.constBegin(); it != event.constEnd(); ++it) {
        if (it.value().isNull()) return false;
    }
    qint64 eventUtc = 0;
    if (event.size() != 4 || !integer(event.value(QStringLiteral("timestampMs")), 1, MaximumExactInteger, &eventUtc)
        || eventUtc > receivedUtc + FutureToleranceMs
        || event.value(QStringLiteral("type")).userType() != QMetaType::QString
        || !exactString(event.value(QStringLiteral("source")), QStringLiteral("native-blackbox-diagnostic"))
        || !projectedPayload(event.value(QStringLiteral("payload")))) return false;
    const QString type = event.value(QStringLiteral("type")).toString();
    if (event.value(QStringLiteral("payload")).toMap().contains(QStringLiteral("engineeringReview"))
        && type != QStringLiteral("fleet")) return false;
    if (event.value(QStringLiteral("payload")).toMap().contains(QStringLiteral("runObservation"))
        && type != QStringLiteral("task")) return false;
    return type == QStringLiteral("command") || type == QStringLiteral("transaction") || type == QStringLiteral("task")
        || type == QStringLiteral("formation") || type == QStringLiteral("fleet") || type == QStringLiteral("intent")
        || type == QStringLiteral("maintenance") || type == QStringLiteral("other");
}

bool boundedContainerDepth(const QByteArray& bytes)
{
    // Bound nesting before Qt creates a JSON tree. This is a size/depth
    // preflight only; JSON syntax and canonical bytes are checked separately.
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (char character : bytes) {
        if (inString) {
            if (escaped) escaped = false;
            else if (character == '\\') escaped = true;
            else if (character == '"') inString = false;
        } else if (character == '"') {
            inString = true;
        } else if (character == '{' || character == '[') {
            if (++depth > MaximumDepth) return false;
        } else if (character == '}' || character == ']') {
            if (--depth < 0) return false;
        }
    }
    return !inString && depth == 0;
}

FleetEventArchiveCodec::Record rejected(const QString& errorCode)
{
    FleetEventArchiveCodec::Record record;
    record.errorCode = errorCode;
    return record;
}
}

bool FleetEventArchiveCodec::runObservationPayloadValid(const QVariantMap& payload)
{
    bool gap = false;
    return runKeys(payload, {QStringLiteral("taskId"), QStringLiteral("startupId"), QStringLiteral("deploymentId"),
            QStringLiteral("targetVehicleIds"), QStringLiteral("vehicleId"), QStringLiteral("runObservation")})
        && runSummary(payload.value(QStringLiteral("runObservation")), &gap)
        && runAssociation(payload, gap, false);
}

QVariantMap FleetEventArchiveCodec::engineeringReviewPayload(int vehicleId, const QString& operation,
    const QVariantMap& result, const QVariantMap& frozenComparison)
{
    if (vehicleId < 1 || vehicleId > 255 || !engineeringOperation(operation)) return {};
    qint64 actualId = 0;
    if ((result.contains(QStringLiteral("vehicleId"))
         && (!integer(result.value(QStringLiteral("vehicleId")), 1, 255, &actualId) || actualId != vehicleId))
        || (result.contains(QStringLiteral("operation")) && !exactString(result.value(QStringLiteral("operation")), operation))) return {};

    const bool baseline = operation == QStringLiteral("approve-baseline") || operation == QStringLiteral("revoke-baseline");
    const QVariantMap receipt = actualMap(result.value(QStringLiteral("commitReceipt")));
    const bool receiptMatches = receipt.size() == 14 && integer(receipt.value(QStringLiteral("schemaVersion")), 1, 1)
        && integer(receipt.value(QStringLiteral("vehicleId")), 1, 255, &actualId) && actualId == vehicleId
        && exactString(receipt.value(QStringLiteral("action")), operation);
    const QString committed = receiptMatches ? triState(receipt.value(QStringLiteral("mutationCommitted")))
        : triState(result.value(QStringLiteral("mutationCommitted")));
    const QString nativeOk = receiptMatches ? triState(receipt.value(QStringLiteral("ok"))) : QStringLiteral("unknown");
    const QString stillCurrent = receiptMatches ? triState(receipt.value(QStringLiteral("stillCurrent"))) : QStringLiteral("unknown");

    ComparisonSummary comparison = comparisonSummary(frozenComparison, vehicleId);
    if (baseline && receiptMatches) {
        const QVariantMap previous = actualMap(receipt.value(QStringLiteral("previousComparison")));
        if (!previous.isEmpty()) comparison = comparisonSummary(previous, vehicleId);
        if (committed == QStringLiteral("yes")) {
            // Keep the reviewed difference/counts, but post-mutation hashes
            // come only from the actual returned receipt, never a proposal.
            const ComparisonSummary post = comparisonSummary(actualMap(receipt.value(QStringLiteral("comparison"))), vehicleId);
            comparison.after = post.after;
            const QVariantMap evidence = actualMap(receipt.value(QStringLiteral("baselineEvidence")));
            comparison.baseline = evidence.size() == 14
                && integer(evidence.value(QStringLiteral("schemaVersion")), 1, 1)
                && exactString(evidence.value(QStringLiteral("source")), QStringLiteral("native-local-configuration-baseline"))
                && exactBool(evidence.value(QStringLiteral("baselineFullKnown")), true)
                ? observedDigest(evidence.value(QStringLiteral("baselineTypedFingerprint"))) : QStringLiteral("");
        }
    }
    const QVariant reviewId = result.value(QStringLiteral("reviewId"));
    const QString reviewHash = !reviewId.isNull() && reviewId.userType() == QMetaType::QString && uuid(reviewId.toString())
        ? hashBytes(reviewId.toString().toUtf8()) : QStringLiteral("");
    const QVariantMap summary {
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("source"), QStringLiteral("local-engineering-review-summary")},
        {QStringLiteral("vehicleId"), vehicleId}, {QStringLiteral("operation"), operation},
        {QStringLiteral("mutationCommitted"), committed}, {QStringLiteral("nativeOk"), nativeOk},
        {QStringLiteral("stillCurrent"), stillCurrent},
        {QStringLiteral("beforeTypedFingerprint"), comparison.before},
        {QStringLiteral("afterTypedFingerprint"), comparison.after},
        {QStringLiteral("baselineTypedFingerprint"), comparison.baseline},
        {QStringLiteral("comparisonKnown"), comparison.known},
        {QStringLiteral("comparisonTruncated"), comparison.truncated},
        {QStringLiteral("reportedChangeCount"), comparison.count},
        {QStringLiteral("displayedChangeCount"), comparison.displayed},
        {QStringLiteral("capabilityCount"), !baseline && receiptMatches && committed == QStringLiteral("yes")
            ? effectiveCapabilityCount(receipt.value(QStringLiteral("effectiveCapabilities"))) : -1},
        {QStringLiteral("reviewIdHash"), reviewHash}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("restoresAuthorization"), false}, {QStringLiteral("hardwareIdentityAuthenticated"), false},
        {QStringLiteral("performanceCertified"), false}, {QStringLiteral("flightCommandReleased"), false}
    };
    if (!engineeringSummary(summary, vehicleId, hashBytes(engineeringEventType(operation).toUtf8()))) return {};
    return {{QStringLiteral("vehicleId"), vehicleId}, {QStringLiteral("engineeringReview"), summary}};
}

FleetEventArchiveCodec::Record FleetEventArchiveCodec::encode(const QVariantMap& exactBlackBoxEvent,
    const QString& runId, quint64 sequence, qint64 receivedUtcMs, qint64 receivedMonotonicMs,
    bool clockDiscontinuityObserved, const QString& sourceEventEpoch, const QString& sourceEventSequence)
{
    if (!uuid(runId) || !uuid(sourceEventEpoch) || sequence == 0
        || sequence > static_cast<quint64>(std::numeric_limits<qint64>::max())
        || !decimal(sourceEventSequence, std::numeric_limits<quint64>::max())
        || receivedUtcMs <= 0 || receivedUtcMs > MaximumExactInteger
        || receivedMonotonicMs < 0 || receivedMonotonicMs > MaximumExactInteger) {
        return rejected(QStringLiteral("archive-receipt-metadata-invalid"));
    }
    qint64 eventUtcMs = 0;
    for (auto it = exactBlackBoxEvent.constBegin(); it != exactBlackBoxEvent.constEnd(); ++it) {
        if (it.value().isNull()) return rejected(QStringLiteral("archive-event-typed-null-invalid"));
    }
    if (exactBlackBoxEvent.size() != 4
        || !integer(exactBlackBoxEvent.value(QStringLiteral("timestampMs")), 1, MaximumExactInteger, &eventUtcMs)
        || eventUtcMs > receivedUtcMs + FutureToleranceMs
        || exactBlackBoxEvent.value(QStringLiteral("type")).userType() != QMetaType::QString
        || exactBlackBoxEvent.value(QStringLiteral("source")).userType() != QMetaType::QString
        || exactBlackBoxEvent.value(QStringLiteral("payload")).userType() != QMetaType::QVariantMap) {
        return rejected(QStringLiteral("archive-event-shape-invalid"));
    }
    const QString originalType = exactBlackBoxEvent.value(QStringLiteral("type")).toString();
    const QString originalSource = exactBlackBoxEvent.value(QStringLiteral("source")).toString();
    qint64 labelBudget = 0;
    if (originalType.isEmpty() || originalType.size() > 128 || originalSource.size() > 256
        || originalType.trimmed() != originalType || originalSource.trimmed() != originalSource
        || !unicodeBudget(originalType, &labelBudget) || !unicodeBudget(originalSource, &labelBudget)) {
        return rejected(QStringLiteral("archive-event-label-invalid"));
    }
    const QVariantMap originalPayload = exactBlackBoxEvent.value(QStringLiteral("payload")).toMap();
    if ((originalType == QStringLiteral("task.observation.transition")
            || originalPayload.contains(QStringLiteral("runObservation")))
        && (originalType != QStringLiteral("task.observation.transition")
            || originalSource != QStringLiteral("SwarmMissionOrchestrator")
            || !runObservationPayloadValid(originalPayload))) return rejected(QStringLiteral("archive-projection-invalid"));
    if (originalPayload.contains(QStringLiteral("engineeringReview"))) {
        qint64 vehicleId = 0;
        if ((originalType != QStringLiteral("fleet.capability.review.result")
             && originalType != QStringLiteral("fleet.configuration.review.result")
             && originalType != QStringLiteral("fleet.performance.declaration.result"))
            || !integer(originalPayload.value(QStringLiteral("vehicleId")), 1, 255, &vehicleId)
            || !engineeringSummary(originalPayload.value(QStringLiteral("engineeringReview")), vehicleId,
                hashBytes(originalType.toUtf8()))) return rejected(QStringLiteral("archive-projection-invalid"));
    }
    QByteArray originalPayloadJson;
    if (!canonical(originalPayload, &originalPayloadJson)) return rejected(QStringLiteral("archive-payload-not-bounded-json"));
    const QString category = eventCategory(originalType);
    const QString projectedSource = QStringLiteral("native-blackbox-diagnostic");
    const QVariantMap projectedEvent {
        {QStringLiteral("timestampMs"), eventUtcMs},
        {QStringLiteral("type"), category},
        {QStringLiteral("source"), projectedSource},
        {QStringLiteral("payload"), projectPayload(originalPayload, originalType, originalSource, originalPayloadJson)}
    };
    const QVariantMap envelope {
        {QStringLiteral("format"), QStringLiteral("merivus.fleet.event-archive")},
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("runId"), runId},
        {QStringLiteral("sequence"), QString::number(sequence)},
        {QStringLiteral("event"), projectedEvent},
        {QStringLiteral("receivedAtUtcMs"), receivedUtcMs},
        {QStringLiteral("receivedAtMonotonicMs"), receivedMonotonicMs},
        {QStringLiteral("clockDiscontinuityObserved"), clockDiscontinuityObserved},
        {QStringLiteral("sourceEventEpoch"), sourceEventEpoch},
        {QStringLiteral("sourceEventSequence"), sourceEventSequence},
        {QStringLiteral("eventProvenance"), QStringLiteral("accepted-native-blackbox-summary-not-aircraft-authentication")},
        {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("authenticated"), false},
        {QStringLiteral("signatureVerified"), false},
        {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("completionVerified"), false},
        {QStringLiteral("restoresAuthorization"), false}
    };
    Record record;
    if (!projectedEnvelope(envelope) || !canonical(envelope, &record.canonicalJson)) {
        return rejected(QStringLiteral("archive-projection-invalid"));
    }
    record.ok = true;
    record.contentHash = hashBytes(record.canonicalJson);
    record.runId = runId;
    record.sequence = QString::number(sequence);
    record.sequenceNumber = static_cast<qint64>(sequence);
    record.eventUtcMs = eventUtcMs;
    record.receivedUtcMs = receivedUtcMs;
    record.receivedMonotonicMs = receivedMonotonicMs;
    record.eventType = category;
    record.eventSource = projectedSource;
    return record;
}

QVariantMap FleetEventArchiveCodec::decodeCanonical(const QByteArray& bytes, const QString& expectedHash, bool* ok)
{
    if (ok) *ok = false;
    if (bytes.isEmpty() || bytes.size() > MaximumBytes || !digest(expectedHash)
        || hashBytes(bytes) != expectedHash || !boundedContainerDepth(bytes)) return {};
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) return {};
    QVariantMap envelope = document.object().toVariantMap();
    if (!projectedEnvelope(envelope)) return {};
    // Qt versions can decode JSON null as a valid Nullptr QVariant. Only the
    // already validated v4 absence slots have that meaning; keep generic and
    // ingress typed-null rejection unchanged, including all older projections.
    QVariantMap event = envelope.value(QStringLiteral("event")).toMap();
    QVariantMap payload = event.value(QStringLiteral("payload")).toMap();
    if (exactString(payload.value(QStringLiteral("payloadProjection")), QStringLiteral("fixed-diagnostic-summary-v4"))) {
        QVariantMap summary = payload.value(QStringLiteral("runObservation")).toMap();
        QVariantMap sidecar = summary.value(QStringLiteral("missionModeExitObservation")).toMap();
        if (!sidecar.value(QStringLiteral("observed")).toBool()) {
            for (const QString& key : {QStringLiteral("first"), QStringLiteral("last")})
                if (sidecar.value(key).userType() == QMetaType::Nullptr) sidecar.insert(key, QVariant());
            summary.insert(QStringLiteral("missionModeExitObservation"), sidecar);
            payload.insert(QStringLiteral("runObservation"), summary);
            event.insert(QStringLiteral("payload"), payload);
            envelope.insert(QStringLiteral("event"), event);
        }
    }
    QByteArray canonicalBytes;
    if (!canonical(envelope, &canonicalBytes) || canonicalBytes != bytes) return {};
    if (ok) *ok = true;
    return envelope;
}
