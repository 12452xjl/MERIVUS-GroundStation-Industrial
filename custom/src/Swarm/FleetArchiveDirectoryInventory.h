#pragma once

#include <QList>
#include <QString>

#include <atomic>

// Read-only native enumeration, not a payload reader or an authority source.
class FleetArchiveDirectoryInventory
{
public:
    enum class Namespace { TelemetryJournals, SealedEventArchives, RetiredEventArchives, EventBackups };

    struct Entry {
        QString basename;
        bool regularFile = false;
        bool symlink = false;
    };

    struct Snapshot {
        bool ok = false;
        QString errorCode;
        QList<Entry> entries;
        QString directoryIdentity;
        int observedEntries = 0;
        bool enumerationCompleted = false;
    };

    static Snapshot scan(const QString& canonicalAbsoluteDirectory,
                         Namespace which,
                         int maximumFiles = 128,
                         const std::atomic_bool* cancelled = nullptr);
};
