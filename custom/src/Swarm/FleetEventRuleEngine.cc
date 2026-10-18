#include "FleetEventRuleEngine.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QList>
#include <QMetaType>
#include <QPair>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>
#include <QVariantHash>
#include <QtGlobal>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr qint64 kMaximumCooldownMs = 24LL * 60LL * 60LL * 1000LL;
constexpr int kMaximumRuleIdLength = 128;
constexpr qint64 kMaximumSnapshotBytes = 32LL * 1024LL * 1024LL;
constexpr int kMaximumConditionCount = 64;
constexpr int kMaximumConditionPathLength = 256;
constexpr int kMaximumConditionTextLength = 4096;
constexpr int kMaximumConditionListLength = 128;
constexpr int kMaximumPendingReviews = 64;
constexpr qint64 kMaximumReviewBytes = 128LL * 1024LL;
constexpr qint64 kMaximumInboxBytes = 4LL * 1024LL * 1024LL;
constexpr int kReviewChunkCharacters = 2048;

QString explicitReviewText(const QString& value)
{
    return value.isNull() ? QStringLiteral("") : value;
}

bool reviewOffset(const QVariant& value, int maximum, int* output)
{
    if (!output || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (!qIsFinite(number) || number < 0.0 || number > maximum || std::floor(number) != number) return false;
    *output = static_cast<int>(number);
    return true;
}

bool reviewSequence(const QString& text, quint64* output)
{
    if (text.isEmpty() || text.size() > 20) return false;
    for (const QChar character : text)
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 value = text.toULongLong(&ok);
    if (!ok || value == 0 || QString::number(value) != text) return false;
    if (output) *output = value;
    return true;
}

// Budget and reject opaque values BEFORE JSON conversion. The snapshot is a
// JSON projection, not a restoration of arbitrary QVariant/native objects.
bool reviewValueBudget(const QVariant& value, int depth, qint64* units)
{
    if (depth > 24 || ++*units > kMaximumReviewBytes) return false;
    const auto add = [units](qint64 amount) {
        if (amount < 0 || amount > kMaximumReviewBytes - *units) return false;
        *units += amount;
        return true;
    };
    const auto text = [&add](const QString& string) {
        return add(string.size() + qint64(2)) && QString::fromUtf8(string.toUtf8()) == string;
    };
    if (!value.isValid() || value.userType() == QMetaType::Nullptr) return true;
    if (value.isNull()) {
        if (value.userType() == QMetaType::Float) return true;
        switch (value.type()) {
        case QVariant::Map: case QVariant::Hash: case QVariant::List: case QVariant::StringList:
        case QVariant::String: case QVariant::ByteArray: case QVariant::Bool:
        case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong:
        case QVariant::Double: case QVariant::Date: case QVariant::Time: case QVariant::DateTime:
            return true;
        default: return false;
        }
    }
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            if (!text(it.key()) || !reviewValueBudget(it.value(), depth + 1, units)) return false;
        return true;
    }
    if (value.type() == QVariant::Hash) {
        const QVariantHash hash = value.toHash();
        for (auto it = hash.constBegin(); it != hash.constEnd(); ++it)
            if (!text(it.key()) || !reviewValueBudget(it.value(), depth + 1, units)) return false;
        return true;
    }
    if (value.type() == QVariant::List) {
        for (const QVariant& child : value.toList())
            if (!reviewValueBudget(child, depth + 1, units)) return false;
        return true;
    }
    if (value.type() == QVariant::StringList) {
        for (const QString& child : value.toStringList()) if (!text(child)) return false;
        return true;
    }
    if (value.type() == QVariant::String) return text(value.toString());
    if (value.type() == QVariant::ByteArray) return add((qint64(value.toByteArray().size()) + 2) / 3 * 4);
    if (value.userType() == QMetaType::Float) return qIsFinite(value.toFloat());
    switch (value.type()) {
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong: case QVariant::ULongLong:
        return std::abs(value.toDouble()) <= 9007199254740991.0;
    case QVariant::Double: return qIsFinite(value.toDouble());
    case QVariant::Date: case QVariant::Time: case QVariant::DateTime: return true;
    default: return false;
    }
}

QJsonValue reviewJsonValue(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return QJsonValue(QJsonValue::Null);
    if (value.type() == QVariant::Map || value.type() == QVariant::Hash) {
        QVariantMap map;
        if (value.type() == QVariant::Map) map = value.toMap();
        else {
            const QVariantHash hash = value.toHash();
            for (auto it = hash.constBegin(); it != hash.constEnd(); ++it) map.insert(it.key(), it.value());
        }
        QJsonObject object;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) object.insert(it.key(), reviewJsonValue(it.value()));
        return object;
    }
    if (value.type() == QVariant::List) {
        QJsonArray array;
        for (const QVariant& child : value.toList()) array.append(reviewJsonValue(child));
        return array;
    }
    if (value.type() == QVariant::StringList) {
        QJsonArray array;
        for (const QString& child : value.toStringList()) array.append(child);
        return array;
    }
    if (value.type() == QVariant::ByteArray) return QString::fromLatin1(value.toByteArray().toBase64());
    return QJsonValue::fromVariant(value);
}

QVariantMap reviewReadError(const QString& error)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), error}};
}

QVariantMap reviewDecisionError(const QString& error)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("committed"), false}, {QStringLiteral("error"), error},
        {QStringLiteral("reviewId"), QStringLiteral("")}, {QStringLiteral("revision"), QStringLiteral("")},
        {QStringLiteral("decision"), QStringLiteral("")}, {QStringLiteral("reason"), QStringLiteral("")},
        {QStringLiteral("ruleId"), QStringLiteral("")}, {QStringLiteral("eventType"), QStringLiteral("")},
        {QStringLiteral("actionType"), QStringLiteral("")}, {QStringLiteral("generatedAtMs"), 0},
        {QStringLiteral("sourceEpoch"), QStringLiteral("")}, {QStringLiteral("sourceSequence"), QStringLiteral("")},
        {QStringLiteral("decidedAtMs"), 0}, {QStringLiteral("flightCommandReleased"), false}};
}

bool strictInteger(const QVariant& value, qint64 minimum, qint64 maximum,
                   qint64* output = nullptr)
{
    if (!value.isValid() || value.isNull() || minimum > maximum) return false;
    qint64 integer = 0;
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::LongLong:
        integer = value.toLongLong();
        break;
    case QVariant::UInt:
    case QVariant::ULongLong:
        if (value.toULongLong() > quint64(std::numeric_limits<qint64>::max())) return false;
        integer = static_cast<qint64>(value.toULongLong());
        break;
    case QVariant::Double: {
        const double number = value.toDouble();
        // The signed upper bound rounds to 2^63 as double: exclude that
        // boundary BEFORE conversion, then compare the exact integer bounds.
        constexpr double signedLimit = 9223372036854775808.0;
        if (!qIsFinite(number) || std::floor(number) != number
            || number < -signedLimit || number >= signedLimit) return false;
        integer = static_cast<qint64>(number);
        break;
    }
    default:
        return false;
    }
    if (integer < minimum || integer > maximum) return false;
    if (output) *output = integer;
    return true;
}

bool strictBool(const QVariant& value, bool* output = nullptr)
{
    if (!value.isValid() || value.isNull() || value.type() != QVariant::Bool) return false;
    if (output) *output = value.toBool();
    return true;
}

bool strictNumber(const QVariant& value, double* output = nullptr)
{
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        break;
    default:
        return false;
    }
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number)) return false;
    if (output) *output = number;
    return true;
}

bool schemaVersionIsOne(const QVariant& value)
{
    qint64 version = 0;
    return strictInteger(value, 1, 1, &version);
}

bool strictEventTimestamp(const QVariant& value, qint64* output)
{
    return strictInteger(value, 1, std::numeric_limits<qint64>::max(), output);
}

QString normalizedEventType(const QVariantMap& event)
{
    return event.value(QStringLiteral("type"),
                       event.value(QStringLiteral("eventType"))).toString().trimmed();
}

QVariant lookupPath(const QVariant& root, const QStringList& parts)
{
    QVariant current = root;
    for (const QString& part : parts) {
        if (!current.isValid()) {
            return QVariant();
        }
        const QVariantMap map = current.toMap();
        if (!map.contains(part)) {
            return QVariant();
        }
        current = map.value(part);
    }
    return current;
}

QVariant eventValue(const QVariantMap& event, const QString& path)
{
    const QString normalizedPath = path.trimmed();
    if (normalizedPath.isEmpty()) {
        return QVariant();
    }

    const QStringList parts = normalizedPath.split(QLatin1Char('.'), Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return QVariant();
    }

    if (parts.first() == QStringLiteral("payload")) {
        return lookupPath(event.value(QStringLiteral("payload")), parts.mid(1));
    }

    const QVariant direct = lookupPath(event, parts);
    if (direct.isValid()) {
        return direct;
    }
    return lookupPath(event.value(QStringLiteral("payload")), parts);
}

bool numericValue(const QVariant& value, double* result)
{
    return strictNumber(value, result);
}

bool validConditionValue(const QVariant& value, int depth = 0)
{
    if (depth > 4 || !value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Bool:
        return true;
    case QVariant::String:
        return value.toString().size() <= kMaximumConditionTextLength;
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        return strictNumber(value);
    case QVariant::StringList:
        if (value.toStringList().size() > kMaximumConditionListLength) return false;
        for (const QString& item : value.toStringList()) {
            if (item.size() > kMaximumConditionTextLength) return false;
        }
        return true;
    case QVariant::List: {
        const QVariantList list = value.toList();
        if (list.size() > kMaximumConditionListLength) return false;
        for (const QVariant& item : list) {
            if (!validConditionValue(item, depth + 1)) return false;
        }
        return true;
    }
    default:
        return false;
    }
}

bool validConditionPath(const QString& path)
{
    const QString normalized = path.trimmed();
    if (normalized.isEmpty() || normalized.size() > kMaximumConditionPathLength) {
        return false;
    }
    const QStringList parts = normalized.split(QLatin1Char('.'), Qt::SkipEmptyParts);
    if (parts.isEmpty()) return false;
    for (const QString& part : parts) {
        if (part.isEmpty() || part.size() > 64) return false;
        for (const QChar character : part) {
            if (!(character.isLetterOrNumber() || character == QLatin1Char('_')
                  || character == QLatin1Char('-'))) {
                return false;
            }
        }
    }
    return true;
}

bool validConditionDefinition(const QString& path, const QVariant& condition)
{
    if (!validConditionPath(path)) return false;
    if (condition.type() != QVariant::Map) {
        return validConditionValue(condition);
    }

    const QVariantMap map = condition.toMap();
    const QSet<QString> allowedKeys = QSet<QString>()
        << QStringLiteral("operator") << QStringLiteral("op")
        << QStringLiteral("value") << QStringLiteral("equals");
    for (const QString& key : map.keys()) {
        if (!allowedKeys.contains(key)) return false;
    }
    if (map.contains(QStringLiteral("operator"))
        && map.contains(QStringLiteral("op"))
        && map.value(QStringLiteral("operator")) != map.value(QStringLiteral("op"))) {
        return false;
    }
    const QString operation = map.value(QStringLiteral("operator"),
                                        map.value(QStringLiteral("op"), QStringLiteral("equals")))
                                  .toString().trimmed().toLower();
    if (operation.isEmpty() || operation.size() > 32) return false;
    const QSet<QString> knownOperations = QSet<QString>()
        << QStringLiteral("equals") << QStringLiteral("eq") << QStringLiteral("==")
        << QStringLiteral("not_equals") << QStringLiteral("neq") << QStringLiteral("ne")
        << QStringLiteral("!=") << QStringLiteral("contains") << QStringLiteral("in")
        << QStringLiteral("not_in") << QStringLiteral("exists") << QStringLiteral("truthy")
        << QStringLiteral("falsy") << QStringLiteral("greater_than") << QStringLiteral("gt")
        << QStringLiteral("greater_or_equal") << QStringLiteral("gte")
        << QStringLiteral(">=") << QStringLiteral("less_than") << QStringLiteral("lt")
        << QStringLiteral("<") << QStringLiteral("less_or_equal") << QStringLiteral("lte")
        << QStringLiteral("<=") << QStringLiteral(">");
    if (!knownOperations.contains(operation)) return false;

    const bool hasValue = map.contains(QStringLiteral("value"));
    const bool hasEquals = map.contains(QStringLiteral("equals"));
    if (hasValue && hasEquals) return false;
    const bool hasExpected = hasValue || hasEquals;
    if (operation == QStringLiteral("exists")) {
        if (hasExpected && !strictBool(hasValue ? map.value(QStringLiteral("value"))
                                                : map.value(QStringLiteral("equals")))) {
            return false;
        }
        return true;
    }
    if (operation == QStringLiteral("truthy") || operation == QStringLiteral("falsy")) {
        return !hasExpected;
    }
    if (!hasExpected) return false;
    const QVariant expected = hasValue ? map.value(QStringLiteral("value"))
                                      : map.value(QStringLiteral("equals"));
    if (!validConditionValue(expected)) return false;
    if ((operation == QStringLiteral("in") || operation == QStringLiteral("not_in"))
        && expected.type() != QVariant::List
        && expected.type() != QVariant::StringList) {
        return false;
    }
    if ((operation == QStringLiteral("greater_than") || operation == QStringLiteral("gt")
         || operation == QStringLiteral(">") || operation == QStringLiteral("greater_or_equal")
         || operation == QStringLiteral("gte") || operation == QStringLiteral(">=")
         || operation == QStringLiteral("less_than") || operation == QStringLiteral("lt")
         || operation == QStringLiteral("<") || operation == QStringLiteral("less_or_equal")
         || operation == QStringLiteral("lte") || operation == QStringLiteral("<="))
        && !strictNumber(expected)) {
        return false;
    }
    return true;
}

bool validConditions(const QVariantMap& conditions)
{
    if (conditions.size() > kMaximumConditionCount) return false;
    for (auto it = conditions.constBegin(); it != conditions.constEnd(); ++it) {
        if (!validConditionDefinition(it.key(), it.value())) return false;
    }
    return true;
}

bool equalValue(const QVariant& actual, const QVariant& expected)
{
    if (!actual.isValid() || !expected.isValid()) {
        return actual.isValid() == expected.isValid();
    }

    double actualNumber = 0.0;
    double expectedNumber = 0.0;
    const bool actualIsNumber = strictNumber(actual, &actualNumber);
    const bool expectedIsNumber = strictNumber(expected, &expectedNumber);
    if (actualIsNumber || expectedIsNumber) {
        if (!actualIsNumber || !expectedIsNumber) return false;
        return qFuzzyCompare(actualNumber + 1.0, expectedNumber + 1.0);
    }

    if (actual.type() == QVariant::Bool || expected.type() == QVariant::Bool) {
        return actual.type() == QVariant::Bool && expected.type() == QVariant::Bool
            && actual.toBool() == expected.toBool();
    }
    if (actual.type() == QVariant::String && expected.type() == QVariant::String) {
        return actual.toString() == expected.toString();
    }
    if (actual.type() == QVariant::StringList && expected.type() == QVariant::StringList) {
        return actual.toStringList() == expected.toStringList();
    }
    return actual == expected;
}

bool truthyValue(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return false;
    }
    if (value.type() == QVariant::Bool) {
        return value.toBool();
    }
    if (value.canConvert<double>() && value.type() != QVariant::String) {
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (ok) {
            return !qFuzzyIsNull(number);
        }
    }
    return !value.toString().trimmed().isEmpty();
}

bool evaluateCondition(const QVariantMap& event, const QString& path, const QVariant& condition)
{
    const QVariant actual = eventValue(event, path);
    QString operation = QStringLiteral("equals");
    QVariant expected = condition;
    bool hasExpected = true;

    if (condition.type() == QVariant::Map) {
        const QVariantMap map = condition.toMap();
        operation = map.value(QStringLiteral("operator"),
                              map.value(QStringLiteral("op"), QStringLiteral("equals")))
                        .toString()
                        .trimmed()
                        .toLower();
        if (map.contains(QStringLiteral("value"))) {
            expected = map.value(QStringLiteral("value"));
        } else if (map.contains(QStringLiteral("equals"))) {
            expected = map.value(QStringLiteral("equals"));
        } else if (operation == QStringLiteral("exists")) {
            hasExpected = false;
        }
    }

    if (operation == QStringLiteral("exists")) {
        bool expectedExists = true;
        if (hasExpected && !strictBool(expected, &expectedExists)) return false;
        return actual.isValid() == expectedExists;
    }
    if (operation == QStringLiteral("truthy")) {
        return truthyValue(actual);
    }
    if (operation == QStringLiteral("falsy")) {
        return !truthyValue(actual);
    }
    if (operation == QStringLiteral("equals") || operation == QStringLiteral("eq") || operation == QStringLiteral("==")) {
        return equalValue(actual, expected);
    }
    if (operation == QStringLiteral("not_equals") || operation == QStringLiteral("neq") || operation == QStringLiteral("ne") || operation == QStringLiteral("!=")) {
        return !equalValue(actual, expected);
    }

    if (operation == QStringLiteral("contains")) {
        if (actual.type() == QVariant::String || actual.type() == QVariant::StringList) {
            if (actual.type() == QVariant::StringList) {
                return actual.toStringList().contains(expected.toString());
            }
            return actual.toString().contains(expected.toString(), Qt::CaseSensitive);
        }
        if (actual.type() == QVariant::List) {
            for (const QVariant& item : actual.toList()) {
                if (equalValue(item, expected)) {
                    return true;
                }
            }
        }
        return false;
    }
    if (operation == QStringLiteral("in") || operation == QStringLiteral("not_in")) {
        bool found = false;
        QVariantList values;
        if (expected.type() == QVariant::StringList) {
            for (const QString& item : expected.toStringList()) values.append(item);
        } else {
            values = expected.toList();
        }
        for (const QVariant& value : values) {
            if (equalValue(actual, value)) {
                found = true;
                break;
            }
        }
        return operation == QStringLiteral("in") ? found : !found;
    }

    double actualNumber = 0.0;
    double expectedNumber = 0.0;
    if (!numericValue(actual, &actualNumber) || !numericValue(expected, &expectedNumber)) {
        return false;
    }
    if (operation == QStringLiteral("greater_than") || operation == QStringLiteral("gt") || operation == QStringLiteral(">")) {
        return actualNumber > expectedNumber;
    }
    if (operation == QStringLiteral("greater_or_equal") || operation == QStringLiteral("gte") || operation == QStringLiteral(">=")) {
        return actualNumber >= expectedNumber;
    }
    if (operation == QStringLiteral("less_than") || operation == QStringLiteral("lt") || operation == QStringLiteral("<")) {
        return actualNumber < expectedNumber;
    }
    if (operation == QStringLiteral("less_or_equal") || operation == QStringLiteral("lte") || operation == QStringLiteral("<=")) {
        return actualNumber <= expectedNumber;
    }
    return false;
}

QVariantList normalizedActions(const QVariantMap& input)
{
    QVariant value = input.value(QStringLiteral("actions"));
    if (!value.isValid()) {
        value = input.value(QStringLiteral("action"));
    }

    QVariantList actions;
    if (value.type() == QVariant::List) {
        actions = value.toList();
    } else if (value.type() == QVariant::Map || value.type() == QVariant::String) {
        actions.append(value);
    }
    return actions;
}

bool validAction(const QVariant& action)
{
    if (action.type() == QVariant::String) {
        return !action.toString().trimmed().isEmpty();
    }
    if (action.type() != QVariant::Map) {
        return false;
    }
    const QVariantMap map = action.toMap();
    const QVariant typeValue = map.contains(QStringLiteral("type"))
        ? map.value(QStringLiteral("type")) : map.value(QStringLiteral("name"));
    return typeValue.type() == QVariant::String
        && !typeValue.toString().trimmed().isEmpty();
}

int rulePriority(const QVariantMap& rule)
{
    qint64 priority = 0;
    if (!strictInteger(rule.value(QStringLiteral("priority"), 0), -100000, 100000, &priority)) {
        return 0;
    }
    return static_cast<int>(priority);
}

qint64 ruleCooldownMs(const QVariantMap& rule)
{
    qint64 cooldown = 0;
    return strictInteger(rule.value(QStringLiteral("cooldownMs"), 0), 0,
                         kMaximumCooldownMs, &cooldown) ? cooldown : 0;
}

} // namespace

FleetEventRuleEngine::FleetEventRuleEngine(QObject* parent)
    : QObject(parent)
{
}

void FleetEventRuleEngine::setEnabled(bool enabled)
{
    if (_enabled == enabled) {
        return;
    }
    _enabled = enabled;
    emit enabledChanged();
}

bool FleetEventRuleEngine::registerRule(const QVariantMap& input)
{
    const QSet<QString> allowedKeys = QSet<QString>()
        << QStringLiteral("id") << QStringLiteral("eventType")
        << QStringLiteral("type") << QStringLiteral("enabled")
        << QStringLiteral("priority") << QStringLiteral("conditions")
        << QStringLiteral("actions") << QStringLiteral("action")
        << QStringLiteral("description") << QStringLiteral("cooldownMs")
        << QStringLiteral("previousEventType");
    for (const QString& key : input.keys()) {
        if (!allowedKeys.contains(key)) return false;
    }

    if (!input.contains(QStringLiteral("id"))
        || input.value(QStringLiteral("id")).type() != QVariant::String) {
        return false;
    }
    const QString id = input.value(QStringLiteral("id")).toString().trimmed();
    if ((input.contains(QStringLiteral("eventType"))
         && input.value(QStringLiteral("eventType")).type() != QVariant::String)
        || (input.contains(QStringLiteral("type"))
            && input.value(QStringLiteral("type")).type() != QVariant::String)) {
        return false;
    }
    const QString eventType = input.contains(QStringLiteral("eventType"))
        ? input.value(QStringLiteral("eventType")).toString().trimmed()
        : input.value(QStringLiteral("type")).toString().trimmed();
    if (input.contains(QStringLiteral("eventType")) && input.contains(QStringLiteral("type"))
        && input.value(QStringLiteral("eventType")).toString().trimmed()
            != input.value(QStringLiteral("type")).toString().trimmed()) {
        return false;
    }
    if (id.isEmpty() || id.size() > kMaximumRuleIdLength
        || eventType.isEmpty() || _rules.size() >= 256) {
        return false;
    }
    if (!rule(id).isEmpty()) {
        return false;
    }

    const QVariantList actions = normalizedActions(input);
    if (actions.isEmpty()) {
        return false;
    }
    for (const QVariant& action : actions) {
        if (!validAction(action)) {
            return false;
        }
    }
    if (input.contains(QStringLiteral("conditions"))) {
        if (input.value(QStringLiteral("conditions")).type() != QVariant::Map
            || !validConditions(input.value(QStringLiteral("conditions")).toMap())) {
            return false;
        }
    }
    bool enabled = true;
    if (input.contains(QStringLiteral("enabled"))
        && !strictBool(input.value(QStringLiteral("enabled")), &enabled)) {
        return false;
    }
    qint64 priority = 0;
    if (input.contains(QStringLiteral("priority"))
        && !strictInteger(input.value(QStringLiteral("priority")), -100000, 100000, &priority)) {
        return false;
    }
    qint64 cooldown = 0;
    if (input.contains(QStringLiteral("cooldownMs"))
        && !strictInteger(input.value(QStringLiteral("cooldownMs")), 0,
                          kMaximumCooldownMs, &cooldown)) {
        return false;
    }
    QString previousEventType;
    if (input.contains(QStringLiteral("previousEventType"))) {
        if (input.value(QStringLiteral("previousEventType")).type() != QVariant::String) {
            return false;
        }
        previousEventType = input.value(QStringLiteral("previousEventType")).toString().trimmed();
        if (previousEventType.isEmpty()) return false;
    }
    if (input.contains(QStringLiteral("description"))) {
        const QVariant description = input.value(QStringLiteral("description"));
        if (description.type() != QVariant::String || description.toString().size() > 4096) {
            return false;
        }
    }

    QVariantMap normalized;
    normalized.insert(QStringLiteral("id"), id);
    normalized.insert(QStringLiteral("eventType"), eventType);
    normalized.insert(QStringLiteral("enabled"), enabled);
    normalized.insert(QStringLiteral("priority"), static_cast<int>(priority));
    normalized.insert(QStringLiteral("cooldownMs"), cooldown);
    normalized.insert(QStringLiteral("conditions"), input.value(QStringLiteral("conditions")).toMap());
    normalized.insert(QStringLiteral("actions"), actions);
    if (!previousEventType.isEmpty()) {
        normalized.insert(QStringLiteral("previousEventType"), previousEventType);
    }
    if (input.contains(QStringLiteral("description"))) {
        normalized.insert(QStringLiteral("description"), input.value(QStringLiteral("description")).toString());
    }

    _rules.append(normalized);
    emit rulesChanged();
    return true;
}

bool FleetEventRuleEngine::setRuleEnabled(const QString& ruleId, bool enabled)
{
    const QString id = ruleId.trimmed();
    for (int index = 0; index < _rules.size(); ++index) {
        QVariantMap current = _rules.at(index).toMap();
        if (current.value(QStringLiteral("id")).toString() != id) {
            continue;
        }
        if (current.value(QStringLiteral("enabled"), true).toBool() == enabled) {
            return true;
        }
        current.insert(QStringLiteral("enabled"), enabled);
        _rules[index] = current;
        if (!enabled) {
            _runtimeState.remove(id);
            _liveRuntimeState.remove(id);
        }
        emit rulesChanged();
        return true;
    }
    return false;
}

bool FleetEventRuleEngine::removeRule(const QString& ruleId)
{
    const QString id = ruleId.trimmed();
    for (int index = 0; index < _rules.size(); ++index) {
        if (_rules.at(index).toMap().value(QStringLiteral("id")).toString() != id) {
            continue;
        }
        _rules.removeAt(index);
        _runtimeState.remove(id);
        _liveRuntimeState.remove(id);
        emit rulesChanged();
        return true;
    }
    return false;
}

void FleetEventRuleEngine::clearRules()
{
    if (_rules.isEmpty() && _runtimeState.isEmpty()) {
        return;
    }
    _rules.clear();
    _runtimeState.clear();
    _liveRuntimeState.clear();
    emit rulesChanged();
}

QVariantMap FleetEventRuleEngine::rule(const QString& ruleId) const
{
    const QString id = ruleId.trimmed();
    for (const QVariant& value : _rules) {
        const QVariantMap current = value.toMap();
        if (current.value(QStringLiteral("id")).toString() == id) {
            return current;
        }
    }
    return QVariantMap();
}

void FleetEventRuleEngine::resetRuntimeState()
{
    _runtimeState.clear();
}

QVariantMap FleetEventRuleEngine::runtimeState(const QString& ruleId) const
{
    const QString id = ruleId.trimmed();
    const auto it = _runtimeState.constFind(id);
    if (it == _runtimeState.constEnd()) return QVariantMap();

    const RuntimeState& state = it.value();
    return QVariantMap{{QStringLiteral("ruleId"), id},
                       {QStringLiteral("lastEventTimestampMs"), state.lastEventTimestampMs},
                       {QStringLiteral("lastMatchTimestampMs"), state.lastMatchTimestampMs},
                       {QStringLiteral("matchCount"), state.matchCount},
                       {QStringLiteral("lastEventType"), state.lastEventType}};
}

QString FleetEventRuleEngine::exportJson() const
{
    QVariantList sortedRules = _rules;
    std::sort(sortedRules.begin(), sortedRules.end(), [](const QVariant& left,
                                                        const QVariant& right) {
        return left.toMap().value(QStringLiteral("id")).toString()
            < right.toMap().value(QStringLiteral("id")).toString();
    });

    const QVariantMap root{
        {QStringLiteral("format"), QStringLiteral("merivus.fleet.event-rules")},
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("engineVersion"), QStringLiteral("1.0.0")},
        {QStringLiteral("enabled"), _enabled},
        {QStringLiteral("rules"), sortedRules},
        {QStringLiteral("flightCommandReleased"), false}
    };
    const QJsonDocument document = QJsonDocument::fromVariant(root);
    return document.isNull()
        ? QString()
        : QString::fromUtf8(document.toJson(QJsonDocument::Compact));
}

bool FleetEventRuleEngine::importJson(const QString& json, bool replaceExisting)
{
    const QByteArray payload = json.toUtf8();
    if (payload.isEmpty() || payload.size() > kMaximumSnapshotBytes) return false;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) return false;
    const QVariantMap root = document.object().toVariantMap();
    const QSet<QString> allowedKeys = QSet<QString>()
        << QStringLiteral("format") << QStringLiteral("schemaVersion")
        << QStringLiteral("engineVersion") << QStringLiteral("enabled")
        << QStringLiteral("rules") << QStringLiteral("flightCommandReleased");
    for (const QString& key : root.keys()) {
        if (!allowedKeys.contains(key)) return false;
    }
    if (root.value(QStringLiteral("format")).type() != QVariant::String
        || root.value(QStringLiteral("format")).toString()
               != QStringLiteral("merivus.fleet.event-rules")
        || !schemaVersionIsOne(root.value(QStringLiteral("schemaVersion")))) {
        return false;
    }
    if (root.contains(QStringLiteral("engineVersion"))) {
        const QVariant version = root.value(QStringLiteral("engineVersion"));
        if (version.type() != QVariant::String || version.toString().trimmed().isEmpty()
            || version.toString().size() > 64) return false;
    }
    bool importedEnabled = true;
    if (root.contains(QStringLiteral("enabled"))
        && !strictBool(root.value(QStringLiteral("enabled")), &importedEnabled)) {
        return false;
    }
    if (root.value(QStringLiteral("rules")).type() != QVariant::List) return false;
    if (root.contains(QStringLiteral("flightCommandReleased"))) {
        bool release = false;
        if (!strictBool(root.value(QStringLiteral("flightCommandReleased")), &release)
            || release) return false;
    }

    FleetEventRuleEngine staged;
    if (!replaceExisting) {
        for (const QVariant& existing : _rules) {
            if (!staged.registerRule(existing.toMap())) return false;
        }
    }
    const QVariantList imported = root.value(QStringLiteral("rules")).toList();
    if (imported.size() > 256) return false;
    for (const QVariant& value : imported) {
        if (value.type() != QVariant::Map || !staged.registerRule(value.toMap())) {
            return false;
        }
    }

    _rules = staged._rules;
    _runtimeState.clear();
    _liveRuntimeState.clear();
    _enabled = importedEnabled;
    emit rulesChanged();
    emit enabledChanged();
    return true;
}

bool FleetEventRuleEngine::saveToFile(const QString& filePath) const
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) return false;
    const QByteArray payload = exportJson().toUtf8();
    if (payload.isEmpty() || payload.size() > kMaximumSnapshotBytes) return false;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    if (file.write(payload) != payload.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

bool FleetEventRuleEngine::loadFromFile(const QString& filePath, bool replaceExisting)
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > kMaximumSnapshotBytes) return false;
    const QByteArray payload = file.read(kMaximumSnapshotBytes + 1);
    if (payload.size() > kMaximumSnapshotBytes) return false;
    const QString text = QString::fromUtf8(payload.constData(), payload.size());
    if (text.toUtf8() != payload) return false;
    return importJson(text, replaceExisting);
}

QVariantList FleetEventRuleEngine::evaluate(const QVariantMap& event) const
{
    return evaluateForState(event, _runtimeState, -1, nullptr);
}

QVariantList FleetEventRuleEngine::evaluateForState(const QVariantMap& event,
    QHash<QString, RuntimeState>& state, int maximumSuggestions, bool* overflow) const
{
    if (overflow) *overflow = false;
    QVariantList suggestions;
    if (!_enabled) {
        return suggestions;
    }

    const QString type = normalizedEventType(event);
    if (type.isEmpty()) {
        return suggestions;
    }

    const bool hasTimestamp = event.contains(QStringLiteral("timestampMs"))
        || event.contains(QStringLiteral("timeMs"));
    qint64 eventTimestamp = 0;
    if (hasTimestamp) {
        const QVariant timestampValue = event.contains(QStringLiteral("timestampMs"))
            ? event.value(QStringLiteral("timestampMs")) : event.value(QStringLiteral("timeMs"));
        if (!strictEventTimestamp(timestampValue, &eventTimestamp)) {
            return suggestions;
        }
    } else {
        eventTimestamp = QDateTime::currentMSecsSinceEpoch();
    }

    QList<QPair<int, QVariantMap>> matchingRules;
    for (const QVariant& value : _rules) {
        const QVariantMap current = value.toMap();
        if (!current.value(QStringLiteral("enabled"), true).toBool()) {
            continue;
        }
        const QString ruleId = current.value(QStringLiteral("id")).toString();
        const RuntimeState previousState = state.value(ruleId);
        RuntimeState& runtime = state[ruleId];
        // Record every valid event for enabled rules, not only events whose
        // type matches the rule. This makes previousEventType useful for a
        // sequence such as A -> B when the rule itself listens for B.
        runtime.lastEventTimestampMs = eventTimestamp;
        runtime.lastEventType = type;

        const QString eventType = current.value(QStringLiteral("eventType")).toString().trimmed();
        if (eventType != QStringLiteral("*") && eventType != type) {
            continue;
        }

        const QString previousEventType = current.value(QStringLiteral("previousEventType"))
            .toString().trimmed();
        const bool previousEventMatches = previousEventType.isEmpty()
            || previousState.lastEventType == previousEventType;
        const qint64 cooldown = ruleCooldownMs(current);
        const bool cooldownActive = cooldown > 0
            && previousState.lastMatchTimestampMs > 0
            && eventTimestamp >= previousState.lastMatchTimestampMs
            && eventTimestamp - previousState.lastMatchTimestampMs < cooldown;

        // Preserve a compact event history even when this rule does not
        // match, so a rule with previousEventType can express a two-event
        // sequence without invoking a command path.
        if (!previousEventMatches || cooldownActive) {
            continue;
        }

        bool matches = true;
        const QVariantMap conditions = current.value(QStringLiteral("conditions")).toMap();
        for (auto condition = conditions.constBegin(); condition != conditions.constEnd(); ++condition) {
            if (!evaluateCondition(event, condition.key(), condition.value())) {
                matches = false;
                break;
            }
        }
        if (matches) {
            if (maximumSuggestions >= 0 && runtime.matchCount == std::numeric_limits<int>::max()) {
                if (overflow) *overflow = true;
                return {};
            }
            runtime.lastMatchTimestampMs = eventTimestamp;
            ++runtime.matchCount;
            matchingRules.append(qMakePair(rulePriority(current), current));
        }
    }

    std::stable_sort(matchingRules.begin(), matchingRules.end(), [](const QPair<int, QVariantMap>& left, const QPair<int, QVariantMap>& right) {
        return left.first > right.first;
    });

    for (const auto& matched : matchingRules) {
        const QVariantMap current = matched.second;
        const QVariantList actions = current.value(QStringLiteral("actions")).toList();
        for (const QVariant& action : actions) {
            if (maximumSuggestions >= 0 && suggestions.size() >= maximumSuggestions) {
                if (overflow) *overflow = true;
                return {};
            }
            QVariantMap suggestion;
            suggestion.insert(QStringLiteral("ruleId"), current.value(QStringLiteral("id")));
            suggestion.insert(QStringLiteral("eventType"), type);
            suggestion.insert(QStringLiteral("priority"), matched.first);
            suggestion.insert(QStringLiteral("recommendationOnly"), true);
            suggestion.insert(QStringLiteral("manualConfirmationRequired"), true);
            suggestion.insert(QStringLiteral("flightCommandReleased"), false);
            suggestion.insert(QStringLiteral("commandDispatch"), QStringLiteral("not-released"));
            suggestion.insert(QStringLiteral("generatedAtMs"), eventTimestamp);
            suggestion.insert(QStringLiteral("cooldownMs"), ruleCooldownMs(current));
            suggestion.insert(QStringLiteral("matchCount"),
                              state.value(current.value(QStringLiteral("id")).toString()).matchCount);
            suggestion.insert(QStringLiteral("stateful"),
                              current.contains(QStringLiteral("previousEventType"))
                              || ruleCooldownMs(current) > 0);
            if (action.type() == QVariant::String) {
                QVariantMap actionMap;
                actionMap.insert(QStringLiteral("type"), action.toString().trimmed());
                suggestion.insert(QStringLiteral("action"), actionMap);
            } else {
                suggestion.insert(QStringLiteral("action"), action.toMap());
            }
            suggestions.append(suggestion);
        }
    }
    return suggestions;
}

bool FleetEventRuleEngine::bindLiveSource(FleetEventBlackBox* source)
{
    if (QThread::currentThread() != thread() || _liveBusy
        || (source && source->thread() != thread())) return false;
    if (_liveSource.data() == source) return true;
    if (_liveBinding == std::numeric_limits<quint64>::max()) return false;
    const QPointer<FleetEventRuleEngine> self(this);
    const QPointer<FleetEventBlackBox> actual(source);
    const quint64 binding = ++_liveBinding;
    _liveBusy = true;
    bool released = false;
    const auto release = [&]() { if (!released) { released = true; if (self) self->_liveBusy = false; } };
    const auto leave = qScopeGuard(release);
    const QVector<QMetaObject::Connection> previous = _liveConnections;
    _liveConnections.clear();
    _liveSource = actual;
    _liveRuntimeState.clear();
    _liveLastSequence = 0;
    _reviewLastError = QStringLiteral("");
    const auto current = [&]() {
        return self && self->_liveBinding == binding && self->_liveSource == actual
            && (!source || (actual && actual->thread() == self->thread()));
    };
    for (const auto& connection : previous) {
        QObject::disconnect(connection);
        if (!current()) return false;
    }
    if (actual) {
        const auto accepted = connect(actual.data(), &FleetEventBlackBox::nativeEventAccepted, this,
            [self, actual, binding](const QString& epoch, const QString& sequence, const QVariantMap& event) {
                if (self && actual) self->acceptLiveEvent(actual.data(), binding, epoch, sequence, event);
            }, Qt::DirectConnection);
        if (!current()) return false;
        _liveConnections.append(accepted);
        const auto destroyed = connect(actual.data(), &QObject::destroyed, this, [self, binding]() {
            if (!self || self->_liveBinding != binding) return;
            self->_liveSource.clear();
            self->_liveRuntimeState.clear();
            self->_liveLastSequence = 0;
            // Retained pending items remain historical, never executable.
            emit self->reviewsChanged();
        });
        if (!current()) return false;
        _liveConnections.append(destroyed);
    }
    release();
    emit reviewsChanged();
    return current();
}

void FleetEventRuleEngine::rejectLiveEvent(const QString& error)
{
    if (_reviewOverflowCount != std::numeric_limits<quint64>::max()) ++_reviewOverflowCount;
    _reviewLastError = error;
}

void FleetEventRuleEngine::acceptLiveEvent(FleetEventBlackBox* source, quint64 binding,
    const QString& epoch, const QString& sequence, const QVariantMap& event)
{
    if (QThread::currentThread() != thread() || !source || _liveSource.data() != source || _liveBinding != binding) return;
    quint64 nativeSequence = 0;
    if (!reviewSequence(sequence, &nativeSequence) || nativeSequence <= _liveLastSequence) return;
    const QPointer<FleetEventRuleEngine> self(this);
    const QPointer<FleetEventBlackBox> actual(source);
    const auto current = [&]() {
        return self && actual && self->_liveSource == actual && self->_liveBinding == binding
            && actual->thread() == self->thread() && actual->nativeSourceEpoch() == epoch;
    };
    if (!current() || !actual->nativeEventReceiptMatches(epoch, sequence, event) || !current()) return;
    // Only after the native receipt matches may event values be inspected.
    if (normalizedEventType(event).startsWith(QStringLiteral("fleet.rule_review."))) return;
    if (_liveBusy) { rejectLiveEvent(QStringLiteral("live-review-reentrant")); return; }
    _liveBusy = true;
    bool released = false;
    const auto release = [&]() { if (!released) { released = true; if (self) self->_liveBusy = false; } };
    const auto leave = qScopeGuard(release);
    // This original native publication is consumed once even if capacity or
    // budget prevents retention. It cannot be replayed later as a new live event.
    _liveLastSequence = nativeSequence;
    const quint64 overflowAtEntry = _reviewOverflowCount;
    const auto fail = [&](const QString& error) {
        if (!current()) return;
        rejectLiveEvent(error);
        release();
        emit self->reviewsChanged();
    };
    qint64 sourceUnits = 0;
    if (!reviewValueBudget(event, 0, &sourceUnits)) { fail(QStringLiteral("live-review-value-budget-or-type")); return; }
    QHash<QString, RuntimeState> candidateState = _liveRuntimeState;
    bool overflow = false;
    const QVariantList suggestions = evaluateForState(event, candidateState,
        kMaximumPendingReviews - _pendingReviews.size(), &overflow);
    if (overflow) { fail(QStringLiteral("live-review-capacity-or-counter")); return; }
    if (quint64(suggestions.size()) > std::numeric_limits<quint64>::max() - _reviewRevision) {
        fail(QStringLiteral("live-review-revision-exhausted")); return;
    }
    QVector<PendingReview> candidates;
    qint64 addedBytes = 0;
    quint64 revision = _reviewRevision;
    QSet<QString> ids;
    for (const auto& item : _pendingReviews) ids.insert(item.row.value(QStringLiteral("reviewId")).toString());
    for (const QVariant& value : suggestions) {
        const QVariantMap suggestion = value.toMap();
        const QString ruleId = suggestion.value(QStringLiteral("ruleId")).toString();
        const QVariantMap definition = rule(ruleId);
        const QVariantMap action = suggestion.value(QStringLiteral("action")).toMap();
        const QString actionType = action.value(QStringLiteral("type"), action.value(QStringLiteral("name"))).toString().trimmed();
        if (definition.isEmpty() || actionType.isEmpty() || actionType.size() > 128) {
            fail(QStringLiteral("live-review-value-budget-or-type")); return;
        }
        const QVariantMap snapshot{{QStringLiteral("schemaVersion"), 1},
            {QStringLiteral("source"), QVariantMap{{QStringLiteral("sourceEpoch"), epoch},
                {QStringLiteral("sourceSequence"), sequence}, {QStringLiteral("event"), event}}},
            {QStringLiteral("rule"), definition}, {QStringLiteral("action"), action},
            {QStringLiteral("suggestion"), suggestion}, {QStringLiteral("recommendationOnly"), true},
            {QStringLiteral("flightCommandReleased"), false}};
        qint64 units = 0;
        if (!reviewValueBudget(snapshot, 0, &units)) { fail(QStringLiteral("live-review-value-budget-or-type")); return; }
        const QByteArray json = QJsonDocument(reviewJsonValue(snapshot).toObject()).toJson(QJsonDocument::Compact);
        if (json.isEmpty() || json.size() > kMaximumReviewBytes
            || json.size() > kMaximumInboxBytes - _pendingReviewBytes - addedBytes) {
            fail(QStringLiteral("live-review-byte-budget")); return;
        }
        const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (id.isEmpty() || ids.contains(id)) { fail(QStringLiteral("live-review-id-collision")); return; }
        ids.insert(id);
        PendingReview entry;
        entry.row = {{QStringLiteral("reviewId"), id}, {QStringLiteral("revision"), QString::number(++revision)},
            {QStringLiteral("ruleId"), ruleId}, {QStringLiteral("eventType"), suggestion.value(QStringLiteral("eventType"))},
            {QStringLiteral("actionType"), actionType}, {QStringLiteral("generatedAtMs"), suggestion.value(QStringLiteral("generatedAtMs"))},
            {QStringLiteral("sourceEpoch"), epoch}, {QStringLiteral("sourceSequence"), sequence}};
        entry.detail = QString::fromUtf8(json);
        entry.encodedBytes = json.size();
        addedBytes += entry.encodedBytes;
        candidates.append(entry);
    }
    if (!current()) return;
    // Commit the complete event batch without external calls or trimming.
    _liveRuntimeState = candidateState;
    _reviewRevision = revision;
    for (const auto& candidate : candidates) _pendingReviews.append(candidate);
    _pendingReviewBytes += addedBytes;
    const bool reviewChanged = !candidates.isEmpty() || !_reviewLastError.isEmpty()
        || _reviewOverflowCount != overflowAtEntry;
    if (_reviewOverflowCount == overflowAtEntry) _reviewLastError = QStringLiteral("");
    release();
    if (reviewChanged) emit reviewsChanged(); // No access to this or the source after publication.
}

QVariantList FleetEventRuleEngine::reviewRows() const
{
    if (QThread::currentThread() != thread()) return {};
    QVariantList rows;
    for (const auto& entry : _pendingReviews) rows.append(entry.row);
    return rows;
}

QVariantMap FleetEventRuleEngine::reviewStatus() const
{
    if (QThread::currentThread() != thread()) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("pendingCount"), _pendingReviews.size()},
        {QStringLiteral("overflowCount"), QString::number(_reviewOverflowCount)}, {QStringLiteral("lastError"), _reviewLastError},
        {QStringLiteral("sessionOnly"), true}, {QStringLiteral("automaticActionsIssued"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetEventRuleEngine::reviewPage(const QVariant& offset) const
{
    if (QThread::currentThread() != thread()) return reviewReadError(QStringLiteral("review-wrong-thread"));
    int first = 0;
    if (!reviewOffset(offset, _pendingReviews.size(), &first)) return reviewReadError(QStringLiteral("review-offset-invalid"));
    const int next = qMin(_pendingReviews.size(), first + 4);
    QVariantList rows;
    for (int index = first; index < next; ++index) rows.append(_pendingReviews.at(index).row);
    return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")},
        {QStringLiteral("offset"), first}, {QStringLiteral("nextOffset"), next},
        {QStringLiteral("totalCount"), _pendingReviews.size()}, {QStringLiteral("hasMore"), next < _pendingReviews.size()},
        {QStringLiteral("rows"), rows}, {QStringLiteral("sessionOnly"), true}};
}

QVariantMap FleetEventRuleEngine::reviewDetail(const QString& reviewId, const QString& revision, const QVariant& offset) const
{
    if (QThread::currentThread() != thread()) return reviewReadError(QStringLiteral("review-wrong-thread"));
    if (!reviewSequence(revision, nullptr)) return reviewReadError(QStringLiteral("review-selection-stale"));
    for (const auto& entry : _pendingReviews) {
        if (entry.row.value(QStringLiteral("reviewId")) != QVariant(reviewId)
            || entry.row.value(QStringLiteral("revision")) != QVariant(revision)) continue;
        int first = 0;
        if (!reviewOffset(offset, entry.detail.size(), &first)
            || (first < entry.detail.size() && entry.detail.at(first).isLowSurrogate()))
            return reviewReadError(QStringLiteral("review-offset-invalid"));
        int next = qMin(entry.detail.size(), first + kReviewChunkCharacters);
        if (next < entry.detail.size() && next > first && entry.detail.at(next - 1).isHighSurrogate()) --next;
        return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")},
            {QStringLiteral("reviewId"), reviewId}, {QStringLiteral("revision"), revision},
            {QStringLiteral("offset"), first}, {QStringLiteral("nextOffset"), next},
            {QStringLiteral("hasMore"), next < entry.detail.size()}, {QStringLiteral("text"), entry.detail.mid(first, next - first)}};
    }
    return reviewReadError(QStringLiteral("review-selection-stale"));
}

QVariantMap FleetEventRuleEngine::decideReview(const QString& reviewId, const QString& revision,
    const QString& decision, const QString& reason)
{
    if (QThread::currentThread() != thread()) return reviewDecisionError(QStringLiteral("review-wrong-thread"));
    if (_liveBusy) return reviewDecisionError(QStringLiteral("review-busy"));
    if (!reviewSequence(revision, nullptr) || (decision != QStringLiteral("reviewed") && decision != QStringLiteral("rejected"))
        || reason.isNull() || reason.size() > 512 || reason != reason.trimmed()
        || (decision == QStringLiteral("rejected") && reason.isEmpty()))
        return reviewDecisionError(QStringLiteral("review-decision-invalid"));
    for (const QChar character : reason)
        if (character.isNull() || character.category() == QChar::Other_Control)
            return reviewDecisionError(QStringLiteral("review-decision-invalid"));
    for (int index = 0; index < _pendingReviews.size(); ++index) {
        const PendingReview& entry = _pendingReviews.at(index);
        if (entry.row.value(QStringLiteral("reviewId")) != QVariant(reviewId)
            || entry.row.value(QStringLiteral("revision")) != QVariant(revision)) continue;
        QVariantMap receipt = entry.row;
        receipt.insert(QStringLiteral("ok"), true);
        receipt.insert(QStringLiteral("committed"), true);
        receipt.insert(QStringLiteral("error"), QStringLiteral(""));
        receipt.insert(QStringLiteral("decision"), decision);
        receipt.insert(QStringLiteral("reason"), explicitReviewText(reason));
        receipt.insert(QStringLiteral("decidedAtMs"), QDateTime::currentMSecsSinceEpoch());
        receipt.insert(QStringLiteral("flightCommandReleased"), false);
        _pendingReviewBytes -= entry.encodedBytes;
        _pendingReviews.removeAt(index);
        emit reviewsChanged();
        return receipt; // Already committed; never borrow a later pending item.
    }
    return reviewDecisionError(QStringLiteral("review-selection-stale"));
}
