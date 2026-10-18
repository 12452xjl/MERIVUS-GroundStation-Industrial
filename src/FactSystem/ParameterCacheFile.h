#pragma once

#include "FactMetaData.h"

#include <QByteArray>
#include <QString>
#include <QVariant>
#include <QVector>
#include <QtGlobal>

namespace ParameterCacheFile {
constexpr qint64 MaximumBytes = 4 * 1024 * 1024;
constexpr int MaximumRows = 16384;

struct Row {
    QString name;
    FactMetaData::ValueType_t factType = FactMetaData::valueTypeInt32;
    QVariant value;
};

struct Document {
    int vehicleId = 0;
    int componentId = 0;
    QVector<Row> rows;
};

bool normalizeCandidate(const QString& name, FactMetaData::ValueType_t factType,
                        const QVariant& actual, Row& row, QString& error);
bool encode(const Document& document, QByteArray& bytes, QString& error);
bool decode(const QByteArray& bytes, Document& document, QString& error);
bool valueMatches(const Row& row, const QVariant& actual);
// Actual PX4 hash domain only: name bytes (no NUL), then INT32/FLOAT LE4.
// The caller, not this codec, obtains the actual known volatile flag.
bool px4CrcContribution(const Row& row, QByteArray& contribution, QString& error);
}
