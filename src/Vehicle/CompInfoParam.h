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
#include <QPointer>

#include <functional>
#include <memory>

class FactMetaData;
class Vehicle;
class FirmwarePlugin;
class Fact;
struct CompInfoParamCatalog;

Q_DECLARE_LOGGING_CATEGORY(CompInfoParamLog)

class CompInfoParam : public CompInfo
{
    Q_OBJECT

public:
    CompInfoParam(uint8_t compId, Vehicle* vehicle, QObject* parent = nullptr);
    ~CompInfoParam() override;

    // Each new Fact receives its own immutable-in-this-catalog snapshot. No
    // template/opaque-owned metadata pointer escapes to a live Fact.
    QPointer<FactMetaData> createMetaDataForFact(Fact* fact);
    // Value-only result; XML fallback can cross guarded virtual calls. False
    // means unknown, NOT a known non-volatile parameter.
    bool parameterIsVolatile(const QString& name, FactMetaData::ValueType_t type, bool& value);
    quint64 metadataRevisionMemory() const;
    void resetMetadata();

    // Overrides from CompInfo
    void setJson(const QString& metadataJsonFileName) override;

    static void _cachePX4MetaDataFile(const QString& metaDataFile);

private:
    std::shared_ptr<CompInfoParamCatalog> _catalogForRequest();
    bool _catalogCurrent(const std::shared_ptr<CompInfoParamCatalog>& catalog, quint64 serial) const;
    FactMetaData* _templateForName(const std::shared_ptr<CompInfoParamCatalog>& catalog, quint64 serial,
                                  const QString& name, FactMetaData::ValueType_t type);
    bool _prepareOpaque(const std::shared_ptr<CompInfoParamCatalog>& catalog, quint64 serial);

    static FirmwarePlugin*  _anyVehicleTypeFirmwarePlugin   (MAV_AUTOPILOT firmwareType);
    static QString          _parameterMetaDataFile          (Vehicle* vehicle, MAV_AUTOPILOT firmwareType, int& majorVersion, int& minorVersion,
                                                              const std::function<bool()>& current = {});

    QPointer<Vehicle> _vehicle;
    std::shared_ptr<CompInfoParamCatalog> _catalog;
    quint64 _serial = 1;
    bool _serialExhausted = false;
    bool _busy = false;
    bool _destroying = false;

    static const char* _cachedMetaDataFilePrefix;
    static const char* _indexedNameTag;
};
