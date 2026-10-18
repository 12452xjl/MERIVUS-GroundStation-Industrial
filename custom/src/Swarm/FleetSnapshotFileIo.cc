#include "FleetSnapshotFileIo.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringList>

namespace {
constexpr qint64 MaximumSupportedBytes = 32LL * 1024LL * 1024LL;

QString code(const QString& prefix, const char* suffix)
{
    return prefix + QLatin1Char('-') + QString::fromLatin1(suffix);
}
} // namespace

bool FleetSnapshotFileIo::localJsonPath(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().isEmpty()
        || value.toString().size() > 4096) return false;
    const QString path = value.toString();
    if (path != path.trimmed() || path.contains(QStringLiteral("://"))
        || QString::fromUtf8(path.toUtf8()) != path || !path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) return false;
    for (QChar c : path) if (c.isNull() || c.category() == QChar::Other_Control) return false;
    const QString slash = QDir::fromNativeSeparators(path);
    if (slash.startsWith(QStringLiteral("//")) || slash.startsWith(QStringLiteral("/dev/"))) return false;
#ifdef Q_OS_WIN
    if (slash.size() < 4 || !((slash.at(0) >= QLatin1Char('A') && slash.at(0) <= QLatin1Char('Z'))
            || (slash.at(0) >= QLatin1Char('a') && slash.at(0) <= QLatin1Char('z')))
        || slash.at(1) != QLatin1Char(':') || slash.at(2) != QLatin1Char('/') || slash.mid(2).contains(QLatin1Char(':'))) return false;
    const QStringList parts = slash.mid(3).split(QLatin1Char('/'));
    for (const QString& part : parts) {
        if (part.isEmpty() || part.endsWith(QLatin1Char('.')) || part.endsWith(QLatin1Char(' '))) return false;
        const QString stem = part.section(QLatin1Char('.'), 0, 0).toUpper();
        const bool numberedDevice = stem.size() == 4
            && (stem.startsWith(QStringLiteral("COM")) || stem.startsWith(QStringLiteral("LPT")))
            && stem.at(3) >= QLatin1Char('1') && stem.at(3) <= QLatin1Char('9');
        if (stem == QStringLiteral("CON") || stem == QStringLiteral("PRN") || stem == QStringLiteral("AUX")
            || stem == QStringLiteral("NUL") || numberedDevice) return false;
    }
#endif
    return QFileInfo(path).isAbsolute();
}

bool FleetSnapshotFileIo::readUtf8(const QString& path, qint64 maximumBytes,
    const std::atomic_bool& cancelled, const QString& errorPrefix, QByteArray* bytes, QString* error)
{
    if (!bytes || !error) return false;
    if (cancelled.load()) { *error = code(errorPrefix, "cancelled"); return false; }
    const QFileInfo info(path);
    if (!info.isFile() || info.isSymLink()) { *error = code(errorPrefix, "file-invalid"); return false; }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { *error = code(errorPrefix, "open-failed"); return false; }
    const qint64 size = file.size();
    if (maximumBytes <= 0 || maximumBytes > MaximumSupportedBytes || size <= 0 || size > maximumBytes) {
        *error = code(errorPrefix, "file-budget"); return false;
    }
    QByteArray input;
    input.reserve(static_cast<int>(size));
    while (input.size() < size) {
        if (cancelled.load()) { *error = code(errorPrefix, "cancelled"); return false; }
        const QByteArray part = file.read(qMin<qint64>(65536, size - input.size()));
        if (part.isEmpty() || file.error() != QFileDevice::NoError) {
            *error = code(errorPrefix, "read-failed"); return false;
        }
        input.append(part);
    }
    const QByteArray tail = file.read(1);
    const bool failed = !tail.isEmpty() || file.size() != size || file.error() != QFileDevice::NoError;
    file.close();
    if (failed || file.error() != QFileDevice::NoError) { *error = code(errorPrefix, "read-failed"); return false; }
    if (QString::fromUtf8(input).toUtf8() != input) { *error = code(errorPrefix, "utf8-invalid"); return false; }
    *bytes = input;
    return true;
}

FleetSnapshotFileIo::SaveResult FleetSnapshotFileIo::saveAndReadback(const QString& path,
    const QByteArray& bytes, qint64 maximumBytes, const std::atomic_bool& cancelled, const QString& errorPrefix)
{
    SaveResult result;
    const QFileInfo target(path);
    if (target.isSymLink() || (target.exists() && !target.isFile())) {
        result.error = code(errorPrefix, "save-target-invalid"); return result;
    }
    if (cancelled.load()) { result.error = code(errorPrefix, "cancelled"); return result; }
    if (maximumBytes <= 0 || maximumBytes > MaximumSupportedBytes || bytes.isEmpty() || bytes.size() > maximumBytes) {
        result.error = code(errorPrefix, "file-budget"); return result;
    }
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    result.fileWriteAttempted = true;
    if (!file.open(QIODevice::WriteOnly)) { result.error = code(errorPrefix, "save-open-failed"); return result; }
    qint64 written = 0;
    while (written < bytes.size()) {
        if (cancelled.load()) {
            file.cancelWriting(); result.error = code(errorPrefix, "cancelled"); return result;
        }
        const qint64 amount = qMin<qint64>(65536, bytes.size() - written);
        if (file.write(bytes.constData() + written, amount) != amount) {
            file.cancelWriting(); result.error = code(errorPrefix, "write-failed"); return result;
        }
        written += amount;
    }
    if (cancelled.load()) { file.cancelWriting(); result.error = code(errorPrefix, "cancelled"); return result; }
    if (!file.commit()) {
        result.fileWrittenKnown = false;
        result.error = code(errorPrefix, "commit-unconfirmed"); return result;
    }
    // A later cancellation/readback error cannot undo an actual commit.
    result.ok = true;
    result.fileWritten = true;
    QByteArray readback;
    if (!readUtf8(path, maximumBytes, cancelled, errorPrefix, &readback, &result.readbackError)) {
        // readUtf8 supplies the separate readback error, including cancel.
    } else if (readback != bytes) result.readbackError = code(errorPrefix, "readback-mismatch");
    else result.readbackVerified = true;
    result.error = result.readbackError;
    return result;
}
