#include "ComponentParameterMetadata.h"

#include <QJsonArray>
#include <QMetaType>
#include <QPointer>
#include <QScopeGuard>
#include <QSet>
#include <QThread>

#include <cmath>
#include <limits>

namespace ComponentParameterMetadata {
namespace {
constexpr int maximumParameters = 16384;
constexpr int maximumItems = 256;
constexpr int maximumString = 65536;
constexpr int maximumNumericString = 128;
constexpr int maximumNodes = 262144;
constexpr qint64 maximumCharacters = 8LL * 1024 * 1024;

bool fail(QString& error, const char* code)
{
    error = QString::fromLatin1(code);
    return false;
}

bool boundedTree(const QJsonValue& root)
{
    struct Node { QJsonValue value; int depth; };
    QVector<Node> pending{{root, 0}};
    int nodes = 0;
    qint64 characters = 0;
    const auto text = [&characters](const QString& value) {
        if (value.size() > maximumString || value.size() > maximumCharacters - characters) return false;
        characters += value.size();
        return true;
    };
    while (!pending.isEmpty()) {
        const Node node = pending.takeLast();
        if (++nodes > maximumNodes || node.depth > 32) return false;
        if (node.value.isString()) {
            if (!text(node.value.toString())) return false;
        } else if (node.value.isObject()) {
            const QJsonObject object = node.value.toObject();
            if (object.size() > 128 || object.size() > maximumNodes - nodes - pending.size()) return false;
            for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
                if (it.key().size() > 128 || !text(it.key())) return false;
                pending.append({it.value(), node.depth + 1});
            }
        } else if (node.value.isArray()) {
            const QJsonArray array = node.value.toArray();
            if (array.size() > maximumNodes - nodes - pending.size()) return false;
            for (const auto& value : array) pending.append({value, node.depth + 1});
        } else if (node.value.isDouble() && !std::isfinite(node.value.toDouble())) return false;
        else if (node.value.isUndefined()) return false;
    }
    return true;
}

bool nameValid(const QString& name)
{
    if (name.isEmpty() || name.size() > 16) return false;
    bool indexed = false;
    for (int i = 0; i < name.size(); ++i) {
        const ushort c = name.at(i).unicode();
        if (c == '{') {
            if (indexed || name.mid(i, 3) != QStringLiteral("{n}")) return false;
            indexed = true;
            i += 2;
        } else if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
            || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
    }
    return true;
}

bool typeBounds(FactMetaData::ValueType_t type, double& minimum, double& maximum, int& bits, bool& unsignedType)
{
    unsignedType = false;
    switch (type) {
    case FactMetaData::valueTypeInt8: minimum = -128; maximum = 127; bits = 8; break;
    case FactMetaData::valueTypeUint8: minimum = 0; maximum = 255; bits = 8; unsignedType = true; break;
    case FactMetaData::valueTypeInt16: minimum = -32768; maximum = 32767; bits = 16; break;
    case FactMetaData::valueTypeUint16: minimum = 0; maximum = 65535; bits = 16; unsignedType = true; break;
    case FactMetaData::valueTypeInt32: minimum = -2147483648.0; maximum = 2147483647.0; bits = 32; break;
    case FactMetaData::valueTypeUint32: minimum = 0; maximum = 4294967295.0; bits = 32; unsignedType = true; break;
    case FactMetaData::valueTypeFloat:
        minimum = -std::numeric_limits<float>::max(); maximum = std::numeric_limits<float>::max(); bits = 0; break;
    default: return false;
    }
    return true;
}

// A narrowly defined v1 generator compatibility spelling, not QVariant
// coercion. No whitespace, plus prefix, hex, macros, bool, null or NaN/Inf.
bool decimal(const QString& text, double& number, bool& integral)
{
    if (text.isEmpty() || text.size() > maximumNumericString) return false;
    int index = 0;
    if (text.at(index) == QLatin1Char('-') && ++index == text.size()) return false;
    const auto digit = [](QChar ch) { return ch >= QLatin1Char('0') && ch <= QLatin1Char('9'); };
    bool nonzero = false;
    int trailingZeros = 0;
    int fractionalDigits = 0;
    int exponent = 0;
    const auto mantissaDigit = [&](QChar ch) {
        nonzero = nonzero || ch != QLatin1Char('0');
        trailingZeros = ch == QLatin1Char('0') ? trailingZeros + 1 : 0;
    };
    if (text.at(index) == QLatin1Char('0')) { mantissaDigit(text.at(index)); ++index; }
    else {
        if (text.at(index) < QLatin1Char('1') || text.at(index) > QLatin1Char('9')) return false;
        while (index < text.size() && digit(text.at(index))) {
            mantissaDigit(text.at(index)); ++index;
        }
    }
    if (index < text.size() && text.at(index) == QLatin1Char('.')) {
        const int start = ++index;
        while (index < text.size() && digit(text.at(index))) {
            mantissaDigit(text.at(index)); ++fractionalDigits; ++index;
        }
        if (index == start) return false;
    }
    if (index < text.size() && (text.at(index) == QLatin1Char('e') || text.at(index) == QLatin1Char('E'))) {
        ++index;
        bool negativeExponent = false;
        if (index < text.size() && (text.at(index) == QLatin1Char('+') || text.at(index) == QLatin1Char('-'))) {
            negativeExponent = text.at(index) == QLatin1Char('-'); ++index;
        }
        const int start = index;
        while (index < text.size() && digit(text.at(index))) {
            // A 128-character mantissa cannot require an exponent this large
            // for integrality. Saturate safely, never overflow an int.
            exponent = qMin(100000, exponent * 10 + text.at(index).unicode() - ushort('0'));
            ++index;
        }
        if (index == start) return false;
        if (negativeExponent) exponent = -exponent;
    }
    if (index != text.size()) return false;
    // Establish mathematical integrality from the original decimal spelling,
    // BEFORE double rounding can erase a fractional tail such as
    // "2147483647.00000001". No regex or floating point equality substitutes.
    integral = !nonzero || exponent >= fractionalDigits || fractionalDigits - exponent <= trailingZeros;
    bool ok = false;
    number = text.toDouble(&ok);
    return ok && std::isfinite(number) && (number != 0 || !nonzero);
}

bool number(const QJsonValue& value, double& output, bool requireIntegralString = false)
{
    if (value.isDouble()) { output = value.toDouble(); return std::isfinite(output); }
    bool integral = false;
    return value.isString() && decimal(value.toString(), output, integral) && (!requireIntegralString || integral);
}

bool typedNumber(double value, FactMetaData::ValueType_t type, QVariant& typed)
{
    double minimum, maximum; int bits; bool unsignedType;
    if (!typeBounds(type, minimum, maximum, bits, unsignedType) || !std::isfinite(value)
        || value < minimum || value > maximum) return false;
    if (bits) {
        if (std::floor(value) != value) return false;
        typed = unsignedType ? QVariant(static_cast<uint>(value)) : QVariant(static_cast<int>(value));
    } else {
        const float narrowed = static_cast<float>(value);
        if (!std::isfinite(narrowed) || (value != 0 && narrowed == 0)) return false;
        typed = QVariant::fromValue(narrowed);
    }
    return true;
}

bool text(const QJsonObject& object, const QString& key, QString& output)
{
    if (!object.contains(key)) return true;
    if (!object.value(key).isString()) return false;
    output = object.value(key).toString();
    return true;
}

bool boolean(const QJsonObject& object, const QString& key, bool& output)
{
    if (!object.contains(key)) return true;
    if (!object.value(key).isBool()) return false;
    output = object.value(key).toBool();
    return true;
}

bool definitionValid(const Definition& value)
{
    double minimum, maximum; int bits; bool unsignedType;
    if (!nameValid(value.name) || !typeBounds(value.type, minimum, maximum, bits, unsignedType)
        || value.decimalPlaces < -1 || value.decimalPlaces > 25) return false;
    const auto typed = [&](const QVariant& input) {
        if (input.isNull()) return false;
        if (bits && input.type() != (unsignedType ? QVariant::UInt : QVariant::Int)) return false;
        if (!bits && input.userType() != QMetaType::Float) return false;
        QVariant result;
        return typedNumber(input.toDouble(), value.type, result) && result == input;
    };
    if (!typed(value.minimum) || !typed(value.maximum) || value.minimum.toDouble() > value.maximum.toDouble()) return false;
    const auto inRange = [&](const QVariant& input) {
        return typed(input) && input.toDouble() >= value.minimum.toDouble() && input.toDouble() <= value.maximum.toDouble();
    };
    if (value.defaultUnknown) {
        if (!value.defaultSpecified || bits || value.defaultValue.isValid()) return false;
    } else if (value.defaultSpecified ? !inRange(value.defaultValue) : value.defaultValue.isValid()) return false;
    if (value.incrementSpecified) {
        if (!std::isfinite(value.increment) || value.increment <= 0) return false;
        QVariant checked;
        if (!bits && !typedNumber(value.increment, value.type, checked)) return false;
    }
    qint64 characters = value.name.size();
    const auto boundedText = [&characters](const QString& item) {
        if (item.size() > maximumString || item.size() > maximumCharacters - characters) return false;
        characters += item.size();
        return true;
    };
    for (const auto& item : {value.shortDescription, value.longDescription, value.units, value.category, value.group})
        if (!boundedText(item)) return false;
    if (value.enumDescriptions.size() != value.enumValues.size() || value.enumValues.size() > maximumItems
        || value.bitmaskDescriptions.size() != value.bitmaskValues.size() || value.bitmaskValues.size() > 32) return false;
    QSet<double> enumValues;
    for (int i = 0; i < value.enumValues.size(); ++i) {
        const QVariant& item = value.enumValues.at(i);
        if (!inRange(item) || enumValues.contains(item.toDouble()) || !boundedText(value.enumDescriptions.at(i))) return false;
        enumValues.insert(item.toDouble());
    }
    QSet<quint32> masks;
    for (int i = 0; i < value.bitmaskValues.size(); ++i) {
        const QVariant& item = value.bitmaskValues.at(i);
        if (!bits || !typed(item) || item.toDouble() <= 0 || !boundedText(value.bitmaskDescriptions.at(i))) return false;
        const quint32 mask = item.toUInt();
        if ((mask & (mask - 1)) != 0 || masks.contains(mask)) return false;
        masks.insert(mask);
    }
    return true;
}

bool row(const QJsonObject& object, Definition& result, QString& error)
{
    if (!object.value(QStringLiteral("name")).isString() || !object.value(QStringLiteral("type")).isString())
        return fail(error, "parameter-name-or-type-invalid");
    result.name = object.value(QStringLiteral("name")).toString();
    bool unknown = false;
    result.type = FactMetaData::stringToType(object.value(QStringLiteral("type")).toString(), unknown);
    double lower, upper; int bits; bool unsignedType;
    if (unknown || !nameValid(result.name) || !typeBounds(result.type, lower, upper, bits, unsignedType))
        return fail(error, "parameter-name-or-type-invalid");
    for (const QString& key : {QStringLiteral("min"), QStringLiteral("max")}) {
        double value;
        if (object.contains(key)) {
            QVariant checked;
            if (!number(object.value(key), value, bits != 0) || !typedNumber(value, result.type, checked))
                return fail(error, "parameter-range-invalid");
            if (key == QStringLiteral("min")) lower = value; else upper = value;
        }
    }
    if (lower > upper || !typedNumber(lower, result.type, result.minimum) || !typedNumber(upper, result.type, result.maximum))
        return fail(error, "parameter-range-invalid");
    QVariant desktopDefault, mobileDefault;
    bool desktopUnknown = false, mobileUnknown = false;
    for (const QString& key : {QStringLiteral("default"), QStringLiteral("mobileDefault")}) {
        if (!object.contains(key)) continue;
        const QJsonValue input = object.value(key);
        QVariant value;
        const bool isUnknown = input.isNull() && !bits;
        if (!isUnknown) {
            double source;
            if (!number(input, source, bits != 0) || source < lower || source > upper || !typedNumber(source, result.type, value))
                return fail(error, "parameter-default-invalid");
        }
        if (key == QStringLiteral("default")) { desktopDefault = value; desktopUnknown = isUnknown; }
        else { mobileDefault = value; mobileUnknown = isUnknown; }
    }
    result.defaultSpecified = object.contains(QStringLiteral("default"));
    result.defaultValue = desktopDefault; result.defaultUnknown = desktopUnknown;
#ifdef __mobile__
    if (object.contains(QStringLiteral("mobileDefault"))) {
        result.defaultSpecified = true; result.defaultValue = mobileDefault; result.defaultUnknown = mobileUnknown;
    }
#else
    Q_UNUSED(mobileDefault);
    Q_UNUSED(mobileUnknown);
#endif
    if (object.contains(QStringLiteral("increment"))) {
        if (!number(object.value(QStringLiteral("increment")), result.increment) || result.increment <= 0)
            return fail(error, "parameter-increment-invalid");
        result.incrementSpecified = true;
    }
    if (object.contains(QStringLiteral("decimalPlaces"))) {
        const auto value = object.value(QStringLiteral("decimalPlaces"));
        const double count = value.toDouble(-1);
        if (!value.isDouble() || !std::isfinite(count) || std::floor(count) != count || count < 0 || count > 25)
            return fail(error, "parameter-decimal-places-invalid");
        result.decimalPlaces = static_cast<int>(count);
    }
    if (!text(object, QStringLiteral("shortDesc"), result.shortDescription)
        || !text(object, QStringLiteral("longDesc"), result.longDescription) || !text(object, QStringLiteral("units"), result.units)
        || !text(object, QStringLiteral("category"), result.category) || !text(object, QStringLiteral("group"), result.group)
        || !boolean(object, QStringLiteral("control"), result.hasControl)
        || !boolean(object, QStringLiteral("qgcRebootRequired"), result.qgcRebootRequired)
        || !boolean(object, QStringLiteral("rebootRequired"), result.rebootRequired)
        || !boolean(object, QStringLiteral("volatile"), result.volatileValue)
        || !boolean(object, QStringLiteral("readOnly"), result.readOnly) || !boolean(object, QStringLiteral("writeOnly"), result.writeOnly))
        return fail(error, "parameter-text-or-flag-invalid");
    result.unitsSpecified = object.contains(QStringLiteral("units"));
    result.categorySpecified = object.contains(QStringLiteral("category"));
    result.groupSpecified = object.contains(QStringLiteral("group"));
    const bool csv = object.contains(QStringLiteral("enumStrings")) || object.contains(QStringLiteral("enumValues"));
    if (csv && object.contains(QStringLiteral("values"))) return fail(error, "parameter-enum-sources-conflict");
    const auto addEnum = [&](const QString& description, const QJsonValue& input) {
        double source; QVariant value;
        if (!number(input, source, bits != 0) || source < lower || source > upper || !typedNumber(source, result.type, value)) return false;
        result.enumDescriptions.append(description); result.enumValues.append(value);
        return true;
    };
    if (csv) {
        if (!object.value(QStringLiteral("enumStrings")).isString() || !object.value(QStringLiteral("enumValues")).isString())
            return fail(error, "parameter-legacy-enum-invalid");
        const QStringList descriptions = object.value(QStringLiteral("enumStrings")).toString().split(QLatin1Char(','), Qt::KeepEmptyParts);
        const QStringList values = object.value(QStringLiteral("enumValues")).toString().split(QLatin1Char(','), Qt::KeepEmptyParts);
        if (values.size() > maximumItems || values.size() != descriptions.size()) return fail(error, "parameter-legacy-enum-invalid");
        for (int i = 0; i < values.size(); ++i)
            if (!addEnum(descriptions.at(i), QJsonValue(values.at(i).trimmed()))) return fail(error, "parameter-legacy-enum-invalid");
    }
    if (object.contains(QStringLiteral("values"))) {
        if (!object.value(QStringLiteral("values")).isArray()) return fail(error, "parameter-values-invalid");
        const QJsonArray values = object.value(QStringLiteral("values")).toArray();
        if (values.size() > maximumItems) return fail(error, "parameter-values-invalid");
        for (const auto& input : values) {
            if (!input.isObject() || !input.toObject().value(QStringLiteral("description")).isString()
                || !addEnum(input.toObject().value(QStringLiteral("description")).toString(), input.toObject().value(QStringLiteral("value"))))
                return fail(error, "parameter-values-invalid");
        }
    }
    if (object.contains(QStringLiteral("bitmask"))) {
        if (!bits || !object.value(QStringLiteral("bitmask")).isArray()) return fail(error, "parameter-bitmask-invalid");
        const QJsonArray values = object.value(QStringLiteral("bitmask")).toArray();
        if (values.size() > 32) return fail(error, "parameter-bitmask-invalid");
        for (const auto& input : values) {
            if (!input.isObject()) return fail(error, "parameter-bitmask-invalid");
            const QJsonObject item = input.toObject();
            const QJsonValue index = item.value(QStringLiteral("index"));
            const double position = index.toDouble(-1);
            if (!item.value(QStringLiteral("description")).isString() || !index.isDouble() || !std::isfinite(position)
                || std::floor(position) != position || position < 0 || position >= (unsignedType ? bits : bits - 1))
                return fail(error, "parameter-bitmask-invalid");
            const quint32 mask = quint32(1) << static_cast<unsigned>(position);
            result.bitmaskDescriptions.append(item.value(QStringLiteral("description")).toString());
            result.bitmaskValues.append(unsignedType ? QVariant(mask) : QVariant(static_cast<int>(mask)));
        }
    }
    return definitionValid(result) || fail(error, "parameter-definition-inconsistent");
}
} // namespace

bool parse(const QJsonDocument& document, QVector<Definition>& definitions, QString& error)
{
    definitions.clear(); error.clear();
    if (!document.isObject() || !boundedTree(QJsonValue(document.object()))) return fail(error, "parameter-document-budget-or-shape-invalid");
    const QJsonObject object = document.object();
    const QJsonValue version = object.value(QStringLiteral("version"));
    if (!version.isDouble() || version.toDouble() != 1) return fail(error, "parameter-version-unsupported");
    if (!object.value(QStringLiteral("parameters")).isArray()) return fail(error, "parameter-list-invalid");
    const QJsonArray parameters = object.value(QStringLiteral("parameters")).toArray();
    if (parameters.size() > maximumParameters) return fail(error, "parameter-list-limit-exceeded");
    QVector<Definition> candidate;
    QSet<QString> names;
    for (const auto& item : parameters) {
        Definition value;
        if (!item.isObject()) return fail(error, "parameter-entry-not-object");
        if (!row(item.toObject(), value, error)) return false;
        if (names.contains(value.name)) return fail(error, "parameter-name-duplicate");
        names.insert(value.name); candidate.append(value);
    }
    definitions = candidate;
    return true;
}

FactMetaData* create(const Definition& definition, QObject* parent)
{
    // The caller may keep its vector in a QObject which a settings/parent
    // callback can replace or destroy. Do not borrow that Definition across it.
    const Definition value = definition;
    if (!definitionValid(value) || (parent && parent->thread() != QThread::currentThread())) return nullptr;
    const QPointer<QObject> owner(parent);
    const QPointer<FactMetaData> metadata(new FactMetaData(value.type));
    bool adopted = false;
    const auto cleanup = qScopeGuard([&]() { if (!adopted && metadata) delete metadata.data(); });
    metadata->setName(value.name);
    metadata->setRawMin(value.minimum); metadata->setRawMax(value.maximum);
    metadata->setDecimalPlaces(value.decimalPlaces);
    metadata->setShortDescription(value.shortDescription); metadata->setLongDescription(value.longDescription);
    if (!value.enumValues.isEmpty()) metadata->setEnumInfo(value.enumDescriptions, value.enumValues);
    if (!value.bitmaskValues.isEmpty()) metadata->setBitmaskInfo(value.bitmaskDescriptions, value.bitmaskValues);
    if (value.unitsSpecified) metadata->setRawUnits(value.units);
    if (!metadata || (parent && !owner)) return nullptr;
    if (value.categorySpecified) metadata->setCategory(value.category);
    if (value.groupSpecified) metadata->setGroup(value.group);
    if (value.incrementSpecified) metadata->setRawIncrement(value.increment);
    // A null Float default is explicitly unknown, not a claimed installed NaN
    // or an invented reset value. Never ask the old setter to infer success.
    if (value.defaultSpecified && !value.defaultUnknown) metadata->setRawDefaultValue(value.defaultValue);
    metadata->setHasControl(value.hasControl);
    metadata->setQGCRebootRequired(value.qgcRebootRequired); metadata->setVehicleRebootRequired(value.rebootRequired);
    metadata->setReadOnly(value.readOnly); metadata->setWriteOnly(value.writeOnly);
    metadata->setVolatileValue(value.volatileValue);
    if (parent) metadata->setParent(owner.data());
    if (!metadata || (parent && (!owner || metadata->parent() != owner.data()))) return nullptr;
    adopted = true;
    return metadata.data();
}
} // namespace ComponentParameterMetadata
