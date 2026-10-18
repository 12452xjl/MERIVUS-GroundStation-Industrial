#include "FleetArchiveDirectoryInventory.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QtGlobal>

#include <algorithm>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(Q_OS_UNIX)
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <limits>
#endif

namespace {
constexpr int kMaximumDirectoryEntries = 4096;
constexpr int kMaximumPathCharacters = 4096;
constexpr int kMaximumNameBytes = 4096;

using Inventory = FleetArchiveDirectoryInventory;

bool cancelledNow(const std::atomic_bool* cancelled)
{
    return cancelled && cancelled->load(std::memory_order_acquire);
}

bool unicodeValid(const QString& value)
{
    for (int index = 0; index < value.size(); ++index) {
        const ushort unit = value.at(index).unicode();
        if (unit == 0) return false;
        if (unit >= 0xd800 && unit <= 0xdbff) {
            if (++index >= value.size()) return false;
            const ushort low = value.at(index).unicode();
            if (low < 0xdc00 || low > 0xdfff) return false;
        } else if (unit >= 0xdc00 && unit <= 0xdfff) return false;
    }
    return true;
}

bool directoryValid(const QString& directory)
{
    // Bound caller-controlled text before normalization or filesystem queries.
    if (directory.isEmpty() || directory.size() > kMaximumPathCharacters || !unicodeValid(directory)) return false;
    const QString normalized = QDir::fromNativeSeparators(directory);
    if (!QDir::isAbsolutePath(normalized) || normalized.startsWith(QStringLiteral("//"))
        || QDir::cleanPath(normalized) != normalized) return false;
#if defined(Q_OS_WIN)
    // Exclude device namespaces, drive-relative paths and alternate streams.
    if (normalized.size() < 3 || normalized.at(1) != QLatin1Char(':') || normalized.at(2) != QLatin1Char('/')
        || !((normalized.at(0) >= QLatin1Char('A') && normalized.at(0) <= QLatin1Char('Z'))
            || (normalized.at(0) >= QLatin1Char('a') && normalized.at(0) <= QLatin1Char('z')))
        || normalized.mid(2).contains(QLatin1Char(':'))) return false;
#endif
    const QFileInfo info(normalized);
    return info.exists() && info.isDir() && info.isReadable() && !info.isSymLink()
        && QDir::fromNativeSeparators(info.canonicalFilePath()) == normalized;
}

bool acceptName(const QString& name, const QString& prefix, int maximumFiles,
                QSet<QString>* seen, Inventory::Snapshot* result)
{
    if (!unicodeValid(name) || name.isEmpty() || name.size() > kMaximumPathCharacters
        || name.contains(QLatin1Char('/'))
#if defined(Q_OS_WIN)
        || name.contains(QLatin1Char('\\'))
#endif
        ) {
        result->errorCode = QStringLiteral("archive-directory-name-rejected"); return false;
    }
    if (name == QStringLiteral(".") || name == QStringLiteral("..")) return true;
    if (++result->observedEntries > kMaximumDirectoryEntries) {
        result->errorCode = QStringLiteral("archive-directory-entry-limit"); return false;
    }
    if (!name.startsWith(prefix, Qt::CaseInsensitive)) return true;
    if (seen->contains(name)) { result->errorCode = QStringLiteral("archive-directory-duplicate-name"); return false; }
    if (result->entries.size() >= maximumFiles) { result->errorCode = QStringLiteral("archive-directory-file-limit"); return false; }
    seen->insert(name);
    Inventory::Entry entry;
    entry.basename = name;
    result->entries.append(entry);
    return true;
}

#if defined(Q_OS_WIN)
class NativeHandle
{
public:
    explicit NativeHandle(HANDLE value = INVALID_HANDLE_VALUE, bool search = false) : _value(value), _search(search) {}
    ~NativeHandle() { close(); }
    NativeHandle(const NativeHandle&) = delete;
    NativeHandle& operator=(const NativeHandle&) = delete;
    HANDLE get() const { return _value; }
    bool valid() const { return _value != INVALID_HANDLE_VALUE && _value != nullptr; }
    bool close()
    {
        if (!valid()) return true;
        const HANDLE value = _value;
        _value = INVALID_HANDLE_VALUE;
        return (_search ? FindClose(value) : CloseHandle(value)) != 0;
    }
private:
    HANDLE _value;
    const bool _search;
};

QString nativePath(const QString& directory)
{
    return QStringLiteral("\\\\?\\") + QDir::toNativeSeparators(QDir::fromNativeSeparators(directory));
}

HANDLE openDirectory(const QString& path)
{
    return CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
}

bool directoryInformation(HANDLE handle, BY_HANDLE_FILE_INFORMATION* information)
{
    return handle != INVALID_HANDLE_VALUE && GetFileType(handle) == FILE_TYPE_DISK
        && GetFileInformationByHandle(handle, information) != 0
        && (information->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0
        && (information->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0;
}

bool sameDirectory(const BY_HANDLE_FILE_INFORMATION& first, const BY_HANDLE_FILE_INFORMATION& second)
{
    return first.dwVolumeSerialNumber == second.dwVolumeSerialNumber
        && first.nFileIndexHigh == second.nFileIndexHigh && first.nFileIndexLow == second.nFileIndexLow
        && first.ftCreationTime.dwHighDateTime == second.ftCreationTime.dwHighDateTime
        && first.ftCreationTime.dwLowDateTime == second.ftCreationTime.dwLowDateTime
        && first.ftLastWriteTime.dwHighDateTime == second.ftLastWriteTime.dwHighDateTime
        && first.ftLastWriteTime.dwLowDateTime == second.ftLastWriteTime.dwLowDateTime
        && first.dwFileAttributes == second.dwFileAttributes && first.nNumberOfLinks == second.nNumberOfLinks;
}

bool nativeScan(const QString& directory, const QString& prefix, int maximumFiles,
                const std::atomic_bool* cancelled, Inventory::Snapshot* result)
{
    const QString normalized = QDir::fromNativeSeparators(directory);
    const QString driveRoot = QDir::toNativeSeparators(normalized.left(3));
    const UINT driveType = GetDriveTypeW(reinterpret_cast<LPCWSTR>(driveRoot.utf16()));
    if (driveType == DRIVE_UNKNOWN || driveType == DRIVE_NO_ROOT_DIR || driveType == DRIVE_REMOTE) {
        result->errorCode = QStringLiteral("archive-directory-drive-rejected"); return false;
    }
    const QString path = nativePath(directory);
    NativeHandle directoryHandle(openDirectory(path));
    BY_HANDLE_FILE_INFORMATION before{};
    if (!directoryHandle.valid() || !directoryInformation(directoryHandle.get(), &before)) {
        result->errorCode = QStringLiteral("archive-directory-open-failed"); return false;
    }
    const QString pattern = path + (path.endsWith(QLatin1Char('\\')) ? QStringLiteral("*") : QStringLiteral("\\*"));
    WIN32_FIND_DATAW data{};
    const HANDLE first = FindFirstFileW(reinterpret_cast<LPCWSTR>(pattern.utf16()), &data);
    const DWORD firstError = first == INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;
    NativeHandle search(first, true);
    if (!search.valid() && firstError != ERROR_FILE_NOT_FOUND) {
        result->errorCode = QStringLiteral("archive-directory-enumeration-failed"); return false;
    }
    QSet<QString> seen;
    if (search.valid()) {
        while (true) {
            if (cancelledNow(cancelled)) { result->errorCode = QStringLiteral("archive-directory-cancelled"); return false; }
            int length = 0;
            const int capacity = static_cast<int>(sizeof(data.cFileName) / sizeof(data.cFileName[0]));
            while (length < capacity && data.cFileName[length] != L'\0') ++length;
            if (length == capacity) { result->errorCode = QStringLiteral("archive-directory-name-rejected"); return false; }
            const QString name = QString::fromWCharArray(data.cFileName, length);
            const int previousSize = result->entries.size();
            if (!acceptName(name, prefix, maximumFiles, &seen, result)) return false;
            if (result->entries.size() != previousSize) {
                const QString childPath = path + (path.endsWith(QLatin1Char('\\')) ? QString() : QStringLiteral("\\")) + name;
                NativeHandle child(openDirectory(childPath));
                BY_HANDLE_FILE_INFORMATION actual{};
                const bool observed = child.valid() && GetFileType(child.get()) == FILE_TYPE_DISK
                    && GetFileInformationByHandle(child.get(), &actual) != 0;
                const bool closed = child.close();
                constexpr DWORD typeMask = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE;
                if (!closed || !observed || (actual.dwFileAttributes & typeMask) != (data.dwFileAttributes & typeMask)) {
                    result->errorCode = QStringLiteral("archive-directory-entry-metadata-failed"); return false;
                }
                Inventory::Entry& entry = result->entries.last();
                entry.symlink = (actual.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
                entry.regularFile = (actual.dwFileAttributes & typeMask) == 0;
            }
            if (cancelledNow(cancelled)) { result->errorCode = QStringLiteral("archive-directory-cancelled"); return false; }
            if (FindNextFileW(search.get(), &data)) continue;
            const DWORD error = GetLastError();
            if (error != ERROR_NO_MORE_FILES) { result->errorCode = QStringLiteral("archive-directory-enumeration-failed"); return false; }
            break;
        }
    }
    // ERROR_FILE_NOT_FOUND is accepted as an empty enumeration only with the
    // independent, real directory handle and the same-path re-open below.
    if (!search.close()) { result->errorCode = QStringLiteral("archive-directory-close-failed"); return false; }
    BY_HANDLE_FILE_INFORMATION after{};
    if (cancelledNow(cancelled) || !directoryInformation(directoryHandle.get(), &after) || !sameDirectory(before, after)
        || !directoryValid(directory)) {
        result->errorCode = cancelledNow(cancelled) ? QStringLiteral("archive-directory-cancelled")
            : QStringLiteral("archive-directory-changed"); return false;
    }
    NativeHandle current(openDirectory(path));
    BY_HANDLE_FILE_INFORMATION reopened{};
    const bool same = current.valid() && directoryInformation(current.get(), &reopened) && sameDirectory(before, reopened);
    const bool currentClosed = current.close();
    const bool originalClosed = directoryHandle.close();
    if (!currentClosed || !originalClosed) { result->errorCode = QStringLiteral("archive-directory-close-failed"); return false; }
    if (!same) { result->errorCode = QStringLiteral("archive-directory-changed"); return false; }
    result->directoryIdentity = QStringLiteral("windows-volume-file-index:%1:%2:%3")
        .arg(static_cast<qulonglong>(before.dwVolumeSerialNumber), 8, 16, QLatin1Char('0'))
        .arg(static_cast<qulonglong>(before.nFileIndexHigh), 8, 16, QLatin1Char('0'))
        .arg(static_cast<qulonglong>(before.nFileIndexLow), 8, 16, QLatin1Char('0'));
    return true;
}
#elif defined(Q_OS_UNIX) && defined(AT_SYMLINK_NOFOLLOW)
class DirectoryStream
{
public:
    explicit DirectoryStream(DIR* value = nullptr) : _value(value) {}
    ~DirectoryStream() { close(); }
    DirectoryStream(const DirectoryStream&) = delete;
    DirectoryStream& operator=(const DirectoryStream&) = delete;
    DIR* get() const { return _value; }
    bool close()
    {
        if (!_value) return true;
        DIR* const value = _value;
        _value = nullptr;
        // No retry: after an error the descriptor might already be released.
        return closedir(value) == 0;
    }
private:
    DIR* _value;
};

bool directoryInformation(DIR* stream, struct stat* information)
{
    if (!stream) return false;
    const int descriptor = dirfd(stream);
    return descriptor >= 0 && fstat(descriptor, information) == 0 && S_ISDIR(information->st_mode);
}

bool sameDirectory(const struct stat& first, const struct stat& second)
{
    bool same = first.st_dev == second.st_dev && first.st_ino == second.st_ino
        && first.st_mode == second.st_mode && first.st_nlink == second.st_nlink
        && first.st_size == second.st_size && first.st_mtime == second.st_mtime && first.st_ctime == second.st_ctime;
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    same = same && first.st_mtim.tv_nsec == second.st_mtim.tv_nsec && first.st_ctim.tv_nsec == second.st_ctim.tv_nsec;
#elif defined(Q_OS_MACOS)
    same = same && first.st_mtimespec.tv_nsec == second.st_mtimespec.tv_nsec && first.st_ctimespec.tv_nsec == second.st_ctimespec.tv_nsec;
#endif
    return same;
}

bool nativeScan(const QString& directory, const QString& prefix, int maximumFiles,
                const std::atomic_bool* cancelled, Inventory::Snapshot* result)
{
    const QByteArray path = QFile::encodeName(QDir::fromNativeSeparators(directory));
    if (path.isEmpty() || path.contains('\0') || QFile::decodeName(path) != QDir::fromNativeSeparators(directory)) {
        result->errorCode = QStringLiteral("archive-directory-encoding-rejected"); return false;
    }
    struct stat pathBefore{};
    if (lstat(path.constData(), &pathBefore) != 0 || !S_ISDIR(pathBefore.st_mode) || S_ISLNK(pathBefore.st_mode)) {
        result->errorCode = QStringLiteral("archive-directory-open-failed"); return false;
    }
    DirectoryStream stream(opendir(path.constData()));
    struct stat before{};
    if (!directoryInformation(stream.get(), &before) || !sameDirectory(pathBefore, before)) {
        result->errorCode = QStringLiteral("archive-directory-open-failed"); return false;
    }
    const int descriptor = dirfd(stream.get());
    QSet<QString> seen;
    while (true) {
        if (cancelledNow(cancelled)) { result->errorCode = QStringLiteral("archive-directory-cancelled"); return false; }
        errno = 0;
        const struct dirent* nativeEntry = readdir(stream.get());
        const int readError = errno;
        if (!nativeEntry) {
            if (readError != 0) { result->errorCode = QStringLiteral("archive-directory-enumeration-failed"); return false; }
            break;
        }
        // d_name is a native NUL-terminated variable-size field, not a fixed
        // sizeof(dirent) buffer. Bound the scan and require lossless conversion.
        int length = 0;
        while (length <= kMaximumNameBytes && nativeEntry->d_name[length] != '\0') ++length;
        if (length > kMaximumNameBytes) { result->errorCode = QStringLiteral("archive-directory-name-rejected"); return false; }
        const QByteArray bytes(nativeEntry->d_name, length);
        const QString name = QFile::decodeName(bytes);
        if (QFile::encodeName(name) != bytes) { result->errorCode = QStringLiteral("archive-directory-encoding-rejected"); return false; }
        const int previousSize = result->entries.size();
        if (!acceptName(name, prefix, maximumFiles, &seen, result)) return false;
        if (result->entries.size() != previousSize) {
            struct stat actual{};
            if (fstatat(descriptor, bytes.constData(), &actual, AT_SYMLINK_NOFOLLOW) != 0) {
                result->errorCode = QStringLiteral("archive-directory-entry-metadata-failed"); return false;
            }
            Inventory::Entry& entry = result->entries.last();
            entry.symlink = S_ISLNK(actual.st_mode);
            entry.regularFile = S_ISREG(actual.st_mode);
        }
    }
    struct stat after{};
    if (cancelledNow(cancelled) || !directoryInformation(stream.get(), &after) || !sameDirectory(before, after)
        || !directoryValid(directory)) {
        result->errorCode = cancelledNow(cancelled) ? QStringLiteral("archive-directory-cancelled")
            : QStringLiteral("archive-directory-changed"); return false;
    }
    struct stat pathAfter{};
    if (lstat(path.constData(), &pathAfter) != 0 || !S_ISDIR(pathAfter.st_mode) || S_ISLNK(pathAfter.st_mode)
        || !sameDirectory(before, pathAfter)) { result->errorCode = QStringLiteral("archive-directory-changed"); return false; }
    DirectoryStream current(opendir(path.constData()));
    struct stat reopened{};
    const bool same = directoryInformation(current.get(), &reopened) && sameDirectory(before, reopened);
    const bool currentClosed = current.close();
    const bool originalClosed = stream.close();
    if (!currentClosed || !originalClosed) { result->errorCode = QStringLiteral("archive-directory-close-failed"); return false; }
    if (!same) { result->errorCode = QStringLiteral("archive-directory-changed"); return false; }
    // uintmax_t is the widest standard unsigned integer; never silently narrow
    // dev_t/ino_t into the public, observation-only directory object identifier.
    static_assert(sizeof(dev_t) <= sizeof(std::uintmax_t), "dev_t must fit uintmax_t");
    static_assert(sizeof(ino_t) <= sizeof(std::uintmax_t), "ino_t must fit uintmax_t");
    const auto decimal = [](std::uintmax_t value) {
        char buffer[std::numeric_limits<std::uintmax_t>::digits10 + 2];
        int end = static_cast<int>(sizeof(buffer));
        int start = end;
        do { buffer[--start] = static_cast<char>('0' + value % 10); value /= 10; } while (value != 0);
        return QString::fromLatin1(buffer + start, end - start);
    };
    result->directoryIdentity = QStringLiteral("posix-device-inode:") + decimal(static_cast<std::uintmax_t>(before.st_dev))
        + QLatin1Char(':') + decimal(static_cast<std::uintmax_t>(before.st_ino));
    return true;
}
#else
bool nativeScan(const QString&, const QString&, int, const std::atomic_bool*, Inventory::Snapshot* result)
{
    result->errorCode = QStringLiteral("archive-directory-platform-unsupported"); return false;
}
#endif
} // namespace

FleetArchiveDirectoryInventory::Snapshot FleetArchiveDirectoryInventory::scan(
    const QString& canonicalAbsoluteDirectory, Namespace which, int maximumFiles, const std::atomic_bool* cancelled)
{
    Snapshot result;
    QString prefix;
    if (which == Namespace::TelemetryJournals) prefix = QStringLiteral("fleet-telemetry-");
    else if (which == Namespace::SealedEventArchives) prefix = QStringLiteral("fleet-events-sealed-");
    else if (which == Namespace::RetiredEventArchives) prefix = QStringLiteral("fleet-events-retired-");
    else if (which == Namespace::EventBackups) prefix = QStringLiteral("fleet-events-backup-");
    else { result.errorCode = QStringLiteral("archive-directory-namespace-rejected"); return result; }
    if (maximumFiles < 1 || maximumFiles > 128 || !directoryValid(canonicalAbsoluteDirectory)) {
        result.errorCode = QStringLiteral("archive-directory-configuration-rejected"); return result;
    }
    if (cancelledNow(cancelled)) { result.errorCode = QStringLiteral("archive-directory-cancelled"); return result; }
    const bool complete = nativeScan(canonicalAbsoluteDirectory, prefix, maximumFiles, cancelled, &result);
    if (!complete || cancelledNow(cancelled)) {
        if (cancelledNow(cancelled)) result.errorCode = QStringLiteral("archive-directory-cancelled");
        if (result.errorCode.isEmpty()) result.errorCode = QStringLiteral("archive-directory-enumeration-failed");
        result.entries.clear(); result.directoryIdentity.clear();
        return result;
    }
    std::sort(result.entries.begin(), result.entries.end(), [](const Entry& first, const Entry& second) {
        return first.basename < second.basename;
    });
    if (cancelledNow(cancelled)) {
        result.entries.clear(); result.directoryIdentity.clear(); result.errorCode = QStringLiteral("archive-directory-cancelled");
        return result;
    }
    result.ok = true;
    result.enumerationCompleted = true;
    return result;
}
