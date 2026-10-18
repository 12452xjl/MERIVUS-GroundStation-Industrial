#include "OfflineParameterFile.h"

#include <QLocale>
#include <QList>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <cmath>
#include <limits>

namespace {
constexpr int maximumLineBytes = 4096;
constexpr int maximumLines = 65536;
constexpr int maximumRows = 16384;
constexpr int maximumNumberBytes = 128;

bool failure(QString& error, int line, const char* code)
{
    // Never echo file-controlled names, values or comments into diagnostics.
    error = QStringLiteral("offline-parameter-line-%1-%2").arg(line).arg(QString::fromLatin1(code));
    return false;
}

bool decimalMagnitude(const QByteArray& text, bool allowNegative, qulonglong positiveLimit,
                      qulonglong negativeLimit, qulonglong& magnitude, bool& negative)
{
    if (text.isEmpty() || text.size() > maximumNumberBytes) return false;
    int index = 0;
    negative = false;
    if (text.at(0) == '+' || text.at(0) == '-') {
        negative = text.at(0) == '-';
        if (negative && !allowNegative) return false; // Includes unsigned -0.
        if (++index == text.size()) return false;
    }
    const qulonglong limit = negative ? negativeLimit : positiveLimit;
    magnitude = 0;
    for (; index < text.size(); ++index) {
        const char character = text.at(index);
        if (character < '0' || character > '9') return false;
        const unsigned digit = static_cast<unsigned>(character - '0');
        if (digit > limit || magnitude > (limit - digit) / 10) return false;
        magnitude = magnitude * 10 + digit;
    }
    return true;
}

bool unsignedDecimal(const QByteArray& text, qulonglong limit, qulonglong& value)
{
    bool negative = false;
    return decimalMagnitude(text, false, limit, 0, value, negative);
}

bool signedDecimal(const QByteArray& text, qlonglong minimum, qlonglong maximum, qlonglong& value)
{
    qulonglong magnitude = 0;
    bool negative = false;
    // -(minimum + 1) is representable even for INT64_MIN.
    const qulonglong negativeLimit = static_cast<qulonglong>(-(minimum + 1)) + 1;
    if (!decimalMagnitude(text, true, static_cast<qulonglong>(maximum), negativeLimit, magnitude, negative)) return false;
    if (!negative) value = static_cast<qlonglong>(magnitude);
    else if (magnitude == static_cast<qulonglong>(std::numeric_limits<qlonglong>::max()) + 1)
        value = std::numeric_limits<qlonglong>::min();
    else value = -static_cast<qlonglong>(magnitude);
    return true;
}

bool decimalRealLexical(const QByteArray& text, bool& negative, bool& nonzeroMantissa)
{
    if (text.isEmpty() || text.size() > maximumNumberBytes) return false;
    int index = 0;
    negative = false;
    nonzeroMantissa = false;
    if (text.at(0) == '+' || text.at(0) == '-') {
        negative = text.at(0) == '-';
        if (++index == text.size()) return false;
    }
    int digits = 0;
    while (index < text.size() && text.at(index) >= '0' && text.at(index) <= '9') {
        nonzeroMantissa = nonzeroMantissa || text.at(index) != '0';
        ++digits; ++index;
    }
    if (index < text.size() && text.at(index) == '.') {
        ++index;
        while (index < text.size() && text.at(index) >= '0' && text.at(index) <= '9') {
            nonzeroMantissa = nonzeroMantissa || text.at(index) != '0';
            ++digits; ++index;
        }
    }
    if (digits == 0) return false;
    if (index < text.size() && (text.at(index) == 'e' || text.at(index) == 'E')) {
        ++index;
        if (index < text.size() && (text.at(index) == '+' || text.at(index) == '-')) ++index;
        const int exponentStart = index;
        while (index < text.size() && text.at(index) >= '0' && text.at(index) <= '9') ++index;
        if (index == exponentStart) return false;
    }
    return index == text.size();
}

bool numericType(int mavType, FactMetaData::ValueType_t& factType, int& variantType)
{
    // MAV_PARAM_TYPE 1..10, identical to ParameterManager's two existing maps.
    // Admission happens before an enum cast; unknown values never fall to INT32.
    switch (mavType) {
    case 1: factType = FactMetaData::valueTypeUint8; variantType = QMetaType::UInt; return true;
    case 2: factType = FactMetaData::valueTypeInt8; variantType = QMetaType::Int; return true;
    case 3: factType = FactMetaData::valueTypeUint16; variantType = QMetaType::UInt; return true;
    case 4: factType = FactMetaData::valueTypeInt16; variantType = QMetaType::Int; return true;
    case 5: factType = FactMetaData::valueTypeUint32; variantType = QMetaType::UInt; return true;
    case 6: factType = FactMetaData::valueTypeInt32; variantType = QMetaType::Int; return true;
    case 7: factType = FactMetaData::valueTypeUint64; variantType = QMetaType::ULongLong; return true;
    case 8: factType = FactMetaData::valueTypeInt64; variantType = QMetaType::LongLong; return true;
    case 9: factType = FactMetaData::valueTypeFloat; variantType = QMetaType::Float; return true;
    case 10: factType = FactMetaData::valueTypeDouble; variantType = QMetaType::Double; return true;
    default: return false;
    }
}

bool decodeValue(int mavType, const QByteArray& text, QVariant& value)
{
    qulonglong unsignedValue = 0;
    qlonglong signedValue = 0;
    switch (mavType) {
    case 1: case 3: case 5: case 7: {
        const qulonglong limit = mavType == 1 ? std::numeric_limits<quint8>::max()
            : mavType == 3 ? std::numeric_limits<quint16>::max()
            : mavType == 5 ? std::numeric_limits<quint32>::max() : std::numeric_limits<qulonglong>::max();
        if (!unsignedDecimal(text, limit, unsignedValue)) return false;
        value = mavType == 7 ? QVariant::fromValue<qulonglong>(unsignedValue)
                             : QVariant::fromValue<uint>(static_cast<uint>(unsignedValue));
        return true;
    }
    case 2: case 4: case 6: case 8: {
        const qlonglong minimum = mavType == 2 ? std::numeric_limits<qint8>::min()
            : mavType == 4 ? std::numeric_limits<qint16>::min()
            : mavType == 6 ? std::numeric_limits<qint32>::min() : std::numeric_limits<qlonglong>::min();
        const qlonglong maximum = mavType == 2 ? std::numeric_limits<qint8>::max()
            : mavType == 4 ? std::numeric_limits<qint16>::max()
            : mavType == 6 ? std::numeric_limits<qint32>::max() : std::numeric_limits<qlonglong>::max();
        if (!signedDecimal(text, minimum, maximum, signedValue)) return false;
        value = mavType == 8 ? QVariant::fromValue<qlonglong>(signedValue)
                             : QVariant::fromValue<int>(static_cast<int>(signedValue));
        return true;
    }
    case 9: case 10: {
        bool negative = false, nonzeroMantissa = false;
        if (!decimalRealLexical(text, negative, nonzeroMantissa)) return false;
        // Zero mantissas stay zero even with a very large lexical exponent;
        // preserve the explicit sign without a lossy QVariant comparison.
        if (!nonzeroMantissa) {
            value = mavType == 9 ? QVariant::fromValue<float>(std::copysign(0.0f, negative ? -1.0f : 1.0f))
                                : QVariant::fromValue<double>(std::copysign(0.0, negative ? -1.0 : 1.0));
            return true;
        }
        bool ok = false;
        if (mavType == 9) {
            const float parsed = QLocale::c().toFloat(QString::fromLatin1(text), &ok);
            if (!ok || !std::isfinite(parsed) || parsed == 0.0f) return false;
            value = QVariant::fromValue<float>(parsed);
        } else {
            const double parsed = QLocale::c().toDouble(QString::fromLatin1(text), &ok);
            if (!ok || !std::isfinite(parsed) || parsed == 0.0) return false;
            value = QVariant::fromValue<double>(parsed);
        }
        return true;
    }
    default: return false;
    }
}

bool storedValueInRange(const OfflineParameterFile::Row& row)
{
    switch (row.mavType) {
    case 1: return row.value.toUInt() <= std::numeric_limits<quint8>::max();
    case 2: return row.value.toInt() >= std::numeric_limits<qint8>::min() && row.value.toInt() <= std::numeric_limits<qint8>::max();
    case 3: return row.value.toUInt() <= std::numeric_limits<quint16>::max();
    case 4: return row.value.toInt() >= std::numeric_limits<qint16>::min() && row.value.toInt() <= std::numeric_limits<qint16>::max();
    case 5: case 6: case 7: case 8: return true; // Exact canonical type already bounds these.
    case 9: return std::isfinite(row.value.value<float>());
    case 10: return std::isfinite(row.value.toDouble());
    default: return false;
    }
}
}

bool OfflineParameterFile::decode(const QByteArray& bytes, Document& document, QString& error)
{
    document = Document();
    error.clear();
    if (bytes.size() > MaximumBytes) return failure(error, 1, "byte-limit");
    Document candidate;
    QMap<int, QSet<QString>> names;
    int offset = bytes.startsWith("\xEF\xBB\xBF") ? 3 : 0;
    int lineNumber = 0;
    while (offset < bytes.size()) {
        if (++lineNumber > maximumLines) return failure(error, lineNumber, "line-limit");
        const int newline = bytes.indexOf('\n', offset);
        const int end = newline < 0 ? bytes.size() : newline;
        int contentEnd = end;
        if (newline >= 0 && contentEnd > offset && bytes.at(contentEnd - 1) == '\r') --contentEnd;
        const int length = contentEnd - offset;
        if (length > maximumLineBytes) return failure(error, lineNumber, "line-byte-limit");
        const QByteArray line = bytes.mid(offset, length);
        offset = newline < 0 ? bytes.size() : newline + 1;
        // Historical shipped Plane comments contain a NUL. Comments are opaque
        // bounded bytes, skipped before validating the actual data character set.
        if (line.startsWith('#')) continue;
        bool blank = true;
        for (const char character : line) {
            if (character != ' ' && character != '\t') { blank = false; break; }
        }
        if (blank) continue;
        for (const char character : line) {
            const unsigned byte = static_cast<unsigned char>(character);
            if (character != '\t' && (byte < 0x20 || byte > 0x7e)) return failure(error, lineNumber, "data-character-invalid");
        }
        const QList<QByteArray> fields = line.split('\t');
        if (fields.size() != 5) return failure(error, lineNumber, "column-count-invalid");
        if (candidate.rows.size() >= maximumRows) return failure(error, lineNumber, "row-limit");
        for (const int index : {0, 1, 3, 4}) {
            if (fields.at(index).isEmpty() || fields.at(index).size() > maximumNumberBytes)
                return failure(error, lineNumber, "number-length-invalid");
        }
        qulonglong system = 0, component = 0, type = 0;
        if (!unsignedDecimal(fields.at(0), 255, system)) return failure(error, lineNumber, "source-id-invalid");
        if (!unsignedDecimal(fields.at(1), 255, component) || component == 0) return failure(error, lineNumber, "component-id-invalid");
        if (!unsignedDecimal(fields.at(4), 10, type) || type == 0) return failure(error, lineNumber, "type-invalid");
        if (candidate.sourceSystemId >= 0 && candidate.sourceSystemId != static_cast<int>(system))
            return failure(error, lineNumber, "source-id-inconsistent");
        const QByteArray name = fields.at(2);
        if (name.isEmpty() || name.size() > 16) return failure(error, lineNumber, "name-invalid");
        for (const char character : name) {
            if (character < 0x21 || character > 0x7e) return failure(error, lineNumber, "name-invalid");
        }
        Row row;
        row.componentId = static_cast<int>(component);
        row.name = QString::fromLatin1(name);
        row.mavType = static_cast<int>(type);
        int variantType = QMetaType::UnknownType;
        if (!numericType(row.mavType, row.factType, variantType) || !decodeValue(row.mavType, fields.at(3), row.value)
            || row.value.isNull() || row.value.userType() != variantType)
            return failure(error, lineNumber, "value-invalid");
        if (names.value(row.componentId).contains(row.name)) return failure(error, lineNumber, "duplicate-name");
        names[row.componentId].insert(row.name);
        candidate.sourceSystemId = static_cast<int>(system);
        candidate.rows.append(row);
    }
    if (candidate.rows.isEmpty()) return failure(error, lineNumber > 0 ? lineNumber : 1, "no-data");
    document = candidate;
    return true;
}

bool OfflineParameterFile::valueMatches(const Row& row, const QVariant& actual)
{
    FactMetaData::ValueType_t factType = FactMetaData::valueTypeInt32;
    int variantType = QMetaType::UnknownType;
    if (!numericType(row.mavType, factType, variantType) || row.factType != factType
        || row.value.isNull() || actual.isNull() || row.value.userType() != variantType
        || actual.userType() != variantType || !storedValueInRange(row)) return false;
    switch (variantType) {
    case QMetaType::UInt: return row.value.toUInt() == actual.toUInt();
    case QMetaType::Int: return row.value.toInt() == actual.toInt();
    case QMetaType::ULongLong: return row.value.toULongLong() == actual.toULongLong();
    case QMetaType::LongLong: return row.value.toLongLong() == actual.toLongLong();
    case QMetaType::Float: {
        const float expected = row.value.value<float>(), received = actual.value<float>();
        return std::isfinite(received) && expected == received
            && (expected != 0.0f || std::signbit(expected) == std::signbit(received));
    }
    case QMetaType::Double: {
        const double expected = row.value.toDouble(), received = actual.toDouble();
        return std::isfinite(received) && expected == received
            && (expected != 0.0 || std::signbit(expected) == std::signbit(received));
    }
    default: return false;
    }
}
