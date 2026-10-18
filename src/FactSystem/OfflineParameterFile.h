#pragma once

#include "FactMetaData.h"

#include <QByteArray>
#include <QString>
#include <QVariant>
#include <QVector>
#include <QtGlobal>

namespace OfflineParameterFile {
constexpr qint64 MaximumBytes = 4 * 1024 * 1024;

struct Row {
    int componentId = 0;
    QString name;
    int mavType = 0;
    FactMetaData::ValueType_t factType = FactMetaData::valueTypeInt32;
    QVariant value;
};

struct Document {
    int sourceSystemId = -1;
    QVector<Row> rows;
};

// Entire-byte-document decode only. No file, QObject, metadata or wire access.
// Failure clears document and returns a fixed diagnostic with a line number.
bool decode(const QByteArray& bytes, Document& document, QString& error);
bool valueMatches(const Row& row, const QVariant& actual);
}
