/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ComponentInformationCache.h"

#include <QFile>
#include <QDirIterator>
#include <QStandardPaths>

QGC_LOGGING_CATEGORY(ComponentInformationCacheLog, "ComponentInformationCacheLog")

ComponentInformationCache::ComponentInformationCache(const QDir& path, int maxNumFiles)
    : _path(path), _maxNumFiles(maxNumFiles)
{
    initializeDirectory();
}

ComponentInformationCache& ComponentInformationCache::defaultInstance()
{
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1String("/QGCCompInfoCache");
    static ComponentInformationCache instance(cacheDir, 50);
    return instance;
}

QString ComponentInformationCache::metaFileName(const QString& fileTag)
{
    return _path.filePath(fileTag+_metaExtension);
}

QString ComponentInformationCache::dataFileName(const QString& fileTag)
{
    return _path.filePath(fileTag+_cacheExtension);
}

QString ComponentInformationCache::access(const QString &fileTag)
{
    QFile meta(metaFileName(fileTag));
    QFile data(dataFileName(fileTag));
    if (!meta.exists() || !data.exists()) {
        qCDebug(ComponentInformationCacheLog) << "Cache miss for" << fileTag;
        return "";
    }

    qCDebug(ComponentInformationCacheLog) << "Cache hit for" << fileTag;

    // mark access
    Meta m{};
    AccessCounterType previousCounter = -1;
    if (meta.open(QIODevice::ReadWrite)) {
        if (meta.read((char*)&m, sizeof(m)) == sizeof(m)) {
            previousCounter = m.accessCounter;
            m.accessCounter = _nextAccessCounter;
            meta.seek(0);
            if (meta.write((const char*)&m, sizeof(m)) != sizeof(m)) {
                qCWarning(ComponentInformationCacheLog) << "Meta write failed" << meta.fileName() << meta.errorString();
            }
        } else {
            qCWarning(ComponentInformationCacheLog) << "Meta read failed" << meta.fileName() << meta.errorString();
        }
        meta.close();
    } else {
        qCWarning(ComponentInformationCacheLog) << "Failed to open" << meta.fileName() << meta.errorString();
    }

    _cachedFiles.remove(previousCounter);
    _cachedFiles[_nextAccessCounter] = fileTag;
    ++_nextAccessCounter;

    return data.fileName();
}

QString ComponentInformationCache::insert(const QString &fileTag, const QString &fileName)
{
    QFile meta(metaFileName(fileTag));
    QFile data(dataFileName(fileTag));
    QFile fileToCache(fileName);
    if (meta.exists() || data.exists()) {
        qCDebug(ComponentInformationCacheLog) << "Not inserting, entry already exists" << fileTag;
        fileToCache.remove();
        return data.fileName();
    }

    // move the file to the cache location
    if (!fileToCache.rename(data.fileName())) {
        qCWarning(ComponentInformationCacheLog) << "File rename failed from:to" << fileName << data.fileName();
        return "";
    }

    // write meta data
    Meta m{};
    m.accessCounter = _nextAccessCounter;
    if (meta.open(QIODevice::WriteOnly)) {
        if (meta.write((const char*)&m, sizeof(m)) != sizeof(m)) {
            qCWarning(ComponentInformationCacheLog) << "Meta write failed" << meta.fileName() << meta.errorString();
        }
        meta.close();
    } else {
        qCWarning(ComponentInformationCacheLog) << "Failed to open" << meta.fileName() << meta.errorString();
    }

    // update internal data
    _cachedFiles[_nextAccessCounter++] = fileTag;
    ++_numFiles;

    removeOldEntries();
    return data.fileName();
}

bool ComponentInformationCache::invalidate(const QString& fileTag)
{
    // Never turn a cache key into a path, a wildcard or a directory request.
    // Keep the older access/insert caller domains unchanged.
    if (fileTag.isEmpty() || fileTag.size() > 128) return false;
    for (const QChar ch : fileTag) {
        if (!((ch >= QLatin1Char('A') && ch <= QLatin1Char('Z'))
            || (ch >= QLatin1Char('a') && ch <= QLatin1Char('z'))
            || (ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            || ch == QLatin1Char('_') || ch == QLatin1Char('-'))) return false;
    }

    const QString metaPath = metaFileName(fileTag);
    const QString dataPath = dataFileName(fileTag);
    // Attempt each exact file once, including when the other removal fails.
    // Do not treat QFileInfo/exists false as confirmed absence (e.g. an IO or
    // permission failure), retry, recurse, or trigger unrelated LRU eviction.
    const bool metaRemoved = QFile::remove(metaPath);
    const bool dataRemoved = QFile::remove(dataPath);
    for (auto it = _cachedFiles.begin(); it != _cachedFiles.end();) {
        if (it.value() == fileTag) it = _cachedFiles.erase(it);
        else ++it;
    }
    _numFiles = _cachedFiles.size();
    const bool removed = metaRemoved && dataRemoved;
    if (!removed) {
        qCWarning(ComponentInformationCacheLog) << "Exact cache invalidation unconfirmed" << fileTag
            << "metadata removed:" << metaRemoved << "data removed:" << dataRemoved;
    }
    return removed;
}

void ComponentInformationCache::initializeDirectory()
{
    if (!_path.exists()) {
        QDir d;
        if (!d.mkpath(_path.path())) {
            qCWarning(ComponentInformationCacheLog) << "Failed to create dir" << _path.path();
        }
    }

    QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot;
    QDirIterator it(_path.path(), filters, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        QString path = it.next();

        if (path.endsWith(_metaExtension)) {
            QFile meta(path);
            QFile data(path.mid(0, path.length()-strlen(_metaExtension))+_cacheExtension);
            bool validationFailed = false;
            if (!data.exists()) {
                validationFailed = true;
            }

            // read meta + validate
            Meta m{};
            const uint32_t expectedMagic = m.magic;
            const uint32_t expectedVersion = m.version;
            if (meta.open(QIODevice::ReadOnly)) {
                if (meta.read((char*)&m, sizeof(m)) == sizeof(m)) {
                    if (m.magic != expectedMagic || m.version != expectedVersion) {
                        validationFailed = true;
                    }
                } else {
                    validationFailed = true;
                }
                meta.close();
            } else {
                validationFailed = true;
            }

            if (validationFailed) {
                qCWarning(ComponentInformationCacheLog) << "Validation failed, removing cache files" << path;
                meta.remove();
                data.remove();
            } else {
                // extract the tag
                QString tag = it.fileName();
                tag = tag.mid(0, tag.length()-strlen(_metaExtension));
                _cachedFiles[m.accessCounter] = tag;

                qCDebug(ComponentInformationCacheLog) << "Found cached file:counter" << meta.fileName() << m.accessCounter;

                if (m.accessCounter >= _nextAccessCounter) {
                    _nextAccessCounter = m.accessCounter + 1;
                }
            }

        } else if (!path.endsWith(_cacheExtension)) {
            QFile::remove(path);
        }
    }
    _numFiles = _cachedFiles.size();
    removeOldEntries();
}

void ComponentInformationCache::removeOldEntries()
{
    while (_numFiles > _maxNumFiles) {
        auto iter = _cachedFiles.begin();
        QFile meta(metaFileName(iter.value()));
        QFile data(dataFileName(iter.value()));
        qCDebug(ComponentInformationCacheLog) << "Removing cache entry num:counter:file" << _numFiles << iter.key() << iter.value();
        meta.remove();
        data.remove();

        _cachedFiles.erase(iter);
        --_numFiles;
    }
}
