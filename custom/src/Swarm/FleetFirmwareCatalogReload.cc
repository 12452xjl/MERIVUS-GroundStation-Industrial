#include "FleetFirmwareCatalogReload.h"

#include "FleetFirmwareAdapterCatalog.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetMissionReviewCoordinator.h"
#include "FleetOperatorSession.h"
#include "FleetRolePolicy.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QThread>
#include <QUuid>

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
const QString Action = QStringLiteral("firmware.catalog.reload");
const QString ApprovalAction = QStringLiteral("firmware.catalog.reload.approve");
constexpr qint64 LifetimeMs = 30000;
constexpr int MaximumFileBytes = 1024 * 1024;
struct BusyGuard {
    QPointer<FleetFirmwareCatalogReload> owner;
    bool* busy;
    bool notifyOnExit = false;
    ~BusyGuard() {
        if (!owner) return;
        *busy = false;
        // Do not unlock a reentrant synchronous caller from this old stack.
        // Only public calls queue this value-notification; Root-check guards
        // must not create a perpetual callback/NOTIFY cycle.
        if (notifyOnExit) QMetaObject::invokeMethod(owner.data(), "changed", Qt::QueuedConnection);
    }
};
bool yes(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull() && value.toBool(); }
bool no(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull() && !value.toBool(); }
bool text(const QString& value, int maximum)
{
    if (value.isEmpty() || value.size() > maximum || value != value.trimmed()
        || QString::fromUtf8(value.toUtf8()) != value) return false;
    for (QChar ch : value) if (ch.isNull() || ch.category() == QChar::Other_Control || ch.category() == QChar::Other_Format) return false;
    return true;
}
bool uuid(const QVariant& value)
{
    return value.type() == QVariant::String && !value.isNull() && !QUuid(value.toString()).isNull()
        && QUuid(value.toString()).toString(QUuid::WithoutBraces) == value.toString();
}
bool decimal(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString string = value.toString();
    if (string.isEmpty() || string.size() > 20 || (string.size() > 1 && string.at(0) == QLatin1Char('0'))) return false;
    for (QChar ch : string) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 parsed = string.toULongLong(&ok);
    return ok && QString::number(parsed) == string;
}
bool digest(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() != 64) return false;
    for (QChar ch : value.toString()) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}
QString hash(const QVariantMap& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}
bool roleMatches(const QVariantMap& identity, const QVariantMap& role)
{
    const QString applicantRole = identity.value(QStringLiteral("role")).toString();
    return identity.size() == 11 && yes(identity.value(QStringLiteral("authenticated")))
        && (applicantRole == QStringLiteral("maintenance") || applicantRole == QStringLiteral("administrator"))
        && role.size() == 9 && role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")).type() == QVariant::String && role.value(QStringLiteral("action")).toString() == Action
        && yes(role.value(QStringLiteral("allowed"))) && yes(role.value(QStringLiteral("requiresSecondApproval")))
        && no(role.value(QStringLiteral("actorAuthenticated"))) && no(role.value(QStringLiteral("executionPermitted")));
}
struct FileSnapshot {
    QString directoryIdentity, baseIdentity, fileIdentity;
    QByteArray bytes;
    QString json;
    QVariantMap observation(const QString& path) const {
        return {{QStringLiteral("fixedPath"), path}, {QStringLiteral("directoryIdentity"), directoryIdentity},
            {QStringLiteral("baseDirectoryIdentity"), baseIdentity}, {QStringLiteral("fileIdentity"), fileIdentity},
            {QStringLiteral("byteLength"), bytes.size()},
            {QStringLiteral("rawSha256"), QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())}};
    }
};
bool canonicalDirectory(const QString& path)
{
    const QFileInfo info(path);
    return info.exists() && info.isDir() && !info.isSymLink()
        && QDir::fromNativeSeparators(info.canonicalFilePath()) == path;
}
#if defined(Q_OS_WIN)
class NativeHandle final {
public:
    explicit NativeHandle(HANDLE value) : _value(value) {}
    ~NativeHandle() { close(); }
    NativeHandle(const NativeHandle&) = delete;
    NativeHandle& operator=(const NativeHandle&) = delete;
    HANDLE get() const { return _value; }
    bool valid() const { return _value != INVALID_HANDLE_VALUE && _value != nullptr; }
    bool close() { if (!valid()) return true; const HANDLE old = _value; _value = INVALID_HANDLE_VALUE; return CloseHandle(old) != 0; }
private:
    HANDLE _value;
};
QString nativePath(const QString& path) { return QStringLiteral("\\\\?\\") + QDir::toNativeSeparators(path); }
QString nativeIdentity(const BY_HANDLE_FILE_INFORMATION& value)
{
    return QStringLiteral("w:%1:%2:%3").arg(value.dwVolumeSerialNumber, 0, 16)
        .arg(value.nFileIndexHigh, 0, 16).arg(value.nFileIndexLow, 0, 16);
}
bool directoryIdentity(const QString& path, QString* output)
{
    const QString native = nativePath(path);
    NativeHandle handle(CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    BY_HANDLE_FILE_INFORMATION info{};
    if (!handle.valid() || GetFileType(handle.get()) != FILE_TYPE_DISK || !GetFileInformationByHandle(handle.get(), &info)
        || !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
    const QString identity = nativeIdentity(info);
    if (!handle.close()) return false;
    *output = identity; return true;
}
bool nativeReadFile(const QString& path, FileSnapshot* output)
{
    const QString native = nativePath(path);
    NativeHandle handle(CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()), GENERIC_READ | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
    BY_HANDLE_FILE_INFORMATION before{}, after{};
    if (!handle.valid() || GetFileType(handle.get()) != FILE_TYPE_DISK || !GetFileInformationByHandle(handle.get(), &before)
        || (before.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
        || before.nNumberOfLinks != 1 || before.nFileSizeHigh != 0 || before.nFileSizeLow > DWORD(MaximumFileBytes)) return false;
    QByteArray bytes(int(before.nFileSizeLow), '\0');
    DWORD offset = 0;
    while (offset < before.nFileSizeLow) {
        DWORD received = 0;
        if (!ReadFile(handle.get(), bytes.data() + offset, before.nFileSizeLow - offset, &received, nullptr) || received == 0) return false;
        offset += received;
    }
    char extra = 0; DWORD extraCount = 0;
    if (!ReadFile(handle.get(), &extra, 1, &extraCount, nullptr) || extraCount != 0
        || !GetFileInformationByHandle(handle.get(), &after) || nativeIdentity(before) != nativeIdentity(after)
        || before.nFileSizeLow != after.nFileSizeLow || after.nFileSizeHigh != 0
        || before.ftLastWriteTime.dwLowDateTime != after.ftLastWriteTime.dwLowDateTime
        || before.ftLastWriteTime.dwHighDateTime != after.ftLastWriteTime.dwHighDateTime || !handle.close()) return false;
    output->fileIdentity = nativeIdentity(after); output->bytes = bytes; return true;
}
#elif defined(Q_OS_UNIX)
class NativeDescriptor final {
public:
    explicit NativeDescriptor(int value) : _value(value) {}
    ~NativeDescriptor() { close(); }
    NativeDescriptor(const NativeDescriptor&) = delete;
    NativeDescriptor& operator=(const NativeDescriptor&) = delete;
    int get() const { return _value; }
    bool valid() const { return _value >= 0; }
    bool close() { if (!valid()) return true; const int old = _value; _value = -1; return ::close(old) == 0; }
private:
    int _value;
};
QString nativeIdentity(const struct stat& value)
{
    return QStringLiteral("p:%1:%2").arg(static_cast<qulonglong>(value.st_dev), 0, 16)
        .arg(static_cast<qulonglong>(value.st_ino), 0, 16);
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
    const QString identity = nativeIdentity(info);
    if (!handle.close()) return false;
    *output = identity; return true;
#else
    Q_UNUSED(path) Q_UNUSED(output)
    return false;
#endif
}
bool nativeReadFile(const QString& path, FileSnapshot* output)
{
#if defined(O_NOFOLLOW)
    const QByteArray encoded = QFile::encodeName(path);
    if (QFile::decodeName(encoded) != path) return false;
    NativeDescriptor handle(::open(encoded.constData(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK
#ifdef O_CLOEXEC
        | O_CLOEXEC
#endif
    ));
    struct stat before{}, after{};
    if (!handle.valid() || ::fstat(handle.get(), &before) != 0 || !S_ISREG(before.st_mode)
        || before.st_nlink != 1 || before.st_size < 0 || before.st_size > MaximumFileBytes) return false;
    QByteArray bytes(int(before.st_size), '\0');
    qint64 offset = 0; int interruptions = 0;
    while (offset < before.st_size) {
        const ssize_t received = ::read(handle.get(), bytes.data() + offset, size_t(before.st_size - offset));
        if (received < 0 && errno == EINTR && ++interruptions <= 8) continue;
        if (received <= 0) return false;
        offset += received;
    }
    char extra = 0;
    ssize_t extraCount = -1;
    do { extraCount = ::read(handle.get(), &extra, 1); } while (extraCount < 0 && errno == EINTR && ++interruptions <= 8);
    if (extraCount != 0 || ::fstat(handle.get(), &after) != 0 || nativeIdentity(before) != nativeIdentity(after)
        || before.st_size != after.st_size || before.st_mtime != after.st_mtime || before.st_ctime != after.st_ctime
        || !handle.close()) return false;
    output->fileIdentity = nativeIdentity(after); output->bytes = bytes; return true;
#else
    Q_UNUSED(path) Q_UNUSED(output)
    return false;
#endif
}
#else
bool directoryIdentity(const QString&, QString*) { return false; }
bool nativeReadFile(const QString&, FileSnapshot*) { return false; }
#endif
bool readFixedFile(const QString& fixedPath, FileSnapshot* output)
{
    if (!output || !text(fixedPath, 4096) || !QDir::isAbsolutePath(fixedPath)
        || fixedPath.startsWith(QStringLiteral("//")) || QDir::cleanPath(fixedPath) != fixedPath
        || QFileInfo(fixedPath).fileName() != QStringLiteral("firmware-adapters-v1.json")) return false;
#if defined(Q_OS_WIN)
    if (fixedPath.size() <= 3 || fixedPath.at(1) != QLatin1Char(':') || fixedPath.at(2) != QLatin1Char('/')
        || !fixedPath.at(0).isLetter() || fixedPath.mid(2).contains(QLatin1Char(':'))) return false;
#endif
    const QString directory = QFileInfo(fixedPath).absolutePath();
    const QString base = QDir::cleanPath(QDir(directory).filePath(QStringLiteral("..")));
    if (QFileInfo(directory).fileName() != QStringLiteral("fleet-os") || !canonicalDirectory(base) || !canonicalDirectory(directory)) return false;
    FileSnapshot snapshot;
    if (!directoryIdentity(base, &snapshot.baseIdentity) || !directoryIdentity(directory, &snapshot.directoryIdentity)
        || !nativeReadFile(fixedPath, &snapshot)) return false;
    QString baseAfter, directoryAfter;
    if (!canonicalDirectory(base) || !canonicalDirectory(directory) || !directoryIdentity(base, &baseAfter)
        || !directoryIdentity(directory, &directoryAfter) || snapshot.baseIdentity != baseAfter
        || snapshot.directoryIdentity != directoryAfter || snapshot.bytes.isEmpty() || snapshot.bytes.size() > MaximumFileBytes) return false;
    snapshot.json = QString::fromUtf8(snapshot.bytes);
    if (snapshot.json.toUtf8() != snapshot.bytes) return false;
    *output = snapshot; return true;
}
} // namespace

FleetFirmwareCatalogReload::FleetFirmwareCatalogReload(FleetMissionReviewCoordinator* actualCoordinator,
    FleetFirmwareAdapterCatalog* actualCatalog, const QString& fixedFilePath)
    : QObject(actualCoordinator), _coordinator(actualCoordinator), _catalog(actualCatalog),
      _fixedFilePath(QDir::cleanPath(QDir::fromNativeSeparators(fixedFilePath))),
      _workflowEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    _clock.start();
    _timer.setInterval(100);
    connect(&_timer, &QTimer::timeout, this, [this]() {
        if (_state != QStringLiteral("review") && _state != QStringLiteral("approving")) return;
        const QPointer<FleetFirmwareCatalogReload> self(this);
        const quint64 generation = _generation;
        if (_clock.elapsed() >= _deadlineMs)
            invalidate(generation, QStringLiteral("original-catalog-reload-review-expired"));
        else if (!sourcesCurrent(generation, false, false) && self)
            invalidate(generation, QStringLiteral("catalog-reload-original-source-changed"));
    });
    _timer.start();
}

FleetFirmwareCatalogReload::~FleetFirmwareCatalogReload()
{
    _state = QStringLiteral("retiring"); _busy = true; _consuming = false;
    _timer.stop();
    disconnectSources();
}

QVariantMap FleetFirmwareCatalogReload::current() const
{
    if (thread() != QThread::currentThread()) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("workflowEpoch"), _workflowEpoch},
        {QStringLiteral("reviewId"), _reviewId}, {QStringLiteral("reviewGeneration"), QString::number(_generation)},
        {QStringLiteral("state"), _state}, {QStringLiteral("reason"), _reason},
        {QStringLiteral("scopeToken"), _scopeToken}, {QStringLiteral("bindingHash"), _bindingHash},
        {QStringLiteral("remainingReviewMs"), qMax<qint64>(0, _deadlineMs - _clock.elapsed())},
        {QStringLiteral("busy"), _busy}, {QStringLiteral("promptInProgress"), _prompt},
        {QStringLiteral("fixedFilePath"), _fixedFilePath}, {QStringLiteral("fileObservation"), _fileObservation},
        {QStringLiteral("catalogIdentity"), _catalogIdentity}, {QStringLiteral("preview"), _preview},
        {QStringLiteral("applicantActorId"), _identity.value(QStringLiteral("actorId"), QString())},
        {QStringLiteral("approverActorId"), _approver.value(QStringLiteral("actorId"), QString())},
        {QStringLiteral("replacementConfirmed"), _replacementConfirmed},
        {QStringLiteral("authenticationConsumed"), _authenticationConsumed},
        {QStringLiteral("reloadCommitted"), _reloadCommitted}, {QStringLiteral("commitReceipt"), _commitReceipt},
        {QStringLiteral("authorizationOwnerRetained"), _root && _coordinator && _coordinator->parent() == _root.data()},
        {QStringLiteral("source"), QStringLiteral("fixed-file-local-engineering-declarations")},
        {QStringLiteral("reviewUiIntegrated"), false}, {QStringLiteral("firmwareIdentityAuthenticated"), false},
        {QStringLiteral("signatureVerified"), false}, {QStringLiteral("executionPermitted"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetFirmwareCatalogReload::result(bool ok, const QString& reason) const
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("reviewId"), _reviewId},
        {QStringLiteral("state"), _state}, {QStringLiteral("reason"), reason},
        {QStringLiteral("reloadCommitted"), _reloadCommitted}, {QStringLiteral("commitReceipt"), _commitReceipt},
        {QStringLiteral("replacementConfirmed"), _replacementConfirmed},
        {QStringLiteral("authenticationConsumed"), _authenticationConsumed},
        {QStringLiteral("reviewUiIntegrated"), false}, {QStringLiteral("firmwareIdentityAuthenticated"), false},
        {QStringLiteral("signatureVerified"), false}, {QStringLiteral("executionPermitted"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetFirmwareCatalogReload::catalogIdentity() const
{
    if (!_catalog || _catalog->thread() != thread()) return {};
    const QVariantMap snapshot = _catalog->snapshot(); // Pure native owned values, no I/O/callback.
    if (!yes(snapshot.value(QStringLiteral("ok"))) || !uuid(snapshot.value(QStringLiteral("catalogEpoch")))
        || !decimal(snapshot.value(QStringLiteral("revision"))) || !digest(snapshot.value(QStringLiteral("catalogFingerprint")))) return {};
    return {{QStringLiteral("catalogEpoch"), snapshot.value(QStringLiteral("catalogEpoch"))},
        {QStringLiteral("revision"), snapshot.value(QStringLiteral("revision"))},
        {QStringLiteral("catalogFingerprint"), snapshot.value(QStringLiteral("catalogFingerprint"))}};
}

bool FleetFirmwareCatalogReload::memoryCurrent(quint64 generation) const
{
    return thread() == QThread::currentThread() && generation != 0 && _generation == generation
        && (_state == QStringLiteral("preparing") || _state == QStringLiteral("review") || _state == QStringLiteral("approving"))
        && _deadlineMs >= 0 && _clock.elapsed() < _deadlineMs && _coordinator && _catalog && _root && _session && _roles
        && _coordinator->thread() == thread() && _catalog->thread() == thread() && _root->thread() == thread()
        && _session->thread() == thread() && _roles->thread() == thread()
        && parent() == _coordinator.data() && _catalog->parent() == _coordinator.data()
        && _coordinator->parent() == _root.data()
        && _coordinator->ownsFirmwareCatalogReload(const_cast<FleetFirmwareCatalogReload*>(this), _catalog.data())
        && !_catalogIdentity.isEmpty() && catalogIdentity() == _catalogIdentity && _clock.elapsed() < _deadlineMs;
}

bool FleetFirmwareCatalogReload::contextMatches(QObject* actualSession, QObject* actualRoles,
    const QVariantMap& expectedPrimary, const QVariantMap& expectedRole) const
{
    if (!memoryCurrent(_generation) || actualSession != _session.data() || actualRoles != _roles.data()
        || expectedPrimary.size() != 5 || expectedRole.size() != 9) return false;
    for (const QString& key : {QStringLiteral("sessionId"), QStringLiteral("sessionEpoch"),
        QStringLiteral("sessionRevision"), QStringLiteral("actorId"), QStringLiteral("role")}) {
        const QVariant value = expectedPrimary.value(key);
        if (value.type() != QVariant::String || value.isNull() || value != _identity.value(key)) return false;
    }
    for (const QString& key : {QStringLiteral("actorId"), QStringLiteral("role"), QStringLiteral("action"),
        QStringLiteral("policyEpoch"), QStringLiteral("policyRevision")}) {
        const QVariant value = expectedRole.value(key);
        if (value.type() != QVariant::String || value.isNull()) return false;
    }
    for (const QString& key : {QStringLiteral("allowed"), QStringLiteral("requiresSecondApproval"),
        QStringLiteral("actorAuthenticated"), QStringLiteral("executionPermitted")}) {
        const QVariant value = expectedRole.value(key);
        if (value.type() != QVariant::Bool || value.isNull()) return false;
    }
    return expectedRole == _role && _roles->actionSnapshotCurrent(_role) && roleMatches(_identity, _role)
        && _clock.elapsed() < _deadlineMs;
}

bool FleetFirmwareCatalogReload::rootSourcesCurrent(quint64 generation) const
{
    if (!memoryCurrent(generation) || _rootChecking) return false;
    const QPointer<FleetFirmwareCatalogReload> self(const_cast<FleetFirmwareCatalogReload*>(this));
    const QPointer<FleetMissionReviewCoordinator> coordinator(_coordinator);
    const QPointer<FleetFirmwareAdapterCatalog> catalog(_catalog);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<QObject> root(_root);
    const QString state = _state, id = _reviewId, scope = _scopeToken, binding = _bindingHash;
    const QVariantMap identity = _identity, role = _role, approver = _approver;
    const QVariantMap file = _fileObservation, originalCatalog = _catalogIdentity, preview = _preview;
    _rootChecking = true;
    const BusyGuard checking{self, &_rootChecking};
    const auto same = [&]() {
        return self && memoryCurrent(generation) && root && root == _root && coordinator && coordinator == _coordinator
            && catalog && catalog == _catalog && session && session == _session && roles && roles == _roles
            && _state == state && _reviewId == id && _scopeToken == scope && _bindingHash == binding
            && _identity == identity && _role == role && _approver == approver
            && _fileObservation == file && _catalogIdentity == originalCatalog && _preview == preview
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data())
            && roles->actionSnapshotCurrent(role) && roleMatches(identity, role);
    };
    if (!same()) return false;
    QVariant matched;
    const bool invoked = QMetaObject::invokeMethod(root.data(), "_fleetFirmwareCatalogReloadSourcesCurrent",
        Qt::DirectConnection, Q_RETURN_ARG(QVariant, matched),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(catalog.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(coordinator.data()))), Q_ARG(QVariant, QVariant(scope)));
    return same() && invoked && yes(matched) && same();
}

bool FleetFirmwareCatalogReload::sourcesCurrent(quint64 generation, bool checkIdentity, bool requireApprover)
{
    const QPointer<FleetFirmwareCatalogReload> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    if (!memoryCurrent(generation) || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data())
        || !roles->actionSnapshotCurrent(_role) || !roleMatches(_identity, _role)
        || !rootSourcesCurrent(generation) || !self || !memoryCurrent(generation)) return false;
    const QVariantMap primary = _identity, approver = _approver;
    if (checkIdentity && (!session->identitySnapshotCurrent(primary) || !self || !memoryCurrent(generation))) return false;
    if (!self || !session || !roles || !memoryCurrent(generation) || primary != _identity || approver != _approver
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data())
        || !roles->actionSnapshotCurrent(_role) || !rootSourcesCurrent(generation)
        || !self || !memoryCurrent(generation)) return false;
    if (requireApprover && (!session->approverSnapshotCurrent(approver) || !self || !memoryCurrent(generation))) return false;
    return self && session && roles && memoryCurrent(generation) && primary == _identity && approver == _approver
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data())
        && roles->actionSnapshotCurrent(_role) && rootSourcesCurrent(generation) && self && memoryCurrent(generation);
}

bool FleetFirmwareCatalogReload::authenticationBindingCurrent(FleetOperatorSession* actualSession,
    const QVariantMap& primary, const QVariantMap& approver, const QString& bindingHash) const
{
    // Called around Session OS checks and after its one-use burn. This entire
    // fence is owned memory: never invoke Root, OS authentication or file I/O.
    return memoryCurrent(_generation) && _busy && _consuming && !_prompt && !_authenticationConsumed
        && actualSession == _session.data() && primary == _identity && approver == _approver
        && digest(_bindingHash) && bindingHash == _bindingHash && _replacementConfirmed
        && approver.value(QStringLiteral("action")).type() == QVariant::String
        && approver.value(QStringLiteral("action")).toString() == ApprovalAction
        && approver.value(QStringLiteral("bindingHash")) == _bindingHash
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data())
        && _roles->actionSnapshotCurrent(_role) && roleMatches(_identity, _role) && _clock.elapsed() < _deadlineMs;
}

bool FleetFirmwareCatalogReload::commitBindingCurrent(quint64 generation) const
{
    // The final Catalog hook must stay callback-free after the last fixed-file
    // read. An account decision is not a firmware signature or command lease.
    return memoryCurrent(generation) && _busy && _consuming && _importing && !_prompt && _authenticationConsumed
        && _replacementConfirmed && !_json.isEmpty() && !_preview.isEmpty() && !_fileObservation.isEmpty()
        && digest(_bindingHash) && FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data())
        && _roles->actionSnapshotCurrent(_role) && roleMatches(_identity, _role)
        && _session->firmwareCatalogReloadConsumedIdentityCurrent(_identity, _approver) && _clock.elapsed() < _deadlineMs;
}

void FleetFirmwareCatalogReload::disconnectSources()
{
    const auto connections = _connections;
    _connections.clear();
    for (const auto& connection : connections) QObject::disconnect(connection);
}

void FleetFirmwareCatalogReload::watchSources(quint64 generation)
{
    const QPointer<FleetFirmwareCatalogReload> self(this);
    const auto stale = [this, self, generation]() {
        if (self) invalidate(generation, QStringLiteral("catalog-reload-original-source-mutated"));
    };
    const auto add = [this, self, generation](auto* sender, auto signal, auto handler) {
        if (!sender) return false;
        const auto connection = QObject::connect(sender, signal, this, handler);
        if (!self || !memoryCurrent(generation)) { QObject::disconnect(connection); return false; }
        _connections.append(connection); return true;
    };
    if (!add(_coordinator.data(), &QObject::destroyed, stale) || !add(_catalog.data(), &QObject::destroyed, stale)
        || !add(_root.data(), &QObject::destroyed, stale) || !add(_session.data(), &QObject::destroyed, stale)
        || !add(_roles.data(), &QObject::destroyed, stale)) return;
    if (!add(_catalog.data(), &FleetFirmwareAdapterCatalog::changed, [this, self, generation]() {
        if (self && !_importing) invalidate(generation, QStringLiteral("catalog-reload-catalog-mutated"));
    })) return;
    if (!add(_session.data(), &FleetOperatorSession::changed, [this, self, generation]() {
        if (self && !_prompt) invalidate(generation, QStringLiteral("catalog-reload-account-mutated"));
    })) return;
    if (!add(_roles.data(), &FleetRolePolicy::policyChanged, stale)
        || !add(_roles.data(), &FleetRolePolicy::currentUserChanged, stale)
        || !add(_roles.data(), &FleetRolePolicy::currentRoleChanged, stale)) return;
}

void FleetFirmwareCatalogReload::invalidate(quint64 generation, const QString& reason)
{
    if (thread() != QThread::currentThread() || generation != _generation || _importing
        || (_state != QStringLiteral("preparing") && _state != QStringLiteral("review") && _state != QStringLiteral("approving"))) return;
    _state = _clock.elapsed() >= _deadlineMs ? QStringLiteral("expired") : QStringLiteral("stale");
    _reason = reason;
    const QPointer<FleetFirmwareCatalogReload> self(this);
    disconnectSources();
    if (self) emit changed();
}

QVariantMap FleetFirmwareCatalogReload::requestPreview(QObject* actualSession, QObject* actualRoles,
    QObject* actualRoot, const QString& scopeToken, bool confirmed)
{
    // Freeze all caller references/strings before the first external getter.
    const QPointer<QObject> requestedSession(actualSession), requestedRoles(actualRoles), requestedRoot(actualRoot);
    const QString scope = scopeToken;
    if (thread() != QThread::currentThread() || _busy || _rootChecking || _importing
        || _state == QStringLiteral("preparing") || _state == QStringLiteral("review") || _state == QStringLiteral("approving")
        || !confirmed || !text(scope, 32768) || FleetOperatorSession::authenticationPromptInProgress()
        || _generation == std::numeric_limits<quint64>::max())
        return result(false, QStringLiteral("catalog-reload-request-invalid-or-busy"));
    const QPointer<FleetFirmwareCatalogReload> self(this);
    QVariantMap fallback = result(false, QStringLiteral("catalog-reload-owner-destroyed"));
    _busy = true; const BusyGuard guard{self, &_busy, true};
    disconnectSources();
    if (!self) return fallback;
    _session = qobject_cast<FleetOperatorSession*>(requestedSession.data());
    _roles = qobject_cast<FleetRolePolicy*>(requestedRoles.data());
    _root = requestedRoot; _scopeToken = scope;
    _identity.clear(); _role.clear(); _approver.clear(); _json.clear(); _fileObservation.clear();
    _catalogIdentity.clear(); _preview.clear(); _bindingHash.clear(); _commitReceipt.clear();
    _authenticationConsumed = false; _replacementConfirmed = false; _reloadCommitted = false;
    _consuming = false; _prompt = false; _importing = false;
    _reviewId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const quint64 generation = ++_generation;
    _deadlineMs = _clock.elapsed() + LifetimeMs;
    _state = QStringLiteral("preparing"); _reason.clear();
    fallback = result(false, QStringLiteral("catalog-reload-owner-destroyed"));
    const auto deny = [&](const QString& reason) {
        if (!self) return fallback;
        if (_generation == generation && (_state == QStringLiteral("preparing") || _state == QStringLiteral("review")
            || _state == QStringLiteral("approving"))) {
            _state = _clock.elapsed() >= _deadlineMs ? QStringLiteral("expired") : QStringLiteral("stale"); _reason = reason;
        }
        const QVariantMap response = result(false, _reason.isEmpty() ? reason : _reason);
        disconnectSources();
        if (self) emit changed();
        return response;
    };
    _catalogIdentity = catalogIdentity();
    if (!memoryCurrent(generation) || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data()))
        return deny(QStringLiteral("actual-catalog-reload-sources-invalid"));
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<FleetFirmwareAdapterCatalog> catalog(_catalog);
    const QVariantMap identity = session->identitySnapshot();
    if (!self) return fallback;
    if (!memoryCurrent(generation) || !session || session != _session || !roles || roles != _roles)
        return deny(QStringLiteral("catalog-reload-account-source-changed"));
    _identity = identity; _role = roles->actionSnapshot(Action);
    if (!roleMatches(_identity, _role) || !sourcesCurrent(generation, true, false) || !self)
        return deny(QStringLiteral("catalog-reload-account-or-role-unavailable"));
    FileSnapshot file;
    if (!sourcesCurrent(generation, true, false) || !self)
        return deny(QStringLiteral("catalog-reload-before-file-source-changed"));
    const bool fileRead = readFixedFile(_fixedFilePath, &file);
    if (!self) return fallback;
    if (!fileRead || !memoryCurrent(generation) || !sourcesCurrent(generation, true, false) || !self)
        return deny(QStringLiteral("catalog-reload-fixed-file-unavailable-or-source-changed"));
    _json = file.json; _fileObservation = file.observation(_fixedFilePath);
    // Freeze the original FULL declarations, not a later rolling snapshot.
    // The native Catalog supplies the replacement descriptors/diff separately.
    const QVariantMap originalSnapshot = catalog->snapshot();
    if (!yes(originalSnapshot.value(QStringLiteral("ok")))
        || originalSnapshot.value(QStringLiteral("entries")).type() != QVariant::List
        || originalSnapshot.value(QStringLiteral("entries")).toList().size() > 64
        || originalSnapshot.value(QStringLiteral("catalogEpoch")) != _catalogIdentity.value(QStringLiteral("catalogEpoch"))
        || originalSnapshot.value(QStringLiteral("revision")) != _catalogIdentity.value(QStringLiteral("revision"))
        || originalSnapshot.value(QStringLiteral("catalogFingerprint")) != _catalogIdentity.value(QStringLiteral("catalogFingerprint")))
        return deny(QStringLiteral("catalog-reload-original-catalog-unavailable"));
    const QVariantMap preview = catalog->previewImport(_json, _catalogIdentity.value(QStringLiteral("revision")).toString());
    if (!self) return fallback;
    if (!memoryCurrent(generation) || !catalog || catalog != _catalog || !yes(preview.value(QStringLiteral("ok")))
        || !yes(preview.value(QStringLiteral("previewOnly"))) || !yes(preview.value(QStringLiteral("wholeReplacement")))
        || !no(preview.value(QStringLiteral("mutationCommitted")))
        || preview.value(QStringLiteral("catalogEpoch")) != _catalogIdentity.value(QStringLiteral("catalogEpoch"))
        || preview.value(QStringLiteral("revision")) != _catalogIdentity.value(QStringLiteral("revision"))
        || preview.value(QStringLiteral("catalogFingerprint")) != _catalogIdentity.value(QStringLiteral("catalogFingerprint"))
        || preview.value(QStringLiteral("inputJsonSha256")) != _fileObservation.value(QStringLiteral("rawSha256"))
        || !digest(preview.value(QStringLiteral("normalizedFingerprint"))))
        return deny(QStringLiteral("catalog-reload-file-preview-rejected"));
    _preview = preview;
    _preview.insert(QStringLiteral("previousEntries"), originalSnapshot.value(QStringLiteral("entries")));
    _bindingHash = hash({{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("workflowEpoch"), _workflowEpoch},
        {QStringLiteral("reviewId"), _reviewId}, {QStringLiteral("generation"), QString::number(generation)},
        {QStringLiteral("action"), ApprovalAction}, {QStringLiteral("scopeToken"), _scopeToken},
        {QStringLiteral("fileObservation"), _fileObservation}, {QStringLiteral("catalogIdentity"), _catalogIdentity},
        {QStringLiteral("preview"), _preview}, {QStringLiteral("primaryIdentity"), _identity}, {QStringLiteral("role"), _role}});
    _state = QStringLiteral("review");
    watchSources(generation);
    if (!self) return fallback;
    if (!sourcesCurrent(generation, true, false) || !self)
        return deny(QStringLiteral("catalog-reload-preview-no-longer-current"));
    const QVariantMap response = result(true, QString());
    emit changed();
    return response;
}

QVariantMap FleetFirmwareCatalogReload::approveAndReload(const QString& requestedReviewId, bool replacementConfirmed)
{
    const QString reviewId = requestedReviewId;
    if (thread() != QThread::currentThread() || _busy || _rootChecking || _importing || _state != QStringLiteral("review")
        || reviewId.isEmpty() || reviewId != _reviewId || !replacementConfirmed
        || FleetOperatorSession::authenticationPromptInProgress())
        return result(false, QStringLiteral("catalog-reload-confirmation-invalid-or-busy"));
    const QPointer<FleetFirmwareCatalogReload> self(this);
    QVariantMap fallback = result(false, QStringLiteral("catalog-reload-owner-destroyed"));
    _busy = true; const BusyGuard guard{self, &_busy, true};
    const quint64 generation = _generation;
    const auto deny = [&](const QString& reason) {
        if (!self) return fallback;
        _prompt = false; _consuming = false; _importing = false;
        if (_generation == generation && (_state == QStringLiteral("review") || _state == QStringLiteral("approving"))) {
            _state = _clock.elapsed() >= _deadlineMs ? QStringLiteral("expired") : QStringLiteral("stale"); _reason = reason;
        }
        const QVariantMap response = result(false, _reason.isEmpty() ? reason : _reason);
        disconnectSources();
        if (self) emit changed();
        return response;
    };
    if (!sourcesCurrent(generation, true, false) || !self)
        return deny(QStringLiteral("catalog-reload-review-no-longer-current"));
    _replacementConfirmed = true;
    _state = QStringLiteral("approving"); _prompt = true;
    // REVIEW may be observed while no controller call is held. APPROVING
    // must already be owned by this Root's actual approval call, BEFORE
    // entering any OS credential prompt, not only after that prompt returns.
    if (!sourcesCurrent(generation, true, false) || !self)
        return deny(QStringLiteral("catalog-reload-approval-controller-context-rejected"));
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetFirmwareAdapterCatalog> catalog(_catalog);
    const QString summary = QStringLiteral("Replace the COMPLETE local firmware engineering declaration catalog from fixed file. "
        "File SHA256:%1; normalized SHA256:%2; descriptors:%3. Existing declaration reviewer IDs are archive data, not this account. "
        "NO firmware signature/authentication, firmware upload, aircraft command or startup permission.")
        .arg(_fileObservation.value(QStringLiteral("rawSha256")).toString(),
            _preview.value(QStringLiteral("normalizedFingerprint")).toString(),
            _preview.value(QStringLiteral("descriptorCount")).toString());
    const QVariantMap approval = session->authenticateApprover(ApprovalAction, _bindingHash, _identity, summary);
    if (!self) return fallback;
    _prompt = false;
    if (!memoryCurrent(generation) || !session || session != _session || !catalog || catalog != _catalog
        || !yes(approval.value(QStringLiteral("ok"))) || approval.value(QStringLiteral("identityReceipt")).type() != QVariant::Map)
        return deny(QStringLiteral("independent-catalog-reload-approval-unavailable"));
    _approver = approval.value(QStringLiteral("identityReceipt")).toMap();
    if (!sourcesCurrent(generation, true, true) || !self)
        return deny(QStringLiteral("catalog-reload-post-approval-source-changed"));
    _consuming = true;
    if (!session->consumeFirmwareCatalogReloadAuthentication(_identity, _approver, this, _bindingHash) || !self)
        return deny(QStringLiteral("catalog-reload-authentication-consumption-failed"));
    if (!memoryCurrent(generation)) return deny(QStringLiteral("catalog-reload-source-changed-after-consumption"));
    _authenticationConsumed = true;
    fallback = result(false, QStringLiteral("catalog-reload-owner-destroyed-after-consumption"));
    if (!sourcesCurrent(generation, true, false) || !self)
        return deny(QStringLiteral("catalog-reload-consumed-source-no-longer-current"));
    // This is the last external file observation. No further account/Root
    // callback occurs before Catalog's pure commit fence and local mutation.
    FileSnapshot finalFile;
    const bool reread = readFixedFile(_fixedFilePath, &finalFile);
    if (!self) return fallback;
    if (!reread || !memoryCurrent(generation) || finalFile.json != _json
        || finalFile.observation(_fixedFilePath) != _fileObservation)
        return deny(QStringLiteral("catalog-reload-fixed-file-changed-after-review"));
    _importing = true;
    if (!commitBindingCurrent(generation)) return deny(QStringLiteral("catalog-reload-final-binding-rejected"));
    // Native import stages values, invokes this passive fence, then mutates
    // once. Keep its mutation receipt even if changed deletes this workflow.
    const QString frozenJson = _json, expectedRevision = _catalogIdentity.value(QStringLiteral("revision")).toString();
    const QVariantMap receipt = catalog->importJson(frozenJson, expectedRevision,
        [self, generation]() { return self && self->commitBindingCurrent(generation); });
    const bool committed = yes(receipt.value(QStringLiteral("mutationCommitted")));
    if (!self) {
        fallback.insert(QStringLiteral("ok"), committed && yes(receipt.value(QStringLiteral("ok"))));
        fallback.insert(QStringLiteral("state"), committed ? QStringLiteral("reloaded") : QStringLiteral("stale"));
        fallback.insert(QStringLiteral("reason"), committed ? QStringLiteral("catalog-reloaded-owner-no-longer-retained")
            : QStringLiteral("catalog-reload-owner-destroyed-before-confirmed-mutation"));
        fallback.insert(QStringLiteral("reloadCommitted"), committed);
        fallback.insert(QStringLiteral("commitReceipt"), receipt);
        return fallback;
    }
    _commitReceipt = receipt; _reloadCommitted = committed; _importing = false; _consuming = false;
    if (!committed || !yes(receipt.value(QStringLiteral("ok"))))
        return deny(QStringLiteral("catalog-reload-import-rejected"));
    _state = QStringLiteral("reloaded"); _reason = QStringLiteral("local-engineering-catalog-replaced-not-firmware-authenticated");
    const QVariantMap response = result(true, _reason);
    disconnectSources();
    if (self) emit changed();
    return response;
}

bool FleetFirmwareCatalogReload::cancel(const QString& requestedReviewId)
{
    const QString reviewId = requestedReviewId;
    if (thread() != QThread::currentThread() || _importing || reviewId.isEmpty() || reviewId != _reviewId
        || (_state != QStringLiteral("preparing") && _state != QStringLiteral("review") && _state != QStringLiteral("approving"))) return false;
    _state = QStringLiteral("cancelled"); _reason = QStringLiteral("operator-cancelled-catalog-reload");
    const QPointer<FleetFirmwareCatalogReload> self(this);
    disconnectSources();
    if (self) emit changed();
    return true;
}
