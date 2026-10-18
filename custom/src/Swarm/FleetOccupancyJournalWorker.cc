#include "FleetOccupancyJournalWorker.h"
#include "FleetOccupancyJournalCodec.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QMutexLocker>
#include <QSaveFile>
#include <QUuid>

#include <algorithm>
#include <limits>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(Q_OS_UNIX)
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace {
using Codec = FleetOccupancyJournalCodec;
constexpr int kMaximumPathCharacters = 4096;
const QString kFileName = QStringLiteral("native-occupancy-v1.json");
const QString kLockName = QStringLiteral(".native-occupancy-v1.lock");

bool textValid(const QString& value, int maximum)
{
    if (value.isEmpty() || value.size() > maximum || value != value.trimmed()) return false;
    for (int index = 0; index < value.size(); ++index) {
        const ushort unit = value.at(index).unicode();
        if (unit == 0 || value.at(index).category() == QChar::Other_Control) return false;
        if (unit >= 0xd800 && unit <= 0xdbff) {
            if (++index >= value.size()) return false;
            const ushort low = value.at(index).unicode();
            if (low < 0xdc00 || low > 0xdfff) return false;
        } else if (unit >= 0xdc00 && unit <= 0xdfff) return false;
    }
    return QString::fromUtf8(value.toUtf8()) == value;
}

bool uuid(const QString& value)
{
    return value.size() == 36 && !QUuid(value).isNull()
        && QUuid(value).toString(QUuid::WithoutBraces) == value;
}

bool directorySyntax(const QString& value)
{
    if (!textValid(value, kMaximumPathCharacters)) return false;
    const QString path = QDir::fromNativeSeparators(value);
    if (!QDir::isAbsolutePath(path) || path.startsWith(QStringLiteral("//"))
        || path == QStringLiteral("/") || QDir::cleanPath(path) != path) return false;
#if defined(Q_OS_WIN)
    if (path.size() <= 3 || path.at(1) != QLatin1Char(':') || path.at(2) != QLatin1Char('/')
        || !path.at(0).isLetter() || path.mid(2).contains(QLatin1Char(':'))) return false;
#endif
    return path.size() + 1 + QStringLiteral("fleet-os/").size() + kFileName.size() <= kMaximumPathCharacters;
}

bool canonicalDirectory(const QString& directory)
{
    const QFileInfo info(directory);
    return info.exists() && info.isDir() && !info.isSymLink()
        && QDir::fromNativeSeparators(info.canonicalFilePath()) == directory;
}

struct FileSnapshot {
    QString identity;
    QByteArray bytes;
    QByteArray hash;
};
enum class ReadResult { Missing, Present, Rejected };

#if defined(Q_OS_WIN)
class NativeHandle final
{
public:
    explicit NativeHandle(HANDLE value) : _value(value) {}
    ~NativeHandle() { close(); }
    NativeHandle(const NativeHandle&) = delete;
    NativeHandle& operator=(const NativeHandle&) = delete;
    HANDLE get() const { return _value; }
    bool valid() const { return _value != INVALID_HANDLE_VALUE && _value != nullptr; }
    bool close() {
        if (!valid()) return true;
        const HANDLE value = _value; _value = INVALID_HANDLE_VALUE;
        return CloseHandle(value) != 0;
    }
private:
    HANDLE _value;
};

QString nativePath(const QString& path)
{
    return QStringLiteral("\\\\?\\") + QDir::toNativeSeparators(path);
}

QString identity(const BY_HANDLE_FILE_INFORMATION& info)
{
    return QStringLiteral("w:%1:%2:%3").arg(info.dwVolumeSerialNumber, 0, 16)
        .arg(info.nFileIndexHigh, 0, 16).arg(info.nFileIndexLow, 0, 16);
}

bool directoryIdentity(const QString& path, QString* output)
{
    const QString native = nativePath(path);
    NativeHandle handle(CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    BY_HANDLE_FILE_INFORMATION info{};
    if (!handle.valid() || GetFileType(handle.get()) != FILE_TYPE_DISK
        || !GetFileInformationByHandle(handle.get(), &info)
        || !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
    const QString result = identity(info);
    if (!handle.close()) return false;
    *output = result; return true;
}

ReadResult readFile(const QString& path, FileSnapshot* output)
{
    const QString native = nativePath(path);
    NativeHandle handle(CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()), GENERIC_READ | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
    if (!handle.valid()) return GetLastError() == ERROR_FILE_NOT_FOUND ? ReadResult::Missing : ReadResult::Rejected;
    BY_HANDLE_FILE_INFORMATION before{}, after{};
    if (GetFileType(handle.get()) != FILE_TYPE_DISK || !GetFileInformationByHandle(handle.get(), &before)
        || (before.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
        || before.nNumberOfLinks != 1 || before.nFileSizeHigh != 0
        || before.nFileSizeLow > static_cast<DWORD>(Codec::MaxFileBytes)) return ReadResult::Rejected;
    QByteArray bytes(static_cast<int>(before.nFileSizeLow), '\0');
    DWORD offset = 0;
    while (offset < before.nFileSizeLow) {
        DWORD read = 0;
        if (!ReadFile(handle.get(), bytes.data() + offset, before.nFileSizeLow - offset, &read, nullptr)
            || read == 0) return ReadResult::Rejected;
        offset += read;
    }
    char extra = 0;
    DWORD read = 0;
    if (!ReadFile(handle.get(), &extra, 1, &read, nullptr) || read != 0
        || !GetFileInformationByHandle(handle.get(), &after)
        || identity(before) != identity(after) || before.nFileSizeLow != after.nFileSizeLow || after.nFileSizeHigh != 0
        || before.ftLastWriteTime.dwLowDateTime != after.ftLastWriteTime.dwLowDateTime
        || before.ftLastWriteTime.dwHighDateTime != after.ftLastWriteTime.dwHighDateTime || !handle.close()) return ReadResult::Rejected;
    output->identity = identity(after); output->bytes = bytes;
    output->hash = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
    return ReadResult::Present;
}
#elif defined(Q_OS_UNIX)
class NativeDescriptor final
{
public:
    explicit NativeDescriptor(int value) : _value(value) {}
    ~NativeDescriptor() { close(); }
    NativeDescriptor(const NativeDescriptor&) = delete;
    NativeDescriptor& operator=(const NativeDescriptor&) = delete;
    int get() const { return _value; }
    bool valid() const { return _value >= 0; }
    bool close() {
        if (!valid()) return true;
        const int value = _value; _value = -1;
        return ::close(value) == 0;
    }
private:
    int _value;
};

QString identity(const struct stat& info)
{
    return QStringLiteral("p:%1:%2").arg(static_cast<qulonglong>(info.st_dev), 0, 16)
        .arg(static_cast<qulonglong>(info.st_ino), 0, 16);
}

bool directoryIdentity(const QString& path, QString* output)
{
#if defined(O_NOFOLLOW) && defined(O_DIRECTORY)
    const QByteArray encoded = QFile::encodeName(path);
    if (QFile::decodeName(encoded) != path) return false;
    NativeDescriptor handle(::open(encoded.constData(), O_RDONLY | O_NOFOLLOW | O_DIRECTORY
#ifdef O_CLOEXEC
        | O_CLOEXEC
#endif
    ));
    struct stat info{};
    if (!handle.valid() || ::fstat(handle.get(), &info) != 0 || !S_ISDIR(info.st_mode)) return false;
    const QString result = identity(info);
    if (!handle.close()) return false;
    *output = result; return true;
#else
    Q_UNUSED(path) Q_UNUSED(output)
    return false;
#endif
}

ReadResult readFile(const QString& path, FileSnapshot* output)
{
#if defined(O_NOFOLLOW)
    const QByteArray encoded = QFile::encodeName(path);
    if (QFile::decodeName(encoded) != path) return ReadResult::Rejected;
    NativeDescriptor handle(::open(encoded.constData(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK
#ifdef O_CLOEXEC
        | O_CLOEXEC
#endif
    ));
    if (!handle.valid()) return errno == ENOENT ? ReadResult::Missing : ReadResult::Rejected;
    struct stat before{}, after{};
    if (::fstat(handle.get(), &before) != 0 || !S_ISREG(before.st_mode) || before.st_nlink != 1
        || before.st_size < 0 || before.st_size > Codec::MaxFileBytes) return ReadResult::Rejected;
    QByteArray bytes(static_cast<int>(before.st_size), '\0');
    qint64 offset = 0;
    int interruptions = 0;
    while (offset < before.st_size) {
        const ssize_t read = ::read(handle.get(), bytes.data() + offset, static_cast<size_t>(before.st_size - offset));
        if (read < 0 && errno == EINTR && ++interruptions <= 8) continue;
        if (read <= 0) return ReadResult::Rejected;
        offset += read;
    }
    char extra = 0;
    if (::read(handle.get(), &extra, 1) != 0 || ::fstat(handle.get(), &after) != 0
        || identity(before) != identity(after) || before.st_size != after.st_size
        || before.st_mtime != after.st_mtime || !handle.close()) return ReadResult::Rejected;
    output->identity = identity(after); output->bytes = bytes;
    output->hash = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
    return ReadResult::Present;
#else
    Q_UNUSED(path) Q_UNUSED(output)
    return ReadResult::Rejected;
#endif
}
#else
bool directoryIdentity(const QString&, QString*) { return false; }
ReadResult readFile(const QString&, FileSnapshot*) { return ReadResult::Rejected; }
#endif

bool sameFile(const FileSnapshot& first, const FileSnapshot& second)
{
    return !first.identity.isEmpty() && first.identity == second.identity
        && first.bytes.size() == second.bytes.size() && first.hash == second.hash && first.bytes == second.bytes;
}

struct StorageScope {
    QString base;
    QString directory;
    QString path;
    QString baseIdentity;
    QString directoryIdentity;
    bool current() const {
        QString actualBase, actualDirectory;
        return canonicalDirectory(base) && canonicalDirectory(directory)
            && ::directoryIdentity(base, &actualBase) && actualBase == baseIdentity
            && ::directoryIdentity(directory, &actualDirectory) && actualDirectory == directoryIdentity;
    }
};

bool prepareStorage(const QString& supplied, StorageScope* output)
{
    if (!directorySyntax(supplied)) return false;
    StorageScope scope;
    scope.base = QDir::fromNativeSeparators(supplied);
    scope.directory = QDir(scope.base).filePath(QStringLiteral("fleet-os"));
    scope.path = QDir(scope.directory).filePath(kFileName);
    const QFileInfo baseBefore(scope.base);
    if (baseBefore.isSymLink() || (baseBefore.exists() && !baseBefore.isDir())) return false;
    // Native integration supplies only its actual AppData path. Never remove,
    // replace or rebuild a bad existing target, and never create caller files.
    if (!baseBefore.exists() && !QDir().mkpath(scope.base)) return false;
    if (!canonicalDirectory(scope.base) || !directoryIdentity(scope.base, &scope.baseIdentity)) return false;
    const QFileInfo childBefore(scope.directory);
    if (childBefore.isSymLink() || (childBefore.exists() && !childBefore.isDir())) return false;
    if (!childBefore.exists() && !QDir(scope.base).mkdir(QStringLiteral("fleet-os"))) return false;
    if (!canonicalDirectory(scope.directory) || !directoryIdentity(scope.directory, &scope.directoryIdentity)
        || !scope.current()) return false;
    *output = scope; return true;
}

bool replaceDocument(const StorageScope& scope, bool expectedExists, const FileSnapshot& expected,
                     const QByteArray& bytes, FileSnapshot* written, bool* mayHaveReplaced, QString* error)
{
    *mayHaveReplaced = false;
    if (bytes.isEmpty() || bytes.size() > Codec::MaxFileBytes || !scope.current()) {
        *error = QStringLiteral("occupancy-storage-scope-changed"); return false;
    }
    FileSnapshot actual;
    const ReadResult state = readFile(scope.path, &actual);
    if ((expectedExists && (state != ReadResult::Present || !sameFile(expected, actual)))
        || (!expectedExists && state != ReadResult::Missing) || !scope.current()) {
        *error = QStringLiteral("occupancy-ledger-baseline-changed"); return false;
    }
    QSaveFile file(scope.path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)
        || !file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)
        || file.write(bytes) != bytes.size()) {
        file.cancelWriting(); *error = QStringLiteral("occupancy-ledger-write-failed"); return false;
    }
    if (!scope.current()) {
        file.cancelWriting(); *error = QStringLiteral("occupancy-storage-scope-changed"); return false;
    }
    FileSnapshot finalBaseline;
    const ReadResult finalState = readFile(scope.path, &finalBaseline);
    if ((expectedExists && (finalState != ReadResult::Present || !sameFile(expected, finalBaseline)))
        || (!expectedExists && finalState != ReadResult::Missing) || !scope.current()) {
        file.cancelWriting(); *error = QStringLiteral("occupancy-ledger-baseline-changed"); return false;
    }
    // This last observation rejects a change seen during temporary-file IO;
    // it is not a rename-time atomic CAS against a malicious external writer.
    // QSaveFile's atomic replacement is not checked fsync or a physical-media
    // durability guarantee. The destination's native file ID changes here.
    *mayHaveReplaced = true;
    if (!file.commit()) { *error = QStringLiteral("occupancy-ledger-commit-failed"); return false; }
    FileSnapshot after;
    QVariantMap decoded;
    if (!scope.current() || readFile(scope.path, &after) != ReadResult::Present
        || after.bytes != bytes || !Codec::decode(after.bytes, &decoded) || !scope.current()) {
        *error = QStringLiteral("occupancy-ledger-readback-failed"); return false;
    }
    *written = after; return true; // Bind the actual replacement, not the old ID.
}

QVariantList orderedIds(const QSet<int>& ids)
{
    QList<int> sorted = ids.values();
    std::sort(sorted.begin(), sorted.end());
    QVariantList result;
    for (int id : sorted) result.append(id);
    return result;
}
} // namespace

FleetOccupancyJournalWorker::FleetOccupancyJournalWorker(const QString& absoluteAppDataDirectory,
                                                       const QString& writerEpoch)
    : QThread(nullptr), _appDataDirectory(absoluteAppDataDirectory), _writerEpoch(writerEpoch),
      _workerRunId(QUuid::createUuid().toString(QUuid::WithoutBraces)),
      _configurationValid(directorySyntax(absoluteAppDataDirectory) && uuid(writerEpoch))
{
    _accepting = _configurationValid;
    _status = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("workerRunId"), _workerRunId},
        {QStringLiteral("writerEpoch"), _writerEpoch}, {QStringLiteral("workerGeneration"), QStringLiteral("1")},
        {QStringLiteral("state"), QStringLiteral("starting")}, {QStringLiteral("errorCode"), QString()},
        {QStringLiteral("ledgerEpoch"), QString()}, {QStringLiteral("ledgerRevision"), QString()},
        {QStringLiteral("offeredRequestId"), QString()}, {QStringLiteral("offeredIdentity"), QVariantMap()},
        {QStringLiteral("committedRequestId"), QString()}, {QStringLiteral("committedIdentity"), QVariantMap()},
        {QStringLiteral("everObservedVehicleIds"), QVariantList()},
        {QStringLiteral("atomicReplacementConfirmed"), false}, {QStringLiteral("replacementMayHaveOccurred"), false},
        {QStringLiteral("osSyncVerified"), false}, {QStringLiteral("durableVerified"), false},
        {QStringLiteral("coverageComplete"), false}, {QStringLiteral("recoveryReviewRequired"), true},
        {QStringLiteral("runOpen"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("authorityGranted"), false}, {QStringLiteral("flightCommandReleased"), false}};
    updateStatusLocked();
}

FleetOccupancyJournalWorker::~FleetOccupancyJournalWorker()
{
    Q_ASSERT(!isRunning()); // No GUI join, wait or forceful termination.
}

bool FleetOccupancyJournalWorker::advanceLocked()
{
    if (_generation == std::numeric_limits<quint64>::max()) {
        _accepting = false; _stopRequested = true; _terminal = true;
        _status.insert(QStringLiteral("state"), QStringLiteral("failed"));
        _status.insert(QStringLiteral("errorCode"), QStringLiteral("occupancy-worker-generation-exhausted"));
        _wake.wakeAll(); return false;
    }
    ++_generation; return true;
}

void FleetOccupancyJournalWorker::updateStatusLocked()
{
    _status.insert(QStringLiteral("workerGeneration"), QString::number(_generation));
    _status.insert(QStringLiteral("accepting"), _accepting && !_terminal);
    _status.insert(QStringLiteral("stopRequested"), _stopRequested);
    _status.insert(QStringLiteral("pending"), _pending);
    _status.insert(QStringLiteral("inFlight"), _inFlight);
    _status.insert(QStringLiteral("busy"), _inFlight || (!_terminal
        && (_pending || _status.value(QStringLiteral("state")).toString() == QStringLiteral("starting"))));
    _status.insert(QStringLiteral("offeredVehicleIds"), orderedIds(_offeredIds));
    _status.insert(QStringLiteral("bootstrapResultAvailable"), !_bootstrapResult.isEmpty());
}

bool FleetOccupancyJournalWorker::offerObservation(const QString& requestId, const QVariantList& actualObservedIds,
                                                  const QVariantMap& actualIdentity4)
{
    QVariantList ids;
    if (!textValid(requestId, 128) || !Codec::vehicleIdsValid(actualObservedIds, &ids)
        || !Codec::identityValid(actualIdentity4)) return false;
    const QVariantMap identityCopy = actualIdentity4;
    QMutexLocker lock(&_mutex);
    if (!_accepting || _terminal || _stopRequested || !advanceLocked()) { updateStatusLocked(); return false; }
    for (const QVariant& value : ids) _offeredIds.insert(value.toInt());
    _pendingRequestId = requestId; _pendingIdentity = identityCopy; _pending = true;
    _status.insert(QStringLiteral("offeredRequestId"), requestId);
    _status.insert(QStringLiteral("offeredIdentity"), identityCopy);
    updateStatusLocked(); _wake.wakeOne(); return true;
}

QVariantMap FleetOccupancyJournalWorker::statusSnapshot() const
{
    QMutexLocker lock(&_mutex); return _status;
}

QVariantMap FleetOccupancyJournalWorker::takeBootstrapResult()
{
    QMutexLocker lock(&_mutex);
    const QVariantMap result = _bootstrapResult;
    if (!result.isEmpty()) advanceLocked();
    _bootstrapResult.clear(); updateStatusLocked(); return result;
}

void FleetOccupancyJournalWorker::requestStop()
{
    QMutexLocker lock(&_mutex);
    if (_terminal || _stopRequested) return;
    _accepting = false; _stopRequested = true;
    if (advanceLocked()) _status.insert(QStringLiteral("state"), QStringLiteral("stopping"));
    updateStatusLocked(); _wake.wakeAll();
}

void FleetOccupancyJournalWorker::fail(const QString& fixedCode, bool replacementMayHaveOccurred)
{
    QMutexLocker lock(&_mutex);
    if (!_terminal) advanceLocked();
    _accepting = false; _stopRequested = true; _terminal = true; _inFlight = false;
    _status.insert(QStringLiteral("state"), QStringLiteral("failed"));
    if (_status.value(QStringLiteral("errorCode")).toString().isEmpty()) _status.insert(QStringLiteral("errorCode"), fixedCode);
    _status.insert(QStringLiteral("replacementMayHaveOccurred"), replacementMayHaveOccurred);
    if (!_bootstrapPublished) {
        _bootstrapPublished = true;
        _bootstrapResult = {{QStringLiteral("ok"), false}, {QStringLiteral("ledgerEpoch"), _status.value(QStringLiteral("ledgerEpoch"))},
            {QStringLiteral("ledgerRevision"), _status.value(QStringLiteral("ledgerRevision"))},
            {QStringLiteral("oldObservedIds"), _status.value(QStringLiteral("everObservedVehicleIds"))},
            {QStringLiteral("newWriterEpoch"), _writerEpoch}, {QStringLiteral("workerRunId"), _workerRunId},
            {QStringLiteral("workerGeneration"), QString::number(_generation)}, {QStringLiteral("needsReview"), true},
            {QStringLiteral("atomicCommitConfirmed"), false}, {QStringLiteral("errorCode"), _status.value(QStringLiteral("errorCode"))}};
    }
    updateStatusLocked(); _wake.wakeAll();
}

void FleetOccupancyJournalWorker::run()
{
    if (!_configurationValid) { fail(QStringLiteral("occupancy-configuration-rejected")); return; }
    StorageScope scope;
    if (!prepareStorage(_appDataDirectory, &scope)) { fail(QStringLiteral("occupancy-directory-rejected")); return; }
    const QString lockPath = QDir(scope.directory).filePath(kLockName);
    FileSnapshot lockBefore;
    if (readFile(lockPath, &lockBefore) == ReadResult::Rejected) { fail(QStringLiteral("occupancy-lock-path-rejected")); return; }
    // Cooperative, same fixed path, whole-run ownership. Zero disables only
    // age expiry; Qt may still detect stale PID/name. Never explicitly reclaim.
    QLockFile owner(lockPath);
    owner.setStaleLockTime(0);
    if (!owner.tryLock(0)) { fail(QStringLiteral("occupancy-owner-lock-unavailable")); return; }
    if (!scope.current()) { fail(QStringLiteral("occupancy-storage-scope-changed")); return; }
    FileSnapshot baseline;
    const ReadResult loaded = readFile(scope.path, &baseline);
    QVariantMap document;
    if (loaded == ReadResult::Rejected || (loaded == ReadResult::Present && !Codec::decode(baseline.bytes, &document))) {
        fail(QStringLiteral("occupancy-existing-ledger-rejected")); return;
    }
    const QVariantList oldIds = document.value(QStringLiteral("everObservedVehicleIds")).toList();
    if (loaded == ReadResult::Missing) {
        document = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("format"), QStringLiteral("merivus-native-occupancy-journal")},
            {QStringLiteral("ledgerEpoch"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
            {QStringLiteral("ledgerRevision"), QStringLiteral("1")}, {QStringLiteral("writerEpoch"), _writerEpoch},
            {QStringLiteral("runOpen"), true}, {QStringLiteral("coverageComplete"), false},
            {QStringLiteral("everObservedVehicleIds"), QVariantList()}, {QStringLiteral("lastObservationIdentity"), QVariantMap()},
            {QStringLiteral("recoveryReviewRequired"), true}};
    } else {
        const quint64 previous = document.value(QStringLiteral("ledgerRevision")).toString().toULongLong();
        if (previous == std::numeric_limits<quint64>::max()) { fail(QStringLiteral("occupancy-ledger-revision-exhausted")); return; }
        document.remove(QStringLiteral("fingerprint"));
        document.insert(QStringLiteral("ledgerRevision"), QString::number(previous + 1));
        document.insert(QStringLiteral("writerEpoch"), _writerEpoch);
    }
    QString error;
    bool mayHaveReplaced = false;
    FileSnapshot written;
    if (!replaceDocument(scope, loaded == ReadResult::Present, baseline, Codec::encode(document), &written, &mayHaveReplaced, &error)) {
        fail(error, mayHaveReplaced); return;
    }
    baseline = written;
    {
        QMutexLocker lock(&_mutex);
        if (_terminal || !advanceLocked()) { updateStatusLocked(); return; }
        for (const QVariant& id : oldIds) _offeredIds.insert(id.toInt());
        _status.insert(QStringLiteral("state"), _stopRequested ? QStringLiteral("stopping") : QStringLiteral("ready"));
        _status.insert(QStringLiteral("ledgerEpoch"), document.value(QStringLiteral("ledgerEpoch")));
        _status.insert(QStringLiteral("ledgerRevision"), document.value(QStringLiteral("ledgerRevision")));
        _status.insert(QStringLiteral("everObservedVehicleIds"), oldIds);
        _status.insert(QStringLiteral("atomicReplacementConfirmed"), true);
        _status.insert(QStringLiteral("replacementMayHaveOccurred"), true);
        _bootstrapPublished = true;
        _bootstrapResult = {{QStringLiteral("ok"), true}, {QStringLiteral("ledgerEpoch"), document.value(QStringLiteral("ledgerEpoch"))},
            {QStringLiteral("ledgerRevision"), document.value(QStringLiteral("ledgerRevision"))}, {QStringLiteral("oldObservedIds"), oldIds},
            {QStringLiteral("newWriterEpoch"), _writerEpoch}, {QStringLiteral("workerRunId"), _workerRunId},
            {QStringLiteral("workerGeneration"), QString::number(_generation)}, {QStringLiteral("needsReview"), true},
            {QStringLiteral("atomicCommitConfirmed"), true}};
        updateStatusLocked();
    }
    while (true) {
        QString request;
        QVariantMap observation;
        QVariantList ids;
        {
            QMutexLocker lock(&_mutex);
            // QWaitCondition atomically releases the mutex while sleeping;
            // no public ingress or GUI poll waits behind filesystem work.
            while (!_pending && !_stopRequested && !_terminal) _wake.wait(&_mutex);
            if (_terminal) { updateStatusLocked(); return; }
            if (!_pending && _stopRequested) {
                _accepting = false; _terminal = true;
                if (advanceLocked()) _status.insert(QStringLiteral("state"), QStringLiteral("stopped"));
                updateStatusLocked(); return;
            }
            if (!advanceLocked()) { updateStatusLocked(); return; }
            request = _pendingRequestId; observation = _pendingIdentity; ids = orderedIds(_offeredIds);
            _pending = false; _inFlight = true;
            _status.insert(QStringLiteral("state"), _stopRequested ? QStringLiteral("stopping") : QStringLiteral("writing"));
            updateStatusLocked();
        }
        const quint64 previous = document.value(QStringLiteral("ledgerRevision")).toString().toULongLong();
        if (previous == std::numeric_limits<quint64>::max()) { fail(QStringLiteral("occupancy-ledger-revision-exhausted")); return; }
        document.insert(QStringLiteral("ledgerRevision"), QString::number(previous + 1));
        document.insert(QStringLiteral("everObservedVehicleIds"), ids);
        document.insert(QStringLiteral("lastObservationIdentity"), observation);
        if (!replaceDocument(scope, true, baseline, Codec::encode(document), &written, &mayHaveReplaced, &error)) {
            fail(error, mayHaveReplaced); return;
        }
        baseline = written;
        {
            QMutexLocker lock(&_mutex);
            _inFlight = false;
            if (_terminal || !advanceLocked()) { updateStatusLocked(); return; }
            _status.insert(QStringLiteral("state"), _stopRequested ? QStringLiteral("stopping") : QStringLiteral("ready"));
            _status.insert(QStringLiteral("ledgerRevision"), document.value(QStringLiteral("ledgerRevision")));
            _status.insert(QStringLiteral("everObservedVehicleIds"), ids);
            _status.insert(QStringLiteral("committedRequestId"), request);
            _status.insert(QStringLiteral("committedIdentity"), observation);
            _status.insert(QStringLiteral("atomicReplacementConfirmed"), true);
            _status.insert(QStringLiteral("replacementMayHaveOccurred"), true);
            updateStatusLocked();
        }
    }
}
