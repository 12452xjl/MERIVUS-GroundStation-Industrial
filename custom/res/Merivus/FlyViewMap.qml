/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                    2.11
import QtQuick.Controls             2.4
import QtLocation                   5.3
import QtPositioning                5.3
import QtQuick.Layouts              1.11

import QGroundControl               1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import Merivus                     1.0

FlightMap {
    id:                         _root

    // Share the narrow command facade owned by GuidedActionsController. A
    // second SwarmController here would create a separate watchdog/session
    // state and make the map disagree with the command center; exposing the
    // raw backend would also leak unrelated command and registry mutators.
    readonly property var swarmController: globals.guidedControllerFlyView
                                             ? globals.guidedControllerFlyView.swarmController
                                             : null

    // Shared fleet state is exposed by the controller so overlays can render
    // health and capability summaries without reaching into QGC internals.
    readonly property var fleetRegistry: swarmController ? swarmController.fleetRegistry : null
    readonly property var fleetHealthSummary: fleetRegistry ? fleetRegistry.fleetHealthSummary : ({})
    readonly property var missionTaskSummaries: globals.guidedControllerFlyView
                                                       ? globals.guidedControllerFlyView.missionTaskSummaries
                                                       : []
    readonly property var formationPlanner: globals.guidedControllerFlyView
                                                    ? globals.guidedControllerFlyView.formationPlanner
                                                    : null
    readonly property var faultToleranceManager: globals.guidedControllerFlyView
                                                        ? globals.guidedControllerFlyView.faultToleranceManager
                                                        : null

    ReviewRangeController {
        id: reviewController
        vehicle: _root._activeVehicle
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    // ============================================================================
    // 区块 1：基础设置与状态管理 (画中画与基础属性)
    // ============================================================================
    allowGCSLocationCenter:     !_missionPlannerOpen
    allowVehicleLocationCenter: !_missionPlannerOpen && !_keepVehicleCentered
    planView:                   false
    zoomLevel:                  QGroundControl.flightMapZoom
    center:                     QGroundControl.flightMapPosition
    tilt:                       reviewController.inspecting ? 50 : 0

    // 画中画 (PiP) 状态管理
    property Item pipState: _pipState
    QGCPipState {
        id:         _pipState
        pipOverlay: _pipOverlay
        isDark:     _isFullWindowItemDark
    }

    property var    rightPanelWidth
    property var    planMasterController
    property bool   pipMode:                false
    property var    toolInsets
    property alias  videoDockTarget:        commandCenterOverlay.videoDockTarget

    // Explicit planning mode is separate from the legacy manual Goto/queue.
    // A press keeps its interaction epoch through release: closing the editor
    // while a pointer is held cannot reinterpret that press as a flight click.
    property bool _missionPlannerOpen: false
    property bool _missionMapPicking: false
    property bool _missionMapReviewOnly: false
    property double _missionPlanningEpoch: 0
    property bool _missionPlanningEpochExhausted: false
    readonly property var _missionDraftController: globals.guidedControllerFlyView
    readonly property var _historyGnssArchive: _missionDraftController ? _missionDraftController.telemetryArchive : null
    readonly property var _historyGnssSelection: _historyGnssArchive ? _historyGnssArchive.gnssMap || ({}) : ({})
    readonly property string _historyGnssKey: _historyGnssSelection.active === true
        ? JSON.stringify([_historyGnssSelection.selectionToken, _historyGnssSelection.reviewToken, _historyGnssSelection.origin]) : ""
    property var _historyGnssLife: ({alive: true, owner: null, token: ""})
    readonly property var _historyGnssGeometry: _historyGnssGeometryFor(_historyGnssSelection)
    on_HistoryGnssKeyChanged: _syncHistoryGnssLayer()
    on_HistoryGnssArchiveChanged: _syncHistoryGnssLayer()
    readonly property var _missionDraft: _missionDraftController
                                         ? _missionDraftController.missionDraft : null
    readonly property var _missionRemainderReview: _missionDraftController
                            ? _missionDraftController.fleetMissionRemainderReview || ({}) : ({})
    readonly property bool _missionRemainderSelected: _missionRemainderReview.accepted === true
    readonly property var _missionRemainderGeometry: _missionRemainderGeometryFor(_missionRemainderReview)
    readonly property var _startupSpatialReview: _missionDraftController
                            ? _missionDraftController.fleetStartupSpatialReview || ({}) : ({})
    readonly property bool _startupSpatialSelected: _startupSpatialReview.mapSelected === true
    readonly property string _startupSpatialSelectionKey: _startupSpatialSelected
                            ? JSON.stringify([_startupSpatialReview.captureToken, _startupSpatialReview.sourceToken, _startupSpatialReview.binding]) : ""
    property var _startupMapRestore: null
    property bool _startupMapCallBusy: false
    property string _startupMapNotice: ""
    property var _startupMapLifetime: ({alive:true, ready:false, ticket:null, syncTicket:null})
    readonly property bool _startupMapActive: _startupMapRestore !== null
    readonly property var _startupMapGeometry: _startupSpatialGeometryFor(_startupSpatialReview)
    readonly property var _startupMissionReview: _missionMapObject(_startupSpatialReview.missionReview) ? _startupSpatialReview.missionReview : ({})
    readonly property string _startupMissionPlanId: _missionMapObject(_startupSpatialReview.binding) && typeof _startupSpatialReview.binding.planId === "string" ? _startupSpatialReview.binding.planId : ""
    readonly property var _startupMissionGeometry: _missionSpatialGeometryFor(_startupMissionReview,_startupMissionPlanId)
    property int _startupSpatialLayer: 0
    readonly property bool _showStartupReferenceLayer: _startupSpatialLayer === 0 || _startupSpatialLayer === 1
    readonly property bool _showStartupMissionRoutes: _startupSpatialLayer === 0 || _startupSpatialLayer === 2
    readonly property bool _showStartupActionRequests: _startupSpatialLayer === 0 || _startupSpatialLayer === 3
    readonly property var _startupSpatialCenter: _showStartupReferenceLayer && _startupMapGeometry.available ? _startupMapGeometry.segments[0].path[0]
        : _showStartupMissionRoutes && _startupMissionGeometry.routes.length ? _startupMissionGeometry.routes[0].path[0]
        : _showStartupActionRequests && _startupMissionGeometry.markers.length ? _startupMissionGeometry.markers[0].coordinate : null
    readonly property var _missionAreaPath: !_startupSpatialSelected && !_startupMapActive && !_missionRemainderSelected && !_missionMapReviewOnly && _missionDraft
                                             ? _missionGeoPath(_missionDraft.vertices, 64) : []
    readonly property var _missionExclusionPaths: _missionAreaPath.length >= 3 && _missionDraft
                  ? _missionExclusionsFor(_missionDraft.settings) : []
    readonly property string _missionBoundaryPolicyText: {
        var settings = _missionDraft ? _missionDraft.settings : null
        if (!settings || settings.boundarySetbackMeters === undefined) return ""
        var distance = settings.boundarySetbackMeters
        if (typeof distance !== "number" || !isFinite(distance) || distance < 0 || distance > 500)
            return qsTr("边界退让声明不可用；图形不代表有效规划域。")
        return qsTr("原输入边界 · 名义退让 %1 m\n未绘制退让后的可用域；不是净空或飞行许可。").arg(distance)
    }
    readonly property var _missionProposalRoutes: _startupSpatialSelected || _startupMapActive ? [] : _missionRemainderSelected
                  ? _missionRemainderGeometry.routes : !_missionMapReviewOnly && _missionDraftController
                  && _missionDraftController.missionDraftPreviewCurrent
                  ? _missionRoutesFor(_missionDraftController.missionDraftPreview) : []

    on_StartupSpatialSelectionKeyChanged: _syncStartupSpatialMap()
    on_MissionDraftControllerChanged: _syncStartupSpatialMap()
    Component.onCompleted: { var g = _historyGnssLife; _startupMapLifetime.ready = true; _syncStartupSpatialMap(); if (g.alive) _syncHistoryGnssLayer() }
    Component.onDestruction: {
        _startupMapLifetime.alive = false
        var g = _historyGnssLife, owner = g.owner, token = g.token
        g.alive = false; g.owner = null; g.token = ""
        if (owner && token) { try { owner.clearGnssMap(token) } catch (error) {} }
    }

    function _historyGnssGeometryFor(selection) {
        var rejected = {available: false, points: [], omittedPolar: 0, origin: "", report: null}
        if (!_missionMapKeys(selection, ["active", "selectionToken", "reviewToken", "origin", "report"]) || selection.active !== true
                || ["native-page", "session-snapshot", "file-history"].indexOf(selection.origin) < 0
                || typeof selection.reviewToken !== "string" || !/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(selection.reviewToken)
                || typeof selection.selectionToken !== "string" || selection.selectionToken.indexOf(selection.reviewToken + ":") !== 0) return rejected
        var serial = selection.selectionToken.substring(selection.reviewToken.length + 1)
        if (!/^[1-9][0-9]{0,15}$/.test(serial) || (serial.length === 16 && serial > "9007199254740991")) return rejected
        var report = selection.report
        if (!_missionMapKeys(report, ["schemaVersion", "source", "ok", "reason", "positionValuesConfirmed", "sourceIdentity", "rowCount",
                "gnssSampleCount", "locatedSampleCount", "unlocatedSampleCount", "qualityCautionSampleCount", "qualityUnknownSampleCount",
                "withinThresholdSampleCount", "groupCount", "samples", "displayPolicy", "hasMore", "footerPresent", "partialTail",
                "historicalOnly", "singlePageOnly", "historyComplete", "rawPayloadExposed", "absoluteCoordinatesExposed", "authenticated",
                "crcVerified", "accuracyVerified", "taskOwnershipVerified", "driftDiagnosed", "trajectorySafetyVerified", "executionPermit",
                "flightCommandReleased", "permissionsRestored"]) || report.schemaVersion !== 1
                || report.source !== "merivus-fleet-telemetry-gnss-review" || report.ok !== true || report.reason !== ""
                || report.positionValuesConfirmed !== true || report.historicalOnly !== true || report.singlePageOnly !== true
                || report.absoluteCoordinatesExposed !== true || !_missionMapInteger(report.rowCount, 0, 64)
                || !Array.isArray(report.samples) || report.samples.length > report.rowCount || report.gnssSampleCount !== report.samples.length
                || !_missionMapInteger(report.locatedSampleCount, 1, report.samples.length)
                || report.unlocatedSampleCount !== report.samples.length - report.locatedSampleCount
                || !_missionMapInteger(report.groupCount, 1, report.samples.length)
                || typeof report.hasMore !== "boolean" || typeof report.footerPresent !== "boolean" || typeof report.partialTail !== "boolean") return rejected
        var flags = ["historyComplete", "rawPayloadExposed", "authenticated", "crcVerified", "accuracyVerified", "taskOwnershipVerified",
            "driftDiagnosed", "trajectorySafetyVerified", "executionPermit", "flightCommandReleased", "permissionsRestored"]
        for (var f = 0; f < flags.length; ++f) if (report[flags[f]] !== false) return rejected
        var identity = report.sourceIdentity
        if (!_missionMapKeys(identity, ["queryId", "catalogId", "runId", "headerHash", "fileHash", "afterSequence", "nextSequence"])) return rejected
        var names = Object.keys(identity)
        for (var n = 0; n < names.length; ++n) if (typeof identity[names[n]] !== "string" || !identity[names[n]].length || identity[names[n]].length > 128) return rejected
        if (!/^[0-9a-f]{64}$/.test(identity.fileHash) || !/^[0-9a-f]{64}$/.test(identity.headerHash)
                || !/^[A-Za-z0-9_-]{1,128}$/.test(identity.queryId)
                || !/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(identity.catalogId)
                || !/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(identity.runId)
                || !/^(0|[1-9][0-9]{0,18})$/.test(identity.afterSequence) || !/^(0|[1-9][0-9]{0,18})$/.test(identity.nextSequence)
                || (identity.afterSequence.length === 19 && identity.afterSequence > "9223372036854775807")
                || (identity.nextSequence.length === 19 && identity.nextSequence > "9223372036854775807")
                || identity.nextSequence.length < identity.afterSequence.length
                || (identity.nextSequence.length === identity.afterSequence.length && identity.nextSequence < identity.afterSequence)
                || !_missionMapKeys(report.displayPolicy, ["maximumHdop", "minimumSatellites", "semantics"])
                || report.displayPolicy.maximumHdop !== 2.5 || report.displayPolicy.minimumSatellites !== 6
                || report.displayPolicy.semantics !== "display-heuristic-not-accuracy") return rejected
        var points = [], caution = 0, unknown = 0, within = 0, located = 0, omittedPolar = 0
        var previous = identity.afterSequence, groups = {}, sourceSequences = {}
        for (var i = 0; i < report.samples.length; ++i) {
            var row = report.samples[i]
            if (!_missionMapKeys(row, ["sequence", "sourceEpoch", "sourceSequence", "groupOrdinal", "vehicleId", "componentId", "receivedUtcMs", "fixType",
                    "positionKnown", "coordinate", "positionReason", "hdopKnown", "hdop", "satellitesKnown", "satellitesVisible", "qualityCode", "hdopCaution", "satelliteCaution"])
                    || !_missionMapInteger(row.vehicleId, 1, 255) || !_missionMapInteger(row.componentId, 1, 255)
                    || !_missionMapInteger(row.groupOrdinal, 1, report.groupCount) || !_missionMapInteger(row.fixType, 0, 255)
                    || !_missionMapInteger(row.receivedUtcMs, 1, 9007199254740991) || typeof row.positionKnown !== "boolean"
                    || typeof row.sequence !== "string" || !/^[1-9][0-9]{0,18}$/.test(row.sequence)
                    || (row.sequence.length === 19 && row.sequence > "9223372036854775807")
                    || row.sequence.length < previous.length || (row.sequence.length === previous.length && row.sequence <= previous)
                    || row.sequence.length > identity.nextSequence.length || (row.sequence.length === identity.nextSequence.length && row.sequence > identity.nextSequence)
                    || typeof row.sourceEpoch !== "string" || !/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(row.sourceEpoch)
                    || typeof row.sourceSequence !== "string" || !/^[1-9][0-9]{0,19}$/.test(row.sourceSequence)
                    || (row.sourceSequence.length === 20 && row.sourceSequence > "18446744073709551615")
                    || typeof row.hdopKnown !== "boolean" || typeof row.satellitesKnown !== "boolean"
                    || typeof row.hdop !== "number" || !isFinite(row.hdop)
                    || (row.hdopKnown ? row.hdop < 0 || row.hdop > 655.34 : row.hdop !== -1)
                    || (row.satellitesKnown ? !_missionMapInteger(row.satellitesVisible, 0, 254) : row.satellitesVisible !== -1)
                    || row.hdopCaution !== (row.hdopKnown && row.hdop > 2.5)
                    || row.satelliteCaution !== (row.satellitesKnown && row.satellitesVisible < 6)) return rejected
            previous = row.sequence
            var groupKey = JSON.stringify([row.sourceEpoch, row.vehicleId, row.componentId])
            if (groups[groupKey] === undefined) groups[groupKey] = Object.keys(groups).length + 1
            if (row.groupOrdinal !== groups[groupKey]) return rejected
            var priorSource = sourceSequences[groupKey]
            if (priorSource !== undefined && (row.sourceSequence.length < priorSource.length
                    || (row.sourceSequence.length === priorSource.length && row.sourceSequence <= priorSource))) return rejected
            sourceSequences[groupKey] = row.sourceSequence
            if (!row.positionKnown) {
                var reason = row.fixType === 0 ? "no-gps" : row.fixType === 1 ? "no-fix" : row.fixType === 2 ? "fix-2d-not-used"
                    : row.fixType === 7 ? "static-fix-not-used" : [3, 4, 5, 6, 8].indexOf(row.fixType) >= 0 ? "coordinates-out-of-range" : "unknown-fix"
                if (!_missionMapKeys(row.coordinate, []) || row.qualityCode !== "unlocated" || row.positionReason !== reason) return rejected
                continue
            }
            if ([3, 4, 5, 6, 8].indexOf(row.fixType) < 0 || row.positionReason !== "reported-position"
                    || !_missionMapKeys(row.coordinate, ["latitude", "longitude"]) || typeof row.coordinate.latitude !== "number"
                    || typeof row.coordinate.longitude !== "number" || !isFinite(row.coordinate.latitude) || !isFinite(row.coordinate.longitude)
                    || row.coordinate.latitude < -90 || row.coordinate.latitude > 90 || row.coordinate.longitude < -180 || row.coordinate.longitude > 180) return rejected
            var quality = row.hdopCaution || row.satelliteCaution ? "caution" : !row.hdopKnown || !row.satellitesKnown ? "unknown" : "within-display-thresholds"
            if (row.qualityCode !== quality) return rejected
            if (quality === "caution") ++caution
            else if (quality === "unknown") ++unknown
            else ++within
            ++located
            // The report retains legal polar coordinates unchanged. The
            // common Mercator base map cannot faithfully place these points.
            if (Math.abs(row.coordinate.latitude) > 85.05112878) { ++omittedPolar; continue }
            points.push({coordinate: QtPositioning.coordinate(row.coordinate.latitude, row.coordinate.longitude),
                group: row.groupOrdinal, vehicleId: row.vehicleId, componentId: row.componentId, sequence: row.sequence,
                quality: quality, fix: row.fixType})
        }
        if (located !== report.locatedSampleCount || caution !== report.qualityCautionSampleCount
                || unknown !== report.qualityUnknownSampleCount || within !== report.withinThresholdSampleCount
                || Object.keys(groups).length !== report.groupCount) return rejected
        return {available: true, points: points, omittedPolar: omittedPolar, origin: selection.origin, report: report}
    }
    function _syncHistoryGnssLayer() {
        var g = _historyGnssLife
        if (!g || !g.alive) return
        var owner = _historyGnssArchive, token = _historyGnssKey && _historyGnssGeometry.available && visible && enabled && !pipMode
            ? _historyGnssSelection.selectionToken : ""
        var oldOwner = g.owner, oldToken = g.token
        g.owner = owner; g.token = token
        if (oldOwner && oldToken && (oldOwner !== owner || oldToken !== token)) {
            try { oldOwner.clearGnssMap(oldToken) } catch (error) {}
            if (!g.alive || owner !== _historyGnssArchive || token !== g.token) return
        }
        if (!token && owner && _historyGnssSelection.active === true) {
            var rejectedToken = _historyGnssSelection.selectionToken
            try { owner.clearGnssMap(rejectedToken) } catch (error) {}
        }
    }
    function _closeHistoryGnssLayer(owner, token) {
        if (!owner || !token || owner !== _historyGnssArchive || token !== _historyGnssSelection.selectionToken) return
        try { owner.clearGnssMap(token) } catch (error) {}
    }

    function _startupSpatialGeometryFor(capture) {
        var rejected = {available:false, error:qsTr("未取得与本次捕获身份一致的完整启动几何；不回退其他路线。"), segments:[], markers:[], polar:false}
        if (!_missionMapKeys(capture,["captureToken","sourceToken","binding","current","mapSelected","view","missionReview","jointReview","error"])
                || !_missionMapText(capture.captureToken,128) || !_missionMapText(capture.sourceToken,4096)
                || typeof capture.current !== "boolean" || capture.mapSelected !== true || !_missionMapObject(capture.binding)
                || !_missionMapText(capture.binding.recordId,128) || !_missionMapInteger(capture.binding.recordRevision,1,9007199254740991)) return rejected
        var view = capture.view
        if (!_missionMapKeys(view,["schemaVersion","reviewKind","reviewId","reviewFingerprint","vehicles","segments","summary","limitations","minimumSeparationMeters","separation","snapshotOnly","executable","flightCommandReleased","clearanceCertified","timingKnown"])
                || view.schemaVersion !== 1 || ["normal","ground"].indexOf(view.reviewKind) < 0 || !_missionMapText(view.reviewId,128)
                || typeof view.reviewFingerprint !== "string" || !/^[0-9a-f]{64}$/.test(view.reviewFingerprint)
                || !Array.isArray(view.vehicles) || view.vehicles.length < 1 || view.vehicles.length > 32 || !Array.isArray(view.segments) || view.segments.length > 96
                || !_missionMapObject(view.summary) || view.summary.vehicleCount !== view.vehicles.length || view.summary.segmentCount !== view.segments.length
                || view.snapshotOnly !== true || view.executable !== false || view.flightCommandReleased !== false || view.clearanceCertified !== false || view.timingKnown !== false) return rejected
        var vehicles = Object.create(null), ids = Object.create(null), converted = [], markers = [], markerByVehicle = Object.create(null), polar = false
        for (var i = 0; i < view.vehicles.length; ++i) {
            var row = view.vehicles[i]
            if (!_missionMapKeys(row,["vehicleId","launchType","availability","reasonCodes","currentAltitudeSummary","takeoffDerivation","segmentIds"])
                    || !_missionMapInteger(row.vehicleId,1,255) || vehicles[row.vehicleId]
                    || ["takeoff","none","unknown"].indexOf(row.launchType) < 0 || ["available","partial","unavailable"].indexOf(row.availability) < 0
                    || !Array.isArray(row.segmentIds) || row.segmentIds.length > 3) return rejected
            vehicles[row.vehicleId] = row
        }
        for (var j = 0; j < view.segments.length; ++j) {
            var segment = view.segments[j]
            if (!_missionMapKeys(segment,["segmentId","vehicleId","kind","from","to","nominalOnly","actualPathCertified","timingKnown"])
                    || !_missionMapText(segment.segmentId,128) || ids[segment.segmentId] || !vehicles[segment.vehicleId]
                    || ["vertical-takeoff","takeoff-request-connection","current-first-waypoint"].indexOf(segment.kind) < 0
                    || segment.nominalOnly !== true || segment.actualPathCertified !== false || segment.timingKnown !== false) return rejected
            var endpoints = [segment.from,segment.to], coordinates = []
            for (var k = 0; k < endpoints.length; ++k) {
                var point = endpoints[k]
                if (!_missionMapKeys(point,["latitude","longitude","altitude"]) || !_missionMapNumber(point.latitude,-90,90)
                        || !_missionMapNumber(point.longitude,-180,180) || !_missionMapNumber(point.altitude,-Number.MAX_VALUE,Number.MAX_VALUE)) return rejected
                // Keep the source's full +/-90 degree domain. The operational
                // map deliberately does not promise polar basemap coverage.
                if (Math.abs(point.latitude) > 85) polar = true
                coordinates.push(QtPositioning.coordinate(point.latitude,point.longitude,point.altitude))
            }
            ids[segment.segmentId] = segment
            converted.push({vehicleId:segment.vehicleId,segmentId:segment.segmentId,kind:segment.kind,path:coordinates,
                fromAltitude:segment.from.altitude,toAltitude:segment.to.altitude})
            var marker = markerByVehicle[segment.vehicleId]
            if (!marker) {
                marker = {vehicleId:segment.vehicleId,coordinate:coordinates[0],lines:[]}
                markerByVehicle[segment.vehicleId] = marker
                markers.push(marker)
            }
            var segmentName = segment.kind === "vertical-takeoff" ? qsTr("垂直段（同 XY）")
                : segment.kind === "takeoff-request-connection" ? qsTr("TAKEOFF 连接（粗线）") : qsTr("首航点参考（细线）")
            marker.lines.push(qsTr("%1：AMSL %2 → %3 m").arg(segmentName).arg(segment.from.altitude).arg(segment.to.altitude))
        }
        for (var m = 0; m < view.vehicles.length; ++m) {
            var member = view.vehicles[m], seen = Object.create(null)
            for (var n = 0; n < member.segmentIds.length; ++n) {
                var id = member.segmentIds[n]
                if (typeof id !== "string" || seen[id] || !ids[id] || ids[id].vehicleId !== member.vehicleId) return rejected
                seen[id] = true
            }
        }
        if (polar) return {available:false,error:qsTr("本地图视图只绘制绝对纬度不超过 85° 的底图参考。该快照含极区坐标；完整数值保留在工作台，不绘制可能不完整的图层。"),segments:[],markers:[],polar:true}
        return {available:converted.length > 0,error:converted.length ? "" : qsTr("本次没有可绘制的完整几何段；请返回工作台查看逐机未知原因。"),segments:converted,markers:markers,polar:false}
    }

    function _missionSpatialGeometryFor(result,expectedPlanId) {
        var rejected={available:false,error:qsTr("完整原计划图层不可用；不回退旧计划，启动参考层独立保留。"),routes:[],routeMarkers:[],markers:[],memberCount:0,pointCount:0,itemCount:0,geometryComplete:false,encodingAvailable:false}
        if(!_missionMapKeys(result,["ok","error","view"])||result.ok!==true||typeof result.error!=="string") {
            if(result&&typeof result.error==="string"&&result.error.length)rejected.error+=" "+result.error
            return rejected
        }
        var view=result.view
        if(!_missionMapKeys(view,["schemaVersion","planId","memberCount","totalPointCount","totalItemCount","geometryComplete","encodingAvailable","encodingError","vehicles","reserveVehicleIds","limitations","snapshotOnly","executable","flightCommandReleased","onboardContentsProven","clearanceCertified","airspace"])
                ||view.schemaVersion!==1||typeof view.planId!=="string"||!/^[0-9a-f]{64}$/.test(view.planId)||view.planId!==expectedPlanId
                ||!_missionMapInteger(view.memberCount,1,32)||!Array.isArray(view.vehicles)||view.vehicles.length!==view.memberCount
                ||!_missionMapInteger(view.totalPointCount,0,8192)||!_missionMapInteger(view.totalItemCount,0,8192)
                ||typeof view.geometryComplete!=="boolean"||typeof view.encodingAvailable!=="boolean"
                ||view.snapshotOnly!==true||view.executable!==false||view.flightCommandReleased!==false||view.onboardContentsProven!==false||view.clearanceCertified!==false)return rejected
        var ids=Object.create(null),routes=[],routeMarkers=[],markers=[],actionGroups=Object.create(null),pointTotal=0,itemTotal=0,allGeometry=true
        var pointKeys=["pointIndex","coordinateSystem","latitude","longitude","altitude","altitudeReference","headingKnown","heading","holdKnown","holdSeconds","elapsedKnown","estimatedElapsedSeconds","sourceTiming"]
        for(var i=0;i<view.vehicles.length;++i){
            var member=view.vehicles[i]
            if(!_missionMapKeys(member,["vehicleId","role","geometryAvailable","geometryError","routes","pointCount","executionProfileAvailable","launchAction","endAction","encodingAvailable","encodingError","encodedItems","payloadHash","unencodedRequirements","sourceSchedule"])
                    ||!_missionMapInteger(member.vehicleId,1,255)||ids[member.vehicleId]||typeof member.role!=="string"||typeof member.geometryAvailable!=="boolean"||typeof member.geometryError!=="string"
                    ||!Array.isArray(member.routes)||member.routes.length>2||!_missionMapInteger(member.pointCount,0,2048)
                    ||typeof member.encodingAvailable!=="boolean"||!Array.isArray(member.encodedItems)||member.encodedItems.length>2048
                    ||typeof member.payloadHash!=="string"||(member.encodingAvailable ? !/^[0-9a-f]{64}$/.test(member.payloadHash)||!member.encodedItems.length : member.payloadHash!==""||member.encodedItems.length!==0))return rejected
            ids[member.vehicleId]=true;itemTotal+=member.encodedItems.length;allGeometry=allGeometry&&member.geometryAvailable
            var seenPaths=Object.create(null),memberPoints=0,memberMarker=null
            for(var j=0;j<member.routes.length;++j){
                var route=member.routes[j]
                if(!_missionMapKeys(route,["sourcePath","sourceScope","sourceMetadata","points"])||["waypoints","returnPath"].indexOf(route.sourcePath)<0||seenPaths[route.sourcePath]
                        ||["member","plan-global"].indexOf(route.sourceScope)<0||!_missionMapObject(route.sourceMetadata)||!Array.isArray(route.points)||route.points.length>2048)return rejected
                seenPaths[route.sourcePath]=true;memberPoints+=route.points.length
                var path=[]
                for(var k=0;k<route.points.length;++k){
                    var point=route.points[k]
                    if(!_missionMapKeys(point,pointKeys)||point.pointIndex!==k||point.coordinateSystem!=="geodetic"||point.altitudeReference!=="amsl"
                            ||!_missionMapNumber(point.latitude,-80,80)||!_missionMapNumber(point.longitude,-180,180)||!_missionMapNumber(point.altitude,-1000,20000)
                            ||typeof point.headingKnown!=="boolean"||typeof point.holdKnown!=="boolean"||typeof point.elapsedKnown!=="boolean"||!_missionMapObject(point.sourceTiming)
                            ||(point.headingKnown ? !_missionMapNumber(point.heading,-Number.MAX_VALUE,Number.MAX_VALUE) : point.heading!==null)
                            ||(point.holdKnown ? !_missionMapNumber(point.holdSeconds,0,86400) : point.holdSeconds!==null)
                            ||(point.elapsedKnown ? !_missionMapNumber(point.estimatedElapsedSeconds,0,Number.MAX_VALUE) : point.estimatedElapsedSeconds!==null))return rejected
                    path.push(QtPositioning.coordinate(point.latitude,point.longitude,point.altitude))
                }
                if(path.length){
                    routes.push({vehicleId:member.vehicleId,role:member.role,returning:route.sourcePath==="returnPath",path:path,pointCount:path.length})
                    if(!memberMarker)memberMarker={vehicleId:member.vehicleId,coordinate:path[0],outboundCount:0,returnCount:0}
                    if(route.sourcePath==="returnPath")memberMarker.returnCount=path.length
                    else{memberMarker.outboundCount=path.length;memberMarker.coordinate=path[0]}
                }
            }
            if(memberPoints!==member.pointCount||(!member.geometryAvailable&&(member.routes.length!==0||memberPoints!==0)))return rejected
            pointTotal+=memberPoints
            if(memberMarker)routeMarkers.push(memberMarker)
            var actions=[member.launchAction,member.endAction]
            for(var n=0;n<actions.length;++n){
                var action=actions[n]
                if(!_missionMapKeys(action,["type","declared","coordinateKnown","coordinate","encodingKnown","encoded","sequenceNumber"])
                        ||typeof action.type!=="string"||typeof action.declared!=="boolean"||typeof action.coordinateKnown!=="boolean"||!_missionMapObject(action.coordinate)
                        ||typeof action.encodingKnown!=="boolean"||typeof action.encoded!=="boolean"||(action.sequenceNumber!==null&&!_missionMapInteger(action.sequenceNumber,0,2047)))return rejected
                if(!action.coordinateKnown)continue
                var coordinate=action.coordinate
                if(!_missionMapKeys(coordinate,["coordinateSystem","altitudeReference","latitude","longitude","altitude"])||coordinate.coordinateSystem!=="geodetic"||coordinate.altitudeReference!=="amsl"
                        ||!_missionMapNumber(coordinate.latitude,-80,80)||!_missionMapNumber(coordinate.longitude,-180,180)||!_missionMapNumber(coordinate.altitude,-1000,20000))return rejected
                // Only declared geographic actions become markers. Never
                // treat speed-reserved zeros or RTL as coordinates/HOME paths.
                if(action.declared&&["takeoff","hold","land"].indexOf(action.type)>=0){
                    var actionKey=String(member.vehicleId)+":"+String(coordinate.latitude)+":"+String(coordinate.longitude),actionMarker=actionGroups[actionKey]
                    if(!actionMarker){
                        actionMarker={vehicleId:member.vehicleId,coordinate:QtPositioning.coordinate(coordinate.latitude,coordinate.longitude,coordinate.altitude),lines:[]}
                        actionGroups[actionKey]=actionMarker;markers.push(actionMarker)
                    }
                    var actionName=action.type==="takeoff"?qsTr("TAKEOFF 起飞"):action.type==="hold"?qsTr("HOLD 无限保持"):qsTr("LAND 降落")
                    actionMarker.lines.push(qsTr("%1请求 · 声明 AMSL %2 m\n协议序号 %3 · %4").arg(actionName).arg(coordinate.altitude)
                        .arg(action.sequenceNumber===null?qsTr("未知"):String(action.sequenceNumber))
                        .arg(action.encodingKnown ? action.encoded?qsTr("本地已编码"):qsTr("本地未编码") : qsTr("本地编码未知")))
                }
            }
        }
        if(pointTotal!==view.totalPointCount||itemTotal!==view.totalItemCount||allGeometry!==view.geometryComplete)return rejected
        return {available:routes.length>0||markers.length>0,error:allGeometry?"":qsTr("部分原计划成员几何不可用；显示的路线不是全组完整覆盖。"),routes:routes,routeMarkers:routeMarkers,markers:markers,
            memberCount:view.memberCount,pointCount:pointTotal,itemCount:itemTotal,geometryComplete:allGeometry,encodingAvailable:view.encodingAvailable}
    }

    function _setStartupSpatialLayer(index) {
        if(!_startupMapActive||!_missionMapInteger(index,0,3)||index===_startupSpatialLayer)return
        if(_missionPlanningEpoch>=9007199254740991){_missionPlanningEpochExhausted=true;_missionMapPicking=false;return}
        _missionPlanningEpoch+=1
        _startupSpatialLayer=index
        mapInteraction.isDrawingBox=false;marqueeBox.visible=false;clickMenu.close()
    }

    function _syncStartupSpatialMap() {
        var guard = _startupMapLifetime
        if (!guard.alive || !guard.ready) return
        var ticket = {}, owner = _missionDraftController, selectionKey = _startupSpatialSelectionKey
        guard.syncTicket = ticket
        try {
            if (_startupSpatialSelected) {
                if (!_startupMapActive) {
                    _startupMapRestore = {open:_missionPlannerOpen,picking:_missionMapPicking,reviewOnly:_missionMapReviewOnly}
                    _setMissionPlannerOpen(true)
                    if (!guard.alive || guard.syncTicket !== ticket || owner !== _missionDraftController
                            || selectionKey !== _startupSpatialSelectionKey) return
                }
                if (_missionPlanningEpoch >= 9007199254740991) {
                    _missionPlanningEpochExhausted = true; _missionMapPicking = false; return
                }
                _missionPlanningEpoch += 1
                _missionMapPicking = false; _missionMapReviewOnly = true
                _startupSpatialLayer = 0
                mapInteraction.isDrawingBox = false; marqueeBox.visible = false; clickMenu.close()
                _startupMapNotice = ""
            } else if (_startupMapActive) {
                var original = _startupMapRestore
                _startupMapRestore = null
                if (_missionPlanningEpoch >= 9007199254740991) {
                    _missionPlanningEpochExhausted = true; _missionMapPicking = false; return
                }
                _missionPlanningEpoch += 1
                _setMissionPlannerOpen(original.open)
                if (!guard.alive || guard.syncTicket !== ticket || owner !== _missionDraftController
                        || selectionKey !== _startupSpatialSelectionKey) return
                _missionMapReviewOnly = original.open && original.reviewOnly
                _missionMapPicking = original.open && original.picking && !_missionRemainderSelected && !_missionMapReviewOnly
                mapInteraction.isDrawingBox = false; marqueeBox.visible = false; clickMenu.close()
            }
        } finally { if (guard.syncTicket === ticket) guard.syncTicket = null }
    }

    function _exitStartupSpatialMap(expectedToken) {
        if (_startupMapCallBusy || _startupMapLifetime.ticket || !_startupMapActive || !_missionDraftController
                || typeof _missionDraftController.selectFleetStartupSpatialMap !== "function"
                || expectedToken !== _startupSpatialReview.captureToken) return
        var guard = _startupMapLifetime, owner = _missionDraftController, ticket = {}, epoch = _missionPlanningEpoch
        guard.ticket = ticket
        _startupMapCallBusy = true
        try {
            if (!guard.alive || owner !== _missionDraftController || !_startupMapActive
                    || epoch !== _missionPlanningEpoch || expectedToken !== _startupSpatialReview.captureToken) return
            var result = owner.selectFleetStartupSpatialMap(expectedToken,false,true)
            if (!guard.alive) return
            _startupMapNotice = result && result.ok === true ? "" : qsTr("结束地图选择未确认：%1；请核对当前选择；若仍选中则继续只读隔离。").arg(result && typeof result.error === "string" ? result.error : qsTr("未知回执"))
        } catch (error) {
            if (guard.alive) _startupMapNotice = qsTr("本地视图选择回执中断；请核对当前显示。未发送飞行指令或自动重试。")
        } finally {
            if (guard.alive) _startupMapCallBusy = false
            if (guard.ticket === ticket) guard.ticket = null
        }
    }

    function _startupSpatialMapSummary() {
        var capture = _startupSpatialReview, view = capture.view || {}, summary = view.summary || {}, separation = view.separation || {}, binding = capture.binding || {}
        return qsTr("启动空间 · %1 · %2\n审阅 %3\n记录 %4 / 版本 %5\n%6\n几何完整目标 %7 / %8；名义段 %9。启动段空间比较完成 %10；候选 %11（不是碰撞预测，也不覆盖完整任务路线）。\n垂直段同 XY，仅标记 AMSL 起止高；三类段独立，不是连续实际航迹。时间、地形、空域与净空均未认证。%12\n\n%13")
            .arg(view.reviewKind === "ground" ? qsTr("地面再审") : view.reviewKind === "normal" ? qsTr("初次审阅") : qsTr("来源未知"))
            .arg(capture.current === true ? qsTr("当前绑定快照") : qsTr("过期历史快照"))
            .arg(typeof view.reviewId === "string" ? view.reviewId : qsTr("未知"))
            .arg(typeof binding.recordId === "string" ? binding.recordId : qsTr("未知"))
            .arg(_missionMapInteger(binding.recordRevision,1,9007199254740991) ? binding.recordRevision : qsTr("未知"))
            .arg(_startupMapGeometry.available ? qsTr("只读名义几何；可用不等于可飞") : _startupMapGeometry.error)
            .arg(typeof summary.availableVehicleCount === "number" ? summary.availableVehicleCount : qsTr("未知"))
            .arg(typeof summary.vehicleCount === "number" ? summary.vehicleCount : qsTr("未知"))
            .arg(typeof summary.segmentCount === "number" ? summary.segmentCount : qsTr("未知"))
            .arg(separation.comparisonComplete === true ? qsTr("是（仅已表示段）") : qsTr("否 / 未知"))
            .arg(typeof separation.candidateCount === "number" ? separation.candidateCount : qsTr("未知"))
            .arg(_startupMapNotice ? "\n" + _startupMapNotice : "")
            .arg(_startupMissionMapSummary())
    }

    function _startupMissionMapSummary() {
        var geometry=_startupMissionGeometry
        return qsTr("完整原计划独立图层：%1\n原计划成员 %2；规划点 %3；本地协议项 %4；整组编码可用 %5。出航粗线 / 规划返程细线，不等于实际编码航迹。\nTAKEOFF / HOLD / LAND 标记是声明动作请求，非执行结果；RTL 不补 HOME 线，速度项不画坐标。规划返程在完整 Builder 成功时已有航点请求，但不是上传证明。\n归档空域报告仅覆盖原规划路线，未覆盖启动段、终结下降、RTL 或动态时序；禁飞区原值在工作台查阅，未在此补画。")
            .arg(geometry.available ? geometry.geometryComplete ? qsTr("几何可展示，非许可") : geometry.error : geometry.error)
            .arg(geometry.memberCount).arg(geometry.pointCount).arg(geometry.itemCount).arg(geometry.encodingAvailable?qsTr("是（仅本地）"):qsTr("否 / 未确认"))
    }

    function _setMissionPlannerOpen(open) {
        if (_startupMapActive && open === false) return // Leave only through the explicit startup-map selector.
        if (open === _missionPlannerOpen) return
        if (_missionPlanningEpoch >= 9007199254740991) {
            _missionPlanningEpochExhausted = true
            _missionPlannerOpen = false
            _missionMapPicking = false
            return
        }
        _missionPlanningEpoch += 1
        _missionPlannerOpen = open === true
        _missionMapPicking = false
        _missionMapReviewOnly = false
        mapInteraction.isDrawingBox = false
        marqueeBox.visible = false
        if (_missionPlannerOpen) {
            panRecenterTimer.stop()
            animateLat.stop()
            animateLong.stop()
            _disableVehicleTracking = true
            clickMenu.close()
            _cancelShiftDraft()
            if (_missionDraftController) {
                _missionDraftController.closeAll()
                // Cancel a still-hidden delayed confirmation as well.
                if (_missionDraftController.confirmDialog) {
                    _missionDraftController.confirmDialog.confirmCancelled()
                }
            }
            gotoLocationItem.hide()
            orbitMapCircle.hide()
        } else {
            panRecenterTimer.restart()
        }
    }

    function _setMissionMapReviewOnly(enabled) {
        if (_startupMapActive) return // Startup and remainder sources never silently reinterpret each other.
        if (!_missionPlannerOpen || pipMode || enabled === _missionMapReviewOnly
                || (enabled === true && !_missionRemainderSelected)) return
        if (_missionPlanningEpoch >= 9007199254740991) {
            _missionPlanningEpochExhausted = true
            _missionMapPicking = false
            return
        }
        // Keep planning isolation active while the details are clipped. A
        // pointer held across either transition cannot become a new gesture.
        _missionPlanningEpoch += 1
        _missionMapPicking = false
        _missionMapReviewOnly = enabled === true
        mapInteraction.isDrawingBox = false
        marqueeBox.visible = false
        clickMenu.close()
    }

    function _missionMapObject(value) {
        return !!value && typeof value === "object" && !Array.isArray(value)
    }
    function _missionMapKeys(value, keys) {
        if (!_missionMapObject(value) || Object.keys(value).length !== keys.length) return false
        for (var i = 0; i < keys.length; ++i)
            if (!Object.prototype.hasOwnProperty.call(value, keys[i])) return false
        return true
    }
    function _missionMapNumber(value, minimum, maximum) {
        return typeof value === "number" && isFinite(value) && value >= minimum && value <= maximum
    }
    function _missionMapInteger(value, minimum, maximum) {
        return _missionMapNumber(value, minimum, maximum) && Math.floor(value) === value
    }
    function _missionMapText(value, maximum) {
        return typeof value === "string" && value.length > 0 && value.length <= maximum
    }
    function _missionRemainderGeometryFor(review) {
        var unavailable = {available: false, error: qsTr("未取得与新记录一致的完整几何投影。"), routes: [],
            recordId: "", recordRevision: 0, vehicleId: 0, pointCount: 0}
        if (!_missionMapObject(review) || review.accepted !== true) return unavailable
        var archive = review.archive, plan = review.plan, view = review.view
        if (review.kind === "reserve-dispatch") {
            // The independent dispatch uses the existing complete mission
            // spatial view, not the remainder schema or a fabricated Run.
            if (!_missionMapText(review.token, 128) || review.flightCommandReleased !== false || review.executable !== false
                    || !_missionMapObject(archive) || !_missionMapText(archive.recordId, 128)
                    || !_missionMapInteger(archive.recordRevision, 1, 9007199254740991)
                    || !_missionMapObject(plan) || plan.planId !== archive.planId
                    || !Array.isArray(plan.members) || plan.members.length !== 1 || !_missionMapObject(plan.members[0])
                    || !_missionMapInteger(plan.members[0].vehicleId, 1, 255)
                    || ["leader", "scout", "relay", "responder"].indexOf(plan.members[0].role) < 0
                    || !_missionMapObject(plan.reserveDispatchPlanningSource) || !_missionMapObject(view)
                    || !Array.isArray(view.vehicles) || view.vehicles.length !== 1 || !_missionMapObject(view.vehicles[0])
                    || !Array.isArray(view.reserveVehicleIds) || view.reserveVehicleIds.length !== 0
                    || view.vehicles[0].vehicleId !== plan.members[0].vehicleId || view.vehicles[0].role !== plan.members[0].role
                    || plan.reserveDispatchPlanningSource.reserveVehicleId !== plan.members[0].vehicleId) return unavailable
            var spatial = _missionSpatialGeometryFor({ok: true, error: "", view: view}, archive.planId)
            if (!spatial.available || spatial.geometryComplete !== true || spatial.memberCount !== 1
                    || spatial.routes.length !== 2 || !_missionMapInteger(spatial.pointCount, 4, 2048)) {
                unavailable.error = qsTr("独立备用出勤的完整规划几何不可用；不回退旧任务或区域草稿。")
                return unavailable
            }
            var dispatchRoutes = [], dispatchPaths = {}
            for (var dispatchIndex = 0; dispatchIndex < spatial.routes.length; ++dispatchIndex) {
                var supplied = spatial.routes[dispatchIndex], pathKey = supplied.returning ? "returnPath" : "waypoints"
                if (dispatchPaths[pathKey] || supplied.vehicleId !== plan.members[0].vehicleId || supplied.path.length < 2
                        || (supplied.returning && supplied.path.length !== 2)) return unavailable
                dispatchPaths[pathKey] = true
                dispatchRoutes.push({vehicleId: supplied.vehicleId, role: supplied.role, returning: supplied.returning,
                    path: supplied.path, reserveDispatch: true, recordId: archive.recordId, recordRevision: archive.recordRevision})
            }
            // Only the complete two planned routes are drawn. The spatial
            // helper's action markers are not actual takeoff/land trajectories.
            return {available: true, error: "", routes: dispatchRoutes, recordId: archive.recordId,
                recordRevision: archive.recordRevision, vehicleId: plan.members[0].vehicleId, pointCount: spatial.pointCount}
        }
        if (!_missionMapText(review.token, 128) || review.flightCommandReleased !== false || review.executable !== false
                || !_missionMapObject(archive) || !_missionMapText(archive.recordId, 128)
                || !_missionMapInteger(archive.recordRevision, 1, 9007199254740991)
                || typeof archive.planId !== "string" || !/^[0-9a-f]{64}$/.test(archive.planId)
                || !_missionMapObject(plan) || plan.planId !== archive.planId
                || !Array.isArray(plan.members) || plan.members.length !== 1
                || !_missionMapObject(plan.members[0]) || !_missionMapObject(view)
                || view.schemaVersion !== 1 || view.planId !== archive.planId
                || view.flightCommandReleased !== false || view.executable !== false
                || !_missionMapInteger(view.vehicleId, 1, 255) || plan.members[0].vehicleId !== view.vehicleId
                || ["leader", "scout", "relay", "responder"].indexOf(view.role) < 0
                || plan.members[0].role !== view.role) return unavailable
        unavailable.recordId = archive.recordId
        unavailable.recordRevision = archive.recordRevision
        unavailable.vehicleId = view.vehicleId
        if (view.geometryAvailable !== true) {
            unavailable.error = qsTr("新记录的完整几何不可用；不会显示旧区域草稿代替。")
            if (_missionMapText(view.geometryError, 160)) unavailable.error += " " + view.geometryError
            return unavailable
        }
        if (view.geometryError !== "" || !Array.isArray(view.routes) || view.routes.length !== 2
                || !_missionMapInteger(view.pointCount, 4, 2048)) return unavailable
        var paths = ["waypoints", "returnPath"], routes = [], total = 0
        var pointKeys = ["pointIndex", "coordinateSystem", "latitude", "longitude", "altitude", "altitudeReference",
            "headingKnown", "heading", "holdKnown", "holdSeconds", "elapsedKnown", "estimatedElapsedSeconds",
            "originalSourcePointIndexKnown", "originalSourcePointIndex"]
        for (var routeIndex = 0; routeIndex < 2; ++routeIndex) {
            var route = view.routes[routeIndex]
            if (!_missionMapKeys(route, ["sourcePath", "points"]) || route.sourcePath !== paths[routeIndex]
                    || !Array.isArray(route.points) || route.points.length < 2 || route.points.length > 2048
                    || (routeIndex === 1 && route.points.length !== 2)) return unavailable
            total += route.points.length
            if (total > 2048) return unavailable
            var coordinates = []
            for (var pointIndex = 0; pointIndex < route.points.length; ++pointIndex) {
                var point = route.points[pointIndex]
                if (!_missionMapKeys(point, pointKeys) || point.pointIndex !== pointIndex
                        || point.coordinateSystem !== "geodetic" || point.altitudeReference !== "amsl"
                        || !_missionMapNumber(point.latitude, -80, 80) || !_missionMapNumber(point.longitude, -180, 180)
                        || !_missionMapNumber(point.altitude, -1000, 20000)
                        || typeof point.headingKnown !== "boolean" || typeof point.holdKnown !== "boolean"
                        || typeof point.elapsedKnown !== "boolean" || typeof point.originalSourcePointIndexKnown !== "boolean"
                        || (point.headingKnown ? !_missionMapNumber(point.heading, -Number.MAX_VALUE, Number.MAX_VALUE) : point.heading !== null)
                        || (point.holdKnown ? !_missionMapNumber(point.holdSeconds, 0, 86400) : point.holdSeconds !== null)
                        || (point.elapsedKnown ? !_missionMapNumber(point.estimatedElapsedSeconds, 0, Number.MAX_VALUE) : point.estimatedElapsedSeconds !== null)
                        || (point.originalSourcePointIndexKnown
                            ? !_missionMapInteger(point.originalSourcePointIndex, 0, 2047) : point.originalSourcePointIndex !== null)) return unavailable
                coordinates.push(QtPositioning.coordinate(point.latitude, point.longitude, point.altitude))
            }
            routes.push({vehicleId: view.vehicleId, role: view.role, returning: routeIndex === 1, path: coordinates,
                remainder: true, recordId: archive.recordId, recordRevision: archive.recordRevision})
        }
        if (total !== view.pointCount) return unavailable
        // Publish only after both complete paths pass; never a valid prefix.
        return {available: true, error: "", routes: routes, recordId: archive.recordId,
            recordRevision: archive.recordRevision, vehicleId: view.vehicleId, pointCount: total}
    }
    function _missionRemainderMapSummary() {
        if (!_missionRemainderSelected)
            return qsTr("独立新任务来源的采用已取消，未显示规划路线。请返回详情选择来源；此处不回退旧区域草稿。")
        var geometry = _missionRemainderGeometry, archive = _missionRemainderReview.archive || {}
        var recordId = _missionMapText(archive.recordId, 128) ? archive.recordId : qsTr("未知")
        var revision = _missionMapInteger(archive.recordRevision, 1, 9007199254740991) ? String(archive.recordRevision) : qsTr("未知")
        var status = _missionRemainderReview.current === true
            ? qsTr("当前新记录 · 仅规划，不可执行") : qsTr("已过期的历史新记录 · 不可执行")
        return (_missionRemainderReview.kind === "reserve-dispatch" ? qsTr("独立备用出勤新任务 · 不代表原任务已接管\n") : "")
            + qsTr("%1\n记录 %2 · 版本 %3\n%4\n仅 WGS84 / AMSL 的二维几何，不证明障碍、地形净空或飞行结果。")
            .arg(status).arg(recordId).arg(revision)
            .arg(geometry.available
                ? qsTr("UAV-%1 · 出航粗线 / 返程细线及文字标签 · 共 %2 个规划点").arg(geometry.vehicleId).arg(geometry.pointCount)
                : geometry.error)
    }

    function _missionGeoPath(points, limit) {
        if (!points || points.length === undefined || points.length > limit) return []
        var path = []
        for (var i = 0; i < points.length; ++i) {
            var point = points[i] || {}
            var latitude = point.latitude === undefined ? point.lat : point.latitude
            var longitude = point.longitude === undefined ? point.lon : point.longitude
            if (typeof latitude !== "number" || typeof longitude !== "number"
                    || !isFinite(latitude) || !isFinite(longitude)
                    || Math.abs(latitude) > 90 || Math.abs(longitude) > 180) return []
            path.push(QtPositioning.coordinate(latitude, longitude))
        }
        return path
    }

    function _missionExclusionsFor(settings) {
        var rings = settings ? settings.exclusionPolygons : undefined
        if (rings === undefined) return []
        if (!Array.isArray(rings) || rings.length > 8) return []
        var result = [], count = _missionDraft ? _missionDraft.vertices.length : 0
        for (var i = 0; i < rings.length; ++i) {
            if (!Array.isArray(rings[i]) || rings[i].length < 3 || rings[i].length > 64) return []
            count += rings[i].length
            if (count > 128) return []
            var path = _missionGeoPath(rings[i], 64)
            if (path.length !== rings[i].length) return []
            result.push({number: i + 1, path: path})
        }
        // Draft input only: drawing a complete set is not topology validation.
        return result
    }

    function _missionRoutesFor(result) {
        var plan = result && result.missionReview ? result.missionReview.plan : null
        var members = plan ? plan.members : null
        if (!members || members.length === undefined || members.length > 32) return []
        var routes = []
        var count = 0
        for (var i = 0; i < members.length; ++i) {
            var member = members[i] || {}
            var paths = [member.waypoints || [], member.returnPath || []]
            for (var j = 0; j < paths.length; ++j) {
                var points = paths[j]
                if (points && points.waypoints) points = points.waypoints
                if (!points || points.length === undefined) return []
                count += points.length
                if (count > 8192) return [] // reject a view, never draw a truncated plan
                var path = _missionGeoPath(points, 8192)
                if (path.length !== points.length) return []
                if (path.length > 0) routes.push({ vehicleId: member.vehicleId,
                    role: member.role || "", returning: j === 1, path: path })
            }
        }
        return routes
    }

    // 【原有】用于保存当前框选的多架无人机 ID 集合
    property var    selectedSwarmIds:       []
    property bool   suppressExclusiveSelectionSync: false

    function setVehicleSelection(vehicleId, selected) {
        if (shiftQueuedCoords.length > 0) _cancelShiftDraft()
        var nextSelection = selectedSwarmIds ? selectedSwarmIds.slice(0) : []
        var selectionIndex = nextSelection.indexOf(vehicleId)

        if (selected && selectionIndex === -1) {
            nextSelection.push(vehicleId)
        } else if (!selected && selectionIndex !== -1) {
            nextSelection.splice(selectionIndex, 1)
        }

        selectedSwarmIds = nextSelection

        if (selected) {
            var selectedVehicle = QGroundControl.multiVehicleManager.getVehicleById(vehicleId)
            if (selectedVehicle) {
                suppressExclusiveSelectionSync = true
                QGroundControl.multiVehicleManager.activeVehicle = selectedVehicle
                suppressExclusiveSelectionSync = false
                _showCurrentTaskRoute(vehicleId)
            }
        }
    }

    function focusVehicle(vehicleId) {
        var vehicle = QGroundControl.multiVehicleManager.getVehicleById(vehicleId)
        if (!vehicle) {
            return
        }

        if (shiftQueuedCoords.length > 0) _cancelShiftDraft()
        selectedSwarmIds = [vehicleId]
        suppressExclusiveSelectionSync = true
        QGroundControl.multiVehicleManager.activeVehicle = vehicle
        suppressExclusiveSelectionSync = false
        _showCurrentTaskRoute(vehicleId)
    }

    function clearVehicleSelection() {
        if (shiftQueuedCoords.length > 0) _cancelShiftDraft()
        selectedSwarmIds = []
        shiftCommittedCoords = []
    }

    // Shift 草稿坐标只用于预览和确认；确认前不进入 MissionManager。
    property var    shiftQueuedCoords:      []
    property var    shiftCommittedCoords:   []
    property var    shiftFrozenVehicleIds:  []
    property var    shiftFrozenReferences:  []
    property bool   shiftReplacementRequired: false
    property var    activeTaskRoutes: ({})
    property bool   shiftMissionConfirmPending: false
    // One-shot approval token set only by the confirmation adapter. This
    // prevents a direct call to the dispatch helper from turning a draft into
    // a mission without the operator confirmation step.
    property bool   _shiftMissionApproved: false

    QtObject {
        id: shiftMissionConfirmationAdapter

        function actionConfirmed() {
            if (_root._missionPlannerOpen || _root._missionPlanningEpochExhausted) {
                _root._cancelShiftDraft(true)
                return
            }
            var adapterEpoch = _root._missionPlanningEpoch
            _root.shiftMissionConfirmPending = false
            if (_root._missionPlannerOpen || adapterEpoch !== _root._missionPlanningEpoch) return
            _root._shiftMissionApproved = true
            if (_root._missionPlannerOpen || adapterEpoch !== _root._missionPlanningEpoch) return
            _root.executeBatchQueuedFly()
        }

        function actionCancelled() {
            _root._cancelShiftDraft(true)
        }
    }

    function _cancelShiftDraft(fromConfirmation) {
        if (!fromConfirmation && shiftMissionConfirmPending
                && globals.guidedControllerFlyView
                && globals.guidedControllerFlyView.confirmDialog
                && globals.guidedControllerFlyView.confirmDialog.mapIndicator === shiftMissionConfirmationAdapter) {
            shiftMissionConfirmPending = false
            globals.guidedControllerFlyView.confirmDialog.confirmCancelled()
        }
        shiftQueuedCoords = []
        shiftFrozenVehicleIds = []
        shiftFrozenReferences = []
        shiftReplacementRequired = false
        shiftMissionConfirmPending = false
        _shiftMissionApproved = false
        _showCurrentTaskRoute(_activeVehicle ? _activeVehicle.id : -1)
    }

    function _showCurrentTaskRoute(vehicleId) {
        var route = activeTaskRoutes[String(vehicleId)]
        shiftCommittedCoords = route ? route.slice(0) : []
    }

    function _cloneRouteMap() {
        var clone = {}
        for (var key in activeTaskRoutes) {
            if (activeTaskRoutes.hasOwnProperty(key)) clone[key] = activeTaskRoutes[key]
        }
        return clone
    }

    function _freezeShiftTargets() {
        var ids = selectedSwarmIds && selectedSwarmIds.length > 0
                ? selectedSwarmIds.slice(0)
                : (_activeVehicle ? [_activeVehicle.id] : [])
        if (ids.length === 0) {
            mainWindow.showMessageDialog(qsTr("无法创建航点队列"), qsTr("请先选择至少一架无人机。"))
            return false
        }

        var references = []
        for (var i = 0; i < ids.length; i++) {
            var vehicle = QGroundControl.multiVehicleManager.getVehicleById(ids[i])
            if (!vehicle || !vehicle.coordinate || !vehicle.coordinate.isValid) {
                mainWindow.showMessageDialog(qsTr("无法创建航点队列"), qsTr("UAV-%1 没有有效位置。").arg(ids[i]))
                return false
            }
            references.push(vehicle.coordinate)
        }
        shiftFrozenVehicleIds = ids
        shiftFrozenReferences = references
        return true
    }

    Connections {
        target: QGroundControl.multiVehicleManager
        function onActiveVehicleChanged(activeVehicle) {
            if (_root.suppressExclusiveSelectionSync) return
            if (_root.shiftQueuedCoords.length > 0) _root._cancelShiftDraft()
            _root.selectedSwarmIds = activeVehicle ? [activeVehicle.id] : []
            _root._showCurrentTaskRoute(activeVehicle ? activeVehicle.id : -1)
        }
    }

    Connections {
        target: swarmController
        function onTemporaryMissionCompleted(vehicleId, clearError) {
            var routes = _root._cloneRouteMap()
            delete routes[String(vehicleId)]
            _root.activeTaskRoutes = routes
            if (_root._activeVehicle && _root._activeVehicle.id === vehicleId) {
                _root.shiftCommittedCoords = []
            }
            if (clearError) {
                mainWindow.showMessageDialog(
                    qsTr("临时任务清理失败"),
                    qsTr("UAV-%1 的旧任务未收到清除 ACK，已标记为残留任务；再次下达队列任务时必须确认替换。").arg(vehicleId))
            }
        }
    }

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property var    _planMasterController:  planMasterController
    property var    _geoFenceController:    planMasterController.geoFenceController
    property var    _rallyPointController:  planMasterController.rallyPointController
    property var    _activeVehicleCoordinate:_activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
    property real   _toolButtonTopMargin:   parent.height - mainWindow.height + (ScreenTools.defaultFontPixelHeight / 2)
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75

    property var    _flyViewSettings:           QGroundControl.settingsManager.flyViewSettings
    property bool   _keepMapCenteredOnVehicle:  _flyViewSettings.keepMapCenteredOnVehicle.rawValue
    property bool   _disableVehicleTracking:    false
    property bool   _keepVehicleCentered:       pipMode ? true : false
    property bool   _saveZoomLevelSetting:      true

    function _adjustMapZoomForPipMode() {
        _saveZoomLevelSetting = false
        if (pipMode) {
            if (QGroundControl.flightMapZoom > 3) {
                zoomLevel = QGroundControl.flightMapZoom - 3
            }
        } else {
            zoomLevel = QGroundControl.flightMapZoom
        }
        _saveZoomLevelSetting = true
    }

    onPipModeChanged: {
        var historyGuard = _historyGnssLife
        if (pipMode) _setMissionPlannerOpen(false)
        _adjustMapZoomForPipMode()
        if (historyGuard && historyGuard.alive) _syncHistoryGnssLayer()
    }

    onVisibleChanged: {
        var historyGuard = _historyGnssLife
        if (!visible) _setMissionPlannerOpen(false)
        if (visible) {
            center = QGroundControl.flightMapPosition
        }
        if (historyGuard && historyGuard.alive) _syncHistoryGnssLayer()
    }
    onEnabledChanged: _syncHistoryGnssLayer()

    onZoomLevelChanged: {
        if (_saveZoomLevelSetting) {
            QGroundControl.flightMapZoom = zoomLevel
        }
    }
    onCenterChanged: {
        QGroundControl.flightMapPosition = center
    }

    // ============================================================================
    // 区块 2：地图平移与无人机自动追踪逻辑
    // ============================================================================
    Connections {
        target: gesture
        function onPanStarted() {       _disableVehicleTracking = true }
        function onFlickStarted() {     _disableVehicleTracking = true }
        function onPanFinished() {      panRecenterTimer.restart() }
        function onFlickFinished() {    panRecenterTimer.restart() }
    }

    function pointInRect(point, rect) {
        return point.x > rect.x && point.x < rect.x + rect.width && point.y > rect.y && point.y < rect.y + rect.height;
    }

    property real _animatedLatitudeStart
    property real _animatedLatitudeStop
    property real _animatedLongitudeStart
    property real _animatedLongitudeStop
    property real animatedLatitude
    property real animatedLongitude

    onAnimatedLatitudeChanged: _root.center = QtPositioning.coordinate(animatedLatitude, animatedLongitude)
    onAnimatedLongitudeChanged: _root.center = QtPositioning.coordinate(animatedLatitude, animatedLongitude)

    NumberAnimation on animatedLatitude { id: animateLat; from: _animatedLatitudeStart; to: _animatedLatitudeStop; duration: 1000 }
    NumberAnimation on animatedLongitude { id: animateLong; from: _animatedLongitudeStart; to: _animatedLongitudeStop; duration: 1000 }

    function animatedMapRecenter(fromCoord, toCoord) {
        _animatedLatitudeStart = fromCoord.latitude
        _animatedLongitudeStart = fromCoord.longitude
        _animatedLatitudeStop = toCoord.latitude
        _animatedLongitudeStop = toCoord.longitude
        animateLat.start()
        animateLong.start()
    }

    function _insetCenterRect() {
        return Qt.rect(toolInsets.leftEdgeCenterInset, toolInsets.topEdgeCenterInset,
                       _root.width - toolInsets.leftEdgeCenterInset - toolInsets.rightEdgeCenterInset,
                       _root.height - toolInsets.topEdgeCenterInset - toolInsets.bottomEdgeCenterInset)
    }

    function _insetCornerRects() {
        return {
            "topleft":      Qt.rect(0,0, toolInsets.leftEdgeTopInset, toolInsets.topEdgeLeftInset),
            "topright":     Qt.rect(_root.width-toolInsets.rightEdgeTopInset,0, toolInsets.rightEdgeTopInset, toolInsets.topEdgeRightInset),
            "bottomleft":   Qt.rect(0,_root.height-toolInsets.bottomEdgeLeftInset, toolInsets.leftEdgeBottomInset, toolInsets.bottomEdgeLeftInset),
            "bottomright":  Qt.rect(_root.width-toolInsets.rightEdgeBottomInset,_root.height-toolInsets.bottomEdgeRightInset, toolInsets.rightEdgeBottomInset, toolInsets.bottomEdgeRightInset)
        }
    }

    function recenterNeeded() {
        var vehiclePoint = _root.fromCoordinate(_activeVehicleCoordinate, false)
        var centerRect = _insetCenterRect()

        if(!pointInRect(vehiclePoint, centerRect)) return true

        var cornerRects = _insetCornerRects()
        if(pointInRect(vehiclePoint, cornerRects["topleft"])) return true
        else if(pointInRect(vehiclePoint, cornerRects["topright"])) return true
        else if(pointInRect(vehiclePoint, cornerRects["bottomleft"])) return true
        else if(pointInRect(vehiclePoint, cornerRects["bottomright"])) return true

        return false
    }

    function updateMapToVehiclePosition() {
        if (_missionPlannerOpen) return
        if (animateLat.running || animateLong.running) return

        if (!_keepMapCenteredOnVehicle && firstVehiclePositionReceived && _activeVehicleCoordinate.isValid && !_disableVehicleTracking) {
            if (_keepVehicleCentered) {
                _root.center = _activeVehicleCoordinate
            } else {
                if (firstVehiclePositionReceived && recenterNeeded()) {
                    var vehiclePoint = _root.fromCoordinate(_activeVehicleCoordinate, false)
                    var centerInsetRect = _insetCenterRect()
                    var centerInsetPoint = Qt.point(centerInsetRect.x + centerInsetRect.width / 2, centerInsetRect.y + centerInsetRect.height / 2)
                    var centerOffset = Qt.point((_root.width / 2) - centerInsetPoint.x, (_root.height / 2) - centerInsetPoint.y)
                    var vehicleOffsetPoint = Qt.point(vehiclePoint.x + centerOffset.x, vehiclePoint.y + centerOffset.y)
                    var vehicleOffsetCoord = _root.toCoordinate(vehicleOffsetPoint, false)
                    animatedMapRecenter(_root.center, vehicleOffsetCoord)
                }
            }
        }
    }

    on_ActiveVehicleCoordinateChanged: {
        if (_missionPlannerOpen) return
        if (_keepMapCenteredOnVehicle && _activeVehicleCoordinate.isValid && !_disableVehicleTracking) {
            _root.center = _activeVehicleCoordinate
        }
    }

    Timer {
        id:         panRecenterTimer
        interval:   10000
        running:    false
        onTriggered: {
            if (_root._missionPlannerOpen) return
            _disableVehicleTracking = false
            updateMapToVehiclePosition()
        }
    }

    Timer {
        interval:       500
        running:        true
        repeat:         true
        onTriggered:    updateMapToVehiclePosition()
    }

    QGCMapPalette { id: mapPal; lightColors: isSatelliteMap }

    Connections {
        target:                 _missionController
        ignoreUnknownSignals:   true
        function onNewItemsFromVehicle() {
            if (_root._missionPlannerOpen) return
            var visualItems = _missionController.visualItems
            if (visualItems && visualItems.count !== 1) {
                mapFitFunctions.fitMapViewportToMissionItems()
                firstVehiclePositionReceived = true
            }
        }
    }

    MapFitFunctions {
        id:                         mapFitFunctions
        map:                        _root
        usePlannedHomePosition:     false
        planMasterController:       _planMasterController
    }

    // ============================================================================
    // 区块 3：地图图层与多机数据可视化
    // ============================================================================

    ObstacleDistanceOverlayMap { id: obstacleDistance; showText: !pipMode }

    MapItemView {
        id: trajectoryItemView
        property var trajectoryColors: ["red", "green", "blue", "magenta", "darkorange", "cyan", "purple", "gold"]
        model: QGroundControl.multiVehicleManager.vehicles

        delegate: MapPolyline {
            id:          trajectoryPolyline
            line.width:  3
            z:           QGroundControl.zOrderTrajectoryLines
            visible:     !pipMode

            line.color: {
                if (object && object.id) {
                    var colorIndex = object.id % trajectoryItemView.trajectoryColors.length;
                    return trajectoryItemView.trajectoryColors[colorIndex];
                }
                return "red";
            }

            Connections {
                target: object ? object.trajectoryPoints : null
                onPointAdded:      trajectoryPolyline.addCoordinate(coordinate)
                onUpdateLastPoint: trajectoryPolyline.replaceCoordinate(trajectoryPolyline.pathLength() - 1, coordinate)
                onPointsCleared:   trajectoryPolyline.path = []
            }
        }
    }

    MapItemView {
        model: QGroundControl.multiVehicleManager.vehicles
        delegate: VehicleMapItem {
            vehicle:        object
            coordinate:     object.coordinate
            map:            _root
            size:           pipMode ? ScreenTools.defaultFontPixelHeight : ScreenTools.defaultFontPixelHeight * 3
            z:              QGroundControl.zOrderVehicles

            opacity: (_root.selectedSwarmIds.length === 0 || _root.selectedSwarmIds.indexOf(object.id) !== -1) ? 1.0 : 0.4

            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
        }
    }

    MapItemView {
        model: QGroundControl.multiVehicleManager.vehicles
        delegate: MapQuickItem {
            coordinate: object.coordinate
            anchorPoint.x: sourceItem.width / 2
            anchorPoint.y: sourceItem.height / 2
            z: QGroundControl.zOrderVehicles + 1
            visible: !pipMode
            sourceItem: Item {
                property bool selected: _root.selectedSwarmIds.indexOf(object.id) !== -1
                property bool dimmed: _root.selectedSwarmIds.length > 0 && !selected
                width: selected ? 54 : 38
                height: width
                opacity: dimmed ? 0.42 : 1.0
                Rectangle {
                    anchors.centerIn: parent
                    visible: parent.selected
                    width: parent.width + 10
                    height: width
                    radius: width / 2
                    color: Qt.rgba(1.0, 0.78, 0.20, 0.10)
                    border.color: Qt.rgba(1.0, 0.78, 0.20, 0.32)
                    border.width: 1
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width
                    height: parent.height
                    radius: width / 2
                    color: "transparent"
                    border.color: parent.selected ? "#FFD56A" : Qt.rgba(1.0, 1.0, 1.0, 0.45)
                    border.width: parent.selected ? 3 : 1
                }
            }
        }
    }

    MapItemView {
        model: QGroundControl.multiVehicleManager.vehicles
        delegate: ProximityRadarMapView {
            vehicle:        object
            coordinate:     object.coordinate
            map:            _root
            z:              QGroundControl.zOrderVehicles
        }
    }

    MapItemView {
        model: QGroundControl.adsbVehicleManager.adsbVehicles
        delegate: VehicleMapItem {
            coordinate:     object.coordinate
            altitude:       object.altitude
            callsign:       object.callsign
            heading:        object.heading
            alert:          object.alert
            map:            _root
            size:           pipMode ? ScreenTools.defaultFontPixelHeight : ScreenTools.defaultFontPixelHeight * 2.5
            z:              QGroundControl.zOrderVehicles
        }
    }

    Repeater {
        model: QGroundControl.multiVehicleManager.vehicles
        PlanMapItems {
            map:                    _root
            largeMapView:           !pipMode
            planMasterController:   masterController
            vehicle:                _vehicle
            visible:                _vehicle && _activeVehicle && _vehicle.id === _activeVehicle.id
            property var _vehicle: object

            PlanMasterController {
                id: masterController
                Component.onCompleted: startStaticActiveVehicle(object)
            }
        }
    }

    MapItemView {
        model: pipMode ? undefined : _missionController.directionArrows
        delegate: MapLineArrow {
            fromCoord:      object ? object.coordinate1 : undefined
            toCoord:        object ? object.coordinate2 : undefined
            arrowPosition:  2
            z:              QGroundControl.zOrderWaypointLines
        }
    }

    CustomMapItems { map: _root; largeMapView: !pipMode }

    GeoFenceMapVisuals {
        map:                    _root
        myGeoFenceController:   _geoFenceController
        interactive:            false
        planView:               false
        homePosition:           _activeVehicle && _activeVehicle.homePosition.isValid ? _activeVehicle.homePosition :  QtPositioning.coordinate()
    }

    MapItemView {
        model: _rallyPointController.points
        delegate: MapQuickItem {
            id:             itemIndicator
            anchorPoint.x:  sourceItem.anchorPointX
            anchorPoint.y:  sourceItem.anchorPointY
            coordinate:     object.coordinate
            z:              QGroundControl.zOrderMapItems

            sourceItem: MissionItemIndexLabel {
                id:         itemIndexLabel
                label:      qsTr("R", "rally point map item label")
            }
        }
    }

    MapItemView {
        model: _activeVehicle ? _activeVehicle.cameraTriggerPoints : 0
        delegate: CameraTriggerIndicator { coordinate: object.coordinate; z: QGroundControl.zOrderTopMost }
    }

    // ============================================================================
    // 区块 4：交互式视觉指示器 (UI 动效)
    // ============================================================================

    MapQuickItem {
        id:             gotoLocationItem
        visible:        false
        z:              QGroundControl.zOrderMapItems
        anchorPoint.x:  sourceItem.width / 2
        anchorPoint.y:  sourceItem.height / 2

        sourceItem: Item {
            width: 60
            height: 60
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: targetDot.top
                anchors.bottomMargin: 6
                width: labelText.width + 16
                height: labelText.height + 6
                color: "#B3000000"
                radius: 4
                border.color: qgcPal.buttonHighlight
                border.width: 1

                Text {
                    id: labelText
                    anchors.centerIn: parent
                    text: "目标航点"
                    color: qgcPal.buttonHighlight
                    font.bold: true
                    font.pixelSize: ScreenTools.defaultFontPixelSize * 0.9
                }
            }

            Rectangle {
                id: targetDot
                anchors.centerIn: parent
                width: 14
                height: 14
                radius: 7
                color: qgcPal.buttonHighlight
                border.color: "white"
                border.width: 2
            }

            Rectangle {
                anchors.centerIn: targetDot
                width: 30
                height: 30
                radius: 15
                color: "transparent"
                border.color: qgcPal.buttonHighlight
                border.width: 2

                SequentialAnimation on scale {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.8; to: 1.5; duration: 1200; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 1.5; to: 0.8; duration: 1200; easing.type: Easing.InOutQuad }
                }
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.8; to: 0.0; duration: 1200; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 0.0; to: 0.8; duration: 1200; easing.type: Easing.InOutQuad }
                }
            }
        }

        property bool inGotoFlightMode: _activeVehicle ? _activeVehicle.flightMode === _activeVehicle.gotoFlightMode : false

        onInGotoFlightModeChanged: {
            if (!inGotoFlightMode && gotoLocationItem.visible) {
                gotoLocationItem.visible = false
            }
        }

        Connections {
            target: QGroundControl.multiVehicleManager
            function onActiveVehicleChanged(activeVehicle) {
                if (!activeVehicle) gotoLocationItem.visible = false
            }
        }

        function show(coord) {
            gotoLocationItem.coordinate = coord
            gotoLocationItem.visible = true
        }

        function hide() { gotoLocationItem.visible = false }
        function actionConfirmed() {}
        function actionCancelled() { hide() }
    }

    QGCMapCircleVisuals {
        id:             orbitMapCircle
        mapControl:     parent
        mapCircle:      _mapCircle
        visible:        false

        property alias center:              _mapCircle.center
        property alias clockwiseRotation:   _mapCircle.clockwiseRotation
        readonly property real defaultRadius: 30

        Connections {
            target: QGroundControl.multiVehicleManager
            function onActiveVehicleChanged(activeVehicle) {
                if (!activeVehicle) orbitMapCircle.visible = false
            }
        }

        function show(coord) {
            _mapCircle.radius.rawValue = defaultRadius
            orbitMapCircle.center = coord
            orbitMapCircle.visible = true
        }

        function hide() { orbitMapCircle.visible = false }
        function actionConfirmed() { hide() }
        function actionCancelled() { hide() }
        function radius() { return _mapCircle.radius.rawValue }

        Component.onCompleted: globals.guidedControllerFlyView.orbitMapCircle = orbitMapCircle

        QGCMapCircle {
            id:                 _mapCircle
            interactive:        true
            radius.rawValue:    30
            showRotation:       true
            clockwiseRotation:  true
        }
    }

    MapQuickItem {
        id:             roiLocationItem
        visible:        _activeVehicle && _activeVehicle.isROIEnabled
        z:              QGroundControl.zOrderMapItems
        anchorPoint.x:  sourceItem.anchorPointX
        anchorPoint.y:  sourceItem.anchorPointY
        sourceItem: MissionItemIndexLabel {
            checked:    true
            index:      -1
            label:      qsTr("ROI here", "Make this a Region Of Interest")
        }

        function show(coord) { roiLocationItem.coordinate = coord }
        function hide() {}
        function actionConfirmed() {}
        function actionCancelled() {}
    }

    QGCMapCircleVisuals {
        id:              orbitTelemetryCircle
        mapControl:     parent
        mapCircle:      _activeVehicle ? _activeVehicle.orbitMapCircle : null
        visible:        _activeVehicle ? _activeVehicle.orbitActive : false
    }

    MapQuickItem {
        id:             orbitCenterIndicator
        anchorPoint.x:  sourceItem.anchorPointX
        anchorPoint.y:  sourceItem.anchorPointY
        coordinate:     _activeVehicle ? _activeVehicle.orbitMapCircle.center : QtPositioning.coordinate()
        visible:        orbitTelemetryCircle.visible

        sourceItem: MissionItemIndexLabel {
            checked:    true
            index:      -1
            label:      qsTr("Orbit", "Orbit waypoint")
        }
    }

    // Planning layers are inspection only; these paths never feed the old
    // Goto/temporary mission adapter. Labels also distinguish return paths.
    MapPolygon {
        path: _root._missionAreaPath
        visible: _root._missionPlannerOpen && !pipMode && path.length >= 3
        color: Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g,
                       qgcPal.buttonHighlight.b, 0.12)
        border.color: qgcPal.buttonHighlight
        border.width: 2
        z: QGroundControl.zOrderTrajectoryLines + 1
    }
    MapQuickItem {
        visible: _root._missionPlannerOpen && !pipMode && _root._missionAreaPath.length >= 3
            && _root._missionBoundaryPolicyText.length > 0
        coordinate: _root._missionAreaPath.length ? _root._missionAreaPath[0] : QtPositioning.coordinate()
        z: QGroundControl.zOrderMapItems
        anchorPoint.y: sourceItem.height + 24
        sourceItem: Rectangle {
            width: ScreenTools.defaultFontPixelWidth * 40
            height: boundaryPolicyLabel.implicitHeight + 12
            color: qgcPal.window
            border.color: qgcPal.buttonHighlight
            radius: 4
            QGCLabel {
                id: boundaryPolicyLabel
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                anchors.margins: 6
                text: _root._missionBoundaryPolicyText
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
            }
        }
    }
    MapItemView {
        model: _root._missionPlannerOpen && !pipMode ? _root._missionExclusionPaths : []
        delegate: MapPolygon {
            path: modelData.path
            color: Qt.rgba(qgcPal.colorOrange.r, qgcPal.colorOrange.g, qgcPal.colorOrange.b, 0.2)
            border.color: qgcPal.colorOrange
            border.width: 2
            z: QGroundControl.zOrderTrajectoryLines + 2
        }
    }
    MapItemView {
        model: _root._missionPlannerOpen && !pipMode ? _root._missionExclusionPaths : []
        delegate: MapQuickItem {
            coordinate: modelData.path[0]
            z: QGroundControl.zOrderMapItems
            sourceItem: QGCLabel {
                text: qsTr("排除区 %1 · 待审几何，非净空证明").arg(modelData.number)
            }
        }
    }
    MapItemView {
        model: _root._missionPlannerOpen && !pipMode ? _root._missionAreaPath : []
        delegate: MapQuickItem {
            coordinate: modelData
            z: QGroundControl.zOrderMapItems
            anchorPoint.x: sourceItem.width / 2
            anchorPoint.y: sourceItem.height / 2
            sourceItem: Rectangle {
                width: vertexLabel.implicitWidth + 12
                height: vertexLabel.implicitHeight + 8
                radius: 4
                color: qgcPal.window
                border.color: qgcPal.buttonHighlight
                QGCLabel {
                    id: vertexLabel
                    anchors.centerIn: parent
                    text: qsTr("点 %1").arg(index + 1)
                }
            }
        }
    }
    MapItemView {
        model: _root.visible && _root.enabled && !pipMode && _root._historyGnssGeometry.available ? _root._historyGnssGeometry.points : []
        delegate: MapQuickItem {
            coordinate: modelData.coordinate
            z: QGroundControl.zOrderTopMost + 205
            anchorPoint.x: sourceItem.width / 2
            anchorPoint.y: sourceItem.height
            sourceItem: Rectangle {
                width: Math.min(ScreenTools.defaultFontPixelWidth * 18, Math.max(44, _root.width - 16))
                height: historyPointText.implicitHeight + 8
                color: qgcPal.window
                border.width: 2
                border.color: modelData.quality === "caution" ? qgcPal.warningText : qgcPal.text
                radius: 3
                QGCLabel {
                    id: historyPointText
                    width: parent.width - 8; anchors.centerIn: parent
                    textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: qsTr("历史G%1 · UAV%2/%3\n#%4 · fix%5 · %6").arg(modelData.group).arg(modelData.vehicleId).arg(modelData.componentId)
                        .arg(modelData.sequence).arg(modelData.fix).arg(modelData.quality === "caution" ? qsTr("阈值提示") : modelData.quality === "unknown" ? qsTr("质量未知") : qsTr("显示阈值内"))
                    Accessible.role: Accessible.StaticText; Accessible.name: text
                }
                // Including the label: do not pass a historical coordinate to
                // the map's legacy click/double-click Goto interaction.
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.AllButtons
                    preventStealing: true
                    propagateComposedEvents: false
                    onPressed: mouse.accepted = true
                    onReleased: mouse.accepted = true
                    onClicked: mouse.accepted = true
                    onDoubleClicked: mouse.accepted = true
                    onPressAndHold: mouse.accepted = true
                }
            }
        }
    }
    Rectangle {
        visible: _root.visible && _root.enabled && !pipMode && _root._historyGnssGeometry.available
        anchors.left: parent.left; anchors.bottom: parent.bottom
        anchors.margins: Math.max(8, ScreenTools.defaultFontPixelWidth)
        width: Math.max(1, Math.min(ScreenTools.defaultFontPixelWidth * 44, _root.width - 16))
        height: historyGnssLegend.implicitHeight + 16
        z: QGroundControl.zOrderTopMost + 220
        color: qgcPal.window; border.color: qgcPal.text; radius: 4
        ColumnLayout {
            id: historyGnssLegend
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8
            spacing: 8
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; font.bold: true; text: _root._historyGnssGeometry.origin === "native-page" ? qsTr("已明确选择：当前原生档案页GNSS历史点，非实际航迹") : _root._historyGnssGeometry.origin === "session-snapshot" ? qsTr("已明确选择：本进程冻结历史报告，未认证，非当前飞机") : qsTr("已明确选择：独立文件历史报告，未认证，未恢复active或权限") }
            Flickable {
                Layout.fillWidth: true; Layout.preferredHeight: Math.min(historyGnssLegendText.implicitHeight, ScreenTools.defaultFontPixelHeight * 6)
                contentWidth: width; contentHeight: historyGnssLegendText.implicitHeight
                clip: true; boundsBehavior: Flickable.StopAtBounds
                activeFocusOnTab: true; Accessible.role: Accessible.Pane; Accessible.name: qsTr("历史GNSS图例，使用上下键或翻页键滚动")
                Keys.onPressed: {
                    var next = contentY
                    if (event.key === Qt.Key_Down) next += ScreenTools.defaultFontPixelHeight * 2
                    else if (event.key === Qt.Key_Up) next -= ScreenTools.defaultFontPixelHeight * 2
                    else if (event.key === Qt.Key_PageDown) next += height
                    else if (event.key === Qt.Key_PageUp) next -= height
                    else if (event.key === Qt.Key_Home) next = 0
                    else if (event.key === Qt.Key_End) next = contentHeight
                    else return
                    contentY = Math.max(0, Math.min(next, contentHeight - height)); event.accepted = true
                }
                ScrollIndicator.vertical: ScrollIndicator {}
                QGCLabel {
                    id: historyGnssLegendText; width: parent.width
                    textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                    text: _root._historyGnssGeometry.available ? qsTr("原定位报告%1；底图显示%2；极区省略%3（|纬度|>85.05112878°，原值仍在历史报告）\n来源组%4；阈值提示%5 / 质量未知%6 / 显示阈值内%7。HDOP无单位，不是米。\n原遥测文件摘要：%8\n未投点%9；后续页%10；Footer%11；尾部不完整%12。点未连线，不诊断漂移、精度、真实性、完整性、任务或安全，不自动移动视野。")
                        .arg(_root._historyGnssGeometry.report.locatedSampleCount).arg(_root._historyGnssGeometry.points.length).arg(_root._historyGnssGeometry.omittedPolar)
                        .arg(_root._historyGnssGeometry.report.groupCount).arg(_root._historyGnssGeometry.report.qualityCautionSampleCount)
                        .arg(_root._historyGnssGeometry.report.qualityUnknownSampleCount).arg(_root._historyGnssGeometry.report.withinThresholdSampleCount)
                        .arg(_root._historyGnssGeometry.report.sourceIdentity.fileHash).arg(_root._historyGnssGeometry.report.unlocatedSampleCount)
                        .arg(_root._historyGnssGeometry.report.hasMore ? qsTr("有") : qsTr("无"))
                        .arg(_root._historyGnssGeometry.report.footerPresent ? qsTr("有") : qsTr("无"))
                        .arg(_root._historyGnssGeometry.report.partialTail ? qsTr("有") : qsTr("无")) : ""
                }
                Rectangle { anchors.fill: parent; color: "transparent"; border.width: parent.activeFocus ? 2 : 0; border.color: qgcPal.buttonHighlight }
            }
            QGCButton {
                Layout.fillWidth: true; Layout.minimumHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2)
                focusPolicy: Qt.StrongFocus; text: qsTr("关闭此历史图层（不停止飞机）"); Accessible.name: text
                property var press: null
                onPressed: press = {owner: _root._historyGnssArchive, token: _root._historyGnssSelection.selectionToken}
                onCanceled: press = null
                onClicked: { var g = _root._historyGnssLife, value = press; press = null; if (g.alive && value) _root._closeHistoryGnssLayer(value.owner, value.token) }
                Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true
            }
        }
        MouseArea { anchors.fill: parent; z: -1; acceptedButtons: Qt.AllButtons; onPressed: mouse.accepted = true; onClicked: mouse.accepted = true; onDoubleClicked: mouse.accepted = true }
    }
    MapItemView {
        model: _root._startupMapActive && _root._showStartupReferenceLayer && !pipMode && _root._startupMapGeometry.available ? _root._startupMapGeometry.segments : []
        delegate: MapPolyline {
            visible: modelData.kind !== "vertical-takeoff"
            path: modelData.path
            line.width: modelData.kind === "takeoff-request-connection" ? 4 : 2
            line.color: modelData.kind === "takeoff-request-connection" ? qgcPal.buttonHighlight : qgcPal.colorOrange
            z: QGroundControl.zOrderTrajectoryLines + 3
        }
    }
    MapItemView {
        model: _root._startupMapActive && _root._showStartupReferenceLayer && !pipMode && _root._startupMapGeometry.available ? _root._startupMapGeometry.markers : []
        delegate: MapQuickItem {
            coordinate: modelData.coordinate
            z: QGroundControl.zOrderMapItems + 1
            anchorPoint.x: sourceItem.width / 2
            anchorPoint.y: sourceItem.height
            sourceItem: Rectangle {
                width: Math.min(ScreenTools.defaultFontPixelWidth * 44, Math.max(100, _root.width - 32))
                height: startupSegmentLabel.implicitHeight + 12
                color: qgcPal.window
                border.color: qgcPal.buttonHighlight
                border.width: 2
                QGCLabel {
                    id: startupSegmentLabel
                    width: parent.width - 12
                    anchors.centerIn: parent
                    textFormat: Text.PlainText; wrapMode: Text.Wrap
                    text: qsTr("UAV-%1 · %2 类独立名义段\n%3\n%4")
                        .arg(modelData.vehicleId).arg(modelData.lines.length).arg(modelData.lines.join("\n"))
                        .arg(_root._startupSpatialReview.current === true ? qsTr("冻结审阅参考，非实际航迹") : qsTr("过期历史，非当前状态"))
                }
            }
        }
    }
    MapItemView {
        model: _root._startupMapActive && _root._showStartupMissionRoutes && !pipMode ? _root._startupMissionGeometry.routes : []
        delegate: MapPolyline {
            path:modelData.path
            line.width:modelData.returning ? 2 : 6
            line.color:modelData.returning ? qgcPal.colorOrange : qgcPal.buttonHighlight
            z:QGroundControl.zOrderTrajectoryLines+1
        }
    }
    MapItemView {
        model: _root._startupMapActive && _root._showStartupMissionRoutes && !pipMode ? _root._startupMissionGeometry.routeMarkers : []
        delegate: MapQuickItem {
            coordinate:modelData.coordinate
            anchorPoint.x:sourceItem.width+8
            anchorPoint.y:-8
            z:QGroundControl.zOrderMapItems
            sourceItem:Rectangle {
                width:Math.min(ScreenTools.defaultFontPixelWidth*30,Math.max(100,_root.width-32))
                height:archivedRouteLabel.implicitHeight+8
                color:qgcPal.window;border.width:1;border.color:qgcPal.buttonHighlight
                QGCLabel {
                    id:archivedRouteLabel
                    width:parent.width-8;anchors.centerIn:parent;textFormat:Text.PlainText;wrapMode:Text.Wrap
                    text:qsTr("机 %1 · 原计划出航 %2 点（粗线）\n规划返程 %3 点（细线，非 RTL）\n非实际航迹；AMSL 非离地高").arg(modelData.vehicleId).arg(modelData.outboundCount).arg(modelData.returnCount)
                }
            }
        }
    }
    MapItemView {
        model: _root._startupMapActive && _root._showStartupActionRequests && !pipMode ? _root._startupMissionGeometry.markers : []
        delegate: MapQuickItem {
            coordinate:modelData.coordinate
            anchorPoint.x:-8
            anchorPoint.y:-8
            z:QGroundControl.zOrderMapItems+2
            sourceItem:Rectangle {
                width:Math.min(ScreenTools.defaultFontPixelWidth*32,Math.max(100,_root.width-32))
                height:archivedActionLabel.implicitHeight+10
                color:qgcPal.window;border.width:2;border.color:qgcPal.colorOrange
                QGCLabel {
                    id:archivedActionLabel
                    width:parent.width-10;anchors.centerIn:parent;textFormat:Text.PlainText;wrapMode:Text.Wrap
                    text:qsTr("机 %1 · 同 XY 的声明动作\n%2\n不是已执行结果").arg(modelData.vehicleId).arg(modelData.lines.join("\n"))
                }
            }
        }
    }
    MapItemView {
        model: _root._missionPlannerOpen && !pipMode ? _root._missionProposalRoutes : []
        delegate: MapPolyline {
            path: modelData.path
            line.width: modelData.returning ? 2 : 4
            line.color: modelData.returning ? qgcPal.colorOrange : qgcPal.buttonHighlight
            z: QGroundControl.zOrderTrajectoryLines + 2
        }
    }
    MapItemView {
        model: _root._missionPlannerOpen && !pipMode ? _root._missionProposalRoutes : []
        delegate: MapQuickItem {
            coordinate: modelData.path[0]
            z: QGroundControl.zOrderMapItems
            anchorPoint.x: sourceItem.width / 2
            anchorPoint.y: sourceItem.height
            sourceItem: Rectangle {
                width: routeLabel.implicitWidth + 12
                height: routeLabel.implicitHeight + 8
                radius: 4
                color: qgcPal.window
                border.color: modelData.returning ? qgcPal.colorOrange : qgcPal.buttonHighlight
                QGCLabel {
                    id: routeLabel
                    anchors.centerIn: parent
                    textFormat: Text.PlainText
                    text: modelData.reserveDispatch === true
                        ? qsTr("UAV-%1 · 独立出勤 · %2 · %3\n新记录 %4… / 版本 %5").arg(modelData.vehicleId)
                            .arg(modelData.returning ? qsTr("自身返程") : qsTr("出航"))
                            .arg(_root._missionRemainderReview.current === true ? qsTr("规划") : qsTr("过期历史"))
                            .arg(modelData.recordId.substring(0, 8)).arg(modelData.recordRevision)
                        : modelData.remainder === true
                        ? qsTr("UAV-%1 · %2 · %3\n新记录 %4… / 版本 %5").arg(modelData.vehicleId)
                            .arg(modelData.returning ? qsTr("返程") : qsTr("出航"))
                            .arg(_root._missionRemainderReview.current === true ? qsTr("规划") : qsTr("过期历史"))
                            .arg(modelData.recordId.substring(0, 8)).arg(modelData.recordRevision)
                        : qsTr("UAV-%1 %2（提案）").arg(modelData.vehicleId)
                            .arg(modelData.returning ? qsTr("返回") : qsTr("出航"))
                }
            }
        }
    }

    // ============================================================================
    // 区块 4.1：Shift 临时任务的草稿轨迹与编号节点
    // ============================================================================

    // 绘制排队未下发的指点航线折线轨迹
    MapPolyline {
        id:          shiftQueuePolyline
        line.width:  3
        line.color:  _root.shiftQueuedCoords.length > 0 ? "#D8B56A" : qgcPal.buttonHighlight
        z:           QGroundControl.zOrderTrajectoryLines + 1
        visible:     !pipMode && (_root.shiftQueuedCoords.length > 0 || _root.shiftCommittedCoords.length > 0)
        path:        _root.shiftQueuedCoords.length > 0 ? _root.shiftQueuedCoords : _root.shiftCommittedCoords
    }

    // 动态生成每个排队位置的节点序号标签
    MapItemView {
        model: _root.shiftQueuedCoords.length > 0 ? _root.shiftQueuedCoords : _root.shiftCommittedCoords
        delegate: MapQuickItem {
            coordinate: modelData
            anchorPoint.x: sourceItem.width / 2
            anchorPoint.y: sourceItem.height / 2
            z: QGroundControl.zOrderTopMost
            sourceItem: Rectangle {
                width: 18
                height: 18
                radius: 9
                color: _root.shiftQueuedCoords.length > 0 ? "#D8B56A" : qgcPal.buttonHighlight
                border.color: "white"
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: index + 1 // 显示实时点击序号 (从 1 开始)
                    color: "white"
                    font.pixelSize: 10
                    font.bold: true
                }
            }
        }
    }

    // ============================================================================
    // 区块 5：全局鼠标交互与菜单 (支持框选和右键平移)
    // ============================================================================

    // 根节点持有键盘焦点，确保 Shift 释放能够结束同一批草稿。
    focus: true
    Keys.onPressed: {
        if (event.key === Qt.Key_Escape && _missionPlannerOpen) {
            if (_startupMapActive) _exitStartupSpatialMap(_startupSpatialReview.captureToken)
            else if (_missionMapReviewOnly) _setMissionMapReviewOnly(false)
            else if (_missionMapPicking) _missionMapPicking = false
            else _setMissionPlannerOpen(false)
            event.accepted = true
        }
    }
    Keys.onReleased: {
        if (_missionPlannerOpen || _missionPlanningEpochExhausted) return
        if (event.key === Qt.Key_Shift) {
            // 松开 Shift 只进入确认流程，确认前不上传临时任务。
            if (_root.shiftQueuedCoords.length > 0) {
                _root.requestBatchQueuedFly()
            }
        }
    }

    // 框选视觉矩形（半透明深蓝底色 + 暗金边框）

    Rectangle {
        id: marqueeBox
        visible: false
        radius: 8
        color: "transparent"
        border.color: _root.shiftQueuedCoords.length > 0 ? "#D8B56A" : qgcPal.buttonHighlight
        border.width: 2
        opacity: 0.0
        z: QGroundControl.zOrderTopMost + 100
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, 0.24) }
            GradientStop { position: 1.0; color: Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, 0.06) }
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: 4
            radius: Math.max(2, parent.radius - 3)
            color: "transparent"
            border.color: Qt.rgba(qgcPal.buttonHighlight.r, qgcPal.buttonHighlight.g, qgcPal.buttonHighlight.b, 0.34)
            border.width: 1
        }

        Behavior on opacity {
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }
    }

    // 全局拖拽与点击事件分流层
    MouseArea {
        id: mapInteraction
        enabled: !reviewController.inspecting
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: false
        preventStealing: true
        onWheel: wheel.accepted = false

        property var startPoint: Qt.point(0, 0)
        property var lastPanPoint: Qt.point(0, 0)
        property bool isDrawingBox: false
        property bool isDragging: false
        property double pressEpoch: -1
        property bool pressWasPlanning: false

        onCanceled: {
            isDrawingBox = false
            marqueeBox.visible = false
            isDragging = false
            pressEpoch = -1
        }

        // 左键点击弹出的快捷操作菜单

        Popup {
            id: clickMenu
            modal: true
            property var coord
            property double interactionEpoch: -1
            function actionsAllowed() {
                return !_root._missionPlannerOpen && !_root._missionPlanningEpochExhausted
                        && interactionEpoch === _root._missionPlanningEpoch
            }

            function setCoordinates(mouseX, mouseY) {
                var newX = mouseX
                var newY = mouseY

                if (newX + clickMenu.width > _root.width) {
                    newX = _root.width - clickMenu.width
                }
                if (newY + clickMenu.height > _root.height) {
                    newY = _root.height - clickMenu.height
                }
                x = newX
                y = newY
            }

            background: Rectangle {
                radius: ScreenTools.defaultFontPixelHeight * 0.5
                color: qgcPal.window
                border.color: qgcPal.text
            }

            ColumnLayout {
                id: mainLayout
                spacing: ScreenTools.defaultFontPixelWidth / 2

                QGCButton {
                    Layout.fillWidth: true
                    text: "Go to location"
                    visible: globals.guidedControllerFlyView.showGotoLocation
                    onClicked: {
                        if (!clickMenu.actionsAllowed()) return
                        if (clickMenu.opened) clickMenu.close()
                        gotoLocationItem.show(clickMenu.coord)
                        if (!clickMenu.actionsAllowed()) return
                        globals.guidedControllerFlyView.confirmAction(globals.guidedControllerFlyView.actionGoto, clickMenu.coord, gotoLocationItem)
                    }
                }

                QGCButton {
                    Layout.fillWidth: true
                    text: "Orbit at location"
                    visible: globals.guidedControllerFlyView.showOrbit
                    onClicked: {
                        if (!clickMenu.actionsAllowed()) return
                        if (clickMenu.opened) clickMenu.close()
                        orbitMapCircle.show(clickMenu.coord)
                        if (!clickMenu.actionsAllowed()) return
                        globals.guidedControllerFlyView.confirmAction(globals.guidedControllerFlyView.actionOrbit, clickMenu.coord, orbitMapCircle)
                    }
                }

                QGCButton {
                    Layout.fillWidth: true
                    text: "ROI at location"
                    visible: globals.guidedControllerFlyView.showROI
                    onClicked: {
                        if (!clickMenu.actionsAllowed()) return
                        if (clickMenu.opened) clickMenu.close()
                        roiLocationItem.show(clickMenu.coord)
                        if (!clickMenu.actionsAllowed()) return
                        globals.guidedControllerFlyView.confirmAction(globals.guidedControllerFlyView.actionROI, clickMenu.coord, roiLocationItem)
                    }
                }

                QGCButton {
                    Layout.fillWidth: true
                    text: "Set home here"
                    visible: globals.guidedControllerFlyView.showSetHome
                    onClicked: {
                        if (!clickMenu.actionsAllowed()) return
                        if (clickMenu.opened) clickMenu.close()
                        if (!clickMenu.actionsAllowed()) return
                        globals.guidedControllerFlyView.confirmAction(globals.guidedControllerFlyView.actionSetHome, clickMenu.coord)
                    }
                }
            }
        }

        onPressed: {
            isDragging = false
            pressEpoch = _root._missionPlanningEpoch
            pressWasPlanning = _root._missionPlannerOpen
            startPoint = Qt.point(mouse.x, mouse.y)
            lastPanPoint = startPoint
            _root.forceActiveFocus() // 地图重新取得焦点，避免 Shift 释放事件遗失。

            if (pressWasPlanning || _root._missionPlanningEpochExhausted) {
                isDrawingBox = false
                marqueeBox.visible = false
                return
            }

            if (mouse.button === Qt.LeftButton) {
                startPoint = Qt.point(mouse.x, mouse.y)
                marqueeBox.x = mouse.x
                marqueeBox.y = mouse.y
                marqueeBox.width = 0
                marqueeBox.height = 0
                marqueeBox.opacity = 0.92
                isDrawingBox = true
                _disableVehicleTracking = true
            } else if (mouse.button === Qt.RightButton) {
                lastPanPoint = Qt.point(mouse.x, mouse.y)
                _disableVehicleTracking = true
            }
        }

        onPositionChanged: {
            if (pressEpoch !== _root._missionPlanningEpoch
                    || _root._missionPlanningEpochExhausted) return
            if (pressWasPlanning || _root._missionPlannerOpen) {
                if (!pressWasPlanning || !_root._missionPlannerOpen) return
                if (Math.abs(mouse.x - startPoint.x) > 3 || Math.abs(mouse.y - startPoint.y) > 3) {
                    isDragging = true
                }
                if (isDragging && (mouse.buttons & (Qt.LeftButton | Qt.RightButton))) {
                    var planningCenter = _root.fromCoordinate(_root.center, false)
                    _root.center = _root.toCoordinate(Qt.point(
                        planningCenter.x - (mouse.x - lastPanPoint.x),
                        planningCenter.y - (mouse.y - lastPanPoint.y)), false)
                }
                lastPanPoint = Qt.point(mouse.x, mouse.y)
                return
            }
            if (Math.abs(mouse.x - startPoint.x) > 3 || Math.abs(mouse.y - startPoint.y) > 3) {
                isDragging = true
            }

            // 窗口切换可能丢失 Shift 释放事件；发现残留草稿时补进确认流程。
            if (_root.shiftQueuedCoords.length > 0 && !(mouse.modifiers & Qt.ShiftModifier)) {
                _root.requestBatchQueuedFly()
            }

            if (isDrawingBox && (mouse.buttons & Qt.LeftButton)) {
                marqueeBox.visible = true
                marqueeBox.x = Math.min(startPoint.x, mouse.x)
                marqueeBox.y = Math.min(startPoint.y, mouse.y)
                marqueeBox.width = Math.abs(mouse.x - startPoint.x)
                marqueeBox.height = Math.abs(mouse.y - startPoint.y)
            } else if (mouse.buttons & Qt.RightButton) {
                isDragging = true
                var deltaX = mouse.x - lastPanPoint.x
                var deltaY = mouse.y - lastPanPoint.y

                var centerPix = _root.fromCoordinate(_root.center, false)
                var newCenterPix = Qt.point(centerPix.x - deltaX, centerPix.y - deltaY)

                _root.center = _root.toCoordinate(newCenterPix, false)
                lastPanPoint = Qt.point(mouse.x, mouse.y)
            }
        }

        onReleased: {
            if (pressEpoch !== _root._missionPlanningEpoch || pressWasPlanning
                    || _root._missionPlannerOpen || _root._missionPlanningEpochExhausted) {
                isDrawingBox = false
                marqueeBox.visible = false
                return
            }
            panRecenterTimer.restart()

            if (mouse.button === Qt.LeftButton && isDrawingBox) {
                isDrawingBox = false
                if (isDragging) {
                    _selectVehiclesInMarquee()
                }
                marqueeBox.opacity = 0.0
                var delayTimer = Qt.createQmlObject('import QtQuick 2.0; Timer { interval: 150; running: true; repeat: false; onTriggered: { marqueeBox.visible = false } }', parent)
            }
        }

        onClicked: {
            if (pressEpoch !== _root._missionPlanningEpoch || _root._missionPlanningEpochExhausted) return
            if (pressWasPlanning || _root._missionPlannerOpen) {
                if (pressWasPlanning && _root._missionPlannerOpen && _root._missionMapPicking
                        && !_root._missionRemainderSelected && !_root._missionMapReviewOnly
                        && !isDragging && mouse.button === Qt.RightButton && _root._missionDraft) {
                    var areaPoint = _root.toCoordinate(Qt.point(mouse.x, mouse.y), false)
                    if (areaPoint && areaPoint.isValid) {
                        _root._missionDraft.appendVertex({ lat: areaPoint.latitude, lon: areaPoint.longitude })
                    }
                }
                return
            }
            if (isDragging) return;
            var clickCoord = _root.toCoordinate(Qt.point(mouse.x, mouse.y), false)

            if (mouse.button === Qt.RightButton) {
                // Shift 建立可预览的临时任务草稿；普通右键保持即时指点语义。
                if (mouse.modifiers & Qt.ShiftModifier) {
                    // 草稿只记录坐标，不立即下发飞控。
                    if (_root.shiftQueuedCoords.length === 0 && !_root._freezeShiftTargets()) return
                    var tempCoords = _root.shiftQueuedCoords.slice(0)
                    tempCoords.push(clickCoord)
                    _root.shiftCommittedCoords = []
                    _root.shiftQueuedCoords = tempCoords // 重新赋值以触发 QML 绑定刷新。

                    // 用排队动效区分“草稿”与已经下发的任务。
                    showMobaClickAnimation(clickCoord, "#D8B56A")
                } else {
                    // 普通右键执行单点指令，并清理不属于本次操作的残留草稿。
                    if (_root.shiftQueuedCoords.length > 0) _root._cancelShiftDraft()
                    executeRightClickFly(clickCoord)
                }
            }
            else if (mouse.button === Qt.LeftButton) {
                if (!globals.guidedControllerFlyView.guidedUIVisible && (globals.guidedControllerFlyView.showGotoLocation || globals.guidedControllerFlyView.showOrbit || globals.guidedControllerFlyView.showROI || globals.guidedControllerFlyView.showSetHome)) {
                    orbitMapCircle.hide()
                    gotoLocationItem.hide()
                    clickMenu.coord = clickCoord
                    clickMenu.interactionEpoch = _root._missionPlanningEpoch
                    clickMenu.setCoordinates(mouse.x, mouse.y)
                    clickMenu.open()
                }
            }
        }
    }

    // 地理坐标转换与无人机筛选逻辑
    function _selectVehiclesInMarquee() {
        if (marqueeBox.width < 5 || marqueeBox.height < 5) return;
        var topLeftGeo = _root.toCoordinate(Qt.point(marqueeBox.x, marqueeBox.y), false)
        var bottomRightGeo = _root.toCoordinate(Qt.point(marqueeBox.x + marqueeBox.width, marqueeBox.y + marqueeBox.height), false)

        var maxLat = Math.max(topLeftGeo.latitude, bottomRightGeo.latitude)
        var minLat = Math.min(topLeftGeo.latitude, bottomRightGeo.latitude)
        var maxLon = Math.max(topLeftGeo.longitude, bottomRightGeo.longitude)
        var minLon = Math.min(topLeftGeo.longitude, bottomRightGeo.longitude)

        var vehicleList = QGroundControl.multiVehicleManager.vehicles
        var selectedVehicleIds = []

        for (var i = 0; i < vehicleList.count; i++) {
            var vehicle = vehicleList.get(i)
            var coord = vehicle.coordinate

            if (coord.latitude >= minLat && coord.latitude <= maxLat &&
                coord.longitude >= minLon && coord.longitude <= maxLon) {
                selectedVehicleIds.push(vehicle.id)
            }
        }

        if (_root.shiftQueuedCoords.length > 0) _root._cancelShiftDraft()
        console.log("框选成功！选中的无人机 ID 集合: ", JSON.stringify(selectedVehicleIds))
        selectedSwarmIds = selectedVehicleIds

        if (selectedVehicleIds.length > 0) {
            var firstSelectedVehicle = QGroundControl.multiVehicleManager.getVehicleById(selectedVehicleIds[0])
            if (firstSelectedVehicle) {
                suppressExclusiveSelectionSync = true
                QGroundControl.multiVehicleManager.activeVehicle = firstSelectedVehicle
                suppressExclusiveSelectionSync = false
                _showCurrentTaskRoute(firstSelectedVehicle.id)
            }
        }
    }

    MapScale {
        id:                 mapScale
        anchors.margins:    _toolsMargin
        anchors.left:       parent.left
        anchors.bottom:     parent.bottom
        anchors.leftMargin: _toolsMargin
        anchors.bottomMargin: _toolsMargin
        mapControl:         _root
        buttonsOnLeft:      false
        visible:            !ScreenTools.isTinyScreen && QGroundControl.corePlugin.options.flyView.showMapScale && mapControl.pipState.state === mapControl.pipState.windowState
        property real centerInset: visible ? parent.height - y : 0
    }

    // ============================================================================
    // 区块 6：自定义 MOBA 风格右键指点飞行与编队逻辑
    // ============================================================================

    // Shift 草稿在确认时冻结目标 ID、参考位置和坐标，三者必须成组传入执行层。
    function _showSwarmCommandResult(result, actionTitle) {
        if (!result) return

        if (!result.ok && result.message && result.message.length > 0) {
            mainWindow.showMessageDialog(actionTitle, result.message)
            return
        }

        if (result.skippedIds && result.skippedIds.length > 0) {
            mainWindow.showMessageDialog(
                qsTr("部分指令已跳过"),
                qsTr("以下无人机未满足任务下发条件（解锁、高度、定位或任务上传空闲状态）：\n无人机 ") + JSON.stringify(result.skippedIds)
            )
        }
    }

    function requestBatchQueuedFly() {
        if (_root._missionPlannerOpen || _root._missionPlanningEpochExhausted) return
        var requestEpoch = _root._missionPlanningEpoch
        if (_root.shiftQueuedCoords.length === 0 || _root.shiftMissionConfirmPending) return

        _root.shiftReplacementRequired = swarmController.hasActiveTemporaryMission(_root.shiftFrozenVehicleIds)
        _root.shiftMissionConfirmPending = true
        _root._shiftMissionApproved = false
        var guidedController = globals.guidedControllerFlyView
        if (!guidedController) {
            _root._cancelShiftDraft()
            return
        }
        if (_root._missionPlannerOpen || requestEpoch !== _root._missionPlanningEpoch) return
        guidedController.confirmAction(
            guidedController.actionQueuedMission,
            {
                vehicleIds: _root.shiftFrozenVehicleIds.slice(0),
                waypointCount: _root.shiftQueuedCoords.length,
                replacementRequired: _root.shiftReplacementRequired
            },
            shiftMissionConfirmationAdapter)
    }

    function _storeTaskRoutes(ids, references, coordinates, dispatchedIds) {
        if (!ids || ids.length === 0 || ids.length !== references.length) return

        var centerLat = 0
        var centerLon = 0
        for (var i = 0; i < references.length; i++) {
            centerLat += references[i].latitude
            centerLon += references[i].longitude
        }
        var centroid = QtPositioning.coordinate(centerLat / references.length, centerLon / references.length)
        var routes = _root._cloneRouteMap()

        for (var vehicleIndex = 0; vehicleIndex < ids.length; vehicleIndex++) {
            if (dispatchedIds && dispatchedIds.indexOf(ids[vehicleIndex]) === -1) continue
            var route = []
            for (var pointIndex = 0; pointIndex < coordinates.length; pointIndex++) {
                var distance = centroid.distanceTo(coordinates[pointIndex])
                var azimuth = centroid.azimuthTo(coordinates[pointIndex])
                var vehiclePoint = references[vehicleIndex].atDistanceAndAzimuth(distance, azimuth)
                vehiclePoint.altitude = coordinates[pointIndex].altitude
                route.push(vehiclePoint)
            }
            routes[String(ids[vehicleIndex])] = route
        }
        _root.activeTaskRoutes = routes
        _root._showCurrentTaskRoute(_root._activeVehicle ? _root._activeVehicle.id : -1)
    }

    function _removeTaskRoutes(ids) {
        var routes = _root._cloneRouteMap()
        for (var i = 0; ids && i < ids.length; i++) delete routes[String(ids[i])]
        _root.activeTaskRoutes = routes
        _root._showCurrentTaskRoute(_root._activeVehicle ? _root._activeVehicle.id : -1)
    }

    function executeBatchQueuedFly() {
        if (_root._missionPlannerOpen || _root._missionPlanningEpochExhausted) {
            _root._cancelShiftDraft(true)
            return
        }
        var executionEpoch = _root._missionPlanningEpoch
        if (_shiftMissionApproved !== true) return
        _shiftMissionApproved = false
        _root.shiftMissionConfirmPending = false
        var coordsArray = _root.shiftQueuedCoords.slice(0)
        var frozenIds = _root.shiftFrozenVehicleIds.slice(0)
        var frozenReferences = _root.shiftFrozenReferences.slice(0)
        var replaceExisting = _root.shiftReplacementRequired
        _root.shiftQueuedCoords = []

        if (coordsArray.length === 0) return

        // GuidedActionConfirm calls the map adapter after the controller's
        // no-op actionQueuedMission branch returns.  Re-evaluate the health
        // gate at this final dispatch boundary so a vehicle that went stale
        // while the dialog was open cannot receive a temporary mission.
        var guidedController = globals.guidedControllerFlyView
        if (guidedController && guidedController.fleetHealthCenter) {
            var gate = guidedController.fleetHealthCenter.evaluateTaskGate(frozenIds, false)
            if (!gate || !gate.allowed) {
                _root._cancelShiftDraft(true)
                var reasons = gate && gate.reasons ? gate.reasons : []
                var lines = []
                for (var reasonIndex = 0; reasonIndex < reasons.length && reasonIndex < 4; reasonIndex++) {
                    var reason = reasons[reasonIndex] || {}
                    var vehicle = Number(reason.vehicleId) > 0 ? qsTr("UAV-%1：").arg(reason.vehicleId) : ""
                    lines.push(vehicle + (reason.message || reason.code || qsTr("健康状态未确认")))
                }
                mainWindow.showMessageDialog(qsTr("临时任务已阻止"),
                                             lines.length > 0 ? lines.join("\n")
                                                              : qsTr("舰队健康门禁未通过。"))
                return
            }
        }

        console.log("Uploading temporary mission route. queued points: ", coordsArray.length)

        if (typeof orbitMapCircle !== "undefined") orbitMapCircle.hide()
        if (typeof roiLocationItem !== "undefined") roiLocationItem.hide()

        gotoLocationItem.show(coordsArray[coordsArray.length - 1])

        if (_root._missionPlannerOpen || _root._missionPlanningEpochExhausted
                || executionEpoch !== _root._missionPlanningEpoch) return
        var result = swarmController.executeQueuedGoto(frozenIds, coordsArray, frozenReferences, replaceExisting)
        if (result && result.ok) {
            _storeTaskRoutes(frozenIds, frozenReferences, coordsArray, result.dispatchedIds)
        }
        _root.shiftFrozenVehicleIds = []
        _root.shiftFrozenReferences = []
        _root.shiftReplacementRequired = false
        _showSwarmCommandResult(result, qsTr("实时航点队列"))
    }
    function executeRightClickFly(targetCoordinate) {
        if (_root._missionPlannerOpen || _root._missionPlanningEpochExhausted) return
        var requestEpoch = _root._missionPlanningEpoch
        if (typeof orbitMapCircle !== "undefined") orbitMapCircle.hide()
        if (typeof roiLocationItem !== "undefined") roiLocationItem.hide()

        showMobaClickAnimation(targetCoordinate)
        gotoLocationItem.show(targetCoordinate)
        var guidedController = globals.guidedControllerFlyView
        if (!guidedController) return

        // Normal right-click Goto must use the same explicit confirmation,
        // frozen target IDs, health gate and transaction path as the map
        // action menu. A direct SwarmController call here could otherwise
        // dispatch while the operator is still selecting a target.
        var frozenIds = selectedSwarmIds && selectedSwarmIds.length > 0
                ? selectedSwarmIds.slice(0) : []
        if (_root._missionPlannerOpen || _root._missionPlanningEpochExhausted
                || requestEpoch !== _root._missionPlanningEpoch) return
        if (frozenIds.length > 0) {
            guidedController.confirmAction(
                        guidedController.actionGoto,
                        { targetCoordinate: targetCoordinate, vehicleIds: frozenIds },
                        gotoLocationItem)
        } else {
            guidedController.confirmAction(guidedController.actionGoto,
                                           targetCoordinate, gotoLocationItem)
        }
    }
    function showMobaClickAnimation(coord, customColorStr) {
        var finalColor = customColorStr ? customColorStr : qgcPal.buttonHighlight
        mobaClickIndicator.applyDynamicColor(finalColor)

        mobaClickIndicator.coordinate = coord
        mobaClickIndicator.visible = true
        mobaClickIndicator.resetAndStart()
    }

    MapQuickItem {
        id: mobaClickIndicator
        coordinate: QtPositioning.coordinate(0, 0)
        visible: false
        anchorPoint.x: sourceItem.width / 2
        anchorPoint.y: sourceItem.height / 2
        z: QGroundControl.zOrderTopMost

        function resetAndStart() {
            outerRing.scale = 0.25
            outerRing.opacity = 0.72
            middleRing.scale = 0.18
            middleRing.opacity = 0.42
            waypointHalo.opacity = 0.45
            clickAnimation.restart()
        }

        function applyDynamicColor(colorValue) {
            waypointDot.color = colorValue
            waypointStem.color = colorValue
            waypointHalo.border.color = colorValue
            middleRing.border.color = colorValue
            outerRing.border.color = colorValue
        }

        sourceItem: Item {
            width: 88
            height: 88

            Rectangle {
                id: waypointHalo
                anchors.centerIn: parent
                width: 24
                height: 24
                radius: 12
                color: "transparent"
                border.color: qgcPal.buttonHighlight
                border.width: 2
                opacity: 0.45
            }

            Rectangle {
                id: waypointStem
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: waypointDot.bottom
                width: 2
                height: 12
                radius: 1
                color: qgcPal.buttonHighlight
                opacity: 0.9
            }

            Rectangle {
                id: waypointDot
                anchors.centerIn: parent
                width: 9
                height: 9
                radius: 5
                color: qgcPal.buttonHighlight
                border.color: Qt.rgba(1, 1, 1, 0.72)
                border.width: 1
            }

            Rectangle {
                id: middleRing
                anchors.centerIn: parent
                width: 52
                height: 52
                radius: 26
                color: "transparent"
                border.color: qgcPal.buttonHighlight
                border.width: 2
                opacity: 0
            }

            Rectangle {
                id: outerRing
                anchors.centerIn: parent
                width: 68
                height: 68
                radius: 34
                color: "transparent"
                border.color: qgcPal.buttonHighlight
                border.width: 3
                opacity: 0
            }

            SequentialAnimation {
                id: clickAnimation
                ParallelAnimation {
                    NumberAnimation { target: middleRing; property: "scale"; from: 0.18; to: 1.25; duration: 520; easing.type: Easing.OutCubic }
                    NumberAnimation { target: middleRing; property: "opacity"; from: 0.42; to: 0.0; duration: 520; easing.type: Easing.OutQuad }
                    NumberAnimation { target: outerRing; property: "scale"; from: 0.25; to: 1.65; duration: 680; easing.type: Easing.OutCubic }
                    NumberAnimation { target: outerRing; property: "opacity"; from: 0.72; to: 0.0; duration: 680; easing.type: Easing.OutQuad }
                    NumberAnimation { target: waypointHalo; property: "opacity"; from: 0.45; to: 0.12; duration: 680; easing.type: Easing.OutQuad }
                }
                onStopped: mobaClickIndicator.visible = false
            }
        }
    }
    MapCircle {
        center: reviewController.anchor
        radius: reviewController.rangeAvailable ? reviewController.radiusMeters : 0
        visible: reviewController.rangeAvailable
        color: reviewController.energyOnly ? "#1cd8b56a" : "#2639d5ad"
        border.color: reviewController.energyOnly ? "transparent" : "#39d5ad"
        border.width: reviewController.energyOnly ? 0 : 3
    }

    function reviewDashSegments() {
        if (!reviewController.rangeAvailable || !reviewController.energyOnly) return []
        var result = []
        for (var azimuth = 0; azimuth < 360; azimuth += 20) {
            result.push([
                reviewController.anchor.atDistanceAndAzimuth(reviewController.radiusMeters, azimuth),
                reviewController.anchor.atDistanceAndAzimuth(reviewController.radiusMeters, azimuth + 10)
            ])
        }
        return result
    }

    MapItemView {
        model: _root.reviewDashSegments()
        delegate: MapPolyline {
            path: modelData
            line.width: 3
            line.color: "#d8b56a"
        }
    }

    CommandCenterOverlay {
        id: commandCenterOverlay
        anchors.fill: parent
        z: QGroundControl.zOrderTopMost + 200
        visible: !pipMode
        selectedIds: _root.selectedSwarmIds
        vehicles: QGroundControl.multiVehicleManager.vehicles
        activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
        toolInsets: _root.toolInsets
        mapControl: _root
        reviewController: reviewController

        onVehicleSelectionRequested: _root.setVehicleSelection(vehicleId, selected)
        onVehicleFocusRequested: _root.focusVehicle(vehicleId)
        onClearSelectionRequested: _root.clearVehicleSelection()
        onMissionPlannerRequested: _root._setMissionPlannerOpen(true)
    }

    Popup {
        id: missionPlannerPopup
        visible: _root._missionPlannerOpen && !pipMode
        modal: false
        focus: true
        closePolicy: Popup.NoAutoClose
        padding: 0
        width: Math.max(1, Math.min(ScreenTools.defaultFontPixelWidth * 58, _root.width - 16))
        height: Math.max(1, Math.min(_root.height - y - 16,
                            _root._missionMapReviewOnly ? missionReviewHeader.implicitHeight + 16
                            : _root._missionMapPicking ? ScreenTools.defaultFontPixelHeight * 16
                                                     : _root.height - y - 16))
        x: _root._missionMapReviewOnly ? Math.max(8, _root.width - width - 8)
                                      : Math.max(8, (_root.width - width) / 2)
        y: Math.min(Math.max(8, _root.toolInsets ? _root.toolInsets.topEdgeCenterInset + 8 : 8),
                    Math.max(8, _root.height - 120))
        z: QGroundControl.zOrderTopMost + 210
        background: Rectangle {
            color: qgcPal.window
            border.color: qgcPal.buttonHighlight
            radius: 6
        }
        contentItem: ColumnLayout {
            spacing: 0
            ColumnLayout {
                id: missionReviewHeader
                visible: _root._startupMapActive || _root._missionRemainderSelected || _root._missionMapReviewOnly
                Layout.fillWidth: true
                Layout.margins: visible ? 8 : 0
                spacing: 8
                QGCButton {
                    id: missionMapReviewButton
                    property string pressedStartupToken: ""
                    property string pressedStartupKey: ""
                    property double pressedEpoch: -1
                    property bool pressedWasStartup: false
                    Layout.fillWidth: true
                    Layout.minimumHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2)
                    focusPolicy: Qt.StrongFocus
                    text: _root._startupMapActive ? qsTr("结束启动空间地图核对（不停止飞机）")
                         : _root._missionMapReviewOnly ? qsTr("返回规划详情（结束只读地图核对）")
                                                     : qsTr("只读地图核对（收起详情，不发指令）")
                    enabled: !_root._startupMapCallBusy
                    Accessible.name: text
                    contentItem: QGCLabel {
                        text: missionMapReviewButton.text
                        textFormat: Text.PlainText
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: missionMapReviewButton.pressed || missionMapReviewButton.hovered
                               ? qgcPal.buttonHighlightText : qgcPal.buttonText
                    }
                    onPressed: {
                        pressedEpoch = _root._missionPlanningEpoch
                        pressedWasStartup = _root._startupMapActive
                        pressedStartupToken = pressedWasStartup ? _root._startupSpatialReview.captureToken : ""
                        pressedStartupKey = _root._startupSpatialSelectionKey
                    }
                    onCanceled: { pressedStartupToken = ""; pressedStartupKey = ""; pressedEpoch = -1 }
                    onClicked: {
                        var token = pressedStartupToken, epoch = pressedEpoch, wasStartup = pressedWasStartup, key = pressedStartupKey
                        pressedStartupToken = ""
                        pressedStartupKey = ""
                        pressedEpoch = -1
                        if (epoch !== _root._missionPlanningEpoch || wasStartup !== _root._startupMapActive
                                || key !== _root._startupSpatialSelectionKey) return
                        if (wasStartup) _root._exitStartupSpatialMap(token)
                        else _root._setMissionMapReviewOnly(!_root._missionMapReviewOnly)
                        forceActiveFocus()
                    }
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.width: missionMapReviewButton.activeFocus ? 2 : 0
                        border.color: qgcPal.buttonHighlight
                    }
                }
                QGCButton {
                    id: centerStartupButton
                    property string pressedCapture: ""
                    property string pressedSourceKey: ""
                    property double pressedEpoch: -1
                    Layout.fillWidth: true
                    Layout.minimumHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2)
                    visible: _root._startupMapActive
                    enabled: _root._startupSpatialCenter !== null
                    focusPolicy: Qt.StrongFocus
                    text: qsTr("定位当前显示图层首点（只移动地图视野）")
                    contentItem: QGCLabel { text: centerStartupButton.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: centerStartupButton._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    onPressed: { pressedCapture = _root._startupSpatialReview.captureToken; pressedSourceKey = _root._startupSpatialSelectionKey; pressedEpoch = _root._missionPlanningEpoch }
                    onCanceled: { pressedCapture = ""; pressedSourceKey = ""; pressedEpoch = -1 }
                    onClicked: {
                        if (_root._startupMapActive && _root._startupSpatialCenter !== null
                                && pressedEpoch === _root._missionPlanningEpoch && pressedSourceKey === _root._startupSpatialSelectionKey
                                && pressedCapture === _root._startupSpatialReview.captureToken)
                            _root.center = _root._startupSpatialCenter
                        pressedCapture = ""
                        pressedSourceKey = ""
                        pressedEpoch = -1
                    }
                    Rectangle { anchors.fill: parent; color: "transparent"; border.width: centerStartupButton.activeFocus ? 2 : 0; border.color: qgcPal.buttonHighlight }
                }
                QGCComboBox {
                    id:startupLayerChoice
                    Layout.fillWidth:true
                    Layout.minimumHeight:Math.max(44,ScreenTools.defaultFontPixelHeight*2)
                    visible:_root._startupMapActive
                    focusPolicy:Qt.StrongFocus
                    Accessible.name:qsTr("选择启动捕获的独立地图图层；不重新审阅或发送指令")
                    model:[qsTr("全部独立图层（不是联合验证航迹）"),qsTr("仅启动名义参考段"),qsTr("仅原计划出航 / 规划返程"),qsTr("仅声明 TAKEOFF / HOLD / LAND 请求")]
                    currentIndex:_root._startupSpatialLayer
                    onActivated:{_root._setStartupSpatialLayer(index);currentIndex=Qt.binding(function(){return _root._startupSpatialLayer})}
                    Rectangle{anchors.fill:parent;color:"transparent";border.width:startupLayerChoice.activeFocus?2:0;border.color:qgcPal.buttonHighlight}
                }
                Flickable {
                    id: missionMapLegendViewport
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(missionMapLegend.implicitHeight, ScreenTools.defaultFontPixelHeight * 8)
                    contentWidth: width
                    contentHeight: missionMapLegend.implicitHeight
                    clip: true
                    activeFocusOnTab: true
                    Accessible.role: Accessible.Pane
                    Accessible.name: _root._startupMapActive ? qsTr("启动空间地图图例，使用上下键或翻页键滚动")
                        : _root._missionRemainderReview.kind === "reserve-dispatch" ? qsTr("独立备用出勤地图图例，使用上下键或翻页键滚动")
                        : qsTr("余段地图图例，使用上下键或翻页键滚动")
                    boundsBehavior: Flickable.StopAtBounds
                    flickableDirection: Flickable.VerticalFlick
                    Keys.onPressed: {
                        var nextY = contentY
                        if (event.key === Qt.Key_Down) nextY += ScreenTools.defaultFontPixelHeight * 2
                        else if (event.key === Qt.Key_Up) nextY -= ScreenTools.defaultFontPixelHeight * 2
                        else if (event.key === Qt.Key_PageDown) nextY += height
                        else if (event.key === Qt.Key_PageUp) nextY -= height
                        else if (event.key === Qt.Key_Home) nextY = 0
                        else if (event.key === Qt.Key_End) nextY = contentHeight
                        else return
                        contentY = Math.max(0, Math.min(nextY, contentHeight - height))
                        event.accepted = true
                    }
                    ScrollIndicator.vertical: ScrollIndicator { }
                    QGCLabel {
                        id: missionMapLegend
                        width: parent.width
                        textFormat: Text.PlainText
                        wrapMode: Text.WrapAnywhere
                        text: _root._startupMapActive ? _root._startupSpatialMapSummary() : _root._missionRemainderMapSummary()
                    }
                    Rectangle {
                        parent: missionMapLegendViewport
                        anchors.fill: parent
                        color: "transparent"
                        border.width: missionMapLegendViewport.activeFocus ? 2 : 0
                        border.color: qgcPal.buttonHighlight
                    }
                }
            }
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: !_root._missionMapReviewOnly
                Layout.minimumHeight: 0
                Layout.maximumHeight: _root._missionMapReviewOnly ? 0 : Number.POSITIVE_INFINITY
                Layout.preferredHeight: _root._missionMapReviewOnly ? 0 : -1
                clip: true
                enabled: !_root._missionMapReviewOnly
                // Do not hide or recreate the editor: child onVisibleChanged
                // handlers would retire capture/consent and lose pending input.
                FleetMissionPlannerPanel {
                    anchors.fill: parent
                    draft: _root._missionDraft
                    controller: _root._missionDraftController
                    selectedVehicleIds: _root.selectedSwarmIds
                    mapPicking: _root._missionMapPicking
                    previewCurrent: controller ? controller.missionDraftPreviewCurrent : false
                    reviewResult: controller ? controller.missionDraftPreview : ({})
                    onCloseRequested: _root._setMissionPlannerOpen(false)
                    onMapPickingRequested: _root._missionMapPicking = enabled === true && _root._missionPlannerOpen
                        && !_root._missionRemainderSelected && !_root._missionMapReviewOnly
                    onPreviewRequested: {
                        _root._missionMapPicking = false
                        if (controller) controller.prepareMissionDraft()
                    }
                }
            }
        }
    }
}
