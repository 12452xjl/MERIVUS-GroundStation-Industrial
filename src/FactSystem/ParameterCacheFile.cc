#include "ParameterCacheFile.h"

#include <QMetaType>
#include <cstring>
#include <limits>

namespace {
constexpr char magic[] = "MVPXC3\r\n";
constexpr int headerBytes = 20;

struct Type {
    int mavType = 0;
    int variantType = QMetaType::UnknownType;
    int width = 0;
};

bool fail(QString& error, const char* code)
{
    error = QString::fromLatin1(code); // Never echo cache-controlled content.
    return false;
}

bool nameValid(const QString& name)
{
    if (name.isEmpty() || name.size() > 16 || name == QStringLiteral("_HASH_CHECK")) return false;
    for (const QChar character : name) {
        if (character.unicode() < 0x21 || character.unicode() > 0x7e) return false;
    }
    return true;
}

bool describe(FactMetaData::ValueType_t factType, Type& type)
{
    switch (factType) {
    case FactMetaData::valueTypeUint8: type = {1, QMetaType::UInt, 1}; return true;
    case FactMetaData::valueTypeInt8: type = {2, QMetaType::Int, 1}; return true;
    case FactMetaData::valueTypeUint16: type = {3, QMetaType::UInt, 2}; return true;
    case FactMetaData::valueTypeInt16: type = {4, QMetaType::Int, 2}; return true;
    case FactMetaData::valueTypeUint32: type = {5, QMetaType::UInt, 4}; return true;
    case FactMetaData::valueTypeInt32: type = {6, QMetaType::Int, 4}; return true;
    case FactMetaData::valueTypeUint64: type = {7, QMetaType::ULongLong, 8}; return true;
    case FactMetaData::valueTypeInt64: type = {8, QMetaType::LongLong, 8}; return true;
    case FactMetaData::valueTypeFloat: type = {9, QMetaType::Float, 4}; return true;
    case FactMetaData::valueTypeDouble: type = {10, QMetaType::Double, 8}; return true;
    default: return false;
    }
}

bool fromMavType(quint64 mavType, FactMetaData::ValueType_t& factType)
{
    // A byte from disk is admitted before producing an enum value.
    switch (mavType) {
    case 1: factType = FactMetaData::valueTypeUint8; return true;
    case 2: factType = FactMetaData::valueTypeInt8; return true;
    case 3: factType = FactMetaData::valueTypeUint16; return true;
    case 4: factType = FactMetaData::valueTypeInt16; return true;
    case 5: factType = FactMetaData::valueTypeUint32; return true;
    case 6: factType = FactMetaData::valueTypeInt32; return true;
    case 7: factType = FactMetaData::valueTypeUint64; return true;
    case 8: factType = FactMetaData::valueTypeInt64; return true;
    case 9: factType = FactMetaData::valueTypeFloat; return true;
    case 10: factType = FactMetaData::valueTypeDouble; return true;
    default: return false;
    }
}

bool normalizeValue(FactMetaData::ValueType_t factType, const QVariant& actual,
                    bool allowSmallUnsignedPromotion, QVariant& canonical)
{
    Type type;
    if (!describe(factType, type) || !actual.isValid() || actual.isNull()) return false;
    if (allowSmallUnsignedPromotion && (type.mavType == 1 || type.mavType == 3)
        && actual.userType() == QMetaType::Int) {
        const int value = actual.toInt();
        const uint maximum = type.mavType == 1 ? std::numeric_limits<quint8>::max() : std::numeric_limits<quint16>::max();
        if (value < 0 || static_cast<uint>(value) > maximum) return false;
        canonical = QVariant::fromValue<uint>(static_cast<uint>(value));
        return true;
    }
    if (actual.userType() != type.variantType) return false;
    switch (type.mavType) {
    case 1: if (actual.toUInt() > std::numeric_limits<quint8>::max()) return false; break;
    case 2: if (actual.toInt() < std::numeric_limits<qint8>::min() || actual.toInt() > std::numeric_limits<qint8>::max()) return false; break;
    case 3: if (actual.toUInt() > std::numeric_limits<quint16>::max()) return false; break;
    case 4: if (actual.toInt() < std::numeric_limits<qint16>::min() || actual.toInt() > std::numeric_limits<qint16>::max()) return false; break;
    case 9:
        if (sizeof(float) != 4 || !std::numeric_limits<float>::is_iec559) return false;
        break;
    case 10:
        if (sizeof(double) != 8 || !std::numeric_limits<double>::is_iec559) return false;
        break;
    default: break;
    }
    // This is a same-metatype QVariant copy, not a floating conversion. Keep
    // every IEEE representation, including NaN payload/sign and signed zero.
    canonical = actual;
    return true;
}

void appendLittleEndian(QByteArray& bytes, quint64 value, int width)
{
    for (int index = 0; index < width; ++index) bytes.append(static_cast<char>((value >> (index * 8)) & 0xff));
}

bool readLittleEndian(const QByteArray& bytes, int& offset, int width, quint64& value)
{
    if (width < 1 || width > 8 || offset < 0 || offset > bytes.size() - width) return false;
    value = 0;
    for (int index = 0; index < width; ++index)
        value |= quint64(static_cast<unsigned char>(bytes.at(offset + index))) << (index * 8);
    offset += width;
    return true;
}

bool primitiveBytes(FactMetaData::ValueType_t factType, const QVariant& value, QByteArray& bytes)
{
    Type type;
    QVariant canonical;
    if (!describe(factType, type) || !normalizeValue(factType, value, false, canonical)) return false;
    quint64 bits = 0;
    switch (type.mavType) {
    case 1: case 3: case 5: bits = value.toUInt(); break;
    case 2: case 4: case 6: bits = static_cast<quint64>(static_cast<qlonglong>(value.toInt())); break;
    case 7: bits = value.toULongLong(); break;
    case 8: bits = static_cast<quint64>(value.toLongLong()); break;
    case 9: {
        quint32 raw = 0;
        // constData is read only AFTER the actual, non-null Float metatype and
        // native IEEE width were checked. Never infer storage from Fact.type.
        std::memcpy(&raw, value.constData(), sizeof(raw));
        bits = raw;
        break;
    }
    case 10: std::memcpy(&bits, value.constData(), sizeof(bits)); break;
    default: return false;
    }
    bytes.clear();
    appendLittleEndian(bytes, bits, type.width);
    return true;
}

bool fromPrimitive(FactMetaData::ValueType_t factType, quint64 bits, QVariant& value)
{
    Type type;
    if (!describe(factType, type)) return false;
    switch (type.mavType) {
    case 1: case 3: case 5: value = QVariant::fromValue<uint>(static_cast<uint>(bits)); return true;
    case 7: value = QVariant::fromValue<qulonglong>(bits); return true;
    case 2: case 4: case 6: case 8: {
        const quint64 sign = quint64(1) << (type.width * 8 - 1);
        const quint64 mask = type.width == 8 ? std::numeric_limits<quint64>::max() : (quint64(1) << (type.width * 8)) - 1;
        // Avoid an out-of-range unsigned-to-signed cast. Negative values are
        // the exact two's-complement file value, including INT64_MIN.
        const qlonglong signedValue = (bits & sign) ? -1 - static_cast<qlonglong>((~bits) & mask) : static_cast<qlonglong>(bits);
        value = type.mavType == 8 ? QVariant::fromValue<qlonglong>(signedValue)
                                 : QVariant::fromValue<int>(static_cast<int>(signedValue));
        return true;
    }
    case 9: {
        if (sizeof(float) != 4 || !std::numeric_limits<float>::is_iec559) return false;
        const quint32 raw = static_cast<quint32>(bits);
        float primitive;
        std::memcpy(&primitive, &raw, sizeof(raw));
        value = QVariant(QMetaType::Float, &primitive);
        return true;
    }
    case 10: {
        if (sizeof(double) != 8 || !std::numeric_limits<double>::is_iec559) return false;
        double primitive;
        std::memcpy(&primitive, &bits, sizeof(bits));
        value = QVariant(QMetaType::Double, &primitive);
        return true;
    }
    default: return false;
    }
}
}

bool ParameterCacheFile::normalizeCandidate(const QString& name, FactMetaData::ValueType_t factType,
    const QVariant& actual, Row& row, QString& error)
{
    const bool floating = factType == FactMetaData::valueTypeFloat || factType == FactMetaData::valueTypeDouble;
    QByteArray originalRepresentation;
    // Snapshot the admitted actual representation before replacing the output
    // row, including when actual aliases that row's value; compare its final copy.
    const bool originalKnown = !floating || primitiveBytes(factType, actual, originalRepresentation);
    const QString candidateName = name;
    const QVariant candidateValue = actual;
    row = Row(); error.clear();
    QVariant canonical;
    if (!nameValid(candidateName)) return fail(error, "parameter-cache-name-invalid");
    if (!originalKnown || !normalizeValue(factType, candidateValue, true, canonical)) return fail(error, "parameter-cache-value-invalid");
    QByteArray representation;
    if (!primitiveBytes(factType, canonical, representation)) return fail(error, "parameter-cache-representation-invalid");
    Row candidate;
    candidate.name = candidateName; candidate.factType = factType; candidate.value = canonical;
    QByteArray copied;
    if (!primitiveBytes(candidate.factType, candidate.value, copied) || copied != representation
        || (floating && copied != originalRepresentation))
        return fail(error, "parameter-cache-representation-changed");
    row = candidate;
    return true;
}

bool ParameterCacheFile::encode(const Document& document, QByteArray& bytes, QString& error)
{
    bytes.clear(); error.clear();
    if (document.vehicleId < 1 || document.vehicleId > 255 || document.componentId < 1 || document.componentId > 255
        || document.rows.isEmpty() || document.rows.size() > MaximumRows) return fail(error, "parameter-cache-header-invalid");
    QByteArray candidate(magic, 8);
    appendLittleEndian(candidate, 3, 2);
    appendLittleEndian(candidate, 0, 2);
    appendLittleEndian(candidate, static_cast<quint64>(document.vehicleId), 2);
    appendLittleEndian(candidate, static_cast<quint64>(document.componentId), 2);
    appendLittleEndian(candidate, static_cast<quint64>(document.rows.size()), 4);
    QString previous;
    for (const Row& row : document.rows) {
        if (!nameValid(row.name) || (!previous.isEmpty() && row.name <= previous)) return fail(error, "parameter-cache-name-order-invalid");
        Type type;
        QByteArray primitive;
        if (!describe(row.factType, type) || !primitiveBytes(row.factType, row.value, primitive))
            return fail(error, "parameter-cache-value-not-canonical");
        const QByteArray name = row.name.toLatin1();
        if (candidate.size() > MaximumBytes - 2 - name.size() - primitive.size()) return fail(error, "parameter-cache-byte-limit");
        candidate.append(static_cast<char>(name.size()));
        candidate.append(static_cast<char>(type.mavType));
        candidate.append(name);
        candidate.append(primitive);
        previous = row.name;
    }
    bytes = candidate;
    return true;
}

bool ParameterCacheFile::decode(const QByteArray& bytes, Document& document, QString& error)
{
    document = Document(); error.clear();
    if (bytes.size() < headerBytes || bytes.size() > MaximumBytes || bytes.left(8) != QByteArray(magic, 8))
        return fail(error, "parameter-cache-header-invalid");
    int offset = 8;
    quint64 version = 0, flags = 0, vehicle = 0, component = 0, count = 0;
    if (!readLittleEndian(bytes, offset, 2, version) || !readLittleEndian(bytes, offset, 2, flags)
        || !readLittleEndian(bytes, offset, 2, vehicle) || !readLittleEndian(bytes, offset, 2, component)
        || !readLittleEndian(bytes, offset, 4, count) || version != 3 || flags != 0
        || vehicle < 1 || vehicle > 255 || component < 1 || component > 255 || count < 1 || count > MaximumRows
        || count > static_cast<quint64>((bytes.size() - headerBytes) / 4))
        return fail(error, "parameter-cache-header-invalid");
    Document candidate;
    candidate.vehicleId = static_cast<int>(vehicle); candidate.componentId = static_cast<int>(component);
    candidate.rows.reserve(static_cast<int>(count));
    QString previous;
    for (quint64 index = 0; index < count; ++index) {
        quint64 length = 0, mavType = 0, bits = 0;
        Row row;
        Type type;
        if (!readLittleEndian(bytes, offset, 1, length) || !readLittleEndian(bytes, offset, 1, mavType)
            || length < 1 || length > 16 || !fromMavType(mavType, row.factType) || !describe(row.factType, type)
            || offset > bytes.size() - static_cast<int>(length)) return fail(error, "parameter-cache-row-invalid");
        row.name = QString::fromLatin1(bytes.constData() + offset, static_cast<int>(length));
        offset += static_cast<int>(length);
        if (!nameValid(row.name) || (!previous.isEmpty() && row.name <= previous)) return fail(error, "parameter-cache-name-order-invalid");
        if (!readLittleEndian(bytes, offset, type.width, bits) || !fromPrimitive(row.factType, bits, row.value))
            return fail(error, "parameter-cache-primitive-invalid");
        candidate.rows.append(row);
        previous = row.name;
    }
    if (offset != bytes.size()) return fail(error, "parameter-cache-trailing-bytes");
    // A platform/Qt floating copy may not silently canonicalize an IEEE value.
    // If every byte cannot be reproduced, treat this cache as a complete miss.
    QByteArray roundtrip;
    QString roundtripError;
    if (!encode(candidate, roundtrip, roundtripError) || roundtrip != bytes)
        return fail(error, "parameter-cache-roundtrip-rejected");
    document = candidate;
    return true;
}

bool ParameterCacheFile::valueMatches(const Row& row, const QVariant& actual)
{
    if (!nameValid(row.name)) return false;
    QByteArray expected, received;
    if (!primitiveBytes(row.factType, row.value, expected)) return false;
    const bool promoted = (row.factType == FactMetaData::valueTypeUint8 || row.factType == FactMetaData::valueTypeUint16)
        && actual.userType() == QMetaType::Int;
    if (!promoted) return primitiveBytes(row.factType, actual, received) && expected == received;
    QVariant canonical;
    return normalizeValue(row.factType, actual, true, canonical)
        && primitiveBytes(row.factType, canonical, received) && expected == received;
}

bool ParameterCacheFile::px4CrcContribution(const Row& row, QByteArray& contribution, QString& error)
{
    contribution.clear(); error.clear();
    if (!nameValid(row.name) || (row.factType != FactMetaData::valueTypeInt32 && row.factType != FactMetaData::valueTypeFloat))
        return fail(error, "parameter-cache-px4-type-invalid");
    QByteArray primitive;
    if (!primitiveBytes(row.factType, row.value, primitive) || primitive.size() != 4)
        return fail(error, "parameter-cache-px4-value-invalid");
    contribution = row.name.toLatin1();
    contribution.append(primitive);
    return true;
}
