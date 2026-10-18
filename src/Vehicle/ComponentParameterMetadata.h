#pragma once

#include "FactMetaData.h"

#include <QJsonDocument>
#include <QStringList>
#include <QVector>

// Parameter-v1-only admission. This does not change the general FactMetaData
// factories or establish metadata authenticity, live parameter values or rights.
namespace ComponentParameterMetadata {

struct Definition
{
    QString name;
    FactMetaData::ValueType_t type = FactMetaData::valueTypeInt32;
    QString shortDescription, longDescription, units, category, group;
    int decimalPlaces = FactMetaData::kUnknownDecimalPlaces;
    QVariant minimum, maximum, defaultValue;
    bool defaultSpecified = false;
    bool defaultUnknown = false;
    double increment = 0;
    bool incrementSpecified = false;
    QStringList enumDescriptions, bitmaskDescriptions;
    QVariantList enumValues, bitmaskValues;
    bool hasControl = true;
    bool qgcRebootRequired = false;
    bool rebootRequired = false;
    bool volatileValue = false;
    bool readOnly = false;
    bool writeOnly = false;
    bool unitsSpecified = false;
    bool categorySpecified = false;
    bool groupSpecified = false;
};

// Pure value operation. On every failure definitions is empty; no successful
// prefix, QObject, default/fallback metadata or app setting is published.
bool parse(const QJsonDocument& document, QVector<Definition>& definitions, QString& error);

// Call on the parent's owning thread in the normal QGC metadata context.
// Returns null for an invalid Definition or deletion/reparenting during final
// adoption. The caller still owns source/operation checks around this call.
FactMetaData* create(const Definition& definition, QObject* parent);

} // namespace ComponentParameterMetadata
