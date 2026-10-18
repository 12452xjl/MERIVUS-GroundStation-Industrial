/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CompInfoGeneral.h"

#include "JsonHelper.h"
#include "FactMetaData.h"
#include "FirmwarePlugin.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"

#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include <cmath>
#include <limits>

QGC_LOGGING_CATEGORY(CompInfoGeneralLog, "CompInfoGeneralLog")

const char* CompInfoGeneral::_jsonMetadataTypesKey = "metadataTypes";

namespace {
bool jsonUnsignedInteger(const QJsonValue& value, double maximum, double* output = nullptr)
{
    if (!value.isDouble()) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < 0 || number > maximum || std::floor(number) != number) return false;
    if (output) *output = number;
    return true;
}

bool jsonUri(const QJsonObject& object, const QString& key, bool required, QString* output)
{
    const auto found = object.constFind(key);
    if (found == object.constEnd()) {
        if (required) return false;
        *output = QStringLiteral("");
        return true;
    }
    if (!found.value().isString()) return false;
    const QString value = found.value().toString();
    if (value.size() > 4096 || value.contains(QChar::Null)) return false;
    *output = value;
    return true;
}
} // namespace

CompInfoGeneral::CompInfoGeneral(uint8_t compId, Vehicle* vehicle, QObject* parent)
    : CompInfo(COMP_METADATA_TYPE_GENERAL, compId, vehicle, parent)
{

}

void CompInfoGeneral::setUris(CompInfo &compInfo) const
{
    const auto& metadataTypeIter = _supportedTypes.constFind(compInfo.type);
    if (metadataTypeIter == _supportedTypes.constEnd()) {
        compInfo._uris = {}; // reset
    } else {
        compInfo._uris = *metadataTypeIter;
    }
}

void CompInfoGeneral::resetMetadata()
{
    _supportedTypes.clear();
    setUriMetaData(QStringLiteral(""), 0);
}

void CompInfoGeneral::setJson(const QString& metadataJsonFileName)
{
    // Retire the old catalogue before every possible early return. Only the
    // complete validated local candidate below may become the new catalogue.
    _supportedTypes.clear();
    if (metadataJsonFileName.isEmpty()) {
        return;
    }

    QString         errorString;
    QJsonDocument   jsonDoc;

    if (!JsonHelper::isJsonFile(metadataJsonFileName, jsonDoc, errorString)) {
        qCWarning(CompInfoGeneralLog) << "Metadata json file open failed: compid:" << compId << errorString;
        return;
    }
    if (!jsonDoc.isObject()) {
        qCWarning(CompInfoGeneralLog) << "Metadata json root is not an object: compid:" << compId;
        return;
    }
    const QJsonObject jsonObj = jsonDoc.object();

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,           QJsonValue::Double, true },
        { _jsonMetadataTypesKey,   QJsonValue::Array,  true },
    };
    if (!JsonHelper::validateKeys(jsonObj, keyInfoList, errorString)) {
        qCWarning(CompInfoGeneralLog) << "Metadata json validation failed: compid:" << compId << errorString;
        return;
    }

    const QJsonValue version = jsonObj.value(JsonHelper::jsonVersionKey);
    if (!version.isDouble() || !std::isfinite(version.toDouble()) || version.toDouble() != 1.0) {
        qCWarning(CompInfoGeneralLog) << "Metadata json unsupported version: compid:" << compId;
        return;
    }

    const QJsonArray rgSupportedTypes = jsonObj.value(_jsonMetadataTypesKey).toArray();
    if (rgSupportedTypes.size() > 128) {
        qCWarning(CompInfoGeneralLog) << "Metadata type catalogue exceeds limit: compid:" << compId;
        return;
    }
    QMap<int, Uris> candidates;
    for (const QJsonValue& typeValue : rgSupportedTypes) {
        if (!typeValue.isObject()) {
            qCWarning(CompInfoGeneralLog) << "Metadata type entry is not an object: compid:" << compId;
            return;
        }
        const QJsonObject entry = typeValue.toObject();
        double typeNumber = 0, crcNumber = 0, fallbackCrcNumber = 0;
        if (!jsonUnsignedInteger(entry.value(QStringLiteral("type")), std::numeric_limits<int>::max(), &typeNumber)
            || !jsonUnsignedInteger(entry.value(QStringLiteral("fileCrc")), std::numeric_limits<uint32_t>::max(), &crcNumber)) {
            qCWarning(CompInfoGeneralLog) << "Metadata type or CRC is invalid: compid:" << compId;
            return;
        }
        const int type = static_cast<int>(typeNumber);
        if (candidates.contains(type)) {
            qCWarning(CompInfoGeneralLog) << "Duplicate metadata type: compid:type" << compId << type;
            return;
        }
        Uris uris;
        if (!jsonUri(entry, QStringLiteral("uri"), true, &uris.uriMetaData) || uris.uriMetaData.isEmpty()
            || !jsonUri(entry, QStringLiteral("uriFallback"), false, &uris.uriMetaDataFallback)
            || !jsonUri(entry, QStringLiteral("translationUri"), false, &uris.uriTranslation)
            || !jsonUri(entry, QStringLiteral("translationUriFallback"), false, &uris.uriTranslationFallback)) {
            qCWarning(CompInfoGeneralLog) << "Metadata URI field is invalid: compid:type" << compId << type;
            return;
        }
        uris.crcMetaData = static_cast<uint32_t>(crcNumber);
        uris.crcMetaDataValid = true; // A numeric CRC of zero is a valid declaration.
        if (entry.contains(QStringLiteral("fileCrcFallback"))) {
            if (!jsonUnsignedInteger(entry.value(QStringLiteral("fileCrcFallback")), std::numeric_limits<uint32_t>::max(), &fallbackCrcNumber)) {
                qCWarning(CompInfoGeneralLog) << "Metadata fallback CRC is invalid: compid:type" << compId << type;
                return;
            }
            uris.crcMetaDataFallback = static_cast<uint32_t>(fallbackCrcNumber);
            uris.crcMetaDataFallbackValid = true;
        }
        // Unknown type IDs and extra fields remain data, not new consumers or
        // dynamic metadata support. Never publish a successful prefix.
        candidates.insert(type, uris);
    }
    _supportedTypes = candidates;
}
