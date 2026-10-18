#include "QGCOwnedDownloadFile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryFile>

QGCOwnedDownloadFilePtr QGCOwnedDownloadFile::create(QString& error)
{
    error.clear();
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QFileInfo directoryInfo(directory);
    if (directory.isEmpty() || !QDir::isAbsolutePath(directory) || !directoryInfo.exists()
        || !directoryInfo.isDir() || !directoryInfo.isWritable()) {
        error = QStringLiteral("owned-download-temporary-directory-unavailable");
        return {};
    }
    QTemporaryFile file(QDir(directory).filePath(QStringLiteral("qgc-owned-download-XXXXXX")));
    if (!file.open()) {
        error = QStringLiteral("owned-download-temporary-create-failed");
        return {};
    }
    const QString path = file.fileName();
    const bool flushed = file.flush();
    const bool clean = file.error() == QFileDevice::NoError;
    file.close();
    const QFileInfo created(path);
    if (!flushed || !clean || file.isOpen() || file.error() != QFileDevice::NoError
        || path.isEmpty() || !QDir::isAbsolutePath(path) || !created.exists()
        || !created.isFile() || created.isSymLink() || created.size() != 0) {
        error = QStringLiteral("owned-download-temporary-close-unconfirmed");
        return {};
    }
    QGCOwnedDownloadFilePtr owner(new QGCOwnedDownloadFile(path));
    file.setAutoRemove(false);
    return owner;
}

QGCOwnedDownloadFile::~QGCOwnedDownloadFile()
{
    // Only the unique original created above. No remote basename, cache path,
    // imported path, directory or snapshot is ever adopted or recursively removed.
    // Removal is best effort, not a durable-cleanup or hostile-path guarantee.
    QFile::remove(_path);
}
