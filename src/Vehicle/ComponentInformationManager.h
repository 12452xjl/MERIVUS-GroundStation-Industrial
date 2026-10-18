/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCLoggingCategory.h"
#include "QGCMAVLink.h"
#include "StateMachine.h"
#include "ComponentInformationCache.h"
#include "ComponentInformationTranslation.h"

#include <QPointer>

#include <memory>

Q_DECLARE_LOGGING_CATEGORY(ComponentInformationManagerLog)

class Vehicle;
class ComponentInformationManager;
class CompInfo;
class CompInfoParam;
class CompInfoGeneral;
struct ComponentMetadataArtifact;
class ComponentMetadataArtifactWorker;
struct ComponentInformationRequestSource;
struct ComponentMetadataFileRequest;
struct ComponentMetadataTranslationRequest;

class RequestMetaDataTypeStateMachine : public StateMachine
{
    Q_OBJECT

public:
    RequestMetaDataTypeStateMachine(ComponentInformationManager* compMgr);
    ~RequestMetaDataTypeStateMachine() override;

    void        request     (CompInfo* compInfo);
    QString     typeToString(void);
    CompInfo*   compInfo    (void);
    void advance() override;

    // Overrides from StateMachine
    int             stateCount      (void) const final;
    const StateFn*  rgStates        (void) const final;
    void            statesCompleted (void) const final;

private:
    static void _stateRequestCompInfo           (StateMachine* stateMachine);
    static void _stateRequestCompInfoDeprecated (StateMachine* stateMachine);
    static void _stateRequestMetaDataJson       (StateMachine* stateMachine);
    static void _stateRequestMetaDataJsonFallback(StateMachine* stateMachine);
    static void _stateRequestTranslationJson    (StateMachine* stateMachine);
    static void _stateRequestTranslate          (StateMachine* stateMachine);
    static void _stateRequestComplete           (StateMachine* stateMachine);
    static bool _uriIsMAVLinkFTP                (const QString& uri);

    bool _current() const;
    bool _idle() const;
    void _retire();
    void _requestMessage(bool deprecated);
    void _requestFile(const QString& cacheFileTag, bool crcValid, quint32 expectedCrc,
                      const QString& uri, bool translation);
    bool _fileCurrent(const std::shared_ptr<ComponentMetadataFileRequest>& request) const;
    void _downloadFile(const std::shared_ptr<ComponentMetadataFileRequest>& request);
    void _downloadComplete(const std::shared_ptr<ComponentMetadataFileRequest>& request,
                           const QString& file, const QString& error);
    void _startArtifact(const std::shared_ptr<ComponentMetadataFileRequest>& request,
                        const QString& file, bool cacheHit);
    void _finishArtifact(const std::shared_ptr<ComponentMetadataFileRequest>& request);

    QPointer<ComponentInformationManager> _compMgr;
    QPointer<CompInfo>               _compInfo;
    std::shared_ptr<ComponentInformationRequestSource> _source;
    quint64                         _serial = 0;
    bool                            _serialExhausted = false;
    QPointer<QObject>               _messageOwner;
    std::shared_ptr<ComponentMetadataFileRequest> _file;
    bool                            _transportBusy = false;
    bool                            _translationBusy = false;
    std::shared_ptr<ComponentMetadataTranslationRequest> _translationRequest;
    QMetaObject::Connection         _translationConnection;
    std::shared_ptr<const ComponentMetadataArtifact> _metadataArtifact;
    std::shared_ptr<const ComponentMetadataArtifact> _translationArtifact;
    QString                         _jsonMetadataFileName;
    QString                         _jsonMetadataTranslatedFileName;
    QString                         _jsonTranslationFileName;

    static const StateFn  _rgStates[];
    static const int      _cStates;

    friend class ComponentInformationManager;
};

class ComponentInformationManager : public StateMachine
{
    Q_OBJECT

public:
    static constexpr int cachedFileMaxAgeSec = 3 * 24 * 3600; ///< 3 days

    ComponentInformationManager(Vehicle* vehicle);
    ~ComponentInformationManager() override;

    typedef void (*RequestAllCompleteFn)(void* requestAllCompleteFnData);

    void                requestAllComponentInformation  (RequestAllCompleteFn requestAllCompletFn, void * requestAllCompleteFnData, QObject* callbackOwner = nullptr);
    Vehicle*            vehicle                         (void);
    CompInfoParam*      compInfoParam                   (uint8_t compId);
    CompInfoGeneral*    compInfoGeneral                 (uint8_t compId);

    // Overrides from StateMachine
    int             stateCount  (void) const final;
    const StateFn*  rgStates    (void) const final;

    ComponentInformationCache& fileCache() { return _fileCache; }
    ComponentInformationTranslation* translation() { return _translation; }

    float progress() const;

    void advance() override;

signals:
    void progressUpdate(float progress);

private:
    bool _sourceCurrent(const std::shared_ptr<ComponentInformationRequestSource>& source) const;
    bool _resetParameterMetadata(const std::shared_ptr<ComponentInformationRequestSource>& source);
    void _retireSource(const std::shared_ptr<ComponentInformationRequestSource>& source);
    void _stateRequestCompInfoComplete  (void);
    bool _isCompTypeSupported           (COMP_METADATA_TYPE type);
    void _updateAllUri                  ();

    static QString _getFileCacheTag(int compInfoType, uint32_t crc, bool isTranslation);

    static void _stateRequestCompInfoGeneral        (StateMachine* stateMachine);
    static void _stateRequestCompInfoGeneralComplete(StateMachine* stateMachine);
    static void _stateRequestCompInfoParam          (StateMachine* stateMachine);
    static void _stateRequestCompInfoEvents         (StateMachine* stateMachine);
    static void _stateRequestCompInfoActuators      (StateMachine* stateMachine);
    static void _stateRequestAllCompInfoComplete    (StateMachine* stateMachine);

    QPointer<Vehicle>                _vehicle;
    std::shared_ptr<ComponentInformationRequestSource> _source;
    quint64                         _serial = 0;
    bool                            _serialExhausted = false;
    bool                            _entryBusy = false;
    bool                            _destroying = false;
    RequestMetaDataTypeStateMachine _requestTypeStateMachine;
    RequestAllCompleteFn            _requestAllCompleteFn       = nullptr;
    void*                           _requestAllCompleteFnData   = nullptr;
    QPointer<QObject>               _requestAllCompleteOwner;
    bool                            _requestAllCompleteOwnerRequired = false;
    QPointer<QGCCachedFileDownload> _cachedFileDownload;
    ComponentInformationCache&      _fileCache;
    QPointer<ComponentInformationTranslation> _translation;

    QMap<uint8_t /* compId */, QMap<COMP_METADATA_TYPE, QPointer<CompInfo>>> _compInfoMap;

    static const StateFn                  _rgStates[];
    static const int                      _cStates;

    friend class RequestMetaDataTypeStateMachine;
};
