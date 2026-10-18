#pragma once

#include <QByteArray>
#include <QString>
#include <QVariant>

#include <atomic>

// Value-only file I/O shared by one-shot historical snapshot workers. This
// owns no thread, schema, QObject, live source or authorization decision.
class FleetSnapshotFileIo final
{
public:
    struct SaveResult {
        bool ok = false;
        QString error = QStringLiteral("");
        bool fileWriteAttempted = false;
        bool fileWrittenKnown = true;
        bool fileWritten = false;
        bool readbackVerified = false;
        QString readbackError = QStringLiteral("");
    };

    static bool localJsonPath(const QVariant& value);
    // errorPrefix is a fixed native domain literal, never file/QML text.
    static bool readUtf8(const QString& path, qint64 maximumBytes,
                         const std::atomic_bool& cancelled, const QString& errorPrefix,
                         QByteArray* bytes, QString* error);
    static SaveResult saveAndReadback(const QString& path, const QByteArray& bytes,
                                      qint64 maximumBytes, const std::atomic_bool& cancelled,
                                      const QString& errorPrefix);
};
