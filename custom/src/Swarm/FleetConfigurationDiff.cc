#include "FleetConfigurationDiff.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QTimeZone>
#include <QVariantHash>

#include <algorithm>
#include <cmath>

namespace {
constexpr int MaximumDepth = 8;
constexpr int MaximumNodes = 2048;
constexpr qint64 MaximumScalarBytes = 128LL * 1024LL;
constexpr int MaximumValueDisplay = 512;
constexpr int MaximumReportDisplay = 64 * 1024;

struct InputBudget { int nodes = 0; qint64 scalarBytes = 0; };

bool bounded(const QVariant& value, int depth, InputBudget* budget)
{
    if (!budget || depth > MaximumDepth || ++budget->nodes > MaximumNodes) return false;
    // Registry accepts null/invalid before its ordinary-type whitelist. Never
    // call a conversion or a custom operator for such a value.
    if (!value.isValid() || value.isNull()) return true;
    if (value.type() == QVariant::Map || value.type() == QVariant::Hash) {
        const QVariantMap map = value.type() == QVariant::Map ? value.toMap() : QVariantMap();
        const QVariantHash hash = value.type() == QVariant::Hash ? value.toHash() : QVariantHash();
        const QStringList keys = value.type() == QVariant::Map ? map.keys() : hash.keys();
        for (const QString& key : keys) {
            if (key.isEmpty() || key.size() > 128) return false;
            budget->scalarBytes += key.toUtf8().size();
            if (budget->scalarBytes > MaximumScalarBytes
                || !bounded(value.type() == QVariant::Map ? map.value(key) : hash.value(key), depth + 1, budget)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        for (const QVariant& item : value.toList()) if (!bounded(item, depth + 1, budget)) return false;
        return true;
    }
    if (value.type() == QVariant::StringList) {
        // Registry counts QStringList elements without increasing their depth.
        for (const QString& item : value.toStringList()) {
            if (++budget->nodes > MaximumNodes) return false;
            budget->scalarBytes += item.toUtf8().size();
            if (budget->scalarBytes > MaximumScalarBytes) return false;
        }
        return true;
    }
    switch (value.type()) {
    case QVariant::String: budget->scalarBytes += value.toString().toUtf8().size(); break;
    case QVariant::ByteArray: budget->scalarBytes += value.toByteArray().size(); break;
    case QVariant::DateTime:
        budget->scalarBytes += value.toDateTime().toUTC().toString(Qt::ISODateWithMs).toUtf8().size();
        break;
    case QVariant::Double: if (!std::isfinite(value.toDouble())) return false; break;
    case QVariant::Bool:
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong: break;
    default: return false;
    }
    return budget->scalarBytes <= MaximumScalarBytes;
}

QString typeLabel(const QVariant& value)
{
    if (!value.isValid()) return QStringLiteral("invalid");
    switch (value.type()) {
    case QVariant::Map: return QStringLiteral("QVariantMap");
    case QVariant::Hash: return QStringLiteral("QVariantHash");
    case QVariant::List: return QStringLiteral("QVariantList");
    case QVariant::StringList: return QStringLiteral("QStringList");
    case QVariant::String: return QStringLiteral("QString");
    case QVariant::ByteArray: return QStringLiteral("QByteArray");
    case QVariant::DateTime: return QStringLiteral("QDateTime");
    case QVariant::Double: return QStringLiteral("Double");
    case QVariant::Bool: return QStringLiteral("Bool");
    case QVariant::Int: return QStringLiteral("Int");
    case QVariant::UInt: return QStringLiteral("UInt");
    case QVariant::LongLong: return QStringLiteral("LongLong");
    case QVariant::ULongLong: return QStringLiteral("ULongLong");
    default: return QStringLiteral("local-type-%1").arg(value.userType());
    }
}

QString doubleText(double number)
{
    // Numeric equality treats -0 and +0 alike; their canonical bytes must too.
    return number == 0.0 ? QStringLiteral("0") : QString::number(number, 'g', 17);
}

void appendBytes(QByteArray* output, const QByteArray& bytes)
{
    output->append(QByteArray::number(bytes.size())); output->append(':'); output->append(bytes);
}

void appendText(QByteArray* output, const QString& text)
{
    // UTF-8 conversion can collapse unpaired UTF-16 surrogates allowed by the
    // older Registry domain. Preserve the actual code units, big endian.
    output->append(QByteArray::number(text.size())); output->append(':');
    for (QChar ch : text) {
        output->append(static_cast<char>((ch.unicode() >> 8) & 0xff));
        output->append(static_cast<char>(ch.unicode() & 0xff));
    }
}

void encodeValue(QByteArray* output, const QVariant& value)
{
    appendBytes(output, QByteArray::number(value.userType()));
    output->append(value.isValid() ? '1' : '0'); output->append(value.isNull() ? '1' : '0');
    if (!value.isValid() || value.isNull()) return;
    if (value.type() == QVariant::Map || value.type() == QVariant::Hash) {
        const QVariantMap map = value.type() == QVariant::Map ? value.toMap() : QVariantMap();
        const QVariantHash hash = value.type() == QVariant::Hash ? value.toHash() : QVariantHash();
        QStringList keys = value.type() == QVariant::Map ? map.keys() : hash.keys();
        std::sort(keys.begin(), keys.end());
        appendBytes(output, QByteArray::number(keys.size()));
        for (const QString& key : keys) {
            appendText(output, key);
            encodeValue(output, value.type() == QVariant::Map ? map.value(key) : hash.value(key));
        }
        return;
    }
    if (value.type() == QVariant::List) {
        const QVariantList list = value.toList(); appendBytes(output, QByteArray::number(list.size()));
        for (const QVariant& item : list) encodeValue(output, item);
        return;
    }
    if (value.type() == QVariant::StringList) {
        const QStringList list = value.toStringList(); appendBytes(output, QByteArray::number(list.size()));
        for (const QString& item : list) encodeValue(output, QVariant(item));
        return;
    }
    switch (value.type()) {
    case QVariant::String: appendText(output, value.toString()); break;
    case QVariant::ByteArray: appendBytes(output, value.toByteArray()); break;
    case QVariant::Bool: output->append(value.toBool() ? '1' : '0'); break;
    case QVariant::Int: appendBytes(output, QByteArray::number(value.toInt())); break;
    case QVariant::UInt: appendBytes(output, QByteArray::number(value.toUInt())); break;
    case QVariant::LongLong: appendBytes(output, QByteArray::number(value.toLongLong())); break;
    case QVariant::ULongLong: appendBytes(output, QByteArray::number(value.toULongLong())); break;
    case QVariant::Double: appendText(output, doubleText(value.toDouble())); break;
    case QVariant::DateTime: {
        const QDateTime date = value.toDateTime();
        output->append(date.isValid() ? '1' : '0');
        if (date.isValid()) appendBytes(output, QByteArray::number(date.toMSecsSinceEpoch()));
        break;
    }
    default: break; // All non-null unsupported types were rejected first.
    }
}

QString fingerprint(const QVariant& value)
{
    QByteArray canonical("merivus-local-typed-configuration-v1:");
    encodeValue(&canonical, value);
    return QString::fromLatin1(QCryptographicHash::hash(canonical, QCryptographicHash::Sha256).toHex());
}

bool equalValue(const QVariant& first, const QVariant& second)
{
    if (first.userType() != second.userType() || first.isValid() != second.isValid()
        || first.isNull() != second.isNull()) return false;
    if (!first.isValid() || first.isNull()) return true;
    if (first.type() == QVariant::Map || first.type() == QVariant::Hash) {
        const QVariantMap a = first.type() == QVariant::Map ? first.toMap() : QVariantMap();
        const QVariantMap b = second.type() == QVariant::Map ? second.toMap() : QVariantMap();
        const QVariantHash ah = first.type() == QVariant::Hash ? first.toHash() : QVariantHash();
        const QVariantHash bh = second.type() == QVariant::Hash ? second.toHash() : QVariantHash();
        const QStringList keys = first.type() == QVariant::Map ? a.keys() : ah.keys();
        if (keys.size() != (second.type() == QVariant::Map ? b.size() : bh.size())) return false;
        for (const QString& key : keys) {
            if (!(first.type() == QVariant::Map ? b.contains(key) : bh.contains(key))
                || !equalValue(first.type() == QVariant::Map ? a.value(key) : ah.value(key),
                               second.type() == QVariant::Map ? b.value(key) : bh.value(key))) return false;
        }
        return true;
    }
    if (first.type() == QVariant::List) {
        const QVariantList a = first.toList(), b = second.toList();
        if (a.size() != b.size()) return false;
        for (int i = 0; i < a.size(); ++i) if (!equalValue(a.at(i), b.at(i))) return false;
        return true;
    }
    if (first.type() == QVariant::StringList) {
        const QStringList a = first.toStringList(), b = second.toStringList();
        if (a.size() != b.size()) return false;
        for (int i = 0; i < a.size(); ++i) if (!equalValue(QVariant(a.at(i)), QVariant(b.at(i)))) return false;
        return true;
    }
    switch (first.type()) {
    case QVariant::String: return first.toString() == second.toString();
    case QVariant::ByteArray: return first.toByteArray() == second.toByteArray();
    case QVariant::Bool: return first.toBool() == second.toBool();
    case QVariant::Int: return first.toInt() == second.toInt();
    case QVariant::UInt: return first.toUInt() == second.toUInt();
    case QVariant::LongLong: return first.toLongLong() == second.toLongLong();
    case QVariant::ULongLong: return first.toULongLong() == second.toULongLong();
    case QVariant::Double: return first.toDouble() == second.toDouble();
    case QVariant::DateTime: {
        const QDateTime a = first.toDateTime(), b = second.toDateTime();
        return a.isValid() == b.isValid() && (!a.isValid() || a.toMSecsSinceEpoch() == b.toMSecsSinceEpoch());
    }
    default: return false;
    }
}

QString pointerPart(QString key)
{
    key.replace(QStringLiteral("~"), QStringLiteral("~0"));
    key.replace(QStringLiteral("/"), QStringLiteral("~1"));
    return key;
}

QString prefix(const QString& value, int maximum)
{
    int size = std::min(maximum, value.size());
    if (size > 0 && size < value.size() && value.at(size - 1).isHighSurrogate() && value.at(size).isLowSurrogate()) --size;
    return value.left(size);
}

QString stringDisplay(const QString& value)
{
    // Explicitly escape control characters and unpaired surrogates for plain
    // text presentation. This output is never parsed back into typed data.
    QString output;
    for (int i = 0; i < value.size(); ++i) {
        const QChar ch = value.at(i);
        const bool pair = ch.isHighSurrogate() && i + 1 < value.size() && value.at(i + 1).isLowSurrogate();
        if (pair) {
            const QChar low = value.at(++i);
            const uint scalar = 0x10000u + ((static_cast<uint>(ch.unicode()) - 0xd800u) << 10)
                + static_cast<uint>(low.unicode()) - 0xdc00u;
            const QChar::Category category = QChar::category(scalar);
            if (category == QChar::Other_Control || category == QChar::Other_Format)
                output += QStringLiteral("\\u%1\\u%2").arg(ch.unicode(), 4, 16, QLatin1Char('0')).arg(low.unicode(), 4, 16, QLatin1Char('0'));
            else { output += ch; output += low; }
        }
        else if (ch == QLatin1Char('\\')) output += QStringLiteral("\\\\");
        else if (ch.category() == QChar::Other_Control || ch.category() == QChar::Other_Format || ch.isSurrogate())
            output += QStringLiteral("\\u%1").arg(ch.unicode(), 4, 16, QLatin1Char('0'));
        else output += ch;
    }
    return output;
}

QVariantMap displayValue(const QVariant& value, bool present)
{
    QString type = present ? typeLabel(value) : QStringLiteral("missing");
    QString representation, display;
    int byteCount = -1, elementCount = -1;
    bool omitted = false;
    if (!present) { representation = QStringLiteral("missing"); display = QStringLiteral("<missing>"); }
    else if (!value.isValid()) { representation = QStringLiteral("invalid"); display = QStringLiteral("<invalid>"); }
    else if (value.isNull()) { representation = QStringLiteral("typed-null"); display = QStringLiteral("null"); }
    else {
        switch (value.type()) {
        case QVariant::Map:
        case QVariant::Hash:
        case QVariant::List:
        case QVariant::StringList:
            elementCount = value.type() == QVariant::Map ? value.toMap().size()
                : value.type() == QVariant::Hash ? value.toHash().size()
                : value.type() == QVariant::List ? value.toList().size() : value.toStringList().size();
            representation = QStringLiteral("container-typed-summary");
            display = QStringLiteral("%1 (%2 entries)").arg(type).arg(elementCount);
            omitted = elementCount != 0;
            break;
        case QVariant::String:
            representation = QStringLiteral("escaped-text"); byteCount = value.toString().toUtf8().size();
            display = stringDisplay(value.toString()); break;
        case QVariant::ByteArray:
            representation = QStringLiteral("hex"); byteCount = value.toByteArray().size();
            display = QString::fromLatin1(value.toByteArray().toHex()); break;
        case QVariant::Bool:
            representation = QStringLiteral("boolean"); display = value.toBool() ? QStringLiteral("true") : QStringLiteral("false"); break;
        case QVariant::Int: representation = QStringLiteral("decimal"); display = QString::number(value.toInt()); break;
        case QVariant::UInt: representation = QStringLiteral("decimal"); display = QString::number(value.toUInt()); break;
        case QVariant::LongLong: representation = QStringLiteral("decimal"); display = QString::number(value.toLongLong()); break;
        case QVariant::ULongLong: representation = QStringLiteral("decimal"); display = QString::number(value.toULongLong()); break;
        case QVariant::Double: representation = QStringLiteral("finite-double"); display = doubleText(value.toDouble()); break;
        case QVariant::DateTime: {
            const QDateTime date = value.toDateTime();
            representation = QStringLiteral("qt-datetime-utc-equivalence");
            const QString zone = date.timeSpec() == Qt::TimeZone ? QString::fromUtf8(date.timeZone().id()) : QStringLiteral("not-explicit-zone");
            display = QStringLiteral("%1; valid=%2; timeSpec=%3; offsetSeconds=%4; zone=%5; utcMs=%6")
                .arg(date.toString(Qt::ISODateWithMs)).arg(date.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
                .arg(static_cast<int>(date.timeSpec())).arg(date.offsetFromUtc()).arg(zone)
                .arg(date.isValid() ? QString::number(date.toMSecsSinceEpoch()) : QStringLiteral("unknown"));
            break;
        }
        default: break;
        }
    }
    if (display.size() > MaximumValueDisplay) { display = prefix(display, MaximumValueDisplay); omitted = true; }
    return {{QStringLiteral("type"), type}, {QStringLiteral("valid"), present && value.isValid()},
        {QStringLiteral("null"), present && value.isNull()}, {QStringLiteral("representation"), representation},
        {QStringLiteral("display"), display}, {QStringLiteral("displayTruncated"), omitted},
        {QStringLiteral("byteCount"), byteCount}, {QStringLiteral("elementCount"), elementCount},
        {QStringLiteral("typedFingerprint"), present ? fingerprint(value) : QString()}};
}

struct Comparison {
    int maximum = 0;
    int count = 0;
    int displayUnits = 0;
    bool truncated = false;
    QVariantList rows;

    void change(const QString& path, const QString& kind, const QVariant& before, bool beforePresent,
                const QVariant& after, bool afterPresent)
    {
        ++count;
        if (rows.size() >= maximum) { truncated = true; return; }
        // Never truncate a pointer into a different field identifier. Omit the
        // entire row if there is no room even for its complete path.
        if (path.size() > MaximumReportDisplay - displayUnits) { truncated = true; return; }
        QVariantMap oldValue = displayValue(before, beforePresent), newValue = displayValue(after, afterPresent);
        int remaining = MaximumReportDisplay - displayUnits - path.size();
        for (QVariantMap* shown : {&oldValue, &newValue}) {
            const QString display = shown->value(QStringLiteral("display")).toString();
            if (display.size() > remaining) {
                shown->insert(QStringLiteral("display"), prefix(display, remaining));
                shown->insert(QStringLiteral("displayTruncated"), true);
            }
            remaining -= shown->value(QStringLiteral("display")).toString().size();
            if (shown->value(QStringLiteral("displayTruncated")).toBool()) truncated = true;
        }
        displayUnits = MaximumReportDisplay - remaining;
        rows.append(QVariantMap{{QStringLiteral("path"), path}, {QStringLiteral("kind"), kind},
            {QStringLiteral("beforePresent"), beforePresent}, {QStringLiteral("afterPresent"), afterPresent},
            {QStringLiteral("beforeValue"), oldValue}, {QStringLiteral("afterValue"), newValue}});
    }
};

void compareValue(Comparison* result, const QString& path, const QVariant& before, bool beforePresent,
                  const QVariant& after, bool afterPresent)
{
    if (!beforePresent) { result->change(path, QStringLiteral("added"), before, false, after, true); return; }
    if (!afterPresent) { result->change(path, QStringLiteral("removed"), before, true, after, false); return; }
    if (before.userType() != after.userType() || before.isValid() != after.isValid()) {
        result->change(path, QStringLiteral("type-changed"), before, true, after, true); return;
    }
    if (before.isNull() != after.isNull()) {
        result->change(path, QStringLiteral("null-changed"), before, true, after, true); return;
    }
    if (!before.isValid() || before.isNull()) return;
    if (before.type() == QVariant::Map || before.type() == QVariant::Hash) {
        const QVariantMap a = before.type() == QVariant::Map ? before.toMap() : QVariantMap();
        const QVariantMap b = after.type() == QVariant::Map ? after.toMap() : QVariantMap();
        const QVariantHash ah = before.type() == QVariant::Hash ? before.toHash() : QVariantHash();
        const QVariantHash bh = after.type() == QVariant::Hash ? after.toHash() : QVariantHash();
        QStringList keys = before.type() == QVariant::Map ? a.keys() : ah.keys();
        keys.append(after.type() == QVariant::Map ? b.keys() : bh.keys());
        std::sort(keys.begin(), keys.end()); keys.removeDuplicates();
        for (const QString& key : keys) compareValue(result, path + QLatin1Char('/') + pointerPart(key),
            before.type() == QVariant::Map ? a.value(key) : ah.value(key), before.type() == QVariant::Map ? a.contains(key) : ah.contains(key),
            after.type() == QVariant::Map ? b.value(key) : bh.value(key), after.type() == QVariant::Map ? b.contains(key) : bh.contains(key));
        return;
    }
    if (before.type() == QVariant::List || before.type() == QVariant::StringList) {
        QVariantList a, b;
        if (before.type() == QVariant::List) { a = before.toList(); b = after.toList(); }
        else {
            for (const QString& item : before.toStringList()) a.append(item);
            for (const QString& item : after.toStringList()) b.append(item);
        }
        for (int i = 0; i < std::max(a.size(), b.size()); ++i) compareValue(result,
            path + QLatin1Char('/') + QString::number(i), i < a.size() ? a.at(i) : QVariant(), i < a.size(),
            i < b.size() ? b.at(i) : QVariant(), i < b.size());
        return;
    }
    if (!equalValue(before, after)) result->change(path, QStringLiteral("value-changed"), before, true, after, true);
}
} // namespace

bool FleetConfigurationDiff::valueValid(const QVariant& value)
{
    InputBudget budget;
    return bounded(value, 0, &budget);
}

QString FleetConfigurationDiff::typedFingerprint(const QVariantMap& configuration)
{
    return valueValid(configuration) ? fingerprint(configuration) : QString();
}

bool FleetConfigurationDiff::exactEqual(const QVariant& first, const QVariant& second)
{
    return valueValid(first) && valueValid(second) && equalValue(first, second);
}

QVariantMap FleetConfigurationDiff::compare(const QVariantMap& before, bool beforeKnown,
    const QVariantMap& after, bool afterKnown, int maxChanges)
{
    QVariantMap report{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("comparisonState"), QStringLiteral("invalid")},
        {QStringLiteral("comparisonKnown"), false}, {QStringLiteral("complete"), false}, {QStringLiteral("truncated"), false},
        {QStringLiteral("reason"), QStringLiteral("configuration-diff-input-invalid")},
        {QStringLiteral("beforeKnown"), beforeKnown}, {QStringLiteral("afterKnown"), afterKnown},
        {QStringLiteral("beforeTypedFingerprint"), QString()}, {QStringLiteral("afterTypedFingerprint"), QString()},
        {QStringLiteral("changeCount"), 0}, {QStringLiteral("reportedChangeCount"), 0}, {QStringLiteral("changes"), QVariantList()}};
    if (maxChanges < 1 || maxChanges > 128 || !valueValid(before) || !valueValid(after)) return report;
    if (beforeKnown) report.insert(QStringLiteral("beforeTypedFingerprint"), fingerprint(before));
    if (afterKnown) report.insert(QStringLiteral("afterTypedFingerprint"), fingerprint(after));
    if (!beforeKnown || !afterKnown) {
        report.insert(QStringLiteral("comparisonState"), QStringLiteral("unknown"));
        report.insert(QStringLiteral("reason"), QStringLiteral("configuration-diff-snapshot-unknown"));
        return report;
    }
    Comparison comparison; comparison.maximum = maxChanges;
    compareValue(&comparison, QString(), before, true, after, true);
    report.insert(QStringLiteral("comparisonKnown"), true);
    report.insert(QStringLiteral("complete"), !comparison.truncated);
    report.insert(QStringLiteral("truncated"), comparison.truncated);
    report.insert(QStringLiteral("comparisonState"), comparison.truncated ? QStringLiteral("truncated")
        : comparison.count == 0 ? QStringLiteral("same") : QStringLiteral("different"));
    report.insert(QStringLiteral("reason"), comparison.truncated ? QStringLiteral("configuration-diff-display-truncated") : QString());
    report.insert(QStringLiteral("changeCount"), comparison.count);
    report.insert(QStringLiteral("reportedChangeCount"), comparison.rows.size());
    report.insert(QStringLiteral("changes"), comparison.rows);
    return report;
}
