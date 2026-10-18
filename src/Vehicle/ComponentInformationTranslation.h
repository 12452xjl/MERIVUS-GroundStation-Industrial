/****************************************************************************
 * (c) 2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 * Licensed under the terms in COPYING.md.
 ****************************************************************************/
#pragma once

#include "QGCLoggingCategory.h"
#include "QGCCachedFileDownload.h"
#include "ComponentMetadataArtifactWorker.h"

#include <QLoggingCategory>
#include <QPointer>
#include <memory>

Q_DECLARE_LOGGING_CATEGORY(ComponentInformationTranslationLog)
struct ComponentTranslationRequest;

class ComponentInformationTranslation : public QObject
{
    Q_OBJECT
public:
    ComponentInformationTranslation(QObject* parent, QGCCachedFileDownload* cachedFileDownload);
    ~ComponentInformationTranslation() override;

    // Compatibility entry: paths are borrowed, never removed. true now means
    // handling accepted; summary errors may be reported asynchronously.
    bool downloadAndTranslate(const QString& summaryJsonFile, const QString& toTranslateJsonFile, int maxCacheAgeSec);

    // Production entry. The actual workers independently retain all input
    // owners. Native completions never use the legacy two-string signal.
    bool downloadAndTranslateArtifacts(const QString& requestId, const ComponentMetadataArtifactPtr& summary,
                                      const ComponentMetadataArtifactPtr& metadata, int maxCacheAgeSec);
    void cancelArtifacts(const QString& requestId, const ComponentMetadataArtifactPtr& summary,
                         const ComponentMetadataArtifactPtr& metadata);

    // Explicit synchronous compatibility entry, including borrowed qrc paths.
    // A successful unique output is owned by its caller.
    QString translateJsonUsingTS(const QString& toTranslateJsonFile, const QString& tsFile);

signals:
    void downloadComplete(QString translatedJsonTempFile, QString errorMsg);
    void artifactTranslationComplete(QString requestId, QString translatedJsonTempFile, QString errorMsg);

private:
    bool _begin(const std::shared_ptr<ComponentTranslationRequest>& request);
    bool _current(const std::shared_ptr<ComponentTranslationRequest>& request) const;
    void _startWorker(const std::shared_ptr<ComponentTranslationRequest>& request, bool summaryPhase);
    void _finishWorker(const std::shared_ptr<ComponentTranslationRequest>& request);
    void _startDownload(const std::shared_ptr<ComponentTranslationRequest>& request, const QString& url);
    void _downloadComplete(const std::shared_ptr<ComponentTranslationRequest>& request, QString file, QString error);
    void _cancel(const std::shared_ptr<ComponentTranslationRequest>& request);
    void _finish(const std::shared_ptr<ComponentTranslationRequest>& request, QString output, QString error);

    QPointer<QGCCachedFileDownload> _cachedFileDownload;
    std::shared_ptr<ComponentTranslationRequest> _request;
    quint64 _generation = 0;
    bool _busy = false;
    bool _translating = false;
    bool _destroying = false;
};
