#pragma once

#include <QElapsedTimer>
#include <QString>
#include <atomic>

// Value/file-only translation. The caller owns and has already started clock;
// neither operation resets its shared 120-second deadline.
namespace ComponentTranslationEngine {
bool resolveSummary(const QString& path, const QString& locale,
                    const std::atomic_bool& cancelled, const QElapsedTimer& clock,
                    QString& url, QString& error);
bool translate(const QString& jsonPath, const QString& tsPath,
               const std::atomic_bool& cancelled, const QElapsedTimer& clock,
               QString& outputPath, QString& error);
}
