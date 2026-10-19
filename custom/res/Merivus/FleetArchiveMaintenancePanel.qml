import QtQuick          2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts  1.11

import QGroundControl.Controls    1.0
import QGroundControl.Palette     1.0
import QGroundControl.ScreenTools 1.0

// Explicit native fixed-directory maintenance only; never task/control restore.
FocusScope {
    id: root
    property var archive: null
    property var lifetime: ({alive: true, epoch: 0, exhausted: false, call: null})
    property bool callBusy: false
    property bool revisionExhausted: false
    property double inventoryRevision: 0
    property double previewRevision: 0
    property double backupCatalogRevision: 0
    property double backupDiagnosticsRevision: 0
    property double selectionRevision: 0
    property string backupDirectory: ""
    property double backupPathRevision: 0
    property bool backupPathExhausted: false
    property string selectedEntryId: ""
    property string consumedInventoryId: ""
    property string consumedPreviewId: ""
    property string cancelledQueryId: ""
    property string localError: ""
    property var lastRequest: ({})
    property var ownedOperation: null
    property var savedInventory: null
    property var savedPreview: null
    property var savedResult: null
    property var savedBackupCatalog: null
    property string consumedBackupCatalogId: ""
    property var savedBackupDiagnostics: null
    property string consumedBackupDiagnosticsId: ""
    property string selectedDiagnosticEntryId: ""
    property double diagnosticSelectionRevision: 0
    property var savedImportPreview: null
    property var confirmedImportPreview: null
    property string consumedImportPreviewId: ""
    property var confirmedPreview: null
    property bool showDetails: false

    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property var current: archive ? archive.current || {} : ({})
    readonly property var options: archive ? archive.options || {} : ({})
    readonly property var actualInventory: archive ? archive.maintenanceInventory || {} : ({})
    readonly property var actualPreview: archive ? archive.maintenancePreview || {} : ({})
    readonly property var actualResult: archive ? archive.maintenanceResult || {} : ({})
    readonly property var actualBackupCatalog: archive ? archive.maintenanceBackupCatalog || {} : ({})
    readonly property var actualBackupDiagnostics: archive ? archive.maintenanceBackupDiagnostics || {} : ({})
    readonly property string optionsStamp: optionStamp(options)
    readonly property string catalogOptionsStamp: optionStamp(options, true)
    readonly property string runId: typeof current.runId === "string" ? current.runId : ""
    readonly property bool sourceReady: !!archive && lifetime.alive && !lifetime.exhausted && !revisionExhausted
        && optionsStamp.length > 0 && typeof archive.maintenance === "function"
        && typeof current.writerActive === "boolean" && typeof current.captureEnabled === "boolean"
        && typeof current.historyRequestPending === "boolean" && typeof current.archivedReaderActive === "boolean"
        && typeof current.archiveMaintenanceActive === "boolean"
    readonly property bool catalogSourceReady: !!archive && lifetime.alive && !lifetime.exhausted && !revisionExhausted
        && catalogOptionsStamp.length > 0 && typeof archive.maintenance === "function"
        && typeof current.writerActive === "boolean" && typeof current.captureEnabled === "boolean"
        && typeof current.historyRequestPending === "boolean" && typeof current.archivedReaderActive === "boolean"
        && typeof current.archiveMaintenanceActive === "boolean"
    readonly property bool historyBusy: current.historyRequestPending === true || current.archivedReaderActive === true
        || current.archiveMaintenanceActive === true
    readonly property bool idle: sourceReady && !callBusy && !historyBusy && current.writerActive === false && current.captureEnabled === false
    readonly property bool inventoryCurrent: !!savedInventory && basisCurrent(savedInventory)
        && savedInventory.nativeResult === actualInventory && savedInventory.revision === inventoryRevision
        && savedInventory.dto.inventoryId !== consumedInventoryId
    readonly property var inventoryRows: savedInventory ? savedInventory.dto.entries : []
    readonly property var selectedEntry: findEntry(inventoryRows, selectedEntryId)
    readonly property bool canPrepareRetire: idle && inventoryCurrent && !!selectedEntry && selectedEntry.kind === "sealed"
    readonly property bool canPrepareRestore: idle && inventoryCurrent && !!selectedEntry && selectedEntry.kind === "retired"
    readonly property bool canPrepareBackup: idle && inventoryCurrent && !!selectedEntry && !backupPathExhausted && backupPathCandidate(backupDirectory)
    readonly property bool canReadBackupCatalog: catalogSourceReady && !callBusy && !historyBusy && current.writerActive === false
        && current.captureEnabled === false && !backupPathExhausted && backupPathCandidate(backupDirectory)
    readonly property bool canReadBackupDiagnostics: canReadBackupCatalog
    readonly property var backupCatalogRows: savedBackupCatalog ? savedBackupCatalog.dto.entries : []
    readonly property bool backupCatalogCurrent: !!savedBackupCatalog && basisCurrent(savedBackupCatalog) && backupBasisCurrent(savedBackupCatalog)
        && savedBackupCatalog.nativeResult === actualBackupCatalog && savedBackupCatalog.revision === backupCatalogRevision
        && savedBackupCatalog.dto.catalogId !== consumedBackupCatalogId
    readonly property var backupDiagnosticsRows: savedBackupDiagnostics ? savedBackupDiagnostics.dto.entries : []
    readonly property bool backupDiagnosticsCurrent: !!savedBackupDiagnostics && basisCurrent(savedBackupDiagnostics) && backupBasisCurrent(savedBackupDiagnostics)
        && savedBackupDiagnostics.nativeResult === actualBackupDiagnostics && savedBackupDiagnostics.revision === backupDiagnosticsRevision
        && savedBackupDiagnostics.dto.diagnosticsId !== consumedBackupDiagnosticsId
    readonly property var selectedDiagnosticEntry: findEntry(backupDiagnosticsRows, selectedDiagnosticEntryId)
    readonly property bool canPrepareImport: idle && backupDiagnosticsCurrent && !!selectedDiagnosticEntry && selectedDiagnosticEntry.status === "readable"
    readonly property bool importPreviewCurrent: !!savedImportPreview && basisCurrent(savedImportPreview) && backupBasisCurrent(savedImportPreview)
        && backupDiagnosticsCurrent && savedImportPreview.diagnostics === savedBackupDiagnostics
        && savedImportPreview.nativeResult === actualPreview && savedImportPreview.revision === previewRevision
        && savedImportPreview.selectionRevision === diagnosticSelectionRevision && savedImportPreview.dto.entryId === selectedDiagnosticEntryId
        && savedImportPreview.dto.status === "prepared" && savedImportPreview.dto.canCommit === true
        && savedImportPreview.dto.previewId !== consumedImportPreviewId && savedImportPreview.dto.destinationDirectory === options.directoryPath
    readonly property bool canCommitImport: idle && importPreviewCurrent && confirmedImportPreview === savedImportPreview
    readonly property bool canDiscardImport: idle && importPreviewCurrent
    readonly property bool previewCurrent: !!savedPreview && basisCurrent(savedPreview) && inventoryCurrent
        && savedPreview.inventory === savedInventory && savedPreview.nativeResult === actualPreview
        && savedPreview.revision === previewRevision && savedPreview.selectionRevision === selectionRevision
        && savedPreview.dto.entryId === selectedEntryId && savedPreview.dto.status === "prepared" && savedPreview.dto.canCommit === true
        && savedPreview.dto.previewId !== consumedPreviewId
        && backupBasisCurrent(savedPreview)
    readonly property bool canCommit: idle && previewCurrent && confirmedPreview === savedPreview
    readonly property bool canDiscard: idle && previewCurrent
    readonly property bool canCancel: !!ownedOperation && operationSourceReady(ownedOperation) && !callBusy && !ownedOperation.adopted
        && basisCurrent(ownedOperation) && uuid(ownedOperation.requestId)
        && ["inventory", "commit", "backup-catalog", "backup-diagnostics", "commit-import"].indexOf(ownedOperation.intent.action) >= 0
        && current.archiveMaintenanceActive === true && workerKnown(current.archiveMaintenanceWorker, ownedOperation)
        && cancelledQueryId !== ownedOperation.requestId

    implicitWidth: ScreenTools.defaultFontPixelWidth * 40
    implicitHeight: form.implicitHeight
    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    function map(value) { return !!value && typeof value === "object" && !Array.isArray(value) }
    function integer(value, min, max) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= min && value <= max }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function digest(value) { return typeof value === "string" && /^[0-9a-f]{64}$/.test(value) }
    function directoryIdentity(value) {
        return typeof value === "string" && value.length <= 256
            && (/^windows-volume-file-index:[0-9a-f]{8}:[0-9a-f]{8}:[0-9a-f]{8}$/.test(value)
                || /^posix-device-inode:(0|[1-9][0-9]{0,79}):(0|[1-9][0-9]{0,79})$/.test(value))
    }
    function decimal(value) { return typeof value === "string" && /^(0|[1-9][0-9]{0,18})$/.test(value) && (value.length < 19 || value <= "9223372036854775807") }
    function backupPathCandidate(value) {
        // A syntax hint only. Native code checks existing/canonical/local
        // directory identity without trimming or rewriting the user's value.
        return typeof value === "string" && value.length > 1 && value.length <= 4096 && value.indexOf("\u0000") < 0
            && value.indexOf("\\") < 0 && value.charAt(value.length - 1) !== "/"
            && (/^[a-zA-Z]:\//.test(value) || (value.charAt(0) === "/" && value.slice(0, 2) !== "//"))
    }
    function backupBasisCurrent(value) {
        return !value.backup || (!backupPathExhausted && value.backupPathRevision === backupPathRevision && value.backupDirectory === backupDirectory)
    }
    function text(value) { return typeof value === "string" && value.length ? value : qsTr("未知 / 未记录") }
    function known(value) { return value === true ? qsTr("是") : value === false ? qsTr("否") : qsTr("未知") }
    function optionStamp(value, allowEmptyDirectory) {
        if (!map(value) || Object.keys(value).length !== 7 || value.schemaVersion !== 1
                || typeof value.directoryPath !== "string" || (!allowEmptyDirectory && !value.directoryPath.length) || value.directoryPath.length > 4096
                || !map(value.policy) || Object.keys(value.policy).length !== 7
                || !integer(value.maxSealedSegments, 1, 128) || !integer(value.pageSize, 1, 256)
                || value.defaultEnabled !== false || value.storesRawPayload !== false) return ""
        try { return JSON.stringify([value.directoryPath, value.policy, value.maxSealedSegments, value.pageSize, false, false]) }
        catch (error) { return "" }
    }
    function copyValue(value) {
        var budget = {nodes: 0, chars: 0}
        function copy(item, depth) {
            if (depth > 12 || ++budget.nodes > 32768) throw new Error("maintenance_value_limit")
            if (item === null || typeof item === "boolean") return item
            if (typeof item === "number") { if (!isFinite(item) || Math.abs(item) > 9007199254740991) throw new Error("maintenance_number_invalid"); return item }
            if (typeof item === "string") { budget.chars += item.length; if (budget.chars > 1048576) throw new Error("maintenance_text_limit"); return item }
            if (!item || typeof item !== "object") throw new Error("maintenance_value_invalid")
            if (Array.isArray(item) || typeof item.length === "number") {
                if (!integer(item.length, 0, 256)) throw new Error("maintenance_array_limit")
                var rows = []; for (var i = 0; i < item.length; ++i) rows.push(copy(item[i], depth + 1)); return rows
            }
            var keys = Object.keys(item), result = {}
            if (keys.length > 64) throw new Error("maintenance_map_limit")
            for (var k = 0; k < keys.length; ++k) {
                var key = keys[k]
                if (key.length > 128 || ["__proto__", "constructor", "prototype", "toJSON"].indexOf(key) >= 0) throw new Error("maintenance_key_invalid")
                budget.chars += key.length; if (budget.chars > 1048576) throw new Error("maintenance_text_limit")
                result[key] = copy(item[key], depth + 1)
            }
            return result
        }
        try { return copy(value, 0) } catch (error) { return null }
    }
    function inert(value) {
        if (!map(value) || value.historicalOnly !== true) return false
        var flags = ["authenticated", "signatureVerified", "permissionsRestored", "executionPermit", "flightCommandReleased", "executable",
            "completionVerified", "restoresAuthorization", "allRecordHashesVerified", "catalogCompletenessVerified", "crossSegmentAtomicSnapshot"]
        for (var i = 0; i < flags.length; ++i) if (value[flags[i]] !== false) return false
        return true
    }
    function descriptor(value, publicEntry) {
        if (!map(value) || Object.keys(value).length !== (publicEntry ? 8 : 7) || !uuid(value.databaseEpoch) || !uuid(value.sealRunId)
                || ["sealed", "retired"].indexOf(value.kind) < 0 || !decimal(value.fileBytes) || !decimal(value.modifiedUtcMs)
                || Number(value.fileBytes) < 512 || Number(value.fileBytes) > 4294967296 || value.modifiedUtcMs === "0"
                || !digest(value.fileFingerprint) || (publicEntry && !uuid(value.entryId))) return false
        return value.basename === "fleet-events-" + value.kind + "-" + value.databaseEpoch + "-" + value.sealRunId + ".sqlite"
    }
    function sameDescriptor(first, second) {
        return first.basename === second.basename && first.kind === second.kind && first.databaseEpoch === second.databaseEpoch
            && first.sealRunId === second.sealRunId && first.fileBytes === second.fileBytes
            && first.fileFingerprint === second.fileFingerprint && first.modifiedUtcMs === second.modifiedUtcMs
    }
    function inventoryKnown(value, publicInventory) {
        if (!inert(value) || Object.keys(value).length !== (publicInventory ? 22 : 19) || value.schemaVersion !== 1
                || value.format !== "merivus-fleet-archive-maintenance-inventory" || !directoryIdentity(value.directoryIdentity)
                || value.enumerationCompleted !== true || !Array.isArray(value.entries) || value.entries.length > 256
                || !integer(value.sealedCount, 0, 128) || !integer(value.retiredCount, 0, 128)
                || value.sealedCount + value.retiredCount !== value.entries.length
                || (publicInventory && (!uuid(value.inventoryId) || !uuid(value.queryId) || typeof value.runId !== "string"))) return false
        var names = {}, ids = {}, sealed = 0
        for (var i = 0; i < value.entries.length; ++i) {
            var row = value.entries[i]
            if (!descriptor(row, publicInventory) || names[row.basename] || (publicInventory && ids[row.entryId])) return false
            names[row.basename] = true; if (publicInventory) ids[row.entryId] = true
            if (row.kind === "sealed") ++sealed
        }
        return sealed === value.sealedCount
    }
    function sameInventory(publicValue, workerValue) {
        if (!inventoryKnown(publicValue, true) || !inventoryKnown(workerValue, false)
                || publicValue.directoryIdentity !== workerValue.directoryIdentity || publicValue.sealedCount !== workerValue.sealedCount
                || publicValue.retiredCount !== workerValue.retiredCount || publicValue.entries.length !== workerValue.entries.length) return false
        for (var i = 0; i < publicValue.entries.length; ++i) if (!sameDescriptor(publicValue.entries[i], workerValue.entries[i])) return false
        return true
    }
    function backupCatalogEntryKnown(value, publicEntry) {
        return map(value) && Object.keys(value).length === (publicEntry ? 9 : 8) && value.kind === "backup"
            && uuid(value.databaseEpoch) && uuid(value.sealRunId) && uuid(value.backupOperationId)
            && value.basename === "fleet-events-backup-" + value.databaseEpoch + "-" + value.sealRunId + "-" + value.backupOperationId + ".sqlite"
            && decimal(value.fileBytes) && Number(value.fileBytes) >= 512 && Number(value.fileBytes) <= 4294967296
            && decimal(value.modifiedUtcMs) && value.modifiedUtcMs !== "0" && digest(value.fileFingerprint)
            && (!publicEntry || uuid(value.entryId))
    }
    function backupCatalogKnown(value, publicCatalog, directory) {
        if (!inert(value) || Object.keys(value).length !== (publicCatalog ? 24 : 21) || value.schemaVersion !== 1
                || value.format !== "merivus-fleet-archive-backup-catalog" || value.destinationDirectory !== directory
                || !backupPathCandidate(value.destinationDirectory) || !directoryIdentity(value.directoryIdentity)
                || !Array.isArray(value.entries) || value.entries.length > 128 || !integer(value.fileCount, 0, 128)
                || value.fileCount !== value.entries.length || value.enumerationCompleted !== true
                || value.originalSourceVerified !== false || value.backupSetCompletenessVerified !== false
                || (publicCatalog && (!uuid(value.catalogId) || !uuid(value.queryId) || typeof value.runId !== "string"))) return false
        var names = {}, ids = {}, previous = ""
        for (var i = 0; i < value.entries.length; ++i) {
            var row = value.entries[i]
            if (!backupCatalogEntryKnown(row, publicCatalog) || row.basename <= previous || names[row.basename] || (publicCatalog && ids[row.entryId])) return false
            names[row.basename] = true; if (publicCatalog) ids[row.entryId] = true
            previous = row.basename
        }
        return true
    }
    function sameBackupCatalog(publicValue, workerValue, directory) {
        if (!backupCatalogKnown(publicValue, true, directory) || !backupCatalogKnown(workerValue, false, directory)
                || publicValue.directoryIdentity !== workerValue.directoryIdentity || publicValue.fileCount !== workerValue.fileCount) return false
        for (var i = 0; i < publicValue.entries.length; ++i) {
            if (!sameDescriptor(publicValue.entries[i], workerValue.entries[i])
                    || publicValue.entries[i].backupOperationId !== workerValue.entries[i].backupOperationId) return false
        }
        return true
    }
    function diagnosticNameKnown(value) {
        if (typeof value !== "string" || value.length < 1 || value.length > 512 || value.indexOf("\u0000") >= 0 || value.indexOf("/") >= 0) return false
        for (var i = 0; i < value.length; ++i) {
            var unit = value.charCodeAt(i)
            if (unit >= 0xd800 && unit <= 0xdbff) {
                if (++i >= value.length) return false
                var low = value.charCodeAt(i)
                if (low < 0xdc00 || low > 0xdfff) return false
            } else if (unit >= 0xdc00 && unit <= 0xdfff) return false
        }
        // A POSIX bad name may contain a backslash. It remains a plain-text
        // rejected row; never normalize it into a usable path or source.
        return true
    }
    function diagnosticReasonKnown(value) {
        return ["archive-backup-name-invalid", "archive-backup-type-rejected", "archive-backup-size-rejected", "archive-backup-sidecars-present",
            "archive-backup-file-rejected", "archive-backup-structure-unconfirmed"].indexOf(value) >= 0
    }
    function backupDiagnosticsKnown(value, publicValue, directory) {
        if (!inert(value) || Object.keys(value).length !== (publicValue ? 26 : 23) || value.schemaVersion !== 1
                || value.format !== "merivus-fleet-archive-backup-diagnostics" || value.destinationDirectory !== directory
                || !backupPathCandidate(directory) || !directoryIdentity(value.directoryIdentity)
                || !Array.isArray(value.entries) || value.entries.length > 128 || !integer(value.candidateCount, 0, 128)
                || value.candidateCount !== value.entries.length || !integer(value.readableCount, 0, 128) || !integer(value.rejectedCount, 0, 128)
                || value.readableCount + value.rejectedCount !== value.candidateCount || value.enumerationCompleted !== true
                || value.originalSourceVerified !== false || value.backupSetCompletenessVerified !== false
                || (publicValue && (!uuid(value.diagnosticsId) || !uuid(value.queryId) || typeof value.runId !== "string"))) return false
        var previous = "", readable = 0, ids = {}
        for (var i = 0; i < value.entries.length; ++i) {
            var row = value.entries[i]
            if (!map(row) || Object.keys(row).length !== (publicValue ? 5 : 4) || !diagnosticNameKnown(row.basename) || row.basename <= previous
                    || !map(row.descriptor) || (publicValue && (!uuid(row.entryId) || ids[row.entryId]))) return false
            if (row.status === "readable") {
                if (row.errorCode !== "" || !backupCatalogEntryKnown(row.descriptor, false) || row.basename !== row.descriptor.basename) return false
                ++readable
            } else if (row.status !== "rejected" || !diagnosticReasonKnown(row.errorCode) || Object.keys(row.descriptor).length !== 0) return false
            previous = row.basename; if (publicValue) ids[row.entryId] = true
        }
        return readable === value.readableCount
    }
    function sameBackupDiagnostics(publicValue, workerValue, directory) {
        if (!backupDiagnosticsKnown(publicValue, true, directory) || !backupDiagnosticsKnown(workerValue, false, directory)
                || publicValue.directoryIdentity !== workerValue.directoryIdentity || publicValue.candidateCount !== workerValue.candidateCount
                || publicValue.readableCount !== workerValue.readableCount || publicValue.rejectedCount !== workerValue.rejectedCount) return false
        for (var i = 0; i < publicValue.entries.length; ++i) {
            var first = publicValue.entries[i], second = workerValue.entries[i]
            if (first.basename !== second.basename || first.status !== second.status || first.errorCode !== second.errorCode) return false
            if (first.status === "readable" && (!sameDescriptor(first.descriptor, second.descriptor)
                    || first.descriptor.backupOperationId !== second.descriptor.backupOperationId)) return false
        }
        return true
    }
    function importFactsKnown(value, sourceBytes) {
        var flags = ["decisionFinal", "stagingCreateAttempted", "stagingCreated", "mayHaveWritten", "copyCompleted", "stagingVerified",
            "publishAttempted", "publishReportedSuccess", "mayHavePublished", "importVerified"]
        for (var i = 0; i < flags.length; ++i) if (typeof value[flags[i]] !== "boolean") return false
        return decimal(value.bytesCopied) && Number(value.bytesCopied) <= Number(sourceBytes)
            && value.decisionFinal === value.stagingCreateAttempted && value.mayHaveWritten === value.stagingCreateAttempted
            && (!value.stagingCreated || value.stagingCreateAttempted) && (value.stagingCreated || value.bytesCopied === "0")
            && (!value.copyCompleted || (value.stagingCreated && value.bytesCopied === sourceBytes))
            && (!value.stagingVerified || value.copyCompleted) && value.publishAttempted === value.mayHavePublished
            && (!value.publishAttempted || value.stagingVerified) && (!value.publishReportedSuccess || value.publishAttempted)
            && (!value.importVerified || (value.publishAttempted && value.stagingVerified))
    }
    function importResultKnown(value, operation) {
        var facts = value.workerResult, preview = operation.preview.dto
        if (!inert(facts) || Object.keys(facts).length !== 38 || facts.schemaVersion !== 1
                || facts.format !== "merivus-fleet-archive-backup-import" || facts.kind !== "import" || facts.operationId !== preview.previewId
                || !backupCatalogEntryKnown(facts.source, false) || !sameDescriptor(facts.source, preview.source)
                || facts.source.backupOperationId !== preview.source.backupOperationId || facts.sourceDirectory !== preview.sourceDirectory
                || facts.expectedDirectoryIdentity !== preview.sourceDirectoryIdentity || typeof facts.observedDirectoryIdentity !== "string"
                || (facts.observedDirectoryIdentity !== "" && !directoryIdentity(facts.observedDirectoryIdentity))
                || facts.destinationDirectory !== preview.destinationDirectory || typeof facts.destinationDirectoryIdentity !== "string"
                || (facts.destinationDirectoryIdentity !== "" && !directoryIdentity(facts.destinationDirectoryIdentity))
                || facts.stagingBasename !== preview.stagingBasename || facts.targetBasename !== preview.targetBasename
                || !map(facts.targetDescriptor) || !importFactsKnown(facts, preview.source.fileBytes) || typeof facts.cancelObserved !== "boolean"
                || typeof facts.errorCode !== "string" || facts.errorCode !== value.errorCode
                || (facts.decisionFinal && (facts.observedDirectoryIdentity !== facts.expectedDirectoryIdentity || !directoryIdentity(facts.destinationDirectoryIdentity)))) return false
        if (!facts.importVerified) return Object.keys(facts.targetDescriptor).length === 0 && value.errorCode.length > 0
            && value.status === (facts.mayHavePublished ? "import-uncertain" : facts.mayHaveWritten ? "import-incomplete" : "not-imported")
        var target = facts.targetDescriptor
        return value.status === "import-verified" && value.errorCode === "" && descriptor(target, false) && target.kind === "retired"
            && target.basename === preview.targetBasename && target.databaseEpoch === preview.source.databaseEpoch
            && target.sealRunId === preview.source.sealRunId && target.fileBytes === preview.source.fileBytes && target.fileFingerprint === preview.source.fileFingerprint
    }
    function importPreviewKnown(value, operation) {
        var diagnostics = operation.diagnostics, entry = diagnostics ? findEntry(diagnostics.dto.entries, operation.intent.entryId) : null
        if (!inert(value) || Object.keys(value).length !== 29 || value.schemaVersion !== 1
                || value.format !== "merivus-fleet-archive-import-preview" || value.action !== "import" || value.historicalImportOnly !== true
                || value.status !== "prepared" || value.canCommit !== true || value.expiresAfterMs !== 30000
                || value.previewId !== operation.requestId || !uuid(value.previewId) || !entry || entry.status !== "readable"
                || value.diagnosticsId !== diagnostics.dto.diagnosticsId || value.entryId !== entry.entryId || value.runId !== operation.runId
                || !backupCatalogEntryKnown(value.source, false) || !sameDescriptor(value.source, entry.descriptor)
                || value.source.backupOperationId !== entry.descriptor.backupOperationId
                || value.sourceDirectory !== diagnostics.dto.destinationDirectory || value.sourceDirectoryIdentity !== diagnostics.dto.directoryIdentity
                || value.destinationDirectory !== options.directoryPath) return false
        return value.stagingBasename === "fleet-events-backup-" + value.source.databaseEpoch + "-" + value.source.sealRunId + "-" + value.previewId + ".sqlite"
            && value.targetBasename === "fleet-events-retired-" + value.source.databaseEpoch + "-" + value.source.sealRunId + ".sqlite"
    }
    function diagnosticSelectionSnapshot(id) {
        return {owner: archive, epoch: lifetime.epoch, optionsStamp: optionsStamp, runId: runId,
            diagnostics: savedBackupDiagnostics, entryId: id, backup: true, backupDirectory: backupDirectory, backupPathRevision: backupPathRevision}
    }
    function selectDiagnostic(pressed) {
        if (!pressed || !idle || !backupDiagnosticsCurrent || !basisCurrent(pressed) || !backupBasisCurrent(pressed)
                || pressed.diagnostics !== savedBackupDiagnostics) return
        var entry = findEntry(savedBackupDiagnostics.dto.entries, pressed.entryId)
        if (!entry || entry.status !== "readable") return
        selectedDiagnosticEntryId = entry.entryId
    }
    function importSnapshot(action) {
        var preview = action === "prepare-import" ? null : savedImportPreview
        return {owner: archive, epoch: lifetime.epoch, optionsStamp: optionsStamp, runId: runId, importOperation: true,
            diagnostics: savedBackupDiagnostics, selectionRevision: diagnosticSelectionRevision, preview: preview, approval: confirmedImportPreview,
            backup: true, backupDirectory: backupDirectory, backupPathRevision: backupPathRevision,
            intent: {action: action, expectedRunId: runId, inventoryId: "", entryId: selectedDiagnosticEntryId,
                previewId: preview ? preview.dto.previewId : "", queryId: "", diagnosticsId: savedBackupDiagnostics ? savedBackupDiagnostics.dto.diagnosticsId : ""}}
    }
    function importPressCurrent(pressed, consumed) {
        if (!pressed || !sourceReady || historyBusy || current.writerActive !== false || current.captureEnabled !== false
                || !basisCurrent(pressed) || !backupBasisCurrent(pressed) || pressed.diagnostics !== savedBackupDiagnostics || !savedBackupDiagnostics
                || !basisCurrent(savedBackupDiagnostics) || !backupBasisCurrent(savedBackupDiagnostics)
                || savedBackupDiagnostics.nativeResult !== actualBackupDiagnostics || savedBackupDiagnostics.revision !== backupDiagnosticsRevision
                || pressed.selectionRevision !== diagnosticSelectionRevision || !selectedDiagnosticEntry || selectedDiagnosticEntry.status !== "readable"
                || pressed.intent.expectedRunId !== runId || pressed.intent.inventoryId !== "" || pressed.intent.queryId !== ""
                || pressed.intent.entryId !== selectedDiagnosticEntryId || pressed.intent.diagnosticsId !== savedBackupDiagnostics.dto.diagnosticsId) return false
        var action = pressed.intent.action
        if (action === "prepare-import") return backupDiagnosticsCurrent && !pressed.preview && pressed.intent.previewId === ""
        if (["commit-import", "discard-import"].indexOf(action) < 0 || !savedImportPreview || pressed.preview !== savedImportPreview
                || !basisCurrent(savedImportPreview) || !backupBasisCurrent(savedImportPreview) || savedImportPreview.diagnostics !== savedBackupDiagnostics
                || savedImportPreview.nativeResult !== actualPreview || savedImportPreview.revision !== previewRevision
                || savedImportPreview.selectionRevision !== diagnosticSelectionRevision || savedImportPreview.dto.entryId !== selectedDiagnosticEntryId
                || savedImportPreview.dto.destinationDirectory !== options.directoryPath || savedImportPreview.dto.status !== "prepared" || savedImportPreview.dto.canCommit !== true
                || pressed.intent.previewId !== savedImportPreview.dto.previewId) return false
        if (consumed) return consumedImportPreviewId === pressed.intent.previewId && confirmedImportPreview === null
            && (action !== "commit-import" || (pressed.approval === pressed.preview && consumedBackupDiagnosticsId === pressed.intent.diagnosticsId))
        return importPreviewCurrent && (action !== "commit-import" || (pressed.approval === savedImportPreview && confirmedImportPreview === savedImportPreview))
    }
    function submitImport(pressed) {
        var life = lifetime
        if (!life.alive || life.call || callBusy || !importPressCurrent(pressed, false)) {
            if (life.alive) localError = qsTr("导入来源、当前可读行、路径或原预览/确认已变化，未发送；请重新诊断并阅读本次预览。")
            return
        }
        var owner = pressed.owner, epoch = life.epoch, ticket = {}, operation = null
        life.call = ticket
        try {
            callBusy = true
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || !importPressCurrent(pressed, false)) return
            localError = ""
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || !importPressCurrent(pressed, false)) return
            var consumed = pressed.intent.action !== "prepare-import"
            if (savedPreview) consumedPreviewId = savedPreview.dto.previewId
            if (savedImportPreview) consumedImportPreviewId = savedImportPreview.dto.previewId
            confirmedPreview = null; confirmedImportPreview = null
            if (pressed.intent.action === "commit-import") {
                consumedInventoryId = savedInventory ? savedInventory.dto.inventoryId : ""
                consumedBackupCatalogId = savedBackupCatalog ? savedBackupCatalog.dto.catalogId : ""
                consumedBackupDiagnosticsId = savedBackupDiagnostics.dto.diagnosticsId
            }
            operation = {owner: owner, epoch: epoch, optionsStamp: pressed.optionsStamp, runId: pressed.runId, requestId: "", importOperation: true,
                intent: pressed.intent, diagnostics: pressed.diagnostics, preview: pressed.preview, selectionRevision: pressed.selectionRevision,
                backup: true, backupDirectory: pressed.backupDirectory, backupPathRevision: pressed.backupPathRevision, adopted: false}
            ownedOperation = operation
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || ownedOperation !== operation || !importPressCurrent(pressed, consumed)) return
            var receipt = null
            try { receipt = owner.maintenance(pressed.intent, true) } catch (error) { receipt = null }
            if (!life.alive) return
            var value = copyValue(receipt)
            if (!receiptKnown(value, pressed.intent) || (pressed.intent.action === "discard-import" && value.accepted && value.requestId !== pressed.intent.previewId)) {
                localError = qsTr("导入回执无法核实，可能已有暂存或发布；不自动重试/回滚/清理，不复活旧确认。请查看原生事实并交工程师核查。")
                return
            }
            lastRequest = value
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || ownedOperation !== operation || !basisCurrent(pressed)) return
            if (!value.accepted) { localError = qsTr("导入请求未接受：%1。旧预览/确认不自动复用，需核对后重新预览。").arg(text(value.reason)); return }
            operation = {owner: owner, epoch: epoch, optionsStamp: pressed.optionsStamp, runId: pressed.runId, requestId: value.requestId, importOperation: true,
                intent: pressed.intent, diagnostics: pressed.diagnostics, preview: pressed.preview, selectionRevision: pressed.selectionRevision,
                backup: true, backupDirectory: pressed.backupDirectory, backupPathRevision: pressed.backupPathRevision, adopted: false}
            ownedOperation = operation
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || ownedOperation !== operation) return
            cancelledQueryId = ""
            if (life.alive && life.call === ticket && life.epoch === epoch && ownedOperation === operation) adoptImport(operation)
        } finally { if (life.alive && life.call === ticket) { life.call = null; callBusy = false } }
    }
    function adoptImport(operation) {
        var life = lifetime, owner = archive, stamp = optionsStamp
        if (!life.alive || ownedOperation !== operation || operation.adopted || !basisCurrent(operation) || !uuid(operation.requestId)) return
        if (operation.intent.action === "prepare-import") {
            if (!backupDiagnosticsCurrent || operation.diagnostics !== savedBackupDiagnostics || operation.selectionRevision !== diagnosticSelectionRevision
                    || operation.intent.entryId !== selectedDiagnosticEntryId || !backupBasisCurrent(operation)) return
            var nativePreview = actualPreview, rev = previewRevision, value = copyValue(nativePreview)
            if (!importPreviewKnown(value, operation)) return
            if (!life.alive || ownedOperation !== operation || archive !== owner || stamp !== optionsStamp || !basisCurrent(operation) || !backupBasisCurrent(operation)
                    || !backupDiagnosticsCurrent || operation.diagnostics !== savedBackupDiagnostics || operation.selectionRevision !== diagnosticSelectionRevision
                    || operation.intent.entryId !== selectedDiagnosticEntryId || rev !== previewRevision || nativePreview !== actualPreview) return
            operation.adopted = true
            savedImportPreview = {owner: owner, epoch: operation.epoch, optionsStamp: stamp, runId: operation.runId, requestId: operation.requestId,
                diagnostics: operation.diagnostics, selectionRevision: operation.selectionRevision, nativeResult: nativePreview, revision: rev, dto: value,
                backup: true, backupDirectory: operation.backupDirectory, backupPathRevision: operation.backupPathRevision}
            confirmedImportPreview = null
            return
        }
        if (operation.intent.action !== "commit-import" || current.archiveMaintenanceActive !== false) return
        var nativeResult = actualResult, result = copyValue(nativeResult)
        if (!resultKnown(result, operation)) return
        if (!life.alive || ownedOperation !== operation || archive !== owner || stamp !== optionsStamp || !basisCurrent(operation)
                || current.archiveMaintenanceActive !== false || nativeResult !== actualResult) return
        operation.adopted = true
        savedResult = {owner: owner, epoch: operation.epoch, optionsStamp: stamp, runId: operation.runId,
            requestId: operation.requestId, nativeResult: nativeResult, dto: result}
    }
    function importConfirmationSnapshot() {
        if (!idle || !importPreviewCurrent) return null
        return {owner: archive, epoch: lifetime.epoch, optionsStamp: optionsStamp, runId: runId, preview: savedImportPreview,
            diagnostics: savedBackupDiagnostics, selectionRevision: diagnosticSelectionRevision, backup: true,
            backupDirectory: backupDirectory, backupPathRevision: backupPathRevision}
    }
    function confirmImport(pressed, checked) {
        if (!checked) { confirmedImportPreview = null; return }
        if (!pressed || !idle || !importPreviewCurrent || !basisCurrent(pressed) || !backupBasisCurrent(pressed) || pressed.preview !== savedImportPreview
                || pressed.diagnostics !== savedBackupDiagnostics || pressed.selectionRevision !== diagnosticSelectionRevision) {
            confirmedImportPreview = null
            localError = qsTr("按下导入确认后来源、路径、所选行或原预览已变，未确认新的预览。")
            return
        }
        confirmedImportPreview = pressed.preview
    }
    function previewKnown(value, operation) {
        var inventory = operation.inventory, entry = inventory ? findEntry(inventory.dto.entries, operation.intent.entryId) : null
        var backup = operation.intent.action === "prepare-backup"
        if (!inert(value) || Object.keys(value).length !== (backup ? 26 : 25) || value.schemaVersion !== (backup ? 2 : 1)
                || value.format !== "merivus-fleet-archive-maintenance-preview" || value.status !== "prepared" || value.canCommit !== true
                || value.previewId !== operation.requestId || !uuid(value.previewId) || !entry
                || value.runId !== operation.runId || value.inventoryId !== operation.intent.inventoryId || value.entryId !== operation.intent.entryId
                || value.action !== (backup ? "backup" : operation.intent.action === "prepare-retire" ? "retire" : "restore")
                || !descriptor(value.source, false) || !sameDescriptor(value.source, entry)
                || value.maximumSealedSegments !== 32 || value.expiresAfterMs !== 30000) return false
        if (backup) return backupBasisCurrent(operation) && value.backupDirectory === operation.intent.backupDirectory
            && value.backupDirectory === operation.backupDirectory && backupPathCandidate(value.backupDirectory)
            && value.targetBasename === "fleet-events-backup-" + entry.databaseEpoch + "-" + entry.sealRunId + "-" + value.previewId + ".sqlite"
        return value.targetBasename === "fleet-events-" + (value.action === "retire" ? "retired" : "sealed")
            + "-" + entry.databaseEpoch + "-" + entry.sealRunId + ".sqlite"
    }
    function resultKnown(value, operation) {
        if (!(inert(value) && Object.keys(value).length === 20 && value.schemaVersion === 1
            && value.format === "merivus-fleet-archive-maintenance-result" && value.queryId === operation.requestId
            && value.runId === operation.runId && value.action === (["inventory", "backup-catalog", "backup-diagnostics"].indexOf(operation.intent.action) >= 0 ? operation.intent.action : operation.preview.dto.action)
            && ["inventory-complete", "backup-catalog-complete", "backup-diagnostics-complete", "move-confirmed", "move-uncertain", "not-moved", "backup-verified", "backup-uncertain", "not-backed-up", "import-verified", "import-uncertain", "import-incomplete", "not-imported", "failed"].indexOf(value.status) >= 0
            && typeof value.errorCode === "string" && map(value.workerResult))) return false
        var facts = value.workerResult
        if (value.status === "failed") return Object.keys(facts).length === 0 && value.errorCode.length > 0
        if (operation.intent.action === "inventory") return value.status === "inventory-complete" && value.errorCode === "" && inventoryKnown(facts, false)
        if (operation.catalogRead) return value.status === "backup-catalog-complete" && value.errorCode === ""
            && backupCatalogKnown(facts, false, operation.backupDirectory)
        if (operation.diagnosticsRead) return value.status === "backup-diagnostics-complete" && value.errorCode === ""
            && backupDiagnosticsKnown(facts, false, operation.backupDirectory)
        if (value.action === "import") return importResultKnown(value, operation)
        if (value.action === "backup") return backupResultKnown(value, operation)
        if (!inert(facts) || Object.keys(facts).length !== 27 || facts.schemaVersion !== 1
                || facts.format !== "merivus-fleet-archive-maintenance-move" || facts.kind !== value.action || !uuid(facts.operationId)
                || !descriptor(facts.source, false) || !sameDescriptor(facts.source, operation.preview.dto.source)
                || facts.targetBasename !== operation.preview.dto.targetBasename
                || facts.expectedDirectoryIdentity !== operation.inventory.dto.directoryIdentity
                || typeof facts.observedDirectoryIdentity !== "string"
                || (facts.observedDirectoryIdentity !== "" && !directoryIdentity(facts.observedDirectoryIdentity))
                || typeof facts.errorCode !== "string" || facts.errorCode !== value.errorCode) return false
        var flags = ["decisionFinal", "renameAttempted", "renameReportedSuccess", "mayHaveMoved", "moveConfirmed", "cancelObserved"]
        for (var i = 0; i < flags.length; ++i) if (typeof facts[flags[i]] !== "boolean") return false
        if (facts.decisionFinal !== facts.renameAttempted || facts.mayHaveMoved !== facts.renameAttempted
                || (facts.renameReportedSuccess && !facts.renameAttempted)
                || (facts.decisionFinal && facts.observedDirectoryIdentity !== facts.expectedDirectoryIdentity)) return false
        return facts.moveConfirmed ? value.status === "move-confirmed" && facts.decisionFinal && value.errorCode === ""
            : value.status === (facts.mayHaveMoved ? "move-uncertain" : "not-moved") && value.errorCode.length > 0
    }
    function backupResultKnown(value, operation) {
        var facts = value.workerResult, preview = operation.preview.dto
        if (!inert(facts) || Object.keys(facts).length !== 31 || facts.schemaVersion !== 1
                || facts.format !== "merivus-fleet-archive-maintenance-backup" || facts.kind !== "backup"
                || facts.operationId !== preview.previewId || !uuid(facts.operationId)
                || !descriptor(facts.source, false) || !sameDescriptor(facts.source, preview.source)
                || facts.targetBasename !== preview.targetBasename || facts.expectedDirectoryIdentity !== operation.inventory.dto.directoryIdentity
                || typeof facts.observedDirectoryIdentity !== "string"
                || (facts.observedDirectoryIdentity !== "" && !directoryIdentity(facts.observedDirectoryIdentity))
                || facts.destinationDirectory !== preview.backupDirectory || !backupPathCandidate(facts.destinationDirectory)
                || typeof facts.destinationDirectoryIdentity !== "string"
                || (facts.destinationDirectoryIdentity !== "" && !directoryIdentity(facts.destinationDirectoryIdentity))
                || !decimal(facts.bytesCopied) || Number(facts.bytesCopied) > Number(facts.source.fileBytes)
                || typeof facts.errorCode !== "string" || facts.errorCode !== value.errorCode) return false
        var flags = ["decisionFinal", "targetCreateAttempted", "targetCreated", "mayHaveWritten", "copyCompleted", "backupVerified", "cancelObserved"]
        for (var i = 0; i < flags.length; ++i) if (typeof facts[flags[i]] !== "boolean") return false
        if (facts.decisionFinal !== facts.targetCreateAttempted || facts.mayHaveWritten !== facts.targetCreateAttempted
                || (facts.targetCreated && !facts.targetCreateAttempted) || (!facts.targetCreated && facts.bytesCopied !== "0")
                || (facts.copyCompleted && (!facts.targetCreated || facts.bytesCopied !== facts.source.fileBytes))
                || (facts.decisionFinal && (facts.observedDirectoryIdentity !== facts.expectedDirectoryIdentity || !directoryIdentity(facts.destinationDirectoryIdentity)))) return false
        return facts.backupVerified ? value.status === "backup-verified" && facts.copyCompleted && value.errorCode === ""
                && facts.observedDirectoryIdentity === facts.expectedDirectoryIdentity && directoryIdentity(facts.destinationDirectoryIdentity)
            : value.status === (facts.mayHaveWritten ? "backup-uncertain" : "not-backed-up") && value.errorCode.length > 0
    }
    function workerKnown(value, operation) {
        if (operation.intent.action === "commit-import") {
            return inert(value) && Object.keys(value).length === 27 && value.requestId === operation.requestId
                && ["created", "opening", "inspecting", "copying", "verifying", "publishing", "result-submitted", "failed", "cancelled"].indexOf(value.state) >= 0
                && typeof value.errorCode === "string" && typeof value.cancelRequested === "boolean"
                && importFactsKnown(value, operation.preview.dto.source.fileBytes)
                && (!value.importVerified || value.errorCode === "")
        }
        if (inspectionRead(operation)) {
            return inert(value) && Object.keys(value).length === 19 && value.requestId === operation.requestId
                && ["created", "opening", "inspecting", "result-submitted", "failed", "cancelled"].indexOf(value.state) >= 0
                && typeof value.errorCode === "string" && value.decisionFinal === false && value.mayHaveMoved === false && value.moveConfirmed === false
                && typeof value.cancelRequested === "boolean"
        }
        if (operation.intent.action === "commit" && operation.preview.dto.action === "backup") {
            return inert(value) && Object.keys(value).length === 23 && value.requestId === operation.requestId
                && ["created", "opening", "inspecting", "copying", "verifying", "result-submitted", "failed", "cancelled"].indexOf(value.state) >= 0
                && typeof value.errorCode === "string" && typeof value.decisionFinal === "boolean" && typeof value.targetCreateAttempted === "boolean"
                && typeof value.targetCreated === "boolean" && typeof value.mayHaveWritten === "boolean" && decimal(value.bytesCopied)
                && typeof value.copyCompleted === "boolean" && typeof value.backupVerified === "boolean" && typeof value.cancelRequested === "boolean"
                && Number(value.bytesCopied) <= Number(operation.preview.dto.source.fileBytes)
                && value.decisionFinal === value.targetCreateAttempted && value.mayHaveWritten === value.targetCreateAttempted
                && (!value.targetCreated || value.targetCreateAttempted) && (value.targetCreated || value.bytesCopied === "0")
                && (!value.copyCompleted || (value.targetCreated && value.bytesCopied === operation.preview.dto.source.fileBytes))
                && (!value.backupVerified || (value.copyCompleted && value.errorCode === ""))
        }
        return inert(value) && Object.keys(value).length === 19 && value.requestId === operation.requestId
            && ["created", "opening", "inspecting", "moving", "confirming", "result-submitted", "failed", "cancelled"].indexOf(value.state) >= 0
            && typeof value.errorCode === "string" && typeof value.decisionFinal === "boolean" && typeof value.mayHaveMoved === "boolean"
            && typeof value.moveConfirmed === "boolean" && typeof value.cancelRequested === "boolean"
    }
    function receiptKnown(value, intent) {
        return map(value) && Object.keys(value).length === 9 && value.schemaVersion === 1 && value.action === intent.action
            && typeof value.accepted === "boolean" && typeof value.reason === "string" && typeof value.requestId === "string"
            && (uuid(value.requestId) || (!value.accepted && value.requestId === "")) && value.requestOnly === true
            && value.historicalOnly === true && value.permissionsRestored === false && value.flightCommandReleased === false
            && (intent.action !== "cancel" || value.requestId === intent.queryId || (!value.accepted && value.requestId === ""))
            && (intent.action !== "discard-preview" || value.requestId === intent.previewId || (!value.accepted && value.requestId === ""))
    }
    function findEntry(rows, id) { for (var i = 0; i < rows.length; ++i) if (rows[i].entryId === id) return rows[i]; return null }
    function inspectionRead(value) { return value.catalogRead === true || value.diagnosticsRead === true }
    function basisStamp(value) { return inspectionRead(value) ? catalogOptionsStamp : optionsStamp }
    function operationSourceReady(value) { return inspectionRead(value) ? catalogSourceReady : sourceReady }
    function basisCurrent(value) { return value.owner === archive && value.optionsStamp === basisStamp(value) && value.runId === runId && value.epoch === lifetime.epoch }
    function pressSnapshot(action) {
        var usesInventory = ["prepare-retire", "prepare-restore", "prepare-backup", "commit", "discard-preview"].indexOf(action) >= 0
        var usesPreview = action === "commit" || action === "discard-preview"
        var catalogRead = action === "backup-catalog"
        var diagnosticsRead = action === "backup-diagnostics", inspection = catalogRead || diagnosticsRead
        var backup = inspection || action === "prepare-backup" || (usesPreview && savedPreview && savedPreview.dto.action === "backup")
        var intent = {action: action, expectedRunId: runId, inventoryId: usesInventory && savedInventory ? savedInventory.dto.inventoryId : "",
            entryId: usesInventory ? selectedEntryId : "", previewId: usesPreview && savedPreview ? savedPreview.dto.previewId : "", queryId: ""}
        if (action === "prepare-backup" || inspection) intent.backupDirectory = backupDirectory
        return {owner: archive, epoch: lifetime.epoch, optionsStamp: inspection ? catalogOptionsStamp : optionsStamp, runId: runId, catalogRead: catalogRead, diagnosticsRead: diagnosticsRead,
            inventory: usesInventory ? savedInventory : null, preview: usesPreview ? savedPreview : null,
            selectionRevision: selectionRevision, approval: confirmedPreview, backup: !!backup,
            backupPathRevision: backupPathRevision, backupDirectory: backupDirectory, intent: intent}
    }
    function pressCurrent(pressed, consumed) {
        if (!pressed || !operationSourceReady(pressed) || historyBusy || current.writerActive !== false || current.captureEnabled !== false
                || !basisCurrent(pressed) || pressed.intent.expectedRunId !== runId) return false
        var action = pressed.intent.action
        if (action === "backup-catalog" || action === "backup-diagnostics") return inspectionRead(pressed)
            && pressed.catalogRead === (action === "backup-catalog") && pressed.diagnosticsRead === (action === "backup-diagnostics")
            && pressed.backup === true && !pressed.inventory && !pressed.preview
            && pressed.intent.inventoryId === "" && pressed.intent.entryId === "" && pressed.intent.previewId === "" && pressed.intent.queryId === ""
            && backupBasisCurrent(pressed) && backupPathCandidate(backupDirectory) && pressed.intent.backupDirectory === backupDirectory
        if (action === "inventory") return !pressed.inventory && !pressed.preview && pressed.intent.inventoryId === ""
            && pressed.intent.entryId === "" && pressed.intent.previewId === "" && pressed.intent.queryId === ""
        if (pressed.inventory !== savedInventory || !savedInventory || savedInventory.nativeResult !== actualInventory
                || !basisCurrent(savedInventory) || savedInventory.revision !== inventoryRevision
                || pressed.intent.inventoryId !== savedInventory.dto.inventoryId || pressed.intent.entryId !== selectedEntryId
                || pressed.selectionRevision !== selectionRevision || !selectedEntry || pressed.intent.queryId !== "") return false
        if (action === "prepare-retire" || action === "prepare-restore") return inventoryCurrent && !pressed.preview && pressed.intent.previewId === ""
            && selectedEntry.kind === (action === "prepare-retire" ? "sealed" : "retired")
        if (action === "prepare-backup") return inventoryCurrent && !pressed.preview && pressed.intent.previewId === "" && pressed.backup === true
            && backupBasisCurrent(pressed) && backupPathCandidate(backupDirectory) && pressed.intent.backupDirectory === backupDirectory
        if (["commit", "discard-preview"].indexOf(action) < 0 || pressed.preview !== savedPreview || !savedPreview
                || savedPreview.inventory !== savedInventory || !basisCurrent(savedPreview) || savedPreview.nativeResult !== actualPreview
                || savedPreview.revision !== previewRevision || savedPreview.selectionRevision !== selectionRevision
                || savedPreview.dto.status !== "prepared" || savedPreview.dto.canCommit !== true
                || pressed.intent.previewId !== savedPreview.dto.previewId || savedPreview.dto.entryId !== selectedEntryId
                || pressed.backup !== (savedPreview.dto.action === "backup") || !backupBasisCurrent(pressed) || !backupBasisCurrent(savedPreview)) return false
        if (consumed) return consumedPreviewId === pressed.intent.previewId && confirmedPreview === null
            && (action !== "commit" || pressed.approval === pressed.preview)
        return previewCurrent && (action !== "commit" || (pressed.approval === savedPreview && confirmedPreview === savedPreview))
    }
    function submit(pressed) {
        var life = lifetime
        if (!life.alive || life.call || callBusy || !pressCurrent(pressed, false)) {
            if (life.alive) localError = qsTr("来源、清单、选择、预览或确认已变化，未发送；请核对当前原生状态并重新预览。")
            return
        }
        var ticket = {}, epoch = life.epoch, owner = pressed.owner, operation = null
        life.call = ticket
        try {
            callBusy = true
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || !pressCurrent(pressed, false)) return
            localError = ""
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || !pressCurrent(pressed, false)) return
            var consumes = pressed.intent.action === "commit" || pressed.intent.action === "discard-preview"
            if (savedPreview) consumedPreviewId = savedPreview.dto.previewId
            if (savedImportPreview) consumedImportPreviewId = savedImportPreview.dto.previewId
            confirmedPreview = null
            confirmedImportPreview = null
            operation = {owner: owner, epoch: epoch, optionsStamp: pressed.optionsStamp, runId: pressed.runId, requestId: "",
                catalogRead: pressed.catalogRead, diagnosticsRead: pressed.diagnosticsRead,
                intent: pressed.intent, inventory: pressed.inventory, preview: pressed.preview, selectionRevision: pressed.selectionRevision,
                backup: pressed.backup, backupPathRevision: pressed.backupPathRevision, backupDirectory: pressed.backupDirectory, adopted: false}
            ownedOperation = operation
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || ownedOperation !== operation || !pressCurrent(pressed, consumes)) return
            // Consume locally before calling out: unknown receipt cannot revive
            // an approved preview, even if an old equal value is later restored.
            if (["inventory", "commit", "backup-catalog", "backup-diagnostics"].indexOf(pressed.intent.action) >= 0) {
                consumedInventoryId = savedInventory ? savedInventory.dto.inventoryId : ""
                consumedBackupCatalogId = savedBackupCatalog ? savedBackupCatalog.dto.catalogId : ""
                consumedBackupDiagnosticsId = savedBackupDiagnostics ? savedBackupDiagnostics.dto.diagnosticsId : ""
            }
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || archive !== owner || ownedOperation !== operation
                    || !pressCurrent(pressed, consumes)) return
            var receipt = null
            try { receipt = owner.maintenance(pressed.intent, true) } catch (error) { receipt = null }
            if (!life.alive) return
            var value = copyValue(receipt)
            if (!receiptKnown(value, pressed.intent)) { localError = qsTr("回执无法核实，效果未知；不自动重试、覆盖、回滚或复活旧确认，请查看实际工作线程及原生结果。"); return }
            lastRequest = value
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || archive !== owner || ownedOperation !== operation || !basisCurrent(pressed)) return
            if (!value.accepted) { localError = qsTr("请求未被接受：%1。旧清单/预览仅供历史阅读；重新读取或重新预览，不自动修复文件。").arg(text(value.reason)); return }
            operation = {owner: owner, epoch: epoch, optionsStamp: pressed.optionsStamp, runId: pressed.runId, requestId: value.requestId,
                catalogRead: pressed.catalogRead, diagnosticsRead: pressed.diagnosticsRead,
                intent: pressed.intent, inventory: pressed.inventory, preview: pressed.preview, selectionRevision: pressed.selectionRevision,
                backup: pressed.backup, backupPathRevision: pressed.backupPathRevision, backupDirectory: pressed.backupDirectory, adopted: false}
            ownedOperation = operation
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || ownedOperation !== operation || archive !== owner) return
            cancelledQueryId = ""
            if (life.alive && life.call === ticket && life.epoch === epoch && ownedOperation === operation) adopt()
        } finally { if (life.alive && life.call === ticket) { life.call = null; callBusy = false } }
    }
    function adopt() {
        var life = lifetime, operation = ownedOperation, owner = archive, stamp = operation ? basisStamp(operation) : optionsStamp
        if (!life.alive || !operation || operation.adopted || !basisCurrent(operation) || !uuid(operation.requestId)) return
        if (operation.importOperation) { adoptImport(operation); return }
        var preparing = ["prepare-retire", "prepare-restore", "prepare-backup"].indexOf(operation.intent.action) >= 0
        if (preparing) {
            if (!inventoryCurrent || operation.inventory !== savedInventory || operation.selectionRevision !== selectionRevision || operation.intent.entryId !== selectedEntryId
                    || !backupBasisCurrent(operation)) return
            var nativePreview = actualPreview, preview = copyValue(nativePreview), rev = previewRevision
            if (!previewKnown(preview, operation)) return
            if (!life.alive || ownedOperation !== operation || archive !== owner || stamp !== basisStamp(operation) || !basisCurrent(operation)
                    || nativePreview !== actualPreview || rev !== previewRevision || operation.inventory !== savedInventory || !inventoryCurrent
                    || operation.selectionRevision !== selectionRevision || operation.intent.entryId !== selectedEntryId || !backupBasisCurrent(operation)) return
            operation.adopted = true
            savedPreview = {owner: owner, epoch: operation.epoch, optionsStamp: stamp, runId: operation.runId, requestId: operation.requestId,
                inventory: operation.inventory, selectionRevision: operation.selectionRevision, nativeResult: nativePreview, revision: rev, dto: preview,
                backup: operation.backup, backupPathRevision: operation.backupPathRevision, backupDirectory: operation.backupDirectory}
            confirmedPreview = null
            return
        }
        if (["inventory", "commit", "backup-catalog", "backup-diagnostics"].indexOf(operation.intent.action) < 0 || current.archiveMaintenanceActive !== false) return
        var nativeResult = actualResult, result = copyValue(nativeResult)
        if (!resultKnown(result, operation)) return
        var nativeInventory = actualInventory, inventory = operation.intent.action === "inventory" && result.status === "inventory-complete" ? copyValue(nativeInventory) : null
        if (operation.intent.action === "inventory" && result.status === "inventory-complete"
                && (!inventory || inventory.queryId !== operation.requestId || inventory.runId !== operation.runId || !sameInventory(inventory, result.workerResult))) return
        var nativeCatalog = actualBackupCatalog, catalogRevision = backupCatalogRevision
        var catalog = operation.catalogRead && result.status === "backup-catalog-complete" ? copyValue(nativeCatalog) : null
        if (operation.catalogRead && result.status === "backup-catalog-complete"
                && (!catalog || catalog.queryId !== operation.requestId || catalog.runId !== operation.runId
                    || !sameBackupCatalog(catalog, result.workerResult, operation.backupDirectory))) return
        var nativeDiagnostics = actualBackupDiagnostics, diagnosticsRevision = backupDiagnosticsRevision
        var diagnostics = operation.diagnosticsRead && result.status === "backup-diagnostics-complete" ? copyValue(nativeDiagnostics) : null
        if (operation.diagnosticsRead && result.status === "backup-diagnostics-complete"
                && (!diagnostics || diagnostics.queryId !== operation.requestId || diagnostics.runId !== operation.runId
                    || !sameBackupDiagnostics(diagnostics, result.workerResult, operation.backupDirectory))) return
        if (!life.alive || ownedOperation !== operation || archive !== owner || stamp !== basisStamp(operation) || !basisCurrent(operation)
                || current.archiveMaintenanceActive !== false || nativeResult !== actualResult || (inventory && nativeInventory !== actualInventory)
                || (catalog && (nativeCatalog !== actualBackupCatalog || catalogRevision !== backupCatalogRevision))
                || (diagnostics && (nativeDiagnostics !== actualBackupDiagnostics || diagnosticsRevision !== backupDiagnosticsRevision))) return
        operation.adopted = true
        savedResult = {owner: owner, epoch: operation.epoch, optionsStamp: stamp, runId: operation.runId, requestId: operation.requestId, nativeResult: nativeResult, dto: result,
            catalogRead: operation.catalogRead, diagnosticsRead: operation.diagnosticsRead}
        if (inventory && life.alive && ownedOperation === operation && archive === owner && stamp === basisStamp(operation) && basisCurrent(operation)
                && current.archiveMaintenanceActive === false && nativeResult === actualResult && nativeInventory === actualInventory) savedInventory = {owner: owner, epoch: operation.epoch, optionsStamp: stamp, runId: operation.runId, requestId: operation.requestId,
            nativeResult: nativeInventory, revision: inventoryRevision, dto: inventory}
        if (catalog && life.alive && ownedOperation === operation && archive === owner && stamp === basisStamp(operation) && basisCurrent(operation)
                && current.archiveMaintenanceActive === false && nativeResult === actualResult && nativeCatalog === actualBackupCatalog
                && catalogRevision === backupCatalogRevision) savedBackupCatalog = {owner: owner, epoch: operation.epoch, optionsStamp: stamp, runId: operation.runId,
            requestId: operation.requestId, nativeResult: nativeCatalog, revision: catalogRevision, dto: catalog, catalogRead: true,
            backup: true, backupPathRevision: operation.backupPathRevision, backupDirectory: operation.backupDirectory}
        if (diagnostics && life.alive && ownedOperation === operation && archive === owner && stamp === basisStamp(operation) && basisCurrent(operation)
                && current.archiveMaintenanceActive === false && nativeResult === actualResult && nativeDiagnostics === actualBackupDiagnostics
                && diagnosticsRevision === backupDiagnosticsRevision) savedBackupDiagnostics = {owner: owner, epoch: operation.epoch, optionsStamp: stamp, runId: operation.runId,
            requestId: operation.requestId, nativeResult: nativeDiagnostics, revision: diagnosticsRevision, dto: diagnostics, diagnosticsRead: true,
            backup: true, backupPathRevision: operation.backupPathRevision, backupDirectory: operation.backupDirectory}
    }
    function cancelSnapshot() {
        if (!canCancel) return null
        return {owner: archive, epoch: lifetime.epoch, optionsStamp: basisStamp(ownedOperation), runId: runId, operation: ownedOperation,
            catalogRead: ownedOperation.catalogRead, diagnosticsRead: ownedOperation.diagnosticsRead,
            intent: {action: "cancel", expectedRunId: runId, inventoryId: "", entryId: "", previewId: "", queryId: ownedOperation.requestId}}
    }
    function cancelCurrent(pressed) {
        return pressed && operationSourceReady(pressed) && basisCurrent(pressed) && pressed.operation === ownedOperation && ownedOperation && !ownedOperation.adopted
            && pressed.intent.queryId === ownedOperation.requestId && cancelledQueryId !== ownedOperation.requestId
            && current.archiveMaintenanceActive === true && workerKnown(current.archiveMaintenanceWorker, ownedOperation)
    }
    function cancel(pressed) {
        var life = lifetime
        if (!life.alive || life.call || callBusy || !cancelCurrent(pressed)) return
        var ticket = {}, epoch = life.epoch, owner = pressed.owner
        life.call = ticket
        try {
            callBusy = true
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || !cancelCurrent(pressed)) return
            var receipt = null
            try { receipt = owner.maintenance(pressed.intent, true) } catch (error) { receipt = null }
            if (!life.alive) return
            var value = copyValue(receipt)
            if (!receiptKnown(value, pressed.intent)) { localError = qsTr("取消回执未知；等待同一实际线程结束。决定后可能已改名或写入副本，取消不保证回滚或清除部分文件。"); return }
            lastRequest = value
            if (!life.alive || life.call !== ticket || life.epoch !== epoch || !basisCurrent(pressed) || ownedOperation !== pressed.operation) return
            if (value.accepted) cancelledQueryId = value.requestId
            else localError = qsTr("取消未被接受：%1；不把此回执当作线程退出、文件未移动或未写副本。").arg(text(value.reason))
            if (life.alive && life.call === ticket && life.epoch === epoch) adopt()
        } finally { if (life.alive && life.call === ticket) { life.call = null; callBusy = false } }
    }
    function workerText() {
        var operation = ownedOperation
        if (!operation || ["inventory", "commit", "backup-catalog", "backup-diagnostics", "commit-import"].indexOf(operation.intent.action) < 0 || !uuid(operation.requestId)) return qsTr("尚无本页持有的实际维护线程；预览回执ID不是工作线程ID。展开、选中和勾选均不读盘。")
        var worker = current.archiveMaintenanceWorker
        if (!workerKnown(worker, operation)) return qsTr("当前工作线程快照无法核实或不属于本页原请求，不冒用其他入口的状态。原请求ID：%1").arg(operation.requestId)
        if (operation.intent.action === "commit-import") return qsTr("原导入请求ID：%1\n实际线程仍占槽：%2 · 状态：%3\n决定 / 暂存创建尝试 / 暂存已创建：%4 / %5 / %6\n可能已写暂存：%7 · 字节：%8\n复制完成 / 暂存读回核实：%9 / %10\n发布尝试 / 系统报告成功 / 可能已发布：%11 / %12 / %13\n最终导入核实：%14 · 取消已请求：%15\n错误：%16\n取消不保证撤销已写暂存或已发布目标，不自动清理、回滚或重试。").arg(operation.requestId)
            .arg(known(current.archiveMaintenanceActive)).arg(text(worker.state)).arg(known(worker.decisionFinal)).arg(known(worker.stagingCreateAttempted))
            .arg(known(worker.stagingCreated)).arg(known(worker.mayHaveWritten)).arg(worker.bytesCopied).arg(known(worker.copyCompleted)).arg(known(worker.stagingVerified))
            .arg(known(worker.publishAttempted)).arg(known(worker.publishReportedSuccess)).arg(known(worker.mayHavePublished)).arg(known(worker.importVerified))
            .arg(known(worker.cancelRequested)).arg(text(worker.errorCode))
        if (operation.diagnosticsRead) return qsTr("本页原逐文件诊断查询ID：%1\n原样查询目录：%2\n实际读取线程仍占槽：%3 · 原生状态：%4\n已请求取消：%5 · 原生错误：%6\n不隐式访问原归档目录，不读取原当前库或取得源锁。完整观察可包含被拒绝项，不等于全部可读；实际结束前不接纳，取消/目录错误不发布成功前缀。").arg(operation.requestId)
            .arg(operation.backupDirectory).arg(known(current.archiveMaintenanceActive)).arg(text(worker.state)).arg(known(worker.cancelRequested)).arg(text(worker.errorCode))
        if (operation.catalogRead) return qsTr("本页原备份目录查询ID：%1\n原样查询目录：%2\n实际读取线程仍占槽：%3 · 原生状态：%4\n已请求取消：%5 · 原生错误：%6\n只读取明确指定的备份目录，不隐式选择原源目录，不打开原当前库或取得源锁；必须实际结束才接纳完整清单，坏末项或取消不发布成功前缀。").arg(operation.requestId)
            .arg(operation.backupDirectory).arg(known(current.archiveMaintenanceActive)).arg(text(worker.state)).arg(known(worker.cancelRequested)).arg(text(worker.errorCode))
        if (operation.intent.action === "commit" && operation.preview.dto.action === "backup") {
            return qsTr("本页原备份请求ID：%1\n实际维护线程仍占槽：%2 · 原生状态：%3\n决定已完成 / 已尝试创建 / 已创建目标：%4 / %5 / %6\n可能已写副本：%7 · 已写字节：%8\n复制完成 / 完整读回核实：%9 / %10\n已请求取消：%11 · 原生错误：%12\n取消只作请求，可能留下部分文件；不自动删除、覆盖、回滚或重试。").arg(operation.requestId)
                .arg(known(current.archiveMaintenanceActive)).arg(text(worker.state)).arg(known(worker.decisionFinal))
                .arg(known(worker.targetCreateAttempted)).arg(known(worker.targetCreated)).arg(known(worker.mayHaveWritten)).arg(worker.bytesCopied)
                .arg(known(worker.copyCompleted)).arg(known(worker.backupVerified)).arg(known(worker.cancelRequested)).arg(text(worker.errorCode))
        }
        return qsTr("本页原请求ID：%1\n实际维护线程仍占槽：%2 · 原生状态：%3\n决定已完成：%4 · 可能已改名：%5 · 已确认改名：%6\n已请求取消：%7 · 原生错误：%8\n取消仅是请求；决定完成后仅观察结果，不能保证中止或回滚。")
            .arg(operation.requestId).arg(known(current.archiveMaintenanceActive)).arg(text(worker.state)).arg(known(worker.decisionFinal))
            .arg(known(worker.mayHaveMoved)).arg(known(worker.moveConfirmed)).arg(known(worker.cancelRequested)).arg(text(worker.errorCode))
    }
    function resultFactsText() {
        if (!savedResult) return qsTr("未取得本页已结束结果事实。")
        var facts = savedResult.dto.workerResult
        if (savedResult.dto.action === "inventory") return qsTr("本次结果是只读维护清单；不涉及改名。")
        if (savedResult.dto.action === "import") return savedResult.dto.status === "failed"
            ? qsTr("导入没有取得完整可核实事实；不能断言无暂存或未发布。保留原预览路径、文件、请求和错误交工程师核查，不自动清理或重试。")
            : qsTr("原导入操作ID：%1\n原备份：%2\n暂存：%3\n最终retired目标：%4\n决定 / 已尝试暂存 / 已创建暂存：%5 / %6 / %7\n可能已写 / 已复制字节：%8 / %9\n复制完成 / 暂存读回核实：%10 / %11\n发布尝试 / 系统报告成功 / 可能已发布：%12 / %13 / %14\n最终导入核实 / 观察取消：%15 / %16\n只有importVerified=true才核实已进入retired保留区；仍不在普通历史检索内。后续需另读原库存→restore独立确认，不自动恢复任务、权限或批准。未确认发布或部分暂存须保留文件交工程师，不删、不回滚、不重试。").arg(facts.operationId)
                .arg(targetPath(facts.sourceDirectory, facts.source.basename)).arg(targetPath(facts.destinationDirectory, facts.stagingBasename))
                .arg(targetPath(facts.destinationDirectory, facts.targetBasename)).arg(known(facts.decisionFinal)).arg(known(facts.stagingCreateAttempted))
                .arg(known(facts.stagingCreated)).arg(known(facts.mayHaveWritten)).arg(facts.bytesCopied).arg(known(facts.copyCompleted)).arg(known(facts.stagingVerified))
                .arg(known(facts.publishAttempted)).arg(known(facts.publishReportedSuccess)).arg(known(facts.mayHavePublished)).arg(known(facts.importVerified)).arg(known(facts.cancelObserved))
        if (savedResult.dto.action === "backup-diagnostics") return savedResult.dto.status === "failed"
            ? qsTr("逐文件诊断没有取得可核实完整观察；不发布成功前缀，旧诊断仅作历史，不自动重试/修复/删除。")
            : qsTr("逐文件诊断原目录：%1\n候选 / 本次可读 / 拒绝：%2 / %3 / %4\n完整仅指本次有界观察完成，不是全部可读、全备份、原源比对或恢复许可。拒绝项保留明确原因，不可作为源文件选择。").arg(facts.destinationDirectory).arg(facts.candidateCount).arg(facts.readableCount).arg(facts.rejectedCount)
        if (savedResult.dto.action === "backup-catalog") return savedResult.dto.status === "failed"
            ? qsTr("本次备份目录读取未取得完整可核实清单；不展示成功前缀，不自动修复或重试。旧清单只作历史阅读，不据此判断当前全部备份。")
            : qsTr("本次只读查询目录：%1\n目录身份：%2 · 完整接纳文件数：%3\n仅证明本次枚举对象当前可读、结构自洽；没有与原源比对，不证明全备份、原始事件真实、签名或可恢复性。").arg(facts.destinationDirectory).arg(facts.directoryIdentity).arg(facts.fileCount)
        if (savedResult.dto.status === "failed") return qsTr("原生结果缺失或未通过接纳，没有可核实文件操作事实；不能据此称文件未变或未写入副本。请保留文件和本页同请求线程状态交工程师核查。")
        if (savedResult.dto.action === "backup") {
            return qsTr("原备份操作ID：%1\n完整目标：%2\n决定已完成 / 已尝试创建 / 已创建目标：%3 / %4 / %5\n可能已写副本：%6 · 已复制 / 原文件字节：%7 / %8\n复制完成 / 完整读回核实：%9 / %10\n观察到取消：%11\n原源目录观察身份：%12\n目标目录观察身份：%13\n读回核实仅针对本次副本；不是每行真实性、全部历史、掉电持久化或任务恢复证明。未核实或取消可能留下部分副本，本面板不删除或自动重试。").arg(facts.operationId)
                .arg(targetPath(facts.destinationDirectory, facts.targetBasename)).arg(known(facts.decisionFinal)).arg(known(facts.targetCreateAttempted))
                .arg(known(facts.targetCreated)).arg(known(facts.mayHaveWritten)).arg(facts.bytesCopied).arg(facts.source.fileBytes)
                .arg(known(facts.copyCompleted)).arg(known(facts.backupVerified)).arg(known(facts.cancelObserved))
                .arg(text(facts.observedDirectoryIdentity)).arg(text(facts.destinationDirectoryIdentity))
        }
        return qsTr("原改名操作ID：%1\n决定已完成 / 已尝试改名：%2 / %3\n系统调用报告成功 / 可能已改名 / 已确认改名：%4 / %5 / %6\n观察到取消：%7\n观察到的目录身份：%8\n确认仅针对本次原文件和目标的改名观察，不是掉电持久化保证、历史完整性或任务恢复。").arg(facts.operationId)
            .arg(known(facts.decisionFinal)).arg(known(facts.renameAttempted)).arg(known(facts.renameReportedSuccess))
            .arg(known(facts.mayHaveMoved)).arg(known(facts.moveConfirmed)).arg(known(facts.cancelObserved)).arg(text(facts.observedDirectoryIdentity))
    }
    function confirmationSnapshot() {
        if (!idle || !previewCurrent) return null
        return {owner: archive, epoch: lifetime.epoch, optionsStamp: optionsStamp, runId: runId,
            inventory: savedInventory, preview: savedPreview, selectionRevision: selectionRevision,
            backup: savedPreview.dto.action === "backup", backupPathRevision: backupPathRevision, backupDirectory: backupDirectory}
    }
    function confirm(pressed, checked) {
        if (!checked) { confirmedPreview = null; return }
        if (!pressed || !idle || !previewCurrent || !basisCurrent(pressed) || pressed.inventory !== savedInventory
                || pressed.preview !== savedPreview || pressed.selectionRevision !== selectionRevision || !backupBasisCurrent(pressed)) {
            confirmedPreview = null
            localError = qsTr("按下确认后来源、选择或预览已变化；没有确认新的预览，请重新阅读影响。")
            return
        }
        confirmedPreview = pressed.preview
    }
    function targetPath(directory, basename) {
        // Display only: native uses the original preview's directory, not this
        // joined text. No path normalization is used to renew authorization.
        return directory + (/[\\/]$/.test(directory) ? "" : directory.indexOf("\\") >= 0 ? "\\" : "/") + basename
    }
    function previewText() {
        if (!savedPreview) return qsTr("尚无本页核实的影响预览。预览不读盘、不改名或复制，不是提交成功；30秒原生单调提交窗口不因改路径或UI时钟而延长。")
        var value = savedPreview.dto
        if (value.action === "backup") return qsTr("本页原备份预览ID：%1\n源文件：%2\n原样目标目录：%3\n完整目标：%4\n原生提交窗口：30秒 · 此原预览仍可新确认：%5\n只创建原生随机UUID新文件，不覆盖、不移动或删除源文件。提交后将完整复制并读回核对；失败/取消可能留下部分文件，不自动删。maximumSealedSegments=32仅为兼容旧政策标记，不是备份数量限制。目标必须是现存本地绝对目录，不能用副本自动恢复任务或批准。").arg(value.previewId)
            .arg(value.source.basename).arg(value.backupDirectory).arg(targetPath(value.backupDirectory, value.targetBasename)).arg(known(previewCurrent))
        return qsTr("本页原预览ID：%1\n动作：%2\n源文件：%3\n目标文件：%4\n最大普通封存段数：%5 · 原生预览有效期：30秒\n此原预览仍可用于新确认：%6\n移出不释放空间；恢复仅重新进入普通检索，不恢复任务、许可或批准。容量已满、目标已存在、源文件变化或旁路文件异常均可能拒绝，不能覆盖或自动清理。")
            .arg(value.previewId).arg(value.action === "retire" ? qsTr("移出普通检索") : qsTr("恢复普通检索"))
            .arg(value.source.basename).arg(value.targetBasename).arg(value.maximumSealedSegments).arg(known(previewCurrent))
    }
    function importPreviewText() {
        if (!savedImportPreview) return qsTr("尚无本页核实导入预览。只允许当前逐文件诊断的可读行；选择和预览不读盘、不复制或发布。")
        var value = savedImportPreview.dto
        return qsTr("原导入预览ID：%1\n原诊断ID：%2 · 原可读entryId：%3\n原备份：%4\n固定目标目录：%5\n暂存文件：%6\n最终retired保留区目标：%7\n原单调提交窗口30秒 · 此原预览仍可新确认：%8\n提交先重新检查原副本，复制到暂存并读回，再尝试不覆盖发布。目标保留区满、同身份sealed/retired已存在、暂存或旁路冲突都可能拒绝，不能覆盖。源备份不删除；失败/取消可能保留部分暂存或已发布目标，不自动删除/回滚/重试。最终核实只入retired，不恢复普通检索、账号、批准或任务；需要后续inventory→restore另行确认。新导入预览与旧改名/备份共用一个原生槽，另一份旧预览仅作历史。").arg(value.previewId).arg(value.diagnosticsId).arg(value.entryId)
            .arg(targetPath(value.sourceDirectory, value.source.basename)).arg(value.destinationDirectory)
            .arg(targetPath(value.destinationDirectory, value.stagingBasename)).arg(targetPath(value.destinationDirectory, value.targetBasename)).arg(known(importPreviewCurrent))
    }
    function backupPathChanged() {
        if (backupPathRevision >= 9007199254740991) backupPathExhausted = true
        else backupPathRevision += 1
        if (confirmedPreview && confirmedPreview.dto.action === "backup") confirmedPreview = null
        confirmedImportPreview = null
        // Do not discard a non-backup approval, extend the native deadline,
        // cancel a live copy, or hide its eventual original-path result.
    }
    function entryText(row) {
        return qsTr("%1%2\n文件：%3\n数据库：%4 · 封存run：%5\n文件字节数：%6 · 文件内容指纹：%7\nentryId：%8\n选择只改变本页；需先生成影响预览，再独立确认。").arg(selectedEntryId === row.entryId ? qsTr("已选 · ") : "")
            .arg(row.kind === "sealed" ? qsTr("普通历史可检索封存段") : qsTr("已移出普通检索，可恢复段"))
            .arg(row.basename).arg(row.databaseEpoch).arg(row.sealRunId).arg(row.fileBytes).arg(row.fileFingerprint).arg(row.entryId)
    }
    function backupCatalogEntryText(row) {
        return qsTr("备份文件：%1\n数据库：%2 · 原封存run：%3\n备份操作ID：%4\n文件字节：%5 · 修改时间UTC毫秒：%6\n文件内容指纹：%7\n本次只读entryId：%8\n仅当前可读/结构自洽；未与原源比对，不恢复任务、权限或批准。").arg(row.basename).arg(row.databaseEpoch).arg(row.sealRunId)
            .arg(row.backupOperationId).arg(row.fileBytes).arg(row.modifiedUtcMs).arg(row.fileFingerprint).arg(row.entryId)
    }
    function escapedName(value) {
        if (!diagnosticNameKnown(value)) return qsTr("未知 / 无效原名")
        var result = ""
        for (var i = 0; i < value.length; ++i) {
            var code = value.charCodeAt(i)
            if (code <= 0x1f || code === 0x7f || (code >= 0x2028 && code <= 0x202e) || (code >= 0x2066 && code <= 0x2069))
                result += "\\u" + ("0000" + code.toString(16)).slice(-4)
            else result += code === 0x5c ? "\\\\" : value.charAt(i)
        }
        return result // Display only, up to 3072 chars from a <=512-unit name.
    }
    function diagnosticReasonText(value) {
        if (value === "archive-backup-name-invalid") return qsTr("备份文件名格式不符合约定，未作为有效备份打开")
        if (value === "archive-backup-type-rejected") return qsTr("文件类型或链接条件不允许")
        if (value === "archive-backup-size-rejected") return qsTr("文件尺寸不在本次政策范围")
        if (value === "archive-backup-sidecars-present") return qsTr("存在journal/wal/shm等旁路文件，不能按当前条件读取")
        if (value === "archive-backup-file-rejected") return qsTr("文件可读性或身份检查未通过")
        if (value === "archive-backup-structure-unconfirmed") return qsTr("数据库结构自洽性未确认")
        return qsTr("未知拒绝原因；不推断可读或可修复")
    }
    function backupDiagnosticEntryText(row) {
        var header = qsTr("候选原名（仅显示转义）：%1\n本次状态：%2\n诊断entryId：%3\n").arg(escapedName(row.basename))
            .arg(row.status === "readable" ? qsTr("可读 / 结构自洽（本次观察）") : qsTr("已拒绝，不可作为源或恢复输入")).arg(row.entryId)
        if (row.status === "rejected") return header + qsTr("原因：%1\n诊断码：%2\n未发布descriptor；不推断数据库字段，不自动删除、修复或重试。").arg(diagnosticReasonText(row.errorCode)).arg(row.errorCode)
        var file = row.descriptor
        return header + qsTr("数据库：%1 · 原封存run：%2\n备份操作ID：%3\n字节：%4 · 修改UTC毫秒：%5\n内容指纹：%6\n未与原源比对，不代表历史完整、签名、权限或恢复能力。").arg(file.databaseEpoch).arg(file.sealRunId)
            .arg(file.backupOperationId).arg(file.fileBytes).arg(file.modifiedUtcMs).arg(file.fileFingerprint)
    }
    function bump(which) {
        if (root[which] >= 9007199254740991) revisionExhausted = true
        else root[which] += 1
        confirmedPreview = null
        confirmedImportPreview = null
    }
    function sourceChanged() {
        var life = lifetime
        if (!life || !life.alive) return
        if (life.epoch >= 9007199254740991) life.exhausted = true; else life.epoch += 1
        ownedOperation = null; confirmedPreview = null; confirmedImportPreview = null; cancelledQueryId = ""
        // Old copies stay visibly historical. Never cancel a different owner.
    }
    onArchiveChanged: sourceChanged()
    onOptionsStampChanged: sourceChanged()
    onCatalogOptionsStampChanged: sourceChanged()
    onRunIdChanged: sourceChanged()
    onSelectedEntryIdChanged: bump("selectionRevision")
    onSelectedDiagnosticEntryIdChanged: bump("diagnosticSelectionRevision")
    onBackupDirectoryChanged: backupPathChanged()
    onActualInventoryChanged: { bump("inventoryRevision"); adopt() }
    onActualPreviewChanged: { bump("previewRevision"); adopt() }
    onActualResultChanged: adopt()
    onActualBackupCatalogChanged: { bump("backupCatalogRevision"); adopt() }
    onActualBackupDiagnosticsChanged: { bump("backupDiagnosticsRevision"); adopt() }
    onCurrentChanged: adopt()
    Component.onDestruction: lifetime.alive = false

    ColumnLayout {
        id: form
        anchors.left: parent.left; anchors.right: parent.right
        spacing: root.pad
        QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("本地封存段维护 · 读取清单 → 影响预览 → 独立确认 → 等待实际结果") }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("以下旧移动／备份流程不清理磁盘：每次仅对一个封存段可恢复移出/恢复，或显式复制并完整读回。移出仍占磁盘，备份额外占磁盘；这些按钮不永久删除、不覆盖、不处理当前未封存库、不恢复任务或批准。525独立永久删除区在下方，必须新做retired原件备份后另行预览与两项确认；不能复用这里的确认。") }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("所有读盘、改名及复制均只在明确点击时发生。需要 captureEnabled=false、录制/历史读取线程实际退出及维护槽空闲；不会自动停录。需持有固定目录协作锁，Qt可能回收死进程锁；协作锁不是抵御恶意并发写入或密码学认证。") }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: root.idle ? qsTr("当前可显式读取原源维护清单。旧清单与旧预览只供历史阅读，不能保证磁盘现状。") : qsTr("当前不可开始原源维护；独立备份目录读取另见下方。请检查原生来源、实际线程、capture开关及调用状态。取消或停止accepted不代表空闲；不要求登录或连接飞机。") }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("1. 读取维护清单（显式读盘）"); enabled: root.idle; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("inventory"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.savedInventory ? qsTr("本页核实清单ID：%1\n原请求ID：%2 · 目录身份：%3\n普通封存 / 已移出段：%4 / %5\n本页来源及原生清单仍一致：%6\n仅固定目录本次枚举，不是全部历史、记录哈希已全部验证或跨库原子快照。").arg(root.savedInventory.dto.inventoryId).arg(root.savedInventory.requestId).arg(root.savedInventory.dto.directoryIdentity).arg(root.savedInventory.dto.sealedCount).arg(root.savedInventory.dto.retiredCount).arg(root.known(root.inventoryCurrent)) : qsTr("尚无本页核实清单；展开面板不扫描目录，其他入口已有值不当作本次成功。") }
        QGCLabel { Layout.fillWidth: true; visible: !!root.savedInventory && !root.inventoryRows.length; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("本次清单没有可维护封存段。不代表不存在当前库、其他文件或全部历史为空；枚举失败/超限时不会提交有效前缀。") }
        ListView {
            Layout.fillWidth: true; Layout.preferredHeight: root.inventoryRows.length ? Math.max(root.rowHeight * 4, Math.min(520, ScreenTools.defaultFontPixelHeight * 24)) : 0
            model: root.inventoryRows; clip: true; spacing: root.pad
            Controls.ScrollBar.vertical: Controls.ScrollBar {}
            delegate: QGCButton {
                width: ListView.view.width; height: Math.max(root.rowHeight, entryLabel.implicitHeight + root.pad * 2); focusPolicy: Qt.StrongFocus
                enabled: root.inventoryCurrent && !root.callBusy && !root.historyBusy; text: root.entryText(modelData); Accessible.name: text
                onClicked: root.selectedEntryId = modelData.entryId
                onActiveFocusChanged: if (activeFocus) ListView.view.positionViewAtIndex(index, ListView.Contain)
                contentItem: QGCLabel { id: entryLabel; text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            }
        }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("已选entryId：%1。更换后旧预览/确认作废，改回同值也不会复活。").arg(root.text(root.selectedEntryId)) }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("2. 预览将所选封存段移出普通检索（不改名）"); enabled: root.canPrepareRetire; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("prepare-retire"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("2. 预览将所选移出段恢复到普通检索（不改名）"); enabled: root.canPrepareRestore; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("prepare-restore"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("显式备份（支持普通封存段及已移出段）") }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("备份目标目录：输入现存本地绝对目录，最多4096字符；Windows示例 E:/FleetBackups，须用正斜杠、不带末尾斜杠，不填文件名、URL、UNC/网络路径。本页不创建目录，也不展开自动读盘的目录选择器。文本原样送原生检查，不转换反斜杠；未在UI确认存在、可写、无链接或与源目录互不包含。") }
        Controls.TextField {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            enabled: (root.sourceReady || root.catalogSourceReady) && !root.callBusy
            text: root.backupDirectory; maximumLength: 4096; selectByMouse: true
            Accessible.name: qsTr("现存本地绝对备份目录，最多4096字符，不含文件名")
            onTextChanged: root.backupDirectory = text
        }
        QGCLabel { Layout.fillWidth: true; visible: root.backupDirectory.length > 0; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("原样输入：%1\n改路径将使原备份预览/确认退休，改回不恢复；不影响移出/恢复预览，不延长原30秒窗口，也不取消已经发出的复制。").arg(root.backupDirectory) }
        QGCLabel { Layout.fillWidth: true; visible: root.backupDirectory.length > 0 && (!root.backupPathCandidate(root.backupDirectory) || root.backupPathExhausted); textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("备份目录格式不满足基本条件，或备份路径版本耗尽。Windows请写 E:/FleetBackups（正斜杠、无末尾斜杠）；不能填网络路径或根目录。存在性、规范路径、链接与权限仍由原生核实。版本耗尽需重开面板，不影响非备份操作。") }
        QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("独立备份目录诊断（无需原库存或选择源文件）") }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("逐文件诊断会列出本次可读项和明确拒绝项；完整观察不等于全部可读。拒绝项不可选择作源或恢复。原名中的控制字符及反斜杠只作可见转义显示，不改变后端原名，不把显示串当路径。") }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("逐文件诊断备份目录（显式读盘，列出拒绝原因）"); enabled: root.canReadBackupDiagnostics; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("backup-diagnostics"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; color: qgcPal.warningText; text: root.savedBackupDiagnostics ? qsTr("本页已核实逐文件诊断ID：%1\n原查询ID：%2\n原目录：%3\n目录身份：%4\n候选 / 本次可读 / 拒绝：%5 / %6 / %7\n仍属当前路径/来源/原生结果：%8\n完整仅指本次候选观察完成，不是全部可读或全备份；有拒绝项不掩盖、不自动处理。未与原源比对，不授予恢复/执行许可。").arg(root.savedBackupDiagnostics.dto.diagnosticsId).arg(root.savedBackupDiagnostics.requestId).arg(root.savedBackupDiagnostics.dto.destinationDirectory).arg(root.savedBackupDiagnostics.dto.directoryIdentity).arg(root.savedBackupDiagnostics.dto.candidateCount).arg(root.savedBackupDiagnostics.dto.readableCount).arg(root.savedBackupDiagnostics.dto.rejectedCount).arg(root.known(root.backupDiagnosticsCurrent)) : qsTr("尚无本页核实的完整逐文件诊断；取消、枚举失败或目录变化不发布成功前缀。") }
        QGCLabel { Layout.fillWidth: true; visible: !!root.savedBackupDiagnostics && !root.backupDiagnosticsRows.length; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("本次没有匹配候选；不表示其他文件、其他目录或全部历史为空，也不证明全部备份已找到。") }
        ListView {
            Layout.fillWidth: true; Layout.preferredHeight: root.backupDiagnosticsRows.length ? Math.max(root.rowHeight * 4, Math.min(520, ScreenTools.defaultFontPixelHeight * 24)) : 0
            model: root.backupDiagnosticsRows; clip: true; spacing: root.pad
            Controls.ScrollBar.vertical: Controls.ScrollBar {}
            delegate: FocusScope {
                width: ListView.view.width; height: backupDiagnosticBody.implicitHeight + root.pad * 2; activeFocusOnTab: true
                Accessible.role: Accessible.StaticText; Accessible.name: backupDiagnosticLabel.text
                onActiveFocusChanged: if (activeFocus) ListView.view.positionViewAtIndex(index, ListView.Contain)
                Rectangle { anchors.fill: parent; color: qgcPal.windowShade; border.color: parent.activeFocus ? qgcPal.warningText : qgcPal.text; border.width: parent.activeFocus ? 3 : 1; radius: 3 }
                ColumnLayout {
                    id: backupDiagnosticBody
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: root.pad; spacing: root.pad
                    QGCLabel { id: backupDiagnosticLabel; Layout.fillWidth: true; text: root.backupDiagnosticEntryText(modelData); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
                    QGCButton {
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        visible: modelData.status === "readable"; enabled: root.idle && root.backupDiagnosticsCurrent
                        text: root.selectedDiagnosticEntryId === modelData.entryId ? qsTr("已选此可读副本（仅本页）") : qsTr("选择此可读副本以预览导入（不读盘）")
                        Accessible.name: text; property var press: null
                        onPressed: press = root.diagnosticSelectionSnapshot(modelData.entryId)
                        onCanceled: press = null
                        onClicked: { var value = press; press = null; root.selectDiagnostic(value) }
                        Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true
                        contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                    }
                }
            }
        }
        QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("可读副本导入retired保留区（不是任务恢复）") }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("已选可读entryId：%1\n固定目标目录：%2\n需要非空原源配置、原当前诊断/路径/选择。拒绝项和旧同值ID不能进入；固定目录由原生再核，不由此处输入。导入不会自动纳入普通检索或恢复任务。").arg(root.text(root.selectedDiagnosticEntryId)).arg(root.text(root.options.directoryPath)) }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("预览所选可读备份导入（不读盘、不暂存或发布）"); enabled: root.canPrepareImport; Accessible.name: text; property var press: null; onPressed: press = root.importSnapshot("prepare-import"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submitImport(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.importPreviewText() }
        Controls.CheckBox {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            enabled: root.idle && root.importPreviewCurrent; checked: root.importPreviewCurrent && root.confirmedImportPreview === root.savedImportPreview
            text: qsTr("我已核对这一份原导入预览的备份、固定目录、暂存与retired目标；只确认此次历史导入，不覆盖或恢复任务。")
            Accessible.name: text; property var press: null
            onPressed: press = root.importConfirmationSnapshot(); onCanceled: press = null
            onClicked: { var value = press; press = null; root.confirmImport(value, checked) }
            Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: parent.indicator.width + root.pad; verticalAlignment: Text.AlignVCenter }
        }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("提交已确认导入（显式暂存、读回并发布到retired）"); enabled: root.canCommitImport; Accessible.name: text; property var press: null; onPressed: press = root.importSnapshot("commit-import"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submitImport(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("放弃本次导入预览（不暂存、不发布）"); enabled: root.canDiscardImport; Accessible.name: text; property var press: null; onPressed: press = root.importSnapshot("discard-import"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submitImport(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("仅点击下面按钮才读取上述指定目录，检查规范备份文件可读性及结构。不要求原归档目录仍存在，不读取或比对原源，不连接飞机、不导入/恢复任务。操作需实际录制/读取/维护线程空闲；可能创建/释放本目录协作锁，不删除坏文件或自动重试。") }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("严格检查备份目录（任一坏项整次失败，显式读盘）"); enabled: root.canReadBackupCatalog; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("backup-catalog"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: root.canReadBackupCatalog ? qsTr("可独立显式读取；不需要先建立原源库存或选择源文件。读取会退休旧库存/预览/目录续用资格，不会执行复制或恢复。") : qsTr("当前不能开始独立读取：检查目标目录基本格式、原生来源/政策及实际空闲状态。原归档目录缺失不是此读取的禁止条件，且不会因此给旧改名/复制动作默认放行。") }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.savedBackupCatalog ? qsTr("本页已核实备份目录ID：%1\n原查询ID：%2\n原样目录：%3\n目录身份：%4\n完整接纳文件数：%5 · 仍属当前路径/来源/原生结果：%6\n旧路径结果保历史，改回同值不复活。仅当前可读/结构自洽；未与原源比对、不代表全备份、签名、权限或恢复能力。").arg(root.savedBackupCatalog.dto.catalogId).arg(root.savedBackupCatalog.requestId).arg(root.savedBackupCatalog.dto.destinationDirectory).arg(root.savedBackupCatalog.dto.directoryIdentity).arg(root.savedBackupCatalog.dto.fileCount).arg(root.known(root.backupCatalogCurrent)) : qsTr("尚无本页成功核实的完整备份清单。其他入口非空属性不当作本次成功；坏末项、取消或失败不展示成功前缀。") }
        QGCLabel { Layout.fillWidth: true; visible: !!root.savedBackupCatalog && !root.backupCatalogRows.length; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("本次目录没有匹配的规范备份文件；不表示不存在其他文件、其他目录备份或全部历史为空。") }
        ListView {
            Layout.fillWidth: true; Layout.preferredHeight: root.backupCatalogRows.length ? Math.max(root.rowHeight * 4, Math.min(520, ScreenTools.defaultFontPixelHeight * 24)) : 0
            model: root.backupCatalogRows; clip: true; spacing: root.pad
            Controls.ScrollBar.vertical: Controls.ScrollBar {}
            delegate: FocusScope {
                width: ListView.view.width; height: backupCatalogLabel.implicitHeight + root.pad * 2; activeFocusOnTab: true
                Accessible.role: Accessible.StaticText; Accessible.name: backupCatalogLabel.text
                onActiveFocusChanged: if (activeFocus) ListView.view.positionViewAtIndex(index, ListView.Contain)
                Rectangle { anchors.fill: parent; color: qgcPal.windowShade; border.color: parent.activeFocus ? qgcPal.warningText : qgcPal.text; border.width: parent.activeFocus ? 3 : 1; radius: 3 }
                QGCLabel { id: backupCatalogLabel; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: root.pad; text: root.backupCatalogEntryText(modelData); textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere }
            }
        }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("2. 预览所选封存段备份（不复制、不读盘）"); enabled: root.canPrepareBackup; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("prepare-backup"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.previewText() }
        Controls.CheckBox {
            Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
            enabled: root.idle && root.previewCurrent
            checked: root.previewCurrent && root.confirmedPreview === root.savedPreview
            text: qsTr("我已阅读本次单文件影响，确认只提交这一份原预览；不删除、不覆盖、不恢复任务。")
            Accessible.name: text
            property var press: null
            onPressed: press = root.confirmationSnapshot()
            onCanceled: press = null
            onClicked: { var value = press; press = null; root.confirm(value, checked) }
            Keys.onReturnPressed: event.accepted = true
            Keys.onEnterPressed: event.accepted = true
            contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: parent.indicator.width + root.pad; verticalAlignment: Text.AlignVCenter }
        }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.savedPreview && root.savedPreview.dto.action === "backup" ? qsTr("3. 提交本次已确认单文件复制与读回（显式写盘）") : qsTr("3. 提交本次已确认单文件改名（显式写盘）"); enabled: root.canCommit; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("commit"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("放弃本次预览（不改名、不复制）"); enabled: root.canDiscard; Accessible.name: text; property var press: null; onPressed: press = root.pressSnapshot("discard-preview"); onCanceled: press = null; onClicked: { var value = press; press = null; root.submit(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("仅请求取消本页实际维护线程（不保证回滚）"); enabled: root.canCancel; Accessible.name: text; property var press: null; onPressed: press = root.cancelSnapshot(); onCanceled: press = null; onClicked: { var value = press; press = null; root.cancel(value) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("最近回执：%1 · accepted=%2\n请求ID：%3 · 原因：%4\naccepted仅表示请求接受；不是清单有效、改名/复制完成、读回通过、磁盘持久化或取消成功。").arg(root.text(root.lastRequest.action)).arg(root.known(root.lastRequest.accepted)).arg(root.text(root.lastRequest.requestId)).arg(root.text(root.lastRequest.reason)) }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.workerText() }
        QGCLabel { Layout.fillWidth: true; visible: !!root.localError; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: root.localError }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.savedResult ? qsTr("本页核实的已结束原请求：%1\n动作：%2 · 原生结果：%3\n错误：%4\n该结果仍属于当前来源及原生结果：%5\n本次结束后旧清单/预览不自动续用；需要明确重新读取清单。失败也不自动重试或推断文件未变。").arg(root.savedResult.requestId).arg(root.savedResult.dto.action).arg(root.savedResult.dto.status).arg(root.savedResult.dto.errorCode === "" ? qsTr("未报告") : root.text(root.savedResult.dto.errorCode)).arg(root.known(root.basisCurrent(root.savedResult) && root.savedResult.nativeResult === root.actualResult)) : qsTr("尚无本页核实的已结束结果；线程仍忙、回执接受或预览成功都不代替实际finished结果。") }
        QGCLabel { Layout.fillWidth: true; visible: !!root.savedResult; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.resultFactsText() }
        QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("若原生报告 mayHaveMoved=true 或 move-uncertain：可能已改名，不能称失败未改变文件。停止重复操作并保留原文件/回执/错误交工程师核查；本面板不承诺掉电原子性、自动回滚或历史完整性。") }
        QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("备份若报告 mayHaveWritten=true、backup-uncertain 或结果未知：可能已创建/写入部分副本；不能用改名字段false或取消accepted声称未写。保留完整目标路径、文件、回执和错误交工程师处理；不会自动删除部分文件或覆盖重试。backup-verified也不证明原始事件真实性、全部记录逐行哈希已验或掉电可靠性。") }
        QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("导入的暂存写入和最终发布是不同事实：import-incomplete可能留有暂存，import-uncertain可能已经发布；未知结果也不能称未写/未发布。保留原备份、暂存/目标路径和回执交工程师核查；不自动删stage、回滚、重试或启用普通检索。只有importVerified=true核实进入retired，之后restore仍需独立确认。") }
        FocusScope {
            id: capacityArea
            Layout.fillWidth: true
            implicitHeight: capacityForm.implicitHeight
            property var life: ({alive: true, epoch: 0, exhausted: false, ticket: null, retiring: false, retirePending: false, cleanups: []})
            property bool busy: false
            property double inputRevision: 0
            property double consentRevision: 0
            property string countText: "8"
            property string mibText: "2048"
            property var saved: null
            property var confirmed: null
            property var owned: null
            property string message: ""
            readonly property var source: root.archive
            readonly property string sourceRun: root.runId
            readonly property var nativePlan: source ? source.capacityPlan || {} : ({})
            readonly property var nativeStatus: source ? source.capacityStatus || {} : ({})
            readonly property bool usable: root.visible && root.enabled && root.sourceReady && source && typeof source.capacity === "function"
                && inputRevision < 9007199254740991 && consentRevision < 9007199254740991
            readonly property bool planCurrent: usable && !!saved && saved.owner === source && saved.epoch === life.epoch
                && saved.revision === inputRevision && saved.stamp === JSON.stringify(nativePlan) && saved.stamp === JSON.stringify(saved.dto)
                && nativePlan.status === "prepared" && nativePlan.canCommit === true
            readonly property bool ownBusy: !!owned && owned.owner === source && nativeStatus.planId === owned.id && nativeStatus.busy === true

            function claims(v) { return v.historicalOnly === true && v.authenticated === false && v.permissionsRestored === false && v.executionPermit === false && v.flightCommandReleased === false }
            function policyNumber(s, max) { return typeof s === "string" && /^(0|[1-9][0-9]*)$/.test(s) && Number(s) <= max ? Number(s) : -1 }
            function planKnown(v) {
                if (!root.map(v) || Object.keys(v).length !== 24 || v.schemaVersion !== 1 || !root.uuid(v.planId) || typeof v.runId !== "string"
                        || !root.uuid(v.inventoryId) || typeof v.sourceDirectory !== "string" || v.sourceDirectory.length > 4096
                        || !root.directoryIdentity(v.directoryIdentity) || !root.backupPathCandidate(v.backupDirectory)
                        || !root.integer(v.maximumSealedCount, 0, 32) || !root.integer(v.maximumSealedMiB, 0, 131072)
                        || !root.integer(v.originalSealedCount, 0, 128) || !root.integer(v.plannedSealedCount, 0, 32)
                        || !root.decimal(v.originalSealedBytes) || !root.decimal(v.plannedSealedBytes) || !root.decimal(v.requiredBackupBytes)
                        || !Array.isArray(v.rows) || !root.integer(v.retireCount, 0, 128) || v.rows.length !== v.retireCount
                        || v.plannedSealedCount + v.retireCount !== v.originalSealedCount || v.expiresAfterMs !== 30000
                        || typeof v.canCommit !== "boolean" || ["prepared", "expired", "retired", "consumed", "discarded"].indexOf(v.status) < 0 || !claims(v)) return false
                for (var i = 0; i < v.rows.length; ++i) {
                    var r = v.rows[i], s = r.source
                    if (!root.map(r) || Object.keys(r).length !== 6 || r.index !== i || !root.descriptor(s, false) || s.kind !== "sealed"
                            || !root.uuid(r.retireOperationId) || !root.uuid(r.backupOperationId)
                            || r.retiredBasename !== "fleet-events-retired-" + s.databaseEpoch + "-" + s.sealRunId + ".sqlite"
                            || r.backupBasename !== "fleet-events-backup-" + s.databaseEpoch + "-" + s.sealRunId + "-" + r.backupOperationId + ".sqlite") return false
                }
                return true
            }
            function statusKnown(v) {
                if (!(root.map(v) && Object.keys(v).length === 20 && v.schemaVersion === 1 && typeof v.planId === "string"
                    && (!v.planId || root.uuid(v.planId)) && typeof v.runId === "string" && typeof v.busy === "boolean"
                    && ["idle", "prepared", "running", "cancelling", "complete", "failed", "cancelled"].indexOf(v.stage) >= 0
                    && ["", "inventory", "retire", "backup"].indexOf(v.phase) >= 0 && typeof v.jobId === "string"
                    && (!v.jobId || root.uuid(v.jobId)) && root.integer(v.currentIndex, -1, 127) && root.integer(v.totalCount, 0, 128)
                    && root.integer(v.completedCount, 0, v.totalCount) && typeof v.cancelRequested === "boolean" && typeof v.ok === "boolean"
                    && typeof v.error === "string" && v.error.length <= 4096 && root.map(v.lastWorkerResult) && Array.isArray(v.results)
                    && v.results.length <= v.totalCount && claims(v))) return false
                var full = Object.keys(v.lastWorkerResult).length
                if (full && ([19, 27, 31].indexOf(full) < 0 || !root.inert(v.lastWorkerResult))) return false
                for (var i = 0; i < v.results.length; ++i) {
                    var row = v.results[i]
                    if (!root.map(row) || Object.keys(row).length !== 4 || row.index !== i || !root.descriptor(row.source, false)
                            || row.source.kind !== "sealed" || !root.map(row.move) || !root.map(row.backup)) return false
                    var m = row.move, b = row.backup
                    if (Object.keys(m).length && (Object.keys(m).length !== 5 || m.known !== true || typeof m.moveConfirmed !== "boolean"
                            || typeof m.mayHaveMoved !== "boolean" || typeof m.renameReportedSuccess !== "boolean" || typeof m.errorCode !== "string" || m.errorCode.length > 4096)) return false
                    if (Object.keys(b).length && (Object.keys(b).length !== 6 || b.known !== true || typeof b.backupVerified !== "boolean"
                            || typeof b.mayHaveWritten !== "boolean" || typeof b.copyCompleted !== "boolean" || !root.decimal(b.bytesCopied)
                            || typeof b.errorCode !== "string" || b.errorCode.length > 4096)) return false
                }
                return true
            }
            function cleanupFor(owner, run, id) {
                return function() {
                    var base = {expectedRunId: run, inventoryId: "", planId: id, maximumSealedCount: -1, maximumSealedMiB: -1, backupDirectory: ""}
                    try { base.action = "cancel"; owner.capacity(base, true) } catch (e) {}
                    try { base.action = "discard"; owner.capacity(base, true) } catch (e2) {}
                }
            }
            function retire() {
                var g = life
                if (g.epoch >= 9007199254740991) g.exhausted = true; else ++g.epoch
                var cleanups = g.cleanups; g.cleanups = []
                if (g.retiring) {
                    g.retirePending = true
                    for (var j = 0; j < cleanups.length; ++j) try { cleanups[j]() } catch (e) {}
                    return
                }
                g.retiring = true
                try {
                    saved = null; if (!g.alive) return
                    confirmed = null; if (!g.alive) return
                    owned = null
                } finally {
                    g.retirePending = false
                    g.retiring = false
                    for (var i = 0; i < cleanups.length; ++i) try { cleanups[i]() } catch (e) {}
                }
            }
            function edited() {
                var g = life, old = saved
                if (g.retiring) return
                if (inputRevision >= 9007199254740991) { retire(); return }
                var clean = old ? cleanupFor(old.owner, old.run, old.dto.planId) : null
                g.retiring = true
                try {
                    inputRevision += 1; if (!g.alive) return
                    confirmed = null; if (!g.alive) return
                    saved = null
                } finally {
                    if (g.retirePending && g.alive) { g.retirePending = false; owned = null }
                    g.retiring = false
                    if (clean) clean()
                }
            }
            function snapshot(action) {
                if (!usable || busy || life.ticket || life.retiring || life.exhausted) return null
                return {action: action, owner: source, run: sourceRun, epoch: life.epoch, revision: inputRevision,
                    consent: consentRevision, saved: saved, confirmed: confirmed, owned: owned,
                    inventory: root.savedInventory, nativeInventory: root.actualInventory,
                    count: policyNumber(countText, 32), mib: policyNumber(mibText, 131072), backup: root.backupDirectory}
            }
            function current(p) { return life.alive && !life.exhausted && usable && p.owner === source && p.run === sourceRun && p.epoch === life.epoch && p.revision === inputRevision }
            function act(p) {
                var g = life
                if (!p || !current(p) || g.ticket || g.retiring || busy) return
                if (p.action === "prepare" && (!root.idle || !root.inventoryCurrent || p.inventory !== root.savedInventory
                        || p.nativeInventory !== root.actualInventory || p.count < 0 || p.mib < 0 || !root.backupPathCandidate(p.backup))) return
                if ((p.action === "commit" || p.action === "discard") && (!root.idle || !planCurrent || p.saved !== saved)) return
                if (p.action === "commit" && (p.confirmed !== p.saved || confirmed !== saved || p.consent !== consentRevision)) return
                if (p.action === "cancel" && (!p.owned || p.owned !== owned || !ownBusy)) return
                var ticket = {}; g.ticket = ticket
                var id = p.action === "prepare" ? "" : p.action === "cancel" ? p.owned.id : p.saved.dto.planId
                var intent = {action: p.action, expectedRunId: p.run, inventoryId: p.action === "prepare" ? p.inventory.dto.inventoryId : "",
                    planId: id, maximumSealedCount: p.action === "prepare" ? p.count : -1,
                    maximumSealedMiB: p.action === "prepare" ? p.mib : -1, backupDirectory: p.action === "prepare" ? p.backup : ""}
                var result = null, acceptedCleanup = null
                // This receipt classifier and cleanup closure remain plain JS
                // if a native call synchronously destroys either UI owner.
                var validReceipt = function(v) {
                    try {
                        return !!v && typeof v === "object" && Object.keys(v).length === 9 && v.schemaVersion === 1
                            && v.action === intent.action && typeof v.accepted === "boolean" && typeof v.reason === "string"
                            && typeof v.requestId === "string" && (/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(v.requestId) || (!v.accepted && v.requestId === ""))
                            && v.requestOnly === true && v.historicalOnly === true && v.permissionsRestored === false && v.flightCommandReleased === false
                            && (intent.action === "prepare" || v.requestId === intent.planId || (!v.accepted && v.requestId === ""))
                    } catch (e) { return false }
                }
                var cleanupId = {value: id}
                var detached = function() {
                    if (!cleanupId.value) return
                    var x = {expectedRunId: p.run, inventoryId: "", planId: cleanupId.value, maximumSealedCount: -1, maximumSealedMiB: -1, backupDirectory: ""}
                    try { x.action = "cancel"; p.owner.capacity(x, true) } catch (e) {}
                    try { x.action = "discard"; p.owner.capacity(x, true) } catch (e2) {}
                }
                try {
                    busy = true; if (!g.alive || !current(p) || g.ticket !== ticket) return
                    if (p.action === "commit" && (!planCurrent || saved !== p.saved || confirmed !== p.confirmed || consentRevision !== p.consent)) return
                    if (p.action === "prepare" && (!root.inventoryCurrent || p.inventory !== root.savedInventory || p.nativeInventory !== root.actualInventory || p.backup !== root.backupDirectory)) return
                    confirmed = null; if (!g.alive || !current(p) || g.ticket !== ticket) return
                    if (p.action === "commit" && (!planCurrent || saved !== p.saved || confirmed !== null || consentRevision !== p.consent)) return
                    result = p.owner.capacity(intent, true)
                    if (validReceipt(result) && result.accepted) { cleanupId.value = result.requestId; acceptedCleanup = detached }
                    if (!g.alive || !current(p) || g.ticket !== ticket || (p.action === "commit" && consentRevision !== p.consent)) { if (acceptedCleanup) acceptedCleanup(); return }
                    if (!validReceipt(result) || !result.accepted) { message = qsTr("容量请求未核实或未接受：%1。可能已有移动/写入，不自动重试；核对原生诊断。").arg(result ? root.text(result.reason) : qsTr("未知")); return }
                    if (p.action === "prepare") {
                        var dto = root.copyValue(p.owner.capacityPlan)
                        if (!g.alive || !current(p) || g.ticket !== ticket) { if (acceptedCleanup) acceptedCleanup(); return }
                        if (!planKnown(dto) || dto.planId !== result.requestId || dto.runId !== p.run || dto.inventoryId !== intent.inventoryId) { if (acceptedCleanup) acceptedCleanup(); message = qsTr("本次容量预览无法核实；未确认或提交。"); return }
                        var value = {owner: p.owner, run: p.run, epoch: p.epoch, revision: p.revision, dto: dto, stamp: JSON.stringify(dto)}
                        g.cleanups = [acceptedCleanup]
                        saved = value; if (!g.alive || !current(p) || saved !== value) return
                        message = qsTr("已生成本次完整计划。请逐项核对路径、hash及数量后独立确认；30秒内仅可接纳一次。")
                    } else if (p.action === "commit") {
                        var claim = {owner: p.owner, run: p.run, id: id}
                        g.cleanups = [acceptedCleanup]
                        owned = claim; if (!g.alive || !current(p) || owned !== claim) return
                        saved = null; if (!g.alive || !current(p)) return
                        message = qsTr("原计划已受理，按原实际worker逐步复核/移动/备份。受理不是完成；任何失败/取消保留已有文件事实，不自动删除。")
                    } else message = p.action === "cancel" ? qsTr("仅请求停止原计划后续步骤及合作取消原worker；可能已经移动/写入，不表示回滚或线程结束。") : qsTr("本次预览已放弃，未自动执行新计划。")
                } finally {
                    if (g.ticket === ticket) g.ticket = null
                    if (g.alive) busy = false
                }
            }
            function confirm(p, checked) {
                var g = life
                if (!p || !current(p) || busy || g.ticket || g.retiring || !planCurrent || p.saved !== saved || p.consent !== consentRevision) return
                var ticket = {}; g.ticket = ticket
                try { if (consentRevision >= 9007199254740991) return; var next = p.consent + 1; consentRevision = next; if (g.alive && current(p) && consentRevision === next && saved === p.saved && planCurrent) confirmed = checked ? saved : null }
                finally { if (g.ticket === ticket) g.ticket = null }
            }
            function focusVisible(item) {
                var p = item.parent
                for (var i = 0; p && i < 32; ++i, p = p.parent) {
                    if (p.contentItem && typeof p.contentY === "number" && typeof p.contentHeight === "number") {
                        var q = item.mapToItem(p.contentItem, 0, 0), y = p.contentY
                        if (q.y < y) y = q.y; else if (q.y + item.height > y + p.height) y = q.y + item.height - p.height
                        p.contentY = Math.max(0, Math.min(Math.max(0, p.contentHeight - p.height), y)); return
                    }
                }
            }
            onSourceChanged: retire()
            onSourceRunChanged: retire()
            onVisibleChanged: if (!visible) retire()
            onEnabledChanged: if (!enabled) retire()
            Connections { target: root; onBackupDirectoryChanged: capacityArea.edited() }
            Component.onDestruction: { var g = life, tasks = g.cleanups; g.alive = false; g.cleanups = []; for (var i = 0; i < tasks.length; ++i) try { tasks[i]() } catch (e) {} }
            ColumnLayout {
                id: capacityForm
                width: parent.width
                spacing: root.pad
                QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("容量编排 · 完整计划 → 独立确认 → 顺序移动与备份（不永久删除）") }
                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("复用上方本页原库存和备份目录。按文件mtime新到旧保留连续前缀（不是任务结束时间），同时满足数量与逻辑字节目标；其余sealed移成retired后新建外部副本并读回。备份增加磁盘占用；移出不释放物理空间。若retired容量不足，先用下方独立清理流程人工处理。") }
                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("保留 sealed 数量上限（0–32，0表示本计划全部移出）") }
                Controls.TextField { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: !capacityArea.busy && !capacityArea.ownBusy; text: capacityArea.countText; maximumLength: 2; selectByMouse: true; Accessible.name: qsTr("sealed保留数量上限0至32"); onTextEdited: { var g = capacityArea.life; capacityArea.countText = text; if (g.alive) capacityArea.edited() } onActiveFocusChanged: if (activeFocus) capacityArea.focusVisible(this) }
                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("保留 sealed 逻辑大小上限（MiB，0–131072；不是磁盘空闲量）") }
                Controls.TextField { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: !capacityArea.busy && !capacityArea.ownBusy; text: capacityArea.mibText; maximumLength: 6; selectByMouse: true; Accessible.name: qsTr("sealed逻辑MiB上限0至131072"); onTextEdited: { var g = capacityArea.life; capacityArea.mibText = text; if (g.alive) capacityArea.edited() } onActiveFocusChanged: if (activeFocus) capacityArea.focusVisible(this) }
                QGCLabel { Layout.fillWidth: true; visible: capacityArea.policyNumber(capacityArea.countText, 32) < 0 || capacityArea.policyNumber(capacityArea.mibText, 131072) < 0; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; text: qsTr("请填范围内完整十进制整数；空白不是0，不自动夹值。") }
                QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: capacityArea.usable && root.idle && root.inventoryCurrent && !capacityArea.busy; text: qsTr("生成本次完整容量预览（不读盘、不移动/备份）"); Accessible.name: text; property var press: null; onPressed: press = capacityArea.snapshot("prepare"); onCanceled: press = null; onClicked: { var g = capacityArea.life, p = press; press = null; if (g.alive) capacityArea.act(p) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; onActiveFocusChanged: if (activeFocus) capacityArea.focusVisible(this); contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText } }
                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: capacityArea.saved ? qsTr("本页计划 %1\n保留 %2 → %3 个，%4 → %5 逻辑bytes；需移出并备份 %6 个、%7 逻辑bytes。\n源：%8\n备份：%9\n原目录身份：%10\n当前仍可独立确认：%11").arg(capacityArea.saved.dto.planId).arg(capacityArea.saved.dto.originalSealedCount).arg(capacityArea.saved.dto.plannedSealedCount).arg(capacityArea.saved.dto.originalSealedBytes).arg(capacityArea.saved.dto.plannedSealedBytes).arg(capacityArea.saved.dto.retireCount).arg(capacityArea.saved.dto.requiredBackupBytes).arg(capacityArea.saved.dto.sourceDirectory).arg(capacityArea.saved.dto.backupDirectory).arg(capacityArea.saved.dto.directoryIdentity).arg(root.known(capacityArea.planCurrent)) : qsTr("没有本页可提交容量计划。现有原生诊断不自动认领为本页确认。") }
                ListView {
                    Layout.fillWidth: true; Layout.preferredHeight: capacityArea.saved && capacityArea.saved.dto.rows.length ? root.rowHeight * 5 : 0
                    clip: true; model: capacityArea.saved ? capacityArea.saved.dto.rows : []; spacing: root.pad
                    Controls.ScrollBar.vertical: Controls.ScrollBar {}
                    delegate: FocusScope {
                        width: ListView.view.width; height: capRow.implicitHeight + root.pad; activeFocusOnTab: true; Accessible.role: Accessible.StaticText; Accessible.name: capRow.text
                        onActiveFocusChanged: if (activeFocus) ListView.view.positionViewAtIndex(index, ListView.Contain)
                        QGCLabel { id: capRow; width: parent.width; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("%1. 原件 %2/%3\n移出名 %4\n副本 %5/%6\nbytes %7 · hash %8 · DB %9 · sealRun %10").arg(modelData.index + 1).arg(capacityArea.saved ? capacityArea.saved.dto.sourceDirectory : "").arg(modelData.source.basename).arg(modelData.retiredBasename).arg(capacityArea.saved ? capacityArea.saved.dto.backupDirectory : "").arg(modelData.backupBasename).arg(modelData.source.fileBytes).arg(modelData.source.fileFingerprint).arg(modelData.source.databaseEpoch).arg(modelData.source.sealRunId) }
                    }
                }
                Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; enabled: capacityArea.planCurrent && !capacityArea.busy; checked: capacityArea.planCurrent && capacityArea.confirmed === capacityArea.saved; text: qsTr("我已逐项核对本次完整路径、hash和容量影响；允许这些sealed移出后备份，知道部分失败/取消不回滚，且本确认不允许永久删除。"); Accessible.name: text; property var press: null; onPressed: press = capacityArea.snapshot("confirm"); onCanceled: press = null; onClicked: { var g = capacityArea.life, p = press, c = checked; press = null; if (g.alive) capacityArea.confirm(p, c) } Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true; onActiveFocusChanged: if (activeFocus) capacityArea.focusVisible(this); contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; leftPadding: parent.indicator.width + root.pad } }
                Repeater {
                    model: ["commit", "discard", "cancel"]
                    delegate: QGCButton {
                        Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus
                        text: modelData === "commit" ? qsTr("提交已确认容量计划（顺序显式读盘/移动/备份）") : modelData === "discard" ? qsTr("放弃本次容量预览") : qsTr("仅请求取消本页原容量计划（不回滚）")
                        enabled: !capacityArea.busy && (modelData === "cancel" ? capacityArea.ownBusy : root.idle && capacityArea.planCurrent && (modelData !== "commit" || capacityArea.confirmed === capacityArea.saved))
                        Accessible.name: text; property var press: null; onPressed: press = capacityArea.snapshot(modelData); onCanceled: press = null
                        onClicked: { var g = capacityArea.life, p = press; press = null; if (g.alive) capacityArea.act(p) }
                        Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true
                        onActiveFocusChanged: if (activeFocus) capacityArea.focusVisible(this)
                        contentItem: QGCLabel { text: parent.text; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText }
                    }
                }
                QGCLabel { Layout.fillWidth: true; visible: capacityArea.message.length > 0; activeFocusOnTab: true; text: capacityArea.message; textFormat: Text.PlainText; wrapMode: Text.WordWrap; color: qgcPal.warningText; onActiveFocusChanged: if (activeFocus) capacityArea.focusVisible(this) }
                QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: capacityArea.statusKnown(capacityArea.nativeStatus) ? qsTr("原生最近容量计划诊断（非本页认领/非任务状态）\nplan %1 · 原job %2\n阶段 %3 / %4 · busy %5 · 已核备份完成 %6/%7\n取消请求 %8 · ok %9 · 原因 %10\n每步只有原worker实际finished才推进；移动、备份、取消和最终容量复核是不同事实。").arg(capacityArea.nativeStatus.planId).arg(capacityArea.nativeStatus.jobId).arg(capacityArea.nativeStatus.stage).arg(capacityArea.nativeStatus.phase).arg(root.known(capacityArea.nativeStatus.busy)).arg(capacityArea.nativeStatus.completedCount).arg(capacityArea.nativeStatus.totalCount).arg(root.known(capacityArea.nativeStatus.cancelRequested)).arg(root.known(capacityArea.nativeStatus.ok)).arg(root.text(capacityArea.nativeStatus.error)) : qsTr("原生容量状态未知，不补成功或未移动/未写。") }
                QGCLabel { Layout.fillWidth: true; visible: capacityArea.planKnown(capacityArea.nativePlan) && capacityArea.statusKnown(capacityArea.nativeStatus) && capacityArea.nativePlan.planId === capacityArea.nativeStatus.planId; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: visible ? qsTr("上述原生诊断对应的原计划目录（只读，不作新确认）\n源：%1\n副本：%2").arg(capacityArea.nativePlan.sourceDirectory).arg(capacityArea.nativePlan.backupDirectory) : "" }
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: capacityArea.statusKnown(capacityArea.nativeStatus) && capacityArea.nativeStatus.results.length ? root.rowHeight * 5 : 0
                    clip: true; model: capacityArea.statusKnown(capacityArea.nativeStatus) ? capacityArea.nativeStatus.results : []; spacing: root.pad
                    Controls.ScrollBar.vertical: Controls.ScrollBar {}
                    delegate: FocusScope {
                        width: ListView.view.width; height: capFact.implicitHeight + root.pad; activeFocusOnTab: true; Accessible.role: Accessible.StaticText; Accessible.name: capFact.text
                        onActiveFocusChanged: if (activeFocus) ListView.view.positionViewAtIndex(index, ListView.Contain)
                        QGCLabel { id: capFact; width: parent.width; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: qsTr("步骤%1 %2\n移动：%3\n备份：%4").arg(modelData.index + 1).arg(root.text(modelData.source.basename)).arg(modelData.move && modelData.move.known === true ? qsTr("confirmed=%1 / mayHaveMoved=%2 / reported=%3 / %4").arg(root.known(modelData.move.moveConfirmed)).arg(root.known(modelData.move.mayHaveMoved)).arg(root.known(modelData.move.renameReportedSuccess)).arg(root.text(modelData.move.errorCode)) : qsTr("未知/未取得原终态；不补false")).arg(modelData.backup && modelData.backup.known === true ? qsTr("verified=%1 / mayHaveWritten=%2 / copied=%3 / bytes=%4 / %5").arg(root.known(modelData.backup.backupVerified)).arg(root.known(modelData.backup.mayHaveWritten)).arg(root.known(modelData.backup.copyCompleted)).arg(root.text(modelData.backup.bytesCopied)).arg(root.text(modelData.backup.errorCode)) : qsTr("未知/未取得原终态；不补false")) }
                    }
                }
                QGCLabel { Layout.fillWidth: true; color: qgcPal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("完成只表示本计划文件步骤与末次清单复核通过，未核物理空闲/异地耐久或历史全集。永久清理仍在下方525：须另做当前retired原件的新备份、独立预览和两项确认；本计划不会准备或继承删除资格。") }
            }
        }
        FleetRetiredArchivePurgePanel {
            Layout.fillWidth: true
            Layout.minimumHeight: implicitHeight
            workspace: root.archive ? root.archive.retiredPurge : null
            sourceActive: root.visible && root.enabled
        }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: root.showDetails ? qsTr("收起本页核实的完整维护DTO") : qsTr("展开本页核实的完整维护DTO（只读）"); Accessible.name: text; onClicked: root.showDetails = !root.showDetails; contentItem: QGCLabel { text: parent.text; color: parent._showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText; textFormat: Text.PlainText; wrapMode: Text.WordWrap } }
        Loader {
            Layout.fillWidth: true; active: root.showDetails
            Layout.preferredHeight: item ? item.implicitHeight : 0
            sourceComponent: Component { Controls.TextArea { width: parent ? parent.width : 1; readOnly: true; selectByMouse: true; textFormat: TextEdit.PlainText; wrapMode: TextEdit.Wrap; color: qgcPal.text; text: JSON.stringify({inventory: root.savedInventory ? root.savedInventory.dto : null, preview: root.savedPreview ? root.savedPreview.dto : null, importPreview: root.savedImportPreview ? root.savedImportPreview.dto : null, result: root.savedResult ? root.savedResult.dto : null, backupCatalog: root.savedBackupCatalog ? root.savedBackupCatalog.dto : null, backupDiagnostics: root.savedBackupDiagnostics ? root.savedBackupDiagnostics.dto : null}, null, 2); Accessible.name: qsTr("本页核实的维护DTO，只读，不作为路径输入或权限恢复"); background: Rectangle { color: qgcPal.windowShade } } }
        }
    }
}
