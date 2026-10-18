/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QMap>
#include <QXmlStreamReader>
#include <QLoggingCategory>
#include <QMutex>
#include <QDir>
#include <QJsonObject>
#include <QElapsedTimer>
#include <QPointer>
#include <QVariantList>
#include <QVariantMap>
#include <QStringList>
#include <functional>
#include <memory>

#include "FactSystem.h"
#include "MAVLinkProtocol.h"
#include "AutoPilotPlugin.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"
#include "OfflineParameterFile.h"

Q_DECLARE_LOGGING_CATEGORY(ParameterManagerVerbose1Log)
Q_DECLARE_LOGGING_CATEGORY(ParameterManagerVerbose2Log)
Q_DECLARE_LOGGING_CATEGORY(ParameterManagerDebugCacheFailureLog)

class ParameterEditorController;

class ParameterManager : public QObject
{
    Q_OBJECT

    friend class ParameterEditorController;
    friend class Vehicle; // Shared native Fleet read-only transport gate

public:
    /// @param uas Uas which this set of facts is associated with
    ParameterManager(Vehicle* vehicle);
    ~ParameterManager() override;

    Q_PROPERTY(bool     parametersReady     READ parametersReady    NOTIFY parametersReadyChanged)      ///< true: Parameters are ready for use
    Q_PROPERTY(bool     missingParameters   READ missingParameters  NOTIFY missingParametersChanged)    ///< true: Parameters are missing from firmware response, false: all parameters received from firmware
    Q_PROPERTY(double   loadProgress        READ loadProgress       NOTIFY loadProgressChanged)
    Q_PROPERTY(bool     pendingWrites       READ pendingWrites      NOTIFY pendingWritesChanged)        ///< true: There are still pending write updates against the vehicle
    Q_PROPERTY(QVariantMap parameterImportState READ parameterImportState NOTIFY parameterImportChanged)

    bool parametersReady    (void) const { return _parametersReady; }
    bool missingParameters  (void) const { return _missingParameters; }
    double loadProgress     (void) const { return _loadProgress; }

    /// @return Directory of parameter caches
    static QDir parameterCacheDir();

    /// @return Location of parameter cache file
    static QString parameterCacheFile(int vehicleId, int componentId);

    // Actual receive-link context is required for live observation provenance.
    // Legacy callers without it remain functional but cannot assert live data.
    struct ParameterImportReceipt; // Native opaque earliest-RAW ticket; never a caller report map.
    void mavlinkMessageReceived(mavlink_message_t message, LinkInterface* sourceLink = nullptr,
                               const std::shared_ptr<ParameterImportReceipt>& importReceipt = {});
    QVariantMap beginParameterImport(const QVector<OfflineParameterFile::Row>& rows,
                                    LinkInterface* expectedPrimary, const WeakLinkInterfacePtr& expectedPrimaryOwner,
                                    std::function<bool()> admissionFence);
    QVariantMap parameterImportState() const;
    Q_INVOKABLE QVariantMap parameterImportStatus(const QString& runId) const;
    Q_INVOKABLE bool cancelParameterImport(const QString& runId);

    /// Native C++ read-only observation of 12 legacy safety parameters, two
    /// fixed XY/Z rate guards, fourteen action-phase parameters and the fixed
    /// multicopter altitude-arrival radius. The legacy
    /// profile content/hash and its exact twelve-row API remain unchanged.
    /// Never requests a parameter, returns a Fact, writes, or displays a popup.
    /// Freshness uses monotonic receipt age, not cache time or wall clock.
    QVariantMap parameterObservation(int componentId, const QString& paramName,
                                     qint64 maximumAgeMs = 30000) const;
    bool parameterObservationSourceMatches(int componentId, const QString& fixedName,
                                           LinkInterface* expectedLink) const;
    // Callback-free, native-only final fence for all twelve stable diagnostic
    // rows. Does not call Link virtuals, lock a weak link, request or write.
    bool parameterObservationsMemoryMatch(int componentId, LinkInterface* expectedPrimary,
                                           const WeakLinkInterfacePtr& expectedPrimaryOwner,
                                           const QVariantList& expectedStableRows,
                                           qint64 maximumAgeMs = 30000) const;
    static const QStringList& executionRateGuardParameterNames();
    // One callback-free final fence for the original twelve rows plus both
    // guards. Each list uses its own fixed sorted order; no caller subset.
    bool parameterExecutionObservationsMemoryMatch(int componentId, LinkInterface* expectedPrimary,
                                           const WeakLinkInterfacePtr& expectedPrimaryOwner,
                                           const QVariantList& legacyStableRows,
                                           const QVariantList& rateGuardStableRows,
                                           qint64 maximumAgeMs = 30000) const;
    static const QStringList& executionActionPhaseParameterNames();
    // The complete fixed 12 + 2 + 14 set in one callback-free memory fence.
    // Each set retains its own fixed sorted order; a caller subset is invalid.
    bool parameterActionExecutionObservationsMemoryMatch(int componentId, LinkInterface* expectedPrimary,
                                           const WeakLinkInterfacePtr& expectedPrimaryOwner,
                                           const QVariantList& legacyStableRows,
                                           const QVariantList& rateGuardStableRows,
                                           const QVariantList& phaseStableRows,
                                           qint64 maximumAgeMs = 30000) const;
    static const QStringList& executionArrivalParameterNames();
    bool parameterArrivalExecutionObservationsMemoryMatch(int componentId, LinkInterface* expectedPrimary,
                                           const WeakLinkInterfacePtr& expectedPrimaryOwner,
                                           const QVariantList& legacyStableRows,
                                           const QVariantList& rateGuardStableRows,
                                           const QVariantList& phaseStableRows,
                                           const QVariantList& arrivalStableRows,
                                           qint64 maximumAgeMs = 30000) const;
    // Pure bounded transport codec, not source authentication. Exact six-key
    // envelope, fixed 12/2/14/1 stable rows. Restores integer metadata and the
    // explicitly bound takeoff-ramp negative zero; distinct outputs, all empty
    // on failure. The legacy memory-check contracts are not widened.
    static bool executionParameterSnapshotRows(const QVariantMap& snapshot,
                                           QVariantList* legacyRows, QVariantList* rateRows,
                                           QVariantList* phaseRows, QVariantList* arrivalRows);
    bool parameterArrivalExecutionSnapshotMemoryMatch(int componentId, LinkInterface* expectedPrimary,
                                           const WeakLinkInterfacePtr& expectedPrimaryOwner,
                                           const QVariantMap& snapshot,
                                           qint64 maximumAgeMs = 30000) const;
    QVariantMap requestFleetParameterObservation(int componentId, const QString& fixedName,
                                                 LinkInterface* expectedLink, std::function<bool()> authorizationFence);

    QList<int> componentIds(void);

    /// Re-request the full set of parameters from the autopilot
    void refreshAllParameters(uint8_t componentID = MAV_COMP_ID_ALL);

    /// Request a refresh on the specific parameter
    void refreshParameter(int componentId, const QString& paramName);

    /// Request a refresh on all parameters that begin with the specified prefix
    void refreshParametersPrefix(int componentId, const QString& namePrefix);

    void resetAllParametersToDefaults();
    void resetAllToVehicleConfiguration();

    /// Returns true if the specifed parameter exists
    ///     @param componentId: Component id or FactSystem::defaultComponentId
    ///     @param name: Parameter name
    bool parameterExists(int componentId, const QString& paramName);

    /// Returns all parameter names
    QStringList parameterNames(int componentId);

    /// Returns the specified Parameter. Returns a default empty fact is parameter does not exists. Also will pop
    /// a missing parameter error to user if parameter does not exist.
    ///     @param componentId: Component id or FactSystem::defaultComponentId
    ///     @param name: Parameter name
    Fact* getParameter(int componentId, const QString& paramName);

    /// Returns error messages from loading
    QString readParametersFromStream(QTextStream& stream);

    void writeParametersToStream(QTextStream& stream);

    bool pendingWrites(void);

    Vehicle* vehicle(void) { return _vehicle; }

    static MAV_PARAM_TYPE               factTypeToMavType(FactMetaData::ValueType_t factType);
    static FactMetaData::ValueType_t    mavTypeToFactType(MAV_PARAM_TYPE mavType);

signals:
    void parametersReadyChanged     (bool parametersReady);
    void missingParametersChanged   (bool missingParameters);
    void loadProgressChanged        (float value);
    void pendingWritesChanged       (bool pendingWrites);
    void factAdded                  (int componentId, Fact* fact);
    void parameterImportChanged();

private slots:
    void    _factRawValueUpdated                (const QVariant& rawValue);

private:
    void    _handleParamValue                   (int componentId, QString parameterName, int parameterCount, int parameterIndex, MAV_PARAM_TYPE mavParamType, QVariant parameterValue, bool liveTelemetry = false, LinkInterface* sourceLink = nullptr, const std::function<bool()>& cacheFence = {});
    void    _factRawValueUpdateWorker           (int componentId, const QString& name, FactMetaData::ValueType_t valueType, const QVariant& rawValue);
    void    _waitingParamTimeout                (void);
    void    _tryCacheLookup                     (void);
    void    _initialRequestTimeout              (void);
    int     _actualComponentId                  (int componentId);
    void    _readParameterRaw                   (int componentId, const QString& paramName, int paramIndex);
    bool    _sendParamSetToVehicle              (int componentId, const QString& paramName, FactMetaData::ValueType_t valueType, const QVariant& value);
    void    _writeLocalParamCache               (int vehicleId, int componentId);
    void    _tryCacheHashLoad                   (int vehicleId, int componentId, quint32 hash, int expectedCount, LinkInterface* originalReceived);
    struct CacheSource;
    std::shared_ptr<CacheSource> _captureCacheSource(int vehicleId, int componentId, LinkInterface* originalReceived = nullptr);
    bool    _cacheSourceMemoryCurrent           (const std::shared_ptr<CacheSource>& source) const;
    bool    _cacheSourceCurrent                 (const std::shared_ptr<CacheSource>& source) const;
    QPointer<Fact> _cacheFact                   (int componentId, const QString& name) const;
    void    _retireCacheAttempt                 ();
    struct ImportRun;
    struct ImportSource;
    std::shared_ptr<ImportSource> _captureImportSource(LinkInterface* expectedPrimary, const WeakLinkInterfacePtr& expectedOwner);
    bool _importSourceMemoryCurrent(const std::shared_ptr<ImportSource>& source) const;
    bool _importSourceCurrent(const std::shared_ptr<ImportSource>& source) const;
    bool _importRowCurrent(const std::shared_ptr<ImportRun>& run, int index, bool checkBaseline) const;
    int _parameterImportPendingCount() const;
    void _driveParameterImport();
    void _retireParameterImport(const QString& reason);
    void _supersedeParameterImport(int componentId, const QString& name);
    void _queueParameterImportChanged();
    std::shared_ptr<ParameterImportReceipt> _captureParameterImportReceipt(const mavlink_message_t& original, LinkInterface* sourceLink) const;
    void _consumeParameterImportReceipt(const std::shared_ptr<ParameterImportReceipt>& receipt);
    void    _loadMetaData                       (void);
    void    _clearMetaData                      (void);
    QString _remapParamNameToVersion            (const QString& paramName);
    void    _loadOfflineEditingParams           (void);
    QString _logVehiclePrefix                   (int componentId);
    void    _setLoadProgress                    (double loadProgress);
    bool    _fillIndexBatchQueue                (bool waitingParamTimeout);
    void    _updateProgressBar                  (void);
    void    _checkInitialLoadComplete           (const std::function<bool()>& cacheFence = {});
    void    _ftpDownloadComplete                (const QString& fileName, const QString& errorMsg);
    void    _ftpDownloadProgress                (float progress);
    bool    _parseParamFile                     (const QString& filename);
    bool    _parameterWriteAllowed              (QString& reason) const;
    bool    _parameterObservationContextAllowed() const;
    bool    _parameterObservationMemoryContextAllowed() const;
    bool    _parameterObservationPendingWrites() const;
    bool    _parameterObservationRowsMemoryMatch(int componentId, LinkInterface* expectedPrimary,
                                          const WeakLinkInterfacePtr& expectedPrimaryOwner,
                                          const QVariantList& legacyStableRows,
                                          const QVariantList& rateGuardStableRows,
                                          const QVariantList& phaseStableRows,
                                          const QVariantList& arrivalStableRows,
                                          bool includeRateGuards, bool includeActionPhase, bool includeArrival,
                                          qint64 maximumAgeMs) const;
    void    _invalidateParameterObservations(const QString& source);
    void    _invalidateParameterObservation(int componentId, const QString& name, const QString& source);
    void    _watchParameterObservationFact(int componentId, const QString& name, Fact* fact, const std::function<bool()>& cacheFence = {});
    quint64 _prepareParameterObservation(int componentId, const QString& name);
    bool    _parameterObservationUpdateCurrent(const QString& name, quint64 token, const QString& epoch) const;
    void    _completeParameterObservation(int componentId, const QString& name, quint64 token,
                                          const QString& epoch, const QVariant& wireValue,
                                          MAV_PARAM_TYPE wireType, bool liveTelemetry,
                                          bool pendingAtReceipt, LinkInterface* sourceLink,
                                          qint64 receivedAtMs, Fact* fact);

    static QVariant _stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool failOk = false);

    Vehicle*            _vehicle;
    MAVLinkProtocol*    _mavlink;

    QMap<int /* comp id */, QMap<QString /* parameter name */, Fact*>> _mapCompId2FactMap;

    double      _loadProgress;                  ///< Parameter load progess, [0.0,1.0]
    bool        _parametersReady;               ///< true: parameter load complete
    bool        _missingParameters;             ///< true: parameter missing from initial load
    bool        _initialLoadComplete;           ///< true: Initial load of all parameters complete, whether successful or not
    bool        _waitingForDefaultComponent;    ///< true: last chance wait for default component params
    bool        _saveRequired;                  ///< true: _saveToEEPROM should be called
    bool        _metaDataAddedToFacts;          ///< true: FactMetaData has been adde to the default component facts
    bool        _logReplay;                     ///< true: running with log replay link

    typedef QPair<int /* FactMetaData::ValueType_t */, QVariant /* Fact::rawValue */> ParamTypeVal;
    typedef QMap<QString /* parameter name */, ParamTypeVal> CacheMapName2ParamTypeVal;

    QMap<int /* component id */, bool>                                              _debugCacheCRC; ///< true: debug cache crc failure
    QMap<int /* component id */, CacheMapName2ParamTypeVal>                         _debugCacheMap;
    QMap<int /* component id */, QMap<QString /* param name */, bool /* seen */>>   _debugCacheParamSeen;

    // Wait counts from previous parameter update cycle
    int _prevWaitingReadParamIndexCount;
    int _prevWaitingReadParamNameCount;
    int _prevWaitingWriteParamNameCount;

    bool _readParamIndexProgressActive =    false;
    bool _readParamNameProgressActive =     false;
    bool _writeParamProgressActive =        false;

    static const int    _maxInitialRequestListRetry = 4;        ///< Maximum retries for request list
    int                 _initialRequestRetryCount;              ///< Current retry count for request list
    static const int    _maxInitialLoadRetrySingleParam = 5;    ///< Maximum retries for initial index based load of a single param
    static const int    _maxReadWriteRetry = 5;                 ///< Maximum retries read/write
    bool                _disableAllRetries;                     ///< true: Don't retry any requests (used for testing)

    bool        _indexBatchQueueActive; ///< true: we are actively batching re-requests for missing index base params, false: index based re-request has not yet started
    QList<int>  _indexBatchQueue;       ///< The current queue of index re-requests

    QMap<int, int>                  _paramCountMap;             ///< Key: Component id, Value: count of parameters in this component
    QMap<int, QMap<int, int> >      _waitingReadParamIndexMap;  ///< Key: Component id, Value: Map { Key: parameter index still waiting for, Value: retry count }
    QMap<int, QMap<QString, int> >  _waitingReadParamNameMap;   ///< Key: Component id, Value: Map { Key: parameter name still waiting for, Value: retry count }
    QMap<int, QMap<QString, int> >  _waitingWriteParamNameMap;  ///< Key: Component id, Value: Map { Key: parameter name still waiting for, Value: retry count }
    QMap<int, QList<int> >          _failedReadParamIndexMap;   ///< Key: Component id, Value: failed parameter index

    int _totalParamCount;                       ///< Number of parameters across all components
    int _waitingWriteParamBatchCount = 0;       ///< Number of parameters which are batched up waiting on write responses
    int _waitingReadParamNameBatchCount = 0;    ///< Number of parameters which are batched up waiting on read responses

    QTimer _initialRequestTimeoutTimer;
    QTimer _waitingParamTimeoutTimer;
    QTimer _parameterImportTimer; // Independent bounded import retries; never stops the legacy read timer.
    std::shared_ptr<ImportRun> _importRun;
    bool _importEntryBusy = false;
    bool _importDriveBusy = false;
    bool _importNotifyQueued = false;
    bool _importDestroying = false;

    Fact _defaultFact;   ///< Used to return default fact, when parameter not found

    /* MavFTP */
    bool               _tryftp;
    bool               _offlineLoadBusy = false; // Constructor-only staged load; never a public reload/authority gate.
    bool               _cacheLoadBusy = false;
    bool               _cacheWriteBusy = false;
    quint64            _cacheActivitySerial = 1;
    quint64            _cacheLoadSerial = 0;
    bool               _cacheActivityExhausted = false;
    int                _cacheLoadComponent = 0;
    QMap<QString, QPointer<Fact>> _cacheLoadFacts; // Original native pins, not a caller table.

    struct ParameterObservation {
        QVariant value;
        int componentId = -1;
        int mavParamType = -1;
        QString source = QStringLiteral("unknown");
        QPointer<LinkInterface> sourceLink;
        QPointer<Fact> fact;
        QPointer<Fact> hookedFact;
        QMetaObject::Connection valueConnection;
        QMetaObject::Connection destroyedConnection;
        qint64 receivedAtMs = -1;
        quint64 contentRevision = 0;
        quint64 updateToken = 0;
        quint64 receiptSequence = 0; // Last successful live RAW commit, never prepare/cache time.
        bool observed = false;
        bool available = false;
        bool updating = false;
        bool pendingAtReceipt = false;
    };
    // Keyed only by the fixed watched names, only for the default autopilot
    // component. Never allocate state for arbitrary caller components/names.
    QMap<QString, ParameterObservation> _parameterObservations;
    QElapsedTimer _parameterObservationClock;
    QString _parameterObservationEpoch;
    quint64 _parameterObservationRevision = 0;
    quint64 _parameterObservationUpdateToken = 0;
    bool _parameterObservationExhausted = false;
    QMap<QString, qint64> _fleetParameterReadLastAtMs; // Fixed 12 + 2 + 14 + 1 names only.
    qint64 _fleetParameterReadLastAnyAtMs = -1;
};
