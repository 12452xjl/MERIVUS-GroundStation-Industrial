#pragma once

#include <QString>
#include <memory>

class QGCOwnedDownloadFile;
using QGCOwnedDownloadFilePtr = std::shared_ptr<const QGCOwnedDownloadFile>;

// A native, single-transfer input owner, not a path-adoption or authority API.
// create() exclusively creates and closes a new empty temporary file. The
// transport pins this owner until its writer is closed; consumers pin it until
// their actual reads have ended, including parentless background workers.
class QGCOwnedDownloadFile final
{
public:
    static QGCOwnedDownloadFilePtr create(QString& error);
    ~QGCOwnedDownloadFile();

    QString path() const { return _path; }
    QGCOwnedDownloadFile(const QGCOwnedDownloadFile&) = delete;
    QGCOwnedDownloadFile& operator=(const QGCOwnedDownloadFile&) = delete;

private:
    explicit QGCOwnedDownloadFile(const QString& path) : _path(path) {}
    const QString _path;
};
