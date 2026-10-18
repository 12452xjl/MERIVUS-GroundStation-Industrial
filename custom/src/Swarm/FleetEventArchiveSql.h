#pragma once

#include "FleetEventArchiveCodec.h"

#include <QString>
#include <QVariantMap>

#include <atomic>

class QSqlDatabase;

// Shared value-only schema and strict diagnostic history reader. All database
// objects remain in their caller's SQL thread. No QObject or control source.
class FleetEventArchiveSql final
{
public:
    static int applicationId();
    static int schemaVersion();
    static qint64 maximumRecordBytes();
    static const QString& metaSchema();
    static const QString& runsSchema();
    static const QString& eventsSchema();

    static bool scalar(QSqlDatabase& database, const QString& fixedSql, qint64* value);
    static bool recordConsistent(const FleetEventArchiveCodec::Record& record);

    // Caller owns its read view, connection, and read-only policy. On success,
    // metadata has exactly databaseEpoch/retiredCount/retainedFromRowId/
    // retainedThroughRowId; the last three are decimal strings.
    static bool inspect(QSqlDatabase& database, QVariantMap* metadata);
    static bool sealedRunMatches(QSqlDatabase& database,
                                 const QString& expectedEpoch,
                                 const QString& sealRunId);

    // Owns BEGIN/COMMIT (or rollback on failure/cancel). Do not nest inside an
    // active transaction. Rechecks schema and actual epoch within that read
    // view. Cancellation is cooperative and cannot interrupt a SQL statement.
    // maximumBytes is the inner JSON budget: [1 MiB - 1024, 8 MiB]. Native
    // policies still require 1..8 MiB; a reader may reserve 1024 for context.
    static bool readPage(QSqlDatabase& database, const QString& runFilter,
                         qint64 cursor, int limit, qint64 maximumBytes,
                         const QString& databaseEpoch, QVariantMap* page,
                         QString* fixedError,
                         const std::atomic_bool* cancelled = nullptr);
};
