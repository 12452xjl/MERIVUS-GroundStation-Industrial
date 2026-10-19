import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Fixed local historical inspection only; no recorder, raw-page or Vehicle API.
FocusScope {
    id: root
    property var archive: null
    property var lifetime: ({alive: true, ticket: null, mapLease: null})
    property bool expanded: false
    property bool callBusy: false
    property bool readConfirmed: false
    property bool positionValues: false
    property bool reviewConfirmed: false
    property bool gnssConfirmed: false
    property double inputEpoch: 0
    property double consentEpoch: 0
    property bool exhausted: false
    property string selectedBasename: ""
    property string vehicleFilter: ""
    property string messageFilter: ""
    property double searchCriteriaEpoch: 0
    property string selectedSearchKey: ""
    property string feedback: ""
    property var pending: null
    property var savedCatalog: null
    property var savedPage: null
    property var savedReport: null
    property var savedGnss: null
    property var savedSearch: null
    property int audienceIndex: 2
    property bool showViewJson: false
    property bool showDossierJson: false
    property string cancelledId: ""
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var status: archive ? archive.current || {} : ({})
    readonly property var options: archive ? archive.options || {} : ({})
    readonly property string optionsStamp: optionStamp(options)
    readonly property string latestId: typeof status.latestHistoryId === "string" ? status.latestHistoryId : ""
    readonly property bool sourceReady: lifetime.alive && !exhausted && !!archive
        && typeof archive.request === "function" && optionsStamp.length > 0
        && typeof status.historyBusy === "boolean" && typeof status.captureEnabled === "boolean"
    readonly property bool busy: status.historyBusy === true
    readonly property bool canRead: sourceReady && !callBusy && !busy && status.captureEnabled === false
        && ["disabled", "stopped", "failed"].indexOf(status.state) >= 0
    readonly property bool catalogCurrent: !!savedCatalog && savedCatalog.owner === archive
        && savedCatalog.optionsStamp === optionsStamp && archive.historyCatalog.catalogId === savedCatalog.dto.catalogId
    readonly property var entries: catalogCurrent ? savedCatalog.dto.entries : []
    readonly property var selectedEntry: findEntry(selectedBasename)
    readonly property bool filterValid: vehicleFilter === "" || (/^[1-9][0-9]{0,2}$/.test(vehicleFilter) && Number(vehicleFilter) <= 255)
    readonly property string filterId: vehicleFilter === "" ? "0" : vehicleFilter
    readonly property bool messageFilterValid: messageFilter === "" || messageFilter === "-1"
        || (/^(0|[1-9][0-9]{0,7})$/.test(messageFilter) && Number(messageFilter) <= 16777215)
    readonly property string messageFilterId: messageFilter === "" ? "-1" : messageFilter
    readonly property bool searchReady: sourceReady && typeof archive.requestSearch === "function"
    readonly property bool searchCurrent: sourceReady && !!savedSearch && savedSearch.owner === archive
        && savedSearch.optionsStamp === optionsStamp && !busy && status.historyResultAccepted === true
        && status.historyCancelled === false && savedSearch.dto.queryId === latestId
        && map(archive.historySearch) && archive.historySearch.queryId === latestId && archive.historySearch.searchId === savedSearch.dto.searchId
        && archive.historySearch.manifestHash === savedSearch.dto.manifestHash
    readonly property bool searchInputsCurrent: searchCurrent && savedSearch.criteriaEpoch === searchCriteriaEpoch
        && filterValid && messageFilterValid && savedSearch.intent.vehicleId === filterId
        && savedSearch.intent.messageId === messageFilterId
    readonly property var searchRows: savedSearch ? savedSearch.dto.rows : []
    readonly property var selectedSearchHit: findSearchHit(selectedSearchKey)
    readonly property bool pageCurrent: !!savedPage && savedPage.owner === archive && savedPage.optionsStamp === optionsStamp
        && !busy && status.historyResultAccepted === true && status.historyCancelled === false
        && savedPage.dto.queryId === latestId && archive.historyPage.queryId === latestId
        && savedPage.dto.fileHash === archive.historyPage.fileHash
    readonly property bool pageInputsCurrent: pageCurrent && selectedBasename === savedPage.intent.basename
        && filterId === savedPage.intent.vehicleId && positionValues === savedPage.intent.includePositionValues
    readonly property bool reportCurrent: pageInputsCurrent && !!savedReport && savedReport.owner === archive
        && savedReport.dto.queryId === latestId && archive.motionReview.queryId === latestId
        && savedReport.dto.fileHash === savedPage.dto.fileHash
    readonly property var reportRows: reportCurrent ? segmentRows(savedReport.dto) : []
    readonly property bool gnssCurrent: pageInputsCurrent && !!savedGnss && savedGnss.owner === archive
        && savedGnss.optionsStamp === optionsStamp && gnssKnown(savedGnss.dto)
        && archive.gnssReviewToken === savedGnss.reviewToken
        && JSON.stringify(savedGnss.dto) === savedGnss.stamp
        && JSON.stringify(archive.gnssReview) === savedGnss.stamp
    readonly property var gnssRows: gnssCurrent ? savedGnss.dto.samples : []
    readonly property bool dossierCurrent: reportCurrent && !!savedReport.dossier
        && savedReport.dossier.dossierId === archive.motionDossier.dossierId
        && savedReport.dossier.contentHash === archive.motionDossier.contentHash
    readonly property string audience: audienceIndex >= 0 && audienceIndex < 4
        ? ["commander", "customer", "engineer", "safety"][audienceIndex] : ""
    readonly property var selectedView: dossierCurrent && audience.length ? savedReport.dossier.views[audience] : null
    readonly property bool canCancel: sourceReady && !callBusy && busy && !!pending
        && pending.owner === archive && pending.optionsStamp === optionsStamp
        && pending.id === latestId && pending.id === status.pendingHistoryId && cancelledId !== pending.id
    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    implicitHeight: form.implicitHeight
    QGCPalette { id: pal; colorGroupEnabled: root.enabled }

    function map(value) { return !!value && typeof value === "object" && !Array.isArray(value) }
    function exact(value, keys) {
        if (!map(value) || Object.keys(value).length !== keys.length) return false
        for (var i = 0; i < keys.length; ++i) if (!Object.prototype.hasOwnProperty.call(value, keys[i])) return false
        return true
    }
    function integer(value, min, max) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= min && value <= max }
    function identifier(value) { return typeof value === "string" && /^[A-Za-z0-9_-]{1,128}$/.test(value) }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function selectionTokenValid(value, reviewToken) {
        if (typeof value !== "string" || !uuid(reviewToken) || value.indexOf(reviewToken + ":") !== 0) return false
        var serial = value.substring(reviewToken.length + 1)
        return /^[1-9][0-9]{0,15}$/.test(serial) && (serial.length < 16 || serial <= "9007199254740991")
    }
    function digest(value) { return typeof value === "string" && /^[0-9a-f]{64}$/.test(value) }
    function decimal(value) { return typeof value === "string" && /^(0|[1-9][0-9]{0,18})$/.test(value) && (value.length < 19 || value <= "9223372036854775807") }
    function sourceSequenceValid(value) { return typeof value === "string" && /^[1-9][0-9]{0,19}$/.test(value) && (value.length < 20 || value <= "18446744073709551615") }
    function copy(value) { try { return JSON.parse(JSON.stringify(value)) } catch (error) { return null } }
    function known(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function numberText(value) { return typeof value === "number" && isFinite(value) && value >= 0 ? value.toFixed(2) : qsTr("未知 / 样本不足") }
    function searchStatusText(value) { return value === "matched" ? qsTr("已验证且有命中") : value === "no-match" ? qsTr("已验证但无命中") : qsTr("不可验证 / 未知") }
    function searchFooterText(value) { return value === "present" ? qsTr("存在") : value === "missing" ? qsTr("缺失") : qsTr("未知") }
    function searchTailText(value) { return value === "partial" ? qsTr("存在未终止尾行") : value === "none" ? qsTr("无未终止尾行") : qsTr("未知") }
    function optionStamp(value) {
        if (!map(value) || Object.keys(value).length !== 7 || value.schemaVersion !== 1
                || typeof value.directoryPath !== "string" || !value.directoryPath.length || value.directoryPath.length > 4096
                || !map(value.readPolicy) || Object.keys(value.readPolicy).length !== 4
                || value.readPolicy.maximumFileMiB !== 64 || value.readPolicy.maximumCatalogFiles !== 128
                || value.readPolicy.maximumPageMiB !== 1 || value.readPolicy.maximumPageFrames !== 64
                || value.pageSize !== 64 || value.historyReadOnly !== true
                || value.positionPayloadPubliclyExposed !== false || value.defaultCaptureEnabled !== false) return ""
        return JSON.stringify([value.directoryPath, value.readPolicy, value.pageSize])
    }
    function inert(value) {
        return map(value) && value.historicalOnly === true && value.readOnly === true
            && value.authenticated === false && value.crcVerified === false && value.permissionsRestored === false
            && value.executionPermit === false && value.executable === false && value.flightCommandReleased === false
            && value.fullTelemetryCaptured === false && value.closureVerified === false
            && value.crossFileAtomicSnapshot === false && value.directoryEnumerationCompletenessVerified === false
    }
    function catalogKnown(value) {
        if (!inert(value) || value.format !== "merivus-fleet-telemetry-catalog" || value.version !== 1
                || !uuid(value.catalogId) || !identifier(value.queryId) || value.directoryInventoryEnumerationCompleted !== true
                || !Array.isArray(value.entries) || value.entries.length > 128) return false
        var seen = {}
        for (var i = 0; i < value.entries.length; ++i) {
            var row = value.entries[i]
            if (!map(row) || Object.keys(row).length !== 8 || !uuid(row.runId)
                    || row.basename !== "fleet-telemetry-" + row.runId + ".ndjson" || seen[row.basename]
                    || typeof row.headerValid !== "boolean" || typeof row.positionTelemetryConsent !== "boolean"
                    || !decimal(row.fileBytes) || !decimal(row.modifiedUtcMs) || typeof row.errorCode !== "string"
                    || (row.headerValid && (!digest(row.headerHash) || row.errorCode !== ""))) return false
            seen[row.basename] = true
        }
        return true
    }
    function pageKnown(value, request) {
        return inert(value) && value.format === "merivus-fleet-telemetry-history-summary" && value.version === 1
            && value.queryId === request.id && value.catalogId === request.intent.catalogId
            && value.basename === request.intent.basename && uuid(value.runId) && digest(value.fileHash)
            && decimal(value.afterSequence) && decimal(value.nextSequence) && decimal(value.totalValidatedFrames)
            && decimal(value.positionWithheldFrames) && decimal(value.lastValidatedSequence)
            && typeof value.hasMore === "boolean" && typeof value.footerPresent === "boolean" && typeof value.partialTail === "boolean"
            && value.payloadPubliclyExposed === false && Array.isArray(value.rows) && value.rows.length <= 64
            && value.returnedFrameCount === value.rows.length
            && (request.intent.action !== "search-open" || value.fileHash === request.openFileHash)
    }
    function decimalAtMost(value, maximum) {
        return decimal(value) && decimal(maximum) && (value.length < maximum.length || (value.length === maximum.length && value <= maximum))
    }
    function searchKnown(value, request) {
        if (!map(value) || Object.keys(value).length !== 36 || value.format !== "merivus-fleet-telemetry-history-search"
                || value.version !== 1 || value.kind !== "search" || value.queryId !== request.id
                || value.catalogId !== request.intent.catalogId || !uuid(value.catalogId) || !uuid(value.searchId)
                || (request.intent.action === "search-next" && value.searchId !== request.intent.searchId)
                || (request.intent.action === "search-next" && (value.manifestHash !== request.manifestHash
                    || value.totalFileCount !== request.totalFileCount))
                || !digest(value.manifestHash) || value.vehicleId !== Number(request.intent.vehicleId)
                || value.messageId !== Number(request.intent.messageId) || !integer(value.vehicleId, 0, 255)
                || !integer(value.messageId, -1, 16777215) || !integer(value.totalFileCount, 0, 128)
                || !integer(value.afterFileIndex, 0, value.totalFileCount) || !integer(value.nextFileIndex, value.afterFileIndex, value.totalFileCount)
                || (request.intent.action === "search-first" && value.afterFileIndex !== 0)
                || (request.intent.action === "search-next" && value.afterFileIndex !== request.afterFileIndex)
                || !Array.isArray(value.rows) || value.rows.length > 64
                || (value.afterFileIndex < value.totalFileCount && value.rows.length === 0)
                || value.inspectedFileCount !== value.rows.length || value.nextFileIndex !== value.afterFileIndex + value.rows.length
                || !integer(value.verifiedFileCount, 0, value.rows.length) || !integer(value.matchedFileCount, 0, value.rows.length)
                || !integer(value.diagnosticFileCount, 0, value.rows.length)
                || typeof value.hasMore !== "boolean" || value.hasMore !== (value.nextFileIndex < value.totalFileCount)
                || typeof value.budgetLimited !== "boolean" || value.readByteBudget !== "136314880"
                || !decimalAtMost(value.reservedReadBytes, value.readByteBudget)
                || value.historicalOnly !== true || value.readOnly !== true || value.payloadPubliclyExposed !== false) return false
        var flags = ["authenticated", "crcVerified", "permissionsRestored", "fullTelemetryCaptured", "executable", "executionPermit",
            "flightCommandReleased", "closureVerified", "crossFileAtomicSnapshot", "directoryEnumerationCompletenessVerified", "fsyncVerified", "durableVerified"]
        for (var f = 0; f < flags.length; ++f) if (value[flags[f]] !== false) return false
        var matched = 0, verified = 0, unavailable = 0
        for (var i = 0; i < value.rows.length; ++i) {
            var row = value.rows[i], descriptor = map(row) ? row.descriptor : null
            if (!map(row) || Object.keys(row).length !== 10 || row.fileIndex !== value.afterFileIndex + i
                    || !map(descriptor) || Object.keys(descriptor).length !== 8 || !uuid(descriptor.runId)
                    || descriptor.basename !== "fleet-telemetry-" + descriptor.runId + ".ndjson"
                    || typeof descriptor.headerValid !== "boolean" || typeof descriptor.positionTelemetryConsent !== "boolean"
                    || !decimal(descriptor.fileBytes) || !decimal(descriptor.modifiedUtcMs) || typeof descriptor.errorCode !== "string"
                    || typeof descriptor.headerHash !== "string" || (descriptor.headerHash !== "" && !digest(descriptor.headerHash))
                    || typeof row.errorCode !== "string" || ["matched", "no-match", "unavailable"].indexOf(row.status) < 0) return false
            if (row.status === "unavailable") {
                if (row.fileHash !== "" || row.matchingFrameCount !== "" || row.firstMatchingSequence !== ""
                        || row.lastMatchingSequence !== "" || row.errorCode === "" || row.footerState !== "unknown" || row.tailState !== "unknown") return false
                unavailable += 1
            } else {
                if (!digest(row.fileHash) || row.errorCode !== "" || !decimal(row.matchingFrameCount)
                        || !decimal(row.firstMatchingSequence) || !decimal(row.lastMatchingSequence)
                        || ["present", "missing"].indexOf(row.footerState) < 0 || ["partial", "none"].indexOf(row.tailState) < 0) return false
                if (row.status === "matched") {
                    if (row.matchingFrameCount === "0" || row.firstMatchingSequence === "0"
                            || !decimalAtMost(row.firstMatchingSequence, row.lastMatchingSequence)) return false
                    matched += 1
                } else if (row.matchingFrameCount !== "0" || row.firstMatchingSequence !== "0" || row.lastMatchingSequence !== "0") return false
                verified += 1
            }
        }
        return matched === value.matchedFileCount && verified === value.verifiedFileCount && unavailable === value.diagnosticFileCount
            && verified + unavailable === value.inspectedFileCount
    }
    function reportKnown(value) {
        if (!map(value) || Object.keys(value).length !== 31 || value.schemaVersion !== 1
                || value.source !== "merivus-fleet-telemetry-motion-review" || value.ok !== true || value.reason !== ""
                || value.positionValuesConfirmed !== true || value.historicalOnly !== true || value.singlePageOnly !== true
                || !pageCurrent || value.queryId !== latestId || value.catalogId !== savedPage.dto.catalogId
                || value.runId !== savedPage.dto.runId || value.fileHash !== savedPage.dto.fileHash
                || value.afterSequence !== savedPage.dto.afterSequence || value.nextSequence !== savedPage.dto.nextSequence
                || value.rowCount !== savedPage.dto.returnedFrameCount || !integer(value.motionRowCount, 0, value.rowCount)
                || !integer(value.acceptedPositionSamples, 0, value.motionRowCount)
                || !integer(value.acceptedTargetSamples, 0, value.motionRowCount)
                || !integer(value.rejectedMotionSamples, 0, value.motionRowCount)
                || value.acceptedPositionSamples + value.acceptedTargetSamples + value.rejectedMotionSamples !== value.motionRowCount
                || !Array.isArray(value.groups) || value.groups.length > 64 || value.groupCount !== value.groups.length
                || !integer(value.segmentCount, 0, 64)) return false
        var flags = ["rawPayloadExposed", "absoluteCoordinatesExposed", "authenticated", "crcVerified", "taskOwnershipVerified",
            "resetFreeVerified", "driftDiagnosed", "trajectorySafetyVerified", "executionPermit", "flightCommandReleased"]
        for (var f = 0; f < flags.length; ++f) if (value[flags[f]] !== false) return false
        var count = 0
        for (var g = 0; g < value.groups.length; ++g) {
            var group = value.groups[g]
            if (!map(group) || Object.keys(group).length !== 5 || !integer(group.groupOrdinal, 1, 64)
                    || !integer(group.vehicleId, 1, 255) || !integer(group.componentId, 1, 255)
                    || !Array.isArray(group.segments) || group.segmentCount !== group.segments.length) return false
            for (var s = 0; s < group.segments.length; ++s) {
                var segment = group.segments[s]
                if (!map(segment) || Object.keys(segment).length !== 16 || !integer(segment.segmentOrdinal, 1, 64)
                        || typeof segment.breakReason !== "string" || segment.breakReason.length > 128) return false
                var names = Object.keys(segment)
                for (var n = 0; n < names.length; ++n) {
                    if (names[n] === "breakReason") continue
                    var number = segment[names[n]]
                    if (typeof number !== "number" || !isFinite(number) || number < -1) return false
                }
                if (++count > 64) return false
            }
        }
        return count === value.segmentCount
    }
    function gnssKnown(value) {
        if (!exact(value, ["schemaVersion", "source", "ok", "reason", "positionValuesConfirmed", "sourceIdentity", "rowCount",
                "gnssSampleCount", "locatedSampleCount", "unlocatedSampleCount", "qualityCautionSampleCount", "qualityUnknownSampleCount",
                "withinThresholdSampleCount", "groupCount", "samples", "displayPolicy", "hasMore", "footerPresent", "partialTail",
                "historicalOnly", "singlePageOnly", "historyComplete", "rawPayloadExposed", "absoluteCoordinatesExposed", "authenticated",
                "crcVerified", "accuracyVerified", "taskOwnershipVerified", "driftDiagnosed", "trajectorySafetyVerified", "executionPermit",
                "flightCommandReleased", "permissionsRestored"]) || value.schemaVersion !== 1
                || value.source !== "merivus-fleet-telemetry-gnss-review" || value.ok !== true || value.reason !== ""
                || value.positionValuesConfirmed !== true || value.historicalOnly !== true || value.singlePageOnly !== true
                || !pageInputsCurrent || !savedPage || savedPage.intent.includePositionValues !== true) return false
        var identity = ["queryId", "catalogId", "runId", "headerHash", "fileHash", "afterSequence", "nextSequence"]
        if (!exact(value.sourceIdentity, identity)) return false
        for (var k = 0; k < identity.length; ++k)
            if (typeof value.sourceIdentity[identity[k]] !== "string" || value.sourceIdentity[identity[k]] !== savedPage.dto[identity[k]]) return false
        var flags = ["historyComplete", "rawPayloadExposed", "authenticated", "crcVerified", "accuracyVerified", "taskOwnershipVerified",
            "driftDiagnosed", "trajectorySafetyVerified", "executionPermit", "flightCommandReleased", "permissionsRestored"]
        for (var f = 0; f < flags.length; ++f) if (value[flags[f]] !== false) return false
        var policy = value.displayPolicy
        if (!exact(policy, ["maximumHdop", "minimumSatellites", "semantics"]) || policy.maximumHdop !== 2.5
                || policy.minimumSatellites !== 6 || policy.semantics !== "display-heuristic-not-accuracy"
                || value.hasMore !== savedPage.dto.hasMore || value.footerPresent !== savedPage.dto.footerPresent
                || value.partialTail !== savedPage.dto.partialTail || value.rowCount !== savedPage.dto.returnedFrameCount
                || !integer(value.rowCount, 0, 64) || !Array.isArray(value.samples) || value.samples.length > value.rowCount
                || value.gnssSampleCount !== value.samples.length || !integer(value.groupCount, 0, value.samples.length)) return false
        var previous = savedPage.dto.afterSequence, groups = {}, sourceSequences = {}, located = 0, caution = 0, unknown = 0, within = 0
        var rowKeys = ["sequence", "sourceEpoch", "sourceSequence", "groupOrdinal", "vehicleId", "componentId", "receivedUtcMs", "fixType",
            "positionKnown", "coordinate", "positionReason", "hdopKnown", "hdop", "satellitesKnown", "satellitesVisible", "qualityCode", "hdopCaution", "satelliteCaution"]
        for (var i = 0; i < value.samples.length; ++i) {
            var row = value.samples[i]
            if (!exact(row, rowKeys) || !decimal(row.sequence) || row.sequence === "0" || decimalAtMost(row.sequence, previous)
                    || !decimalAtMost(row.sequence, savedPage.dto.nextSequence) || !uuid(row.sourceEpoch)
                    || !sourceSequenceValid(row.sourceSequence) || !integer(row.groupOrdinal, 1, 64)
                    || !integer(row.vehicleId, 1, 255) || !integer(row.componentId, 1, 255) || !integer(row.receivedUtcMs, 1, 9007199254740991)
                    || !integer(row.fixType, 0, 255) || typeof row.positionKnown !== "boolean" || typeof row.hdopKnown !== "boolean"
                    || typeof row.satellitesKnown !== "boolean" || typeof row.hdopCaution !== "boolean" || typeof row.satelliteCaution !== "boolean"
                    || typeof row.hdop !== "number" || !isFinite(row.hdop)
                    || (row.hdopKnown ? row.hdop < 0 || row.hdop > 655.34 : row.hdop !== -1)
                    || (row.satellitesKnown ? !integer(row.satellitesVisible, 0, 254) : row.satellitesVisible !== -1)
                    || row.hdopCaution !== (row.hdopKnown && row.hdop > 2.5)
                    || row.satelliteCaution !== (row.satellitesKnown && row.satellitesVisible < 6)) return false
            previous = row.sequence
            var groupKey = JSON.stringify([row.sourceEpoch, row.vehicleId, row.componentId])
            if (groups[groupKey] === undefined) groups[groupKey] = Object.keys(groups).length + 1
            if (row.groupOrdinal !== groups[groupKey]) return false
            var priorSource = sourceSequences[groupKey]
            if (priorSource !== undefined && (row.sourceSequence.length < priorSource.length
                    || (row.sourceSequence.length === priorSource.length && row.sourceSequence <= priorSource))) return false
            sourceSequences[groupKey] = row.sourceSequence
            var pageRow = null
            for (var p = 0; p < savedPage.dto.rows.length; ++p) if (savedPage.dto.rows[p].sequence === row.sequence) pageRow = savedPage.dto.rows[p]
            if (!pageRow || pageRow.messageId !== 24 || pageRow.vehicleId !== row.vehicleId || pageRow.componentId !== row.componentId) return false
            if (row.positionKnown) {
                if ([3, 4, 5, 6, 8].indexOf(row.fixType) < 0 || row.positionReason !== "reported-position"
                        || !exact(row.coordinate, ["latitude", "longitude"]) || typeof row.coordinate.latitude !== "number"
                        || typeof row.coordinate.longitude !== "number" || !isFinite(row.coordinate.latitude) || !isFinite(row.coordinate.longitude)
                        || row.coordinate.latitude < -90 || row.coordinate.latitude > 90 || row.coordinate.longitude < -180 || row.coordinate.longitude > 180) return false
                var quality = row.hdopCaution || row.satelliteCaution ? "caution"
                    : !row.hdopKnown || !row.satellitesKnown ? "unknown" : "within-display-thresholds"
                if (row.qualityCode !== quality) return false
                ++located
                if (quality === "caution") ++caution
                else if (quality === "unknown") ++unknown
                else ++within
            } else {
                var reason = row.fixType === 0 ? "no-gps" : row.fixType === 1 ? "no-fix" : row.fixType === 2 ? "fix-2d-not-used"
                    : row.fixType === 7 ? "static-fix-not-used" : [3, 4, 5, 6, 8].indexOf(row.fixType) >= 0 ? "coordinates-out-of-range" : "unknown-fix"
                if (!exact(row.coordinate, []) || row.positionReason !== reason || row.qualityCode !== "unlocated") return false
            }
        }
        return value.locatedSampleCount === located && value.unlocatedSampleCount === value.samples.length - located
            && value.qualityCautionSampleCount === caution && value.qualityUnknownSampleCount === unknown
            && value.withinThresholdSampleCount === within && value.groupCount === Object.keys(groups).length
            && value.absoluteCoordinatesExposed === (located > 0)
    }
    function gnssRowText(row) {
        return qsTr("组%1 · 机号%2 / 组件%3 · 原帧%4 · 同源序号%5\n来源epoch：%6\n本机接收UTC ms：%7；fixType：%8；位置：%9\nHDOP（无单位，不是米）：%10；卫星数：%11；显示诊断：%12")
            .arg(row.groupOrdinal).arg(row.vehicleId).arg(row.componentId).arg(row.sequence).arg(row.sourceSequence).arg(row.sourceEpoch)
            .arg(row.receivedUtcMs).arg(row.fixType).arg(row.positionKnown ? row.coordinate.latitude.toFixed(7) + ", " + row.coordinate.longitude.toFixed(7) : row.positionReason)
            .arg(row.hdopKnown ? String(row.hdop) : qsTr("未知")).arg(row.satellitesKnown ? String(row.satellitesVisible) : qsTr("未知"))
            .arg(row.qualityCode === "caution" ? qsTr("阈值提示") : row.qualityCode === "unknown" ? qsTr("质量未知") : row.qualityCode === "unlocated" ? qsTr("不投点") : qsTr("在显示阈值内（非精度认证）"))
    }
    function dossierKnown(value, motion) {
        if (!map(value) || Object.keys(value).length !== 21 || value.schemaVersion !== 1
                || value.source !== "merivus-fleet-telemetry-observation-report" || value.ok !== true || value.reason !== ""
                || !digest(value.contentHash) || value.dossierId !== "telemetry-observation-" + value.contentHash
                || value.historicalOnly !== true || value.readOnly !== true || !reportKnown(motion)
                || !map(value.sourceIdentity) || Object.keys(value.sourceIdentity).length !== 6
                || !map(value.coverage) || Object.keys(value.coverage).length !== 8
                || !Array.isArray(value.dataQualityFindings) || value.dataQualityFindings.length > 16
                || !map(value.motionReview) || JSON.stringify(value.motionReview) !== JSON.stringify(motion)
                || !map(value.views) || Object.keys(value.views).length !== 4) return false
        var flags = ["authenticated", "signatureVerified", "taskAssociationVerified", "completionVerified",
            "executionPermit", "flightCommandReleased", "permissionsRestored", "runtimeValidated"]
        for (var f = 0; f < flags.length; ++f) if (value[flags[f]] !== false) return false
        var identity = ["queryId", "catalogId", "runId", "fileHash", "afterSequence", "nextSequence"]
        for (var i = 0; i < identity.length; ++i)
            if (typeof value.sourceIdentity[identity[i]] !== "string" || value.sourceIdentity[identity[i]] !== motion[identity[i]]) return false
        var coverage = value.coverage, page = savedPage.dto
        if (coverage.totalValidatedFrames !== page.totalValidatedFrames || coverage.returnedFrameCount !== page.returnedFrameCount
                || coverage.positionWithheldFrames !== page.positionWithheldFrames || coverage.hasMore !== page.hasMore
                || coverage.footerPresent !== page.footerPresent || coverage.partialTail !== page.partialTail
                || coverage.fullTelemetryCaptured !== false || coverage.closureVerified !== false) return false
        for (var d = 0; d < value.dataQualityFindings.length; ++d) {
            var finding = value.dataQualityFindings[d]
            if (!map(finding) || Object.keys(finding).length !== 3 || !identifier(finding.code)
                    || !integer(finding.count, 0, 256) || typeof finding.detail !== "string" || finding.detail.length > 4096) return false
        }
        var names = ["commander", "customer", "engineer", "safety"]
        for (var v = 0; v < names.length; ++v) {
            var view = value.views[names[v]]
            if (!map(view) || Object.keys(view).length !== 6 || view.audience !== names[v]
                    || typeof view.headline !== "string" || view.headline.length > 4096
                    || view.historicalOnly !== true || view.flightCommandReleased !== false
                    || !Array.isArray(view.sections) || view.sections.length > 8
                    || !Array.isArray(view.limitations) || view.limitations.length > 16) return false
            for (var s = 0; s < view.sections.length; ++s) {
                var section = view.sections[s]
                if (!map(section) || Object.keys(section).length !== 3 || !identifier(section.code)
                        || typeof section.title !== "string" || section.title.length > 4096
                        || typeof section.text !== "string" || section.text.length > 4096) return false
            }
            for (var l = 0; l < view.limitations.length; ++l)
                if (typeof view.limitations[l] !== "string" || view.limitations[l].length > 4096) return false
        }
        return true
    }
    function displayJson(value) {
        try { return JSON.stringify(value, null, 2) }
        catch (error) { return qsTr("结构化文本不可展示；不据此推断资料完整或结论成立。") }
    }
    function segmentRows(report) {
        var rows = []
        for (var i = 0; i < report.groups.length; ++i) {
            var group = report.groups[i]
            for (var s = 0; s < group.segments.length; ++s)
                rows.push({groupOrdinal: group.groupOrdinal, vehicleId: group.vehicleId, componentId: group.componentId, segment: group.segments[s]})
        }
        return rows
    }
    function findEntry(name) { for (var i = 0; i < entries.length; ++i) if (entries[i].basename === name) return entries[i]; return null }
    function fileLabels() { var labels = [qsTr("请选择本次实际目录条目")]; for (var i = 0; i < entries.length; ++i) labels.push(entries[i].basename + (entries[i].headerValid ? "" : qsTr("（不可读取）"))); return labels }
    function fileIndex() { for (var i = 0; i < entries.length; ++i) if (entries[i].basename === selectedBasename) return i + 1; return 0 }
    function searchHitKey(row) { return map(row) ? JSON.stringify(row) : "" }
    function findSearchHit(key) {
        if (!key || !searchInputsCurrent) return null
        for (var i = 0; i < searchRows.length; ++i) {
            var row = searchRows[i]
            if (row.status === "matched" && row.matchingFrameCount !== "0" && row.descriptor.basename === selectedBasename
                    && searchHitKey(row) === key) return row
        }
        return null
    }
    function bumpSearchCriteria() {
        if (searchCriteriaEpoch >= 9007199254740991) exhausted = true
        else searchCriteriaEpoch += 1
        selectedSearchKey = ""
    }
    function hitSnapshot(row) {
        if (!lifetime.alive || lifetime.ticket || callBusy || !canRead || !visible || !enabled || !expanded
                || !searchInputsCurrent || !map(row) || row.status !== "matched" || row.matchingFrameCount === "0") return null
        var key = searchHitKey(row), found = false
        for (var i = 0; i < searchRows.length; ++i) if (searchHitKey(searchRows[i]) === key) found = true
        if (!found) return null
        return {owner: archive, optionsStamp: optionsStamp, queryId: latestId, searchId: savedSearch.dto.searchId,
            manifestHash: savedSearch.dto.manifestHash, inputEpoch: inputEpoch, consentEpoch: consentEpoch,
            criteriaEpoch: searchCriteriaEpoch, key: key, row: copy(row)}
    }
    function hitContextCurrent(value, epoch, consent) {
        if (!value || !lifetime.alive || value.owner !== archive || value.optionsStamp !== optionsStamp
                || value.queryId !== latestId || value.criteriaEpoch !== searchCriteriaEpoch || inputEpoch !== epoch
                || consentEpoch !== consent || !visible || !enabled || !expanded || !searchInputsCurrent
                || savedSearch.dto.searchId !== value.searchId || savedSearch.dto.manifestHash !== value.manifestHash) return false
        for (var i = 0; i < searchRows.length; ++i) if (searchHitKey(searchRows[i]) === value.key) return true
        return false
    }
    function selectSearchHit(value) {
        var guard = lifetime
        if (!value || guard.ticket || callBusy || !hitContextCurrent(value, value.inputEpoch, value.consentEpoch)) return
        var ticket = {queryChanges: 0}; guard.ticket = ticket
        callBusy = true
        try {
            if (!guard.alive || guard.ticket !== ticket || !hitContextCurrent(value, value.inputEpoch, value.consentEpoch)) return
            var expectedConsent = consentEpoch + (readConfirmed ? 1 : 0) + (reviewConfirmed ? 1 : 0) + (gnssConfirmed ? 1 : 0)
            var expectedEpoch = inputEpoch + 1
            invalidateInput()
            if (!guard.alive || guard.ticket !== ticket || !hitContextCurrent(value, expectedEpoch, expectedConsent)
                    || readConfirmed || reviewConfirmed || gnssConfirmed) return
            selectedSearchKey = ""
            if (!guard.alive || guard.ticket !== ticket || !hitContextCurrent(value, expectedEpoch, expectedConsent)) return
            if (selectedBasename !== value.row.descriptor.basename) expectedEpoch += 1
            selectedBasename = value.row.descriptor.basename
            if (!guard.alive || guard.ticket !== ticket || !hitContextCurrent(value, expectedEpoch, expectedConsent)) return
            selectedSearchKey = value.key
            if (!guard.alive || guard.ticket !== ticket || !hitContextCurrent(value, expectedEpoch, expectedConsent)) return
        } finally {
            if (guard.alive && guard.ticket === ticket) callBusy = false
            if (guard.ticket === ticket) guard.ticket = null
        }
    }
    function bumpConsent() { if (consentEpoch >= 9007199254740991) exhausted = true; else consentEpoch += 1 }
    function invalidateInput() {
        var guard = lifetime
        if (inputEpoch >= 9007199254740991) exhausted = true; else inputEpoch += 1
        if (!guard.alive) return
        retireGnssMap()
        if (!guard.alive) return
        readConfirmed = false; if (!guard.alive) return
        reviewConfirmed = false; if (!guard.alive) return
        gnssConfirmed = false; if (!guard.alive) return
        savedGnss = null; if (!guard.alive) return
        savedReport = null
    }
    function snapshot(action) {
        if (!lifetime.alive || lifetime.ticket || !visible || !enabled || !expanded || !sourceReady || callBusy) return null
        if (action === "cancel" ? !canCancel : !canRead) return null
        if (action === "review" ? !reviewConfirmed : action === "gnss-review" ? !gnssConfirmed : action !== "cancel" && !readConfirmed) return null
        if (["search-first", "search-next", "search-open"].indexOf(action) >= 0) {
            if (!searchReady || !filterValid || !messageFilterValid) return null
            var searchIntent = {action: action, expectedQueryId: latestId, catalogId: "", searchId: "", basename: "",
                vehicleId: filterId, messageId: messageFilterId, includePositionValues: false}
            var openHash = "", afterFileIndex = 0, manifestHash = "", totalFileCount = 0
            if (action === "search-first") {
                if (!catalogCurrent) return null
                searchIntent.catalogId = savedCatalog.dto.catalogId
            } else {
                if (!searchInputsCurrent) return null
                searchIntent.catalogId = savedSearch.dto.catalogId; searchIntent.searchId = savedSearch.dto.searchId
                manifestHash = savedSearch.dto.manifestHash; totalFileCount = savedSearch.dto.totalFileCount
                if (action === "search-next") {
                    if (savedSearch.dto.hasMore !== true) return null
                    afterFileIndex = savedSearch.dto.nextFileIndex
                } else {
                    if (!selectedSearchHit || (positionValues && selectedSearchHit.descriptor.positionTelemetryConsent !== true)) return null
                    searchIntent.basename = selectedSearchHit.descriptor.basename
                    searchIntent.includePositionValues = positionValues
                    openHash = selectedSearchHit.fileHash
                }
            }
            return {owner: archive, optionsStamp: optionsStamp, inputEpoch: inputEpoch, consentEpoch: consentEpoch,
                readConfirmed: readConfirmed, reviewConfirmed: reviewConfirmed, gnssConfirmed: gnssConfirmed, intent: searchIntent,
                search: true, criteriaEpoch: searchCriteriaEpoch, openFileHash: openHash,
                hitKey: action === "search-open" ? selectedSearchKey : "", afterFileIndex: afterFileIndex,
                manifestHash: manifestHash, totalFileCount: totalFileCount}
        }
        var intent = {action: action, expectedQueryId: latestId, catalogId: "", basename: "", vehicleId: "", includePositionValues: false}
        if (action === "first") {
            if (!catalogCurrent || !selectedEntry || selectedEntry.headerValid !== true || !filterValid
                    || (positionValues && selectedEntry.positionTelemetryConsent !== true)) return null
            intent.catalogId = savedCatalog.dto.catalogId; intent.basename = selectedBasename
            intent.vehicleId = filterId; intent.includePositionValues = positionValues
        } else if (action === "next" || action === "review" || action === "gnss-review") {
            if (!pageInputsCurrent || (action === "next" && savedPage.dto.hasMore !== true)
                    || ((action === "review" || action === "gnss-review") && savedPage.intent.includePositionValues !== true)) return null
            intent.catalogId = savedPage.dto.catalogId; intent.basename = savedPage.dto.basename
            intent.vehicleId = savedPage.intent.vehicleId; intent.includePositionValues = savedPage.intent.includePositionValues
        } else if (action !== "catalog" && action !== "cancel") return null
        return {owner: archive, optionsStamp: optionsStamp, inputEpoch: inputEpoch, consentEpoch: consentEpoch,
            readConfirmed: readConfirmed, reviewConfirmed: reviewConfirmed, gnssConfirmed: gnssConfirmed, intent: intent}
    }
    function contextCurrent(value) {
        return !!value && lifetime.alive && value.owner === archive && value.optionsStamp === optionsStamp
            && value.inputEpoch === inputEpoch && value.intent.expectedQueryId === latestId
            && visible && enabled && expanded && sourceReady
            && (value.search !== true || (searchReady && value.criteriaEpoch === searchCriteriaEpoch
                && (value.intent.action !== "search-open" || (value.hitKey === selectedSearchKey && !!selectedSearchHit
                    && selectedSearchHit.fileHash === value.openFileHash))))
    }
    function submittedContextCurrent(value, ticket, expectedConsent, requestId) {
        var queryAdvance = ["catalog", "first", "next", "search-first", "search-next", "search-open"].indexOf(value.intent.action) >= 0 ? 1 : 0
        return lifetime.alive && lifetime.ticket === ticket && value.owner === archive
            && value.optionsStamp === optionsStamp && ticket.queryChanges === queryAdvance
            && inputEpoch === value.inputEpoch + queryAdvance && consentEpoch === expectedConsent
            && !readConfirmed && !reviewConfirmed && !gnssConfirmed && visible && enabled && expanded && sourceReady
            && latestId === requestId && (queryAdvance === 1 || latestId === value.intent.expectedQueryId)
            && (value.search !== true || value.criteriaEpoch === searchCriteriaEpoch)
    }
    function submit(value) {
        var guard = lifetime
        if (!value || guard.ticket || callBusy || !contextCurrent(value) || value.consentEpoch !== consentEpoch) return
        var check = snapshot(value.intent.action)
        if (!check || JSON.stringify(check.intent) !== JSON.stringify(value.intent)
                || check.openFileHash !== value.openFileHash || check.hitKey !== value.hitKey) return
        var ticket = {queryChanges: 0}; guard.ticket = ticket
        callBusy = true
        try {
            if (!guard.alive || guard.ticket !== ticket || !contextCurrent(value) || value.consentEpoch !== consentEpoch) return
            var expectedConsent = consentEpoch + (readConfirmed ? 1 : 0) + (reviewConfirmed ? 1 : 0) + (gnssConfirmed ? 1 : 0)
            readConfirmed = false; if (!guard.alive) return
            reviewConfirmed = false; if (!guard.alive) return
            gnssConfirmed = false; if (!guard.alive) return
            if (!contextCurrent(value) || consentEpoch !== expectedConsent || readConfirmed || reviewConfirmed || gnssConfirmed) return
            if (value.intent.action === "review") {
                savedReport = null
                if (!guard.alive || !contextCurrent(value) || consentEpoch !== expectedConsent || readConfirmed || reviewConfirmed || gnssConfirmed) return
            }
            if (value.intent.action === "gnss-review") {
                retireGnssMap()
                if (!guard.alive || !contextCurrent(value) || consentEpoch !== expectedConsent) return
                savedGnss = null
                if (!guard.alive || !contextCurrent(value) || consentEpoch !== expectedConsent) return
            }
            var receipt = value.search === true ? value.owner.requestSearch(value.intent, true) : value.owner.request(value.intent, true)
            if (!guard.alive || guard.ticket !== ticket || value.owner !== archive || value.optionsStamp !== optionsStamp) return
            if (!map(receipt) || Object.keys(receipt).length !== 11 || receipt.schemaVersion !== 1
                    || receipt.action !== value.intent.action || receipt.accepted !== true || !identifier(receipt.requestId)
                    || receipt.requestOnly !== true || receipt.historicalOnly !== true || receipt.readOnly !== true
                    || receipt.permissionsRestored !== false || receipt.flightCommandReleased !== false || receipt.executionPermit !== false) {
                feedback = qsTr("请求未被确认接受：%1。可能已发生读取；不自动重试，请核对原生状态。").arg(map(receipt) && typeof receipt.reason === "string" ? receipt.reason : qsTr("未知返回"))
                return
            }
            // A normal accepted read changes latestId once. Any additional
            // input/query/visibility notification retires the old intent;
            // never adopt a derived position value on an obsolete consent.
            if (!submittedContextCurrent(value, ticket, expectedConsent, receipt.requestId)) {
                feedback = qsTr("请求可能已受理，但界面来源或确认已变化；不采用旧意图结果，也不自动重试。")
                return
            }
            if (value.intent.action === "gnss-review") {
                var gnss = copy(archive.gnssReview), reportToken = archive.gnssReviewToken
                if (!guard.alive || !submittedContextCurrent(value, ticket, expectedConsent, receipt.requestId)) return
                if (gnssKnown(gnss) && uuid(reportToken)) {
                    var adopted = {owner: archive, optionsStamp: optionsStamp, dto: gnss, stamp: JSON.stringify(gnss), reviewToken: reportToken}
                    savedGnss = adopted
                    if (!guard.alive) return
                    if (!submittedContextCurrent(value, ticket, expectedConsent, receipt.requestId) || !gnssCurrent
                            || archive.gnssReviewToken !== reportToken) { if (savedGnss === adopted) savedGnss = null; return }
                    feedback = qsTr("已显示本页同包GNSS报告值及质量诊断；地图仍关闭，须另行明确选择。不是精度、飘移、事故或任务结论。")
                } else feedback = qsTr("GNSS报告未通过本页完整身份核对；不显示部分坐标，也不回退旧报告。")
            } else if (value.intent.action === "review") {
                var report = copy(archive.motionReview)
                var dossier = copy(archive.motionDossier)
                if (dossierKnown(dossier, report)) {
                    savedReport = {owner: archive, dto: report, dossier: dossier}
                    if (!guard.alive) return
                    if (!submittedContextCurrent(value, ticket, expectedConsent, receipt.requestId)) { savedReport = null; return }
                    feedback = qsTr("已生成本页报告值摘要与四视角资料报告；不是事故原因、任务完成或性能认证。")
                }
                else feedback = qsTr("摘要未通过本页对应核对；不显示旧值，请重新读取或核对工程日志。")
            } else if (value.intent.action === "cancel") {
                cancelledId = receipt.requestId
                if (!guard.alive || !submittedContextCurrent(value, ticket, expectedConsent, receipt.requestId)) return
                feedback = qsTr("仅已请求取消；原读取器实际结束前不能开始新读取。")
            } else if (receipt.requestId === latestId) {
                pending = {owner: archive, optionsStamp: optionsStamp, id: receipt.requestId, intent: value.intent,
                    kind: value.search === true && value.intent.action !== "search-open" ? "search" : "page",
                    criteriaEpoch: value.criteriaEpoch, openFileHash: value.openFileHash, afterFileIndex: value.afterFileIndex,
                    manifestHash: value.manifestHash, totalFileCount: value.totalFileCount}
                if (!guard.alive || !submittedContextCurrent(value, ticket, expectedConsent, receipt.requestId)) return
                savedReport = null
                if (!guard.alive || !submittedContextCurrent(value, ticket, expectedConsent, receipt.requestId)) return
                cancelledId = ""
                if (!guard.alive || !submittedContextCurrent(value, ticket, expectedConsent, receipt.requestId)) return
                feedback = qsTr("只读请求已受理；等待结果及读取线程均结束。")
            } else feedback = qsTr("请求来源已变化，结果不归属本次界面；不自动续读。")
        } catch (error) {
            if (guard.alive) feedback = qsTr("读取结果不明；不自动重试、不据此推断完成。")
        } finally {
            // Keep the plain ticket while callBusy=false notifies observers.
            if (guard.alive && guard.ticket === ticket) callBusy = false
            if (guard.ticket === ticket) guard.ticket = null
            if (guard.alive) settle()
        }
    }
    function retireGnssMap() {
        var guard = lifetime, lease = guard.mapLease
        if (!lease) return
        guard.mapLease = null
        lease.revocation.cancelled = true
        try { if (lease.selectionToken && lease.owner && typeof lease.owner.clearGnssMap === "function") lease.owner.clearGnssMap(lease.selectionToken) }
        catch (error) { /* Exact lease only; never clear latest. */ }
    }
    function mapSnapshot() {
        if (!lifetime.alive || lifetime.ticket || callBusy || !canRead || !visible || !enabled || !expanded || !gnssCurrent
                || !uuid(savedGnss.reviewToken) || archive.gnssReviewToken !== savedGnss.reviewToken
                || typeof archive.selectGnssMap !== "function" || typeof archive.clearGnssMap !== "function") return null
        return {owner: archive, report: savedGnss, epoch: inputEpoch, consent: consentEpoch, token: savedGnss.reviewToken}
    }
    function selectGnssMap(value) {
        var guard = lifetime
        if (!value || guard.ticket || callBusy) return
        function current() { return guard.alive && value.owner === archive && savedGnss === value.report && gnssCurrent
            && inputEpoch === value.epoch && consentEpoch === value.consent && visible && enabled && expanded
            && archive.gnssReviewToken === value.token }
        if (!current()) return
        var ticket = {}, lease = {owner: value.owner, selectionToken: "", revocation: {cancelled: false}}
        guard.ticket = ticket
        try {
            callBusy = true
            if (!current() || guard.ticket !== ticket) return
            retireGnssMap()
            if (!current() || guard.ticket !== ticket) return
            guard.mapLease = lease
            var receipt = value.owner.selectGnssMap(value.token, true, lease.revocation)
            // The callback may have destroyed this Panel. Recognize only this
            // returned token with plain values before any QObject helper call.
            var suffix = receipt && typeof receipt.selectionToken === "string" && receipt.selectionToken.indexOf(value.token + ":") === 0
                ? receipt.selectionToken.substring(value.token.length + 1) : ""
            var returnedToken = receipt && receipt.accepted === true && /^[1-9][0-9]{0,15}$/.test(suffix)
                && (suffix.length < 16 || suffix <= "9007199254740991") ? receipt.selectionToken : ""
            if (returnedToken) lease.selectionToken = returnedToken
            if (!guard.alive || guard.ticket !== ticket || guard.mapLease !== lease || lease.revocation.cancelled) {
                lease.revocation.cancelled = true
                if (returnedToken) { try { value.owner.clearGnssMap(returnedToken) } catch (error) {} }
                return
            }
            var valid = exact(receipt, ["accepted", "selectionToken", "reason", "readOnly", "historicalOnly"])
                && receipt.accepted === true && selectionTokenValid(receipt.selectionToken, value.token) && receipt.reason === ""
                && receipt.readOnly === true && receipt.historicalOnly === true
            if (!current()) {
                lease.revocation.cancelled = true
                if (returnedToken) { try { value.owner.clearGnssMap(returnedToken) } catch (error) {} }
                return
            }
            if (!valid) {
                lease.revocation.cancelled = true
                if (guard.mapLease === lease) guard.mapLease = null
                if (returnedToken) { try { value.owner.clearGnssMap(returnedToken) } catch (error) {} }
                if (!guard.alive || guard.ticket !== ticket || !current()) return
                feedback = qsTr("未取得地图选择回执；不会采用共享最新图层或自动重试。")
                return
            }
            feedback = qsTr("已明确选择本页历史GNSS点图层；不移动视野、不连线、不发送飞行指令。离开本页会只退休自己的选择。")
        } catch (error) {
            lease.revocation.cancelled = true
            if (guard.mapLease === lease) guard.mapLease = null
            if (lease.selectionToken) { try { value.owner.clearGnssMap(lease.selectionToken) } catch (cleanupError) {} }
            if (guard.alive && guard.ticket === ticket && current()) feedback = qsTr("地图选择结局不明；不采用共享最新选择，不自动重试。")
        } finally {
            if (guard.alive && guard.ticket === ticket) callBusy = false
            if (guard.ticket === ticket) guard.ticket = null
        }
    }
    function settle() {
        var guard = lifetime
        if (!guard.alive || guard.ticket || callBusy || !pending || pending.owner !== archive || pending.optionsStamp !== optionsStamp
                || pending.id !== latestId || busy) return
        var request = pending
        var ticket = {queryChanges: 0}; guard.ticket = ticket
        function current() {
            return guard.alive && guard.ticket === ticket && pending === request && request.owner === archive
                && request.optionsStamp === optionsStamp && request.id === latestId && !busy
        }
        callBusy = true
        try {
            if (!current()) return
            if (status.historyResultAccepted !== true || status.historyCancelled !== false) {
                feedback = qsTr("未取得本次成功页：%1。取消或失败不代表档案为空。").arg(status.historyErrorCode || qsTr("未知原因"))
            } else if (request.intent.action === "catalog") {
                var catalog = copy(archive.historyCatalog)
                if (!catalogKnown(catalog) || catalog.queryId !== request.id) feedback = qsTr("目录返回不对应本次请求。请重新明确读取。")
                else {
                    savedCatalog = {owner: archive, optionsStamp: optionsStamp, dto: catalog}; if (!current()) return
                    savedPage = null; if (!current()) return
                    savedReport = null; if (!current()) return
                    feedback = qsTr("目录读取结束；这里只是实际枚举条目，不证明历史全集。")
                }
            } else if (request.kind === "search") {
                var search = copy(archive.historySearch)
                if (!searchKnown(search, request)) feedback = qsTr("搜索返回不对应本次完整条件；不采用为可打开的命中。请重新确认搜索。")
                else {
                    savedSearch = {owner: archive, optionsStamp: optionsStamp, dto: search, intent: request.intent,
                        criteriaEpoch: request.criteriaEpoch}; if (!current()) return
                    selectedSearchKey = ""; if (!current()) return
                    savedReport = null; if (!current()) return
                    feedback = qsTr("本批文件元数据搜索结束；未自动选取、打开文件或显示位置数值。请检查未验证条目与剩余批次。")
                }
            } else {
                var page = copy(archive.historyPage)
                if (!pageKnown(page, request)) feedback = qsTr("历史页对应核对未通过，不采纳为新结果。")
                else {
                    savedPage = {owner: archive, optionsStamp: optionsStamp, dto: page, intent: request.intent}; if (!current()) return
                    savedReport = null; if (!current()) return
                    feedback = request.intent.action === "search-open"
                        ? qsTr("已核同一文件摘要并打开整个首页；只按机号筛选，消息号不筛页，首页可无命中。位置派生报告仍须另行确认。")
                        : qsTr("本页读取结束。位置派生摘要需要下方另一次明确确认。")
                }
            }
            if (current()) pending = null // Clear only this actual pending request.
        } finally {
            if (guard.alive && guard.ticket === ticket) callBusy = false
            if (guard.ticket === ticket) guard.ticket = null
        }
    }
    onReadConfirmedChanged: { var g = lifetime; bumpConsent(); if (g.alive) retireGnssMap() }
    onReviewConfirmedChanged: { var g = lifetime; bumpConsent(); if (g.alive) retireGnssMap() }
    onGnssConfirmedChanged: { var g = lifetime; bumpConsent(); if (g.alive) retireGnssMap() }
    onSelectedBasenameChanged: { selectedSearchKey = ""; invalidateInput() }
    onVehicleFilterChanged: { bumpSearchCriteria(); invalidateInput() }
    onMessageFilterChanged: { bumpSearchCriteria(); invalidateInput() }
    onPositionValuesChanged: { var g = lifetime; invalidateInput(); if (g.alive) savedReport = null }
    onLatestIdChanged: { if (lifetime.ticket) lifetime.ticket.queryChanges += 1; selectedSearchKey = ""; invalidateInput() }
    onOptionsStampChanged: { bumpSearchCriteria(); invalidateInput() }
    onExpandedChanged: { selectedSearchKey = ""; invalidateInput() }
    onVisibleChanged: if (!visible) { selectedSearchKey = ""; invalidateInput() }
    onEnabledChanged: if (!enabled) { selectedSearchKey = ""; invalidateInput() }
    onArchiveChanged: {
        var g = lifetime
        bumpSearchCriteria(); if (!g.alive) return
        invalidateInput(); if (!g.alive) return
        pending = null; if (!g.alive) return
        savedCatalog = null; if (!g.alive) return
        savedPage = null; if (!g.alive) return
        savedSearch = null; if (!g.alive) return
        savedReport = null
    }
    Component.onDestruction: { var lease = lifetime.mapLease; lifetime.alive = false; lifetime.mapLease = null; if (lease) { lease.revocation.cancelled = true; if (lease.selectionToken) { try { lease.owner.clearGnssMap(lease.selectionToken) } catch (error) {} } } }
    Connections {
        target: root.archive
        ignoreUnknownSignals: true
        function onCurrentChanged() { root.settle() }
        function onHistoryCatalogChanged() { root.settle() }
        function onHistoryPageChanged() { root.settle() }
        function onHistorySearchChanged() { root.settle() }
    }

    ColumnLayout {
        id: form
        width: root.width
        spacing: root.pad
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.expanded ? qsTr("收起遥测档案只读审阅") : qsTr("遥测档案与本页运动报告值审阅"); Accessible.name: text; onClicked: root.expanded = !root.expanded }
        ColumnLayout {
            Layout.fillWidth: true
            visible: root.expanded
            spacing: root.pad
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("只读取已存在的本机遥测档案。不启动录制、不建目录、不导入ULog、不回灌实时状态或发飞行指令。最多128文件、单文件64 MiB、每页64帧；不自动续读。") }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("固定目录：%1").arg(root.options.directoryPath || qsTr("不可用")) }
            QGCLabel { Layout.fillWidth: true; visible: !root.sourceReady; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: pal.warningText; text: qsTr("历史接口或读取策略不可用；不会退回任意文件路径。请工程师核对配置。") }
            QGCLabel { Layout.fillWidth: true; visible: root.busy; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("正在只读查询 / 等待读取器结束：%1").arg(root.latestId) }
            Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("确认本次读盘"); checked: root.readConfirmed; enabled: root.canRead; onToggled: root.readConfirmed = checked; Accessible.name: qsTr("确认本次显式读盘，不连续自动读取") }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("读取固定目录清单"); enabled: root.canRead && root.readConfirmed; Accessible.name: text; property var press: null; onPressed: press = root.snapshot("catalog"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("档案选择（首次加载不自动选取）") }
            Controls.ComboBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; model: root.fileLabels(); currentIndex: root.fileIndex(); enabled: !root.callBusy && !root.busy; Accessible.name: qsTr("本次清单内的遥测档案"); onActivated: root.selectedBasename = index > 0 && index <= root.entries.length ? root.entries[index - 1].basename : "" }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.selectedEntry ? qsTr("记录字节：%1；头部合法：%2；档案位置记录同意：%3；目录诊断：%4").arg(root.selectedEntry.fileBytes).arg(root.known(root.selectedEntry.headerValid)).arg(root.known(root.selectedEntry.positionTelemetryConsent)).arg(root.selectedEntry.errorCode || qsTr("无")) : qsTr("暂无选择；目录为空不等于从未飞行或记录完整。") }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("机号筛选（空为全部；1–255）") }
            Controls.TextField { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; maximumLength: 3; focusPolicy: Qt.StrongFocus; text: root.vehicleFilter; selectByMouse: true; inputMethodHints: Qt.ImhDigitsOnly; enabled: !root.callBusy && !root.busy; onTextChanged: root.vehicleFilter = text; Accessible.name: qsTr("遥测档案机号筛选，空为全部") }
            QGCLabel { Layout.fillWidth: true; visible: !root.filterValid; color: pal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("机号只允许空值或无前导零的1–255，请修改后重新确认读盘。") }
            QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("跨档案元数据搜索（不关联任务、不显示位置载荷）") }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("使用本次已读取目录，按上方机号及下方消息号逐文件查找。一次最多检查64个文件，预留读取总预算130 MiB；不会自动续批。每个已检查文件均保留一行，包括无命中与不可验证的文件。") }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("消息号（空或-1为全部；0为HEARTBEAT；最大16777215）") }
            Controls.TextField { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; maximumLength: 8; focusPolicy: Qt.StrongFocus; text: root.messageFilter; selectByMouse: true; inputMethodHints: Qt.ImhFormattedNumbersOnly; enabled: !root.callBusy && !root.busy; onTextChanged: root.messageFilter = text; Accessible.name: qsTr("元数据搜索消息号，空或负一为全部，零为HEARTBEAT") }
            QGCLabel { Layout.fillWidth: true; visible: !root.messageFilterValid; color: pal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("消息号只允许空、-1或无前导零的0–16777215。修改条件后须重新确认搜索，旧命中不能继续打开。") }
            QGCLabel { Layout.fillWidth: true; visible: !root.searchReady; color: pal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("跨档案搜索接口不可用；普通单文件只读入口仍独立保留，不退回任意路径或原始查询。") }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("搜索本次目录首批文件"); enabled: root.canRead && root.searchReady && root.readConfirmed && root.catalogCurrent && root.filterValid && root.messageFilterValid; Accessible.name: text; property var press: null; onPressed: press = root.snapshot("search-first"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("搜索原条件下一批文件"); enabled: root.canRead && root.searchReady && root.readConfirmed && root.searchInputsCurrent && root.savedSearch.dto.hasMore === true; Accessible.name: text; property var press: null; onPressed: press = root.snapshot("search-next"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.savedSearch ? qsTr("保存显示的搜索批次：%1\n搜索ID：%2；目录：%3\n文件范围 [%4, %5) / %6；本批检查%7，已验证%8，有命中%9，不可验证%10\n仍有后续批次：%11；受读取预算限制：%12；预留字节：%13 / %14\n当前条件仍可操作：%15").arg(root.savedSearch.dto.queryId).arg(root.savedSearch.dto.searchId).arg(root.savedSearch.dto.catalogId).arg(root.savedSearch.dto.afterFileIndex).arg(root.savedSearch.dto.nextFileIndex).arg(root.savedSearch.dto.totalFileCount).arg(root.savedSearch.dto.inspectedFileCount).arg(root.savedSearch.dto.verifiedFileCount).arg(root.savedSearch.dto.matchedFileCount).arg(root.savedSearch.dto.diagnosticFileCount).arg(root.known(root.savedSearch.dto.hasMore)).arg(root.known(root.savedSearch.dto.budgetLimited)).arg(root.savedSearch.dto.reservedReadBytes).arg(root.savedSearch.dto.readByteBudget).arg(root.known(root.searchInputsCurrent)) : qsTr("尚无本界面发起并核对的搜索结果；展开或修改条件不会自动读盘。") }
            QGCLabel { Layout.fillWidth: true; visible: !!root.savedSearch && !root.searchInputsCurrent; color: pal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("下列是保留显示的旧批次，不是当前可操作结果。查询、筛选、取消或来源变化后，必须重新明确搜索；不会自动选首个命中。") }
            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: root.searchRows.length ? Math.min(580, root.rowHeight * 9) : 0
                model: root.searchRows
                clip: true
                spacing: root.pad
                Controls.ScrollBar.vertical: Controls.ScrollBar {}
                delegate: FocusScope {
                    width: ListView.view.width
                    height: searchRow.implicitHeight + root.pad * 2
                    activeFocusOnTab: true
                    Accessible.role: Accessible.StaticText
                    Accessible.name: searchRowText.text
                    Rectangle { anchors.fill: parent; color: pal.windowShade; border.width: parent.activeFocus ? 3 : 1; border.color: parent.activeFocus ? pal.warningText : pal.text; radius: 3 }
                    ColumnLayout {
                        id: searchRow
                        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: root.pad
                        spacing: root.pad
                        QGCLabel {
                            id: searchRowText
                            Layout.fillWidth: true
                            textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                            text: qsTr("文件索引%1 · %2\n%3\n目录字节：%4；匹配帧：%5；首 / 末匹配序号：%6 / %7\nFooter：%8；尾行：%9；诊断：%10\n本次完整文件摘要：%11")
                                .arg(modelData.fileIndex).arg(root.searchStatusText(modelData.status)).arg(modelData.descriptor.basename)
                                .arg(modelData.descriptor.fileBytes).arg(modelData.matchingFrameCount === "" ? qsTr("未知") : modelData.matchingFrameCount)
                                .arg(modelData.firstMatchingSequence === "" ? qsTr("未知") : modelData.firstMatchingSequence)
                                .arg(modelData.lastMatchingSequence === "" ? qsTr("未知") : modelData.lastMatchingSequence)
                                .arg(root.searchFooterText(modelData.footerState)).arg(root.searchTailText(modelData.tailState))
                                .arg(modelData.errorCode || qsTr("无")).arg(modelData.fileHash || qsTr("未知 / 未验证"))
                        }
                        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("选择此命中文件（不读取）"); visible: modelData.status === "matched"; enabled: root.canRead && root.searchInputsCurrent; Accessible.name: qsTr("明确选择命中文件 %1，不自动读取").arg(modelData.descriptor.basename); property var press: null; onPressed: press = root.hitSnapshot(modelData); onCanceled: press = null; onClicked: { var value = press; press = null; root.selectSearchHit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
                    }
                }
            }
            QGCLabel { Layout.fillWidth: true; color: pal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("无命中不是从未飞行；不可验证不是0帧。未检查后续批次不计入本批结论。首末序号仅为文件元数据，不用于跳转。跨文件没有原子快照、来源认证、任务归属或完整性保证；搜索结果不能作为运动报告输入。") }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.selectedSearchHit ? qsTr("明确选择：%1\n打开时须保持此完整文件摘要：%2\n已同步上方档案选择和机号条件；请重新勾选本次读盘确认。").arg(root.selectedSearchHit.descriptor.basename).arg(root.selectedSearchHit.fileHash) : qsTr("尚未明确选择当前搜索命中；无命中和不可验证文件不能走命中打开入口。") }
            Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("本页包含位置载荷"); checked: root.positionValues; enabled: !root.callBusy && !root.busy; onToggled: root.positionValues = checked; Accessible.name: qsTr("允许原生读取本页位置载荷；GNSS报告坐标另需独立确认") }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("默认隐藏位置消息。此选择只允许原生读取；下方GNSS另一次明确同意后才公开同包reported经纬度。派生距离与速度也属位置相关数据。档案未允许位置记录时，不能通过此选项补同意。选文件、机号或位置选项后，需要重新确认读盘；改变续页条件须从首页重读。") }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("命中打开将重验原文件摘要并读取该文件首页，只按机号筛选，不按搜索消息号筛选，也不从首个命中序号跳转。首页可以没有目标消息。搜索本身始终不返回位置载荷；打开及之后位置派生报告分别需要确认。") }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("按命中文件摘要打开整个首页"); enabled: root.canRead && root.searchReady && root.readConfirmed && !!root.selectedSearchHit && (!root.positionValues || root.selectedSearchHit.descriptor.positionTelemetryConsent === true); Accessible.name: text; property var press: null; onPressed: press = root.snapshot("search-open"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("读取所选档案首页"); enabled: root.canRead && root.readConfirmed && root.catalogCurrent && !!root.selectedEntry && root.selectedEntry.headerValid === true && root.filterValid && (!root.positionValues || root.selectedEntry.positionTelemetryConsent === true); Accessible.name: text; property var press: null; onPressed: press = root.snapshot("first"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("读取原查询下一页"); enabled: root.canRead && root.readConfirmed && root.pageInputsCurrent && root.savedPage.dto.hasMore === true; Accessible.name: text; property var press: null; onPressed: press = root.snapshot("next"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("请求取消本界面发起的查询"); enabled: root.canCancel; Accessible.name: text; property var press: null; onPressed: press = root.snapshot("cancel"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
            QGCLabel { Layout.fillWidth: true; activeFocusOnTab: true; Accessible.role: Accessible.StaticText; Accessible.name: text; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.feedback || qsTr("尚未发起读取；展开面板不会读盘。") }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.savedPage ? qsTr("已保存显示的页：%1\n档案：%2；该页机号筛选：%3；包含位置载荷：%4\n整文件已校验完整帧：%5；本页返回帧：%6；本查询隐私隐藏的位置帧：%7\n仍有下一页：%8；存在Footer：%9；未终止尾行：%10\n仍为当前已完成原生页：%11").arg(root.savedPage.dto.queryId).arg(root.savedPage.dto.basename).arg(root.savedPage.intent.vehicleId === "0" ? qsTr("全部") : root.savedPage.intent.vehicleId).arg(root.known(root.savedPage.intent.includePositionValues)).arg(root.savedPage.dto.totalValidatedFrames).arg(root.savedPage.dto.returnedFrameCount).arg(root.savedPage.dto.positionWithheldFrames).arg(root.known(root.savedPage.dto.hasMore)).arg(root.known(root.savedPage.dto.footerPresent)).arg(root.known(root.savedPage.dto.partialTail)).arg(root.known(root.pageCurrent)) : qsTr("尚无本界面核对成功的历史页。") }
            QGCLabel { Layout.fillWidth: true; color: pal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("三种帧数范围不同，不能相减推算丢包。Footer存在不代表完整、落盘、认证或任务成功；缺Footer和尾行诊断独立保留。旧页可保留显示，但新查询、取消或来源变化后不可继续使用旧摘要。") }
            FleetMaintenanceHistoryPanel {
                Layout.fillWidth: true
                archive: root.archive
                page: root.pageInputsCurrent && root.savedPage ? root.savedPage.dto : null
                active: root.expanded && root.visible && root.enabled
            }
            QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("本页GNSS同包质量诊断（历史报告值）") }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("独立同意后仅解读本页GPS_RAW_INT同包fix、经纬度、HDOP及卫星数，最多64条。坏fix不取旧坐标补点。HDOP无单位，不是米或精度保证；阈值仅用于显示提示。不会跨消息拼接、连线、诊断飘移或判断任务成功。") }
            Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("确认显示本页GNSS坐标"); checked: root.gnssConfirmed; enabled: root.canRead && root.pageInputsCurrent && root.savedPage.intent.includePositionValues === true; onToggled: root.gnssConfirmed = checked; Accessible.name: qsTr("独立同意显示本页同包GNSS报告坐标和质量值，不采用运动报告确认") }
            QGCButton {
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("生成本页GNSS诊断"); enabled: root.canRead && root.pageInputsCurrent && root.gnssConfirmed && root.savedPage.intent.includePositionValues === true
                Accessible.name: text; property var press: null
                onPressed: press = root.snapshot("gnss-review"); onCanceled: press = null
                onClicked: { var guard = root.lifetime, value = press; press = null; if (guard.alive) root.submit(value) }
                Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true
            }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.gnssCurrent ? qsTr("当前仅此页：GNSS %1；投点%2 / 不投点%3；来源组%4\n定位行：阈值提示%5 / 质量未知%6 / 显示阈值内%7（不是精度认证）\n查询%8；文件摘要%9；序号(%10,%11]\n后续页：%12；Footer：%13；尾部不完整：%14。均不证明历史完整。")
                .arg(root.savedGnss.dto.gnssSampleCount).arg(root.savedGnss.dto.locatedSampleCount).arg(root.savedGnss.dto.unlocatedSampleCount).arg(root.savedGnss.dto.groupCount)
                .arg(root.savedGnss.dto.qualityCautionSampleCount).arg(root.savedGnss.dto.qualityUnknownSampleCount).arg(root.savedGnss.dto.withinThresholdSampleCount)
                .arg(root.savedGnss.dto.sourceIdentity.queryId).arg(root.savedGnss.dto.sourceIdentity.fileHash).arg(root.savedGnss.dto.sourceIdentity.afterSequence).arg(root.savedGnss.dto.sourceIdentity.nextSequence)
                .arg(root.known(root.savedGnss.dto.hasMore)).arg(root.known(root.savedGnss.dto.footerPresent)).arg(root.known(root.savedGnss.dto.partialTail)) : qsTr("尚无本界面当前已确认GNSS报告；生成后地图也不会自动显示。") }
            ListView {
                Layout.fillWidth: true; Layout.preferredHeight: root.gnssRows.length ? Math.min(580, root.rowHeight * 9) : 0
                model: root.gnssRows; clip: true; spacing: root.pad
                Controls.ScrollBar.vertical: Controls.ScrollBar {}
                delegate: FocusScope {
                    width: ListView.view.width; height: gnssText.implicitHeight + root.pad * 2
                    activeFocusOnTab: true; Accessible.role: Accessible.StaticText; Accessible.name: gnssText.text
                    Rectangle { anchors.fill: parent; color: pal.windowShade; border.width: parent.activeFocus ? 3 : 1; border.color: parent.activeFocus ? pal.warningText : pal.text; radius: 3 }
                    QGCLabel { id: gnssText; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: root.pad; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.gnssRowText(modelData) }
                }
            }
            QGCButton {
                Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                text: qsTr("明确在地图显示此页历史点（不定位视野）"); enabled: root.canRead && root.gnssCurrent && root.savedGnss.dto.locatedSampleCount > 0
                contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: pal.buttonText }
                Accessible.name: text; property var press: null
                onPressed: press = root.mapSnapshot(); onCanceled: press = null
                onClicked: { var guard = root.lifetime, value = press; press = null; if (guard.alive) root.selectGnssMap(value) }
                Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true
            }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("关闭本页选择的GNSS图层"); enabled: !root.callBusy && root.archive && root.map(root.archive.gnssMap) && root.archive.gnssMap.active === true && !!root.lifetime.mapLease; Accessible.name: text; onClicked: root.retireGnssMap() }
            QGCLabel { Layout.fillWidth: true; color: pal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("地图默认关闭，只画独立报告点，不是航迹、路线、任务归属或精度/安全证明。无需且不会自动居中；离开/改条件只清本页原选择，不清其他面板后来选的图层。") }
            FleetGnssMemoryWorkspacePanel {
                Layout.fillWidth: true
                workspace: root.archive ? root.archive.gnssMemory : null
                reportToken: root.gnssCurrent ? root.savedGnss.reviewToken : ""
                reportPreview: root.gnssCurrent ? root.savedGnss.dto : null
                sourceActive: root.expanded && root.visible && root.enabled
            }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("以下确认将显示本页位置和速度派生数值，并生成四种展示视角的资料报告；不保存或发送文件。") }
            Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("确认显示本页观察报告"); checked: root.reviewConfirmed; enabled: root.canRead && root.pageInputsCurrent && root.savedPage.intent.includePositionValues === true; onToggled: root.reviewConfirmed = checked; Accessible.name: qsTr("另行同意显示本页位置速度派生数值及四视角资料报告，不保存或发送") }
            QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("生成当前单页观察报告"); enabled: root.canRead && root.pageInputsCurrent && root.reviewConfirmed && root.savedPage.intent.includePositionValues === true; Accessible.name: text; property var press: null; onPressed: press = root.snapshot("review"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
            QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.reportCurrent ? qsTr("仅本页：运动帧%1；有效位置样本%2；有效目标消息%3；拒绝运动样本%4；来源组%5；分段%6。").arg(root.savedReport.dto.motionRowCount).arg(root.savedReport.dto.acceptedPositionSamples).arg(root.savedReport.dto.acceptedTargetSamples).arg(root.savedReport.dto.rejectedMotionSamples).arg(root.savedReport.dto.groupCount).arg(root.savedReport.dto.segmentCount) : qsTr("无当前有效派生摘要；不会自动显示旧页位置数值。") }
            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: root.reportRows.length ? Math.min(520, root.rowHeight * 7) : 0
                model: root.reportRows
                clip: true
                spacing: root.pad
                Controls.ScrollBar.vertical: Controls.ScrollBar {}
                delegate: FocusScope {
                    width: ListView.view.width
                    height: rowText.implicitHeight + root.pad * 2
                    activeFocusOnTab: true
                    Accessible.role: Accessible.StaticText
                    Accessible.name: rowText.text
                    Rectangle { anchors.fill: parent; color: pal.windowShade; border.width: parent.activeFocus ? 3 : 1; border.color: parent.activeFocus ? pal.warningText : pal.text; radius: 3 }
                    QGCLabel {
                        id: rowText
                        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: root.pad
                        textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        text: qsTr("组%1 · 机号%2 / 组件%3 · 段%4 · 原生断段原因：%5\n位置样本%6；接收跨度%7 ms；最大接收间隔%8 ms\n相对首样本最大水平 / 垂直报告值变化：%9 / %10 m\n最大水平 / 垂直报告速度：%11 / %12 m/s\n目标消息%13（可用XY位置%14 / XY速度%15）；接收跨度%16 ms；最大间隔%17 ms\n目标最大水平报告值变化%18 m；目标最大水平报告速度%19 m/s")
                            .arg(modelData.groupOrdinal).arg(modelData.vehicleId).arg(modelData.componentId).arg(modelData.segment.segmentOrdinal).arg(modelData.segment.breakReason)
                            .arg(modelData.segment.positionSampleCount).arg(root.numberText(modelData.segment.positionReceiveSpanMs)).arg(root.numberText(modelData.segment.positionReceiveMaxGapMs))
                            .arg(root.numberText(modelData.segment.maxHorizontalDisplacementMeters)).arg(root.numberText(modelData.segment.maxVerticalDisplacementMeters))
                            .arg(root.numberText(modelData.segment.maxHorizontalSpeedMps)).arg(root.numberText(modelData.segment.maxVerticalSpeedMps))
                            .arg(modelData.segment.targetSampleCount).arg(modelData.segment.targetPositionSampleCount).arg(modelData.segment.targetVelocitySampleCount)
                            .arg(root.numberText(modelData.segment.targetReceiveSpanMs)).arg(root.numberText(modelData.segment.targetReceiveMaxGapMs))
                            .arg(root.numberText(modelData.segment.maxHorizontalTargetChangeMeters)).arg(root.numberText(modelData.segment.maxHorizontalTargetSpeedMps))
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                visible: root.dossierCurrent
                spacing: root.pad
                QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("当前单页资料报告（未关联任务）") }
                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.dossierCurrent ? qsTr("卷宗ID：%1\n内容摘要：%2\n这是内容对应标识，不是签名、账号认证、来源真实性或验收结论。").arg(root.savedReport.dossier.dossierId).arg(root.savedReport.dossier.contentHash) : "" }
                QGCLabel { Layout.fillWidth: true; color: pal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("下列仅是内容展示视角，不是登录角色或访问控制。客户视图的可复制文本不含文件身份与逐机数值；完整工程卷宗仍含已确认的派生数据，请人工核对分享对象。本功能不会保存、上传或自动发送报告。") }
                Controls.ComboBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; model: [qsTr("指挥视角"), qsTr("客户摘要视角"), qsTr("工程师视角"), qsTr("安全审阅视角（非放行）")]; currentIndex: root.audienceIndex; Accessible.name: qsTr("历史观察报告展示视角，不是访问权限"); onActivated: { root.audienceIndex = index; root.showViewJson = false; root.showDossierJson = false } }
                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: root.selectedView ? root.selectedView.headline : qsTr("当前视角不可用，不回退其他视角。") }
                Repeater {
                    model: root.selectedView ? root.selectedView.sections : []
                    delegate: QGCLabel { Layout.fillWidth: true; activeFocusOnTab: true; Accessible.role: Accessible.StaticText; Accessible.name: text; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: modelData.title + "\n" + modelData.text }
                }
                QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("资料质量发现（不是飞行故障判定）") }
                Repeater {
                    model: root.dossierCurrent ? root.savedReport.dossier.dataQualityFindings : []
                    delegate: QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("%1 · 计数%2\n%3").arg(modelData.code).arg(modelData.count).arg(modelData.detail) }
                }
                QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("结论边界") }
                Repeater {
                    model: root.selectedView ? root.selectedView.limitations : []
                    delegate: QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: modelData }
                }
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.showViewJson ? qsTr("收起当前视角结构化文本") : qsTr("展开当前视角结构化文本（可手动复制）"); enabled: !!root.selectedView; Accessible.name: text; onClicked: root.showViewJson = !root.showViewJson }
                Loader {
                    Layout.fillWidth: true
                    active: root.dossierCurrent && root.showViewJson && !!root.selectedView
                    Layout.preferredHeight: item ? item.implicitHeight : 0
                    sourceComponent: Component { Controls.TextArea { width: parent ? parent.width : 1; readOnly: true; selectByMouse: true; textFormat: TextEdit.PlainText; wrapMode: TextEdit.Wrap; color: pal.text; text: root.displayJson(root.selectedView); Accessible.name: qsTr("当前报告视角结构化文本，只读，人工选择复制"); background: Rectangle { color: pal.windowShade } } }
                }
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; visible: root.audience === "engineer"; focusPolicy: Qt.StrongFocus; text: root.showDossierJson ? qsTr("收起完整工程卷宗") : qsTr("展开完整工程卷宗 JSON"); Accessible.name: qsTr("展开或收起完整工程卷宗，含全部已确认位置派生数值"); onClicked: root.showDossierJson = !root.showDossierJson }
                Loader {
                    Layout.fillWidth: true
                    active: root.dossierCurrent && root.audience === "engineer" && root.showDossierJson
                    Layout.preferredHeight: item ? item.implicitHeight : 0
                    sourceComponent: Component { Controls.TextArea { width: parent ? parent.width : 1; readOnly: true; selectByMouse: true; textFormat: TextEdit.PlainText; wrapMode: TextEdit.Wrap; color: pal.text; text: root.displayJson(root.dossierCurrent ? root.savedReport.dossier : null); Accessible.name: qsTr("完整工程卷宗，只读，含已确认的全部派生数值，不是签名或任务结果"); background: Rectangle { color: pal.windowShade } } }
                }
            }
            QGCLabel { Layout.fillWidth: true; color: pal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("数值仅为同页、同源分段的报告值，不是实际位移、悬停误差或飘移原因。未知与0分开；不配对目标与位置、不插值、不跨页累计，也不证明无原点重置、完整遥测、断桨、性能、任务完成、轨迹安全或飞行许可。不替代ULog复盘。") }
        }
    }
}
