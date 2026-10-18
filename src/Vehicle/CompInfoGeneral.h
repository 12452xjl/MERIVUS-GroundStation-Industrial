/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "CompInfo.h"
#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "FactMetaData.h"

#include <QObject>
#include <QMap>

class FactMetaData;
class Vehicle;
class FirmwarePlugin;

Q_DECLARE_LOGGING_CATEGORY(CompInfoGeneralLog)

class CompInfoGeneral : public CompInfo
{
    Q_OBJECT

public:
    CompInfoGeneral(uint8_t compId, Vehicle* vehicle, QObject* parent = nullptr);

    bool isMetaDataTypeSupported(COMP_METADATA_TYPE type) { return _supportedTypes.contains(type); }

    void setUris(CompInfo& compInfo) const;

    // Start a genuinely new GENERAL request with no inherited catalogue or
    // declaration. Native only; an empty/failed response cannot revive it.
    void resetMetadata();

    // Overrides from CompInfo
    void setJson(const QString& metadataJsonFileName) override;

private:
    // Preserve unknown nonnegative JSON type identifiers as integers without
    // converting them into potentially out-of-range C++ enum values.
    QMap<int, Uris>   _supportedTypes;

    static const char*          _jsonMetadataTypesKey;
};
