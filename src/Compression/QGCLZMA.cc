/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCLZMA.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QScopeGuard>
#include <QtDebug>

#include <mutex>

#include "xz.h"


static std::once_flag crc_init;

bool QGCLZMA::inflateLZMAFile(const QString& lzmaFilename, const QString& decompressedFilename)
{
    QFile inputFile(lzmaFilename);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        qWarning() << "QGCLZMA::inflateLZMAFile: open input file failed" << lzmaFilename << inputFile.errorString();
        return false;
    }

    QFile outputFile(decompressedFilename);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "QGCLZMA::inflateLZMAFile: open input file failed" << outputFile.fileName() << outputFile.errorString();
        return false;
    }


    std::call_once(crc_init, []() {
        xz_crc32_init();
        xz_crc64_init();
    });


    xz_dec *s = xz_dec_init(XZ_DYNALLOC, (uint32_t)-1);
    if (s == nullptr) {
        qWarning() << "QGCLZMA::inflateLZMAFile: Memory allocation failed";
        return false;
    }

    const int buf_size = 4 * 1024;
    uint8_t in[buf_size];
    uint8_t out[buf_size];

    xz_buf b;
    b.in = in;
    b.in_pos = 0;
    b.in_size = 0;
    b.out = out;
    b.out_pos = 0;
    b.out_size = buf_size;

    while (true) {
        if (b.in_pos == b.in_size) {
            b.in_size = static_cast<size_t>(inputFile.read((char*)in, sizeof(in)));
            b.in_pos = 0;
        }

        xz_ret ret = xz_dec_run(s, &b);

        if (b.out_pos == sizeof(out)) {
            size_t cBytesWritten = (size_t)outputFile.write((char*)out, static_cast<int>(b.out_pos));
            if (cBytesWritten != b.out_pos) {
                qWarning() << "QGCLZMA::inflateLZMAFile: output file write failed:" << outputFile.fileName() << outputFile.errorString();
                goto error;
            }

            b.out_pos = 0;
        }

        if (ret == XZ_OK)
            continue;

        if (ret == XZ_UNSUPPORTED_CHECK) {
            qWarning() << "QGCLZMA::inflateLZMAFile: Unsupported check; not verifying file integrity";
            continue;
        }

        size_t cBytesWritten = (size_t)outputFile.write((char*)out, static_cast<int>(b.out_pos));
        if (cBytesWritten != b.out_pos) {
            qWarning() << "QGCLZMA::inflateLZMAFile: output file write failed:" << outputFile.fileName() << outputFile.errorString();
            goto error;
        }

        switch (ret) {
        case XZ_STREAM_END:
            xz_dec_end(s);
            return true;

        case XZ_MEM_ERROR:
            qWarning() << "QGCLZMA::inflateLZMAFile: Memory allocation failed";
            goto error;

        case XZ_MEMLIMIT_ERROR:
            qWarning() << "QGCLZMA::inflateLZMAFile: Memory usage limit reached";
            goto error;

        case XZ_FORMAT_ERROR:
            qWarning() << "QGCLZMA::inflateLZMAFile: Not a .xz file";
            goto error;

        case XZ_OPTIONS_ERROR:
            qWarning() << "QGCLZMA::inflateLZMAFile: Unsupported options in the .xz headers";
            goto error;

        case XZ_DATA_ERROR:
        case XZ_BUF_ERROR:
            qWarning() << "QGCLZMA::inflateLZMAFile: File is corrupt";
            goto error;

        default:
            qWarning() << "QGCLZMA::inflateLZMAFile: Bug!";
            goto error;
        }
    }

    xz_dec_end(s);
    return true;

error:
    xz_dec_end(s);
    return false;

}

bool QGCLZMA::inflateLZMAFile(const QString& lzmaFilename, const QString& decompressedFilename,
                             qint64 maximumInputBytes, qint64 maximumOutputBytes,
                             quint32 maximumDictionaryBytes, const std::atomic_bool& cancelled,
                             QString* error)
{
    constexpr qint64 maximumBytes = 32LL * 1024 * 1024;
    constexpr quint32 maximumDictionary = 64U * 1024 * 1024;
    QString code = QStringLiteral("");
    const auto fail = [&code](const QString& reason) { if (code.isEmpty()) code = reason; };
    if (error) *error = QStringLiteral("");
    QFile input(lzmaFilename), output(decompressedFilename);
    const auto finish = [&](bool success) {
        if (output.isOpen()) {
            const bool hadError = output.error() != QFileDevice::NoError;
            output.close();
            if (hadError || output.error() != QFileDevice::NoError) fail(QStringLiteral("metadata-xz-output-close-unconfirmed"));
        }
        if (input.isOpen()) {
            const bool hadError = input.error() != QFileDevice::NoError;
            input.close();
            if (hadError || input.error() != QFileDevice::NoError) fail(QStringLiteral("metadata-xz-input-close-unconfirmed"));
        }
        if (cancelled.load()) fail(QStringLiteral("metadata-artifact-cancelled"));
        if (error) *error = code;
        return success && code.isEmpty();
    };
    const QFileInfo source(lzmaFilename), target(decompressedFilename);
    if (maximumInputBytes < 1 || maximumInputBytes > maximumBytes || maximumOutputBytes < 1 || maximumOutputBytes > maximumBytes
        || maximumDictionaryBytes < 1 || maximumDictionaryBytes > maximumDictionary
        || !source.exists() || !source.isFile() || source.isSymLink() || !source.isReadable()
        || source.size() < 1 || source.size() > maximumInputBytes
        || !target.exists() || !target.isFile() || target.isSymLink()
        || source.canonicalFilePath() == target.canonicalFilePath()) {
        fail(QStringLiteral("metadata-xz-input-or-limit-invalid")); return finish(false);
    }
    if (cancelled.load()) { fail(QStringLiteral("metadata-artifact-cancelled")); return finish(false); }
    if (!input.open(QIODevice::ReadOnly) || input.size() != source.size()) {
        fail(QStringLiteral("metadata-xz-input-open-failed")); return finish(false);
    }
    if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered)) {
        fail(QStringLiteral("metadata-xz-output-open-failed")); return finish(false);
    }
    std::call_once(crc_init, []() { xz_crc32_init(); xz_crc64_init(); });
    xz_dec* decoder = xz_dec_init(XZ_DYNALLOC, maximumDictionaryBytes);
    if (!decoder) { fail(QStringLiteral("metadata-xz-allocation-failed")); return finish(false); }
    const auto releaseDecoder = qScopeGuard([decoder]() { xz_dec_end(decoder); });
    constexpr int blockBytes = 64 * 1024;
    uint8_t incoming[blockBytes], outgoing[blockBytes];
    xz_buf buffer{};
    buffer.in = incoming; buffer.out = outgoing; buffer.out_size = sizeof(outgoing);
    qint64 inputBytes = 0, outputBytes = 0;
    while (true) {
        if (cancelled.load()) { fail(QStringLiteral("metadata-artifact-cancelled")); return finish(false); }
        if (buffer.in_pos == buffer.in_size) {
            const qint64 count = input.read(reinterpret_cast<char*>(incoming), sizeof(incoming));
            // Check the signed result before converting to xz's size_t.
            if (count < 0 || input.error() != QFileDevice::NoError) {
                fail(QStringLiteral("metadata-xz-input-read-failed")); return finish(false);
            }
            if (count > maximumInputBytes - inputBytes) {
                fail(QStringLiteral("metadata-xz-input-limit-exceeded")); return finish(false);
            }
            inputBytes += count;
            buffer.in_pos = 0; buffer.in_size = static_cast<size_t>(count);
        }
        const xz_ret result = xz_dec_run(decoder, &buffer);
        if (buffer.out_pos > static_cast<size_t>(maximumOutputBytes - outputBytes)) {
            fail(QStringLiteral("metadata-xz-output-limit-exceeded")); return finish(false);
        }
        if (buffer.out_pos > 0) {
            const qint64 count = static_cast<qint64>(buffer.out_pos);
            // Do not retry a short write or call its prefix a complete stream.
            if (output.write(reinterpret_cast<const char*>(outgoing), count) != count || output.error() != QFileDevice::NoError) {
                fail(QStringLiteral("metadata-xz-output-write-failed")); return finish(false);
            }
            outputBytes += count; buffer.out_pos = 0;
        }
        if (result == XZ_OK) continue;
        if (result != XZ_STREAM_END) {
            switch (result) {
            case XZ_UNSUPPORTED_CHECK: fail(QStringLiteral("metadata-xz-check-unsupported")); break;
            case XZ_MEM_ERROR: fail(QStringLiteral("metadata-xz-allocation-failed")); break;
            case XZ_MEMLIMIT_ERROR: fail(QStringLiteral("metadata-xz-dictionary-limit-exceeded")); break;
            case XZ_OPTIONS_ERROR: fail(QStringLiteral("metadata-xz-options-unsupported")); break;
            case XZ_FORMAT_ERROR: fail(QStringLiteral("metadata-xz-format-invalid")); break;
            default: fail(QStringLiteral("metadata-xz-stream-invalid")); break;
            }
            return finish(false);
        }
        // Concatenated streams and padding are deliberately not accepted by
        // this bounded metadata overload. Nothing after STREAM_END is ignored.
        char extra = 0;
        const qint64 trailing = input.read(&extra, 1);
        if (buffer.in_pos != buffer.in_size || trailing != 0 || input.error() != QFileDevice::NoError
            || inputBytes != source.size() || input.size() != source.size() || !input.atEnd()) {
            fail(QStringLiteral("metadata-xz-trailing-or-incomplete-input")); return finish(false);
        }
        if (cancelled.load()) { fail(QStringLiteral("metadata-artifact-cancelled")); return finish(false); }
        if (output.size() != outputBytes || !output.flush()) { fail(QStringLiteral("metadata-xz-output-flush-failed")); return finish(false); }
        return finish(true);
    }
}
