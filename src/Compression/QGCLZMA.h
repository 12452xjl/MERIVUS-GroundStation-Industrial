/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QString>

#include <atomic>

class QGCLZMA
{
public:
    /// Decompresses the specified file to the specified directory
    ///     @param lzmaFilename         Fully qualified path to lzma file
    ///     @param decompressedFilename Fully qualified path to for file to decompress to
    static bool inflateLZMAFile(const QString& lzmaFilename, const QString& decompressedFilename);

    // Strict, cancellable XZ-only path for a caller-owned, already-created
    // output file. Limits cannot exceed 32 MiB input/output or 64 MiB dict.
    // Rejects unsupported checks and any bytes after the first XZ stream.
    // Partial output remains the caller's responsibility; no path is removed.
    static bool inflateLZMAFile(const QString& lzmaFilename, const QString& decompressedFilename,
                                qint64 maximumInputBytes, qint64 maximumOutputBytes,
                                quint32 maximumDictionaryBytes, const std::atomic_bool& cancelled,
                                QString* error = nullptr);
};
