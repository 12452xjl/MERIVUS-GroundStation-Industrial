import QtQuick 2.12
import QtQuick.Controls 2.4 as Controls
import QtQuick.Layouts 1.11
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

// Current private native history page -> explicit historical maintenance append.
// The parent passes no page when its selection/consent no longer owns that page.
FocusScope {
    id: root
    property var archive: null
    property var page: null
    property bool active: false
    property var lifetime: ({alive: true, ticket: null})
    property double epoch: 0
    property double consentEpoch: 0
    property bool exhausted: false
    property bool callBusy: false
    property bool previewConfirmed: false
    property bool appendConfirmed: false
    property int selectedIndex: -1
    property var savedSources: null
    property var savedPreview: null
    property var appendReceipt: null
    property string feedback: ""
    readonly property real pad: ScreenTools.defaultFontPixelWidth
    readonly property real rowHeight: Math.max(44, ScreenTools.defaultFontPixelHeight * 2.5)
    readonly property string pageStamp: pageIdentity(page)
    readonly property var nativePreview: archive ? archive.maintenanceHistoryPreview || {} : ({})
    readonly property bool ready: lifetime.alive && !exhausted && active && visible && enabled
        && !!archive && pageStamp.length > 0 && typeof archive.maintenanceSources === "function"
        && typeof archive.prepareMaintenance === "function" && typeof archive.appendMaintenance === "function"
        && archive.current.historyBusy === false && archive.current.historyResultAccepted === true
        && archive.current.historyCancelled === false && archive.current.latestHistoryId === page.queryId
        && archive.historyPage.queryId === page.queryId && archive.historyPage.fileHash === page.fileHash
    readonly property var candidates: savedSources && savedSources.owner === archive
        && savedSources.pageStamp === pageStamp ? savedSources.dto.candidates : []
    readonly property var selectedSource: selectedIndex >= 0 && selectedIndex < candidates.length ? candidates[selectedIndex] : null
    readonly property bool previewCurrent: ready && !!savedPreview && savedPreview.owner === archive
        && savedPreview.pageStamp === pageStamp && savedPreview.selection === selectionKey(selectedSource)
        && nativePreview.current === true && nativePreview.previewId === savedPreview.dto.previewId
        && JSON.stringify(nativePreview) === JSON.stringify(savedPreview.dto)
    implicitHeight: form.implicitHeight
    QGCPalette { id: pal; colorGroupEnabled: root.enabled }

    function map(value) { return !!value && typeof value === "object" && !Array.isArray(value) }
    function integer(value, low, high) { return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= low && value <= high }
    function uuid(value) { return typeof value === "string" && /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value) && !/^0{8}-0{4}-0{4}-0{4}-0{12}$/.test(value) }
    function digest(value) { return typeof value === "string" && /^[0-9a-f]{64}$/.test(value) }
    function decimal(value) { return typeof value === "string" && /^(0|[1-9][0-9]{0,18})$/.test(value) && (value.length < 19 || value <= "9223372036854775807") }
    function atMost(a, b) { return decimal(a) && decimal(b) && (a.length < b.length || (a.length === b.length && a <= b)) }
    function copy(value) { try { return JSON.parse(JSON.stringify(value)) } catch (error) { return null } }
    function pageIdentity(value) {
        if (!map(value) || typeof value.queryId !== "string" || !value.queryId.length
                || !uuid(value.runId) || !digest(value.headerHash) || !digest(value.fileHash)
                || !decimal(value.afterSequence) || !decimal(value.nextSequence)
                || typeof value.basename !== "string" || value.historicalOnly !== true
                || value.authenticated !== false || value.permissionsRestored !== false
                || value.executionPermit !== false || value.flightCommandReleased !== false) return ""
        return JSON.stringify([value.queryId, value.catalogId, value.runId, value.headerHash,
            value.fileHash, value.afterSequence, value.nextSequence, value.basename])
    }
    function selectionKey(value) {
        return map(value) ? JSON.stringify([value.sourceEpoch, value.vehicleId, value.componentId, value.batteryId]) : ""
    }
    function candidateKnown(value) {
        return map(value) && Object.keys(value).length === 5 && uuid(value.sourceEpoch)
            && integer(value.vehicleId, 1, 255) && integer(value.componentId, 1, 255)
            && integer(value.batteryId, 0, 255) && integer(value.sampleCount, 1, 64)
    }
    function candidatesKnown(value, queryId) {
        if (!map(value) || Object.keys(value).length !== 6 || value.schemaVersion !== 1
                || value.queryId !== queryId || value.historicalOnly !== true || value.authenticated !== false
                || typeof value.reason !== "string" || !Array.isArray(value.candidates) || value.candidates.length > 64) return false
        var seen = {}, total = 0
        for (var i = 0; i < value.candidates.length; ++i) {
            var item = value.candidates[i], key = selectionKey(item)
            if (!candidateKnown(item) || seen[key]) return false
            seen[key] = true
            total += item.sampleCount
        }
        return total <= 64
    }
    function previewKnown(value, candidate, currentPage) {
        if (!map(value) || Object.keys(value).length !== 14 || value.schemaVersion !== 1 || !uuid(value.previewId)
                || value.queryId !== currentPage.queryId || value.current !== true || typeof value.appendable !== "boolean"
                || typeof value.reason !== "string" || value.historicalOnly !== true || value.authenticated !== false
                || value.physicalIdentityVerified !== false || value.executionPermit !== false || value.flightCommandReleased !== false
                || !map(value.source) || Object.keys(value.source).length !== 7
                || value.source.runId !== currentPage.runId || value.source.headerHash !== currentPage.headerHash
                || value.source.fileHash !== currentPage.fileHash || selectionKey(value.source) !== selectionKey(candidate)
                || !map(value.metrics) || Object.keys(value.metrics).length > 2
                || !map(value.evidence) || Object.keys(value.evidence).length !== 21) return false
        var evidence = value.evidence
        if (evidence.pageAfterSequence !== currentPage.afterSequence || evidence.pageNextSequence !== currentPage.nextSequence
                || !decimal(evidence.firstSequence) || evidence.firstSequence === "0" || !decimal(evidence.lastSequence)
                || !atMost(evidence.firstSequence, evidence.lastSequence) || atMost(evidence.firstSequence, evidence.pageAfterSequence)
                || !atMost(evidence.lastSequence, evidence.pageNextSequence) || evidence.sampleCount !== candidate.sampleCount
                || typeof evidence.utcRegressionObserved !== "boolean" || evidence.completeRecordPrefixOnly !== true
                || evidence.historyComplete !== false || typeof evidence.footerPresent !== "boolean"
                || typeof evidence.partialTail !== "boolean" || typeof evidence.hasMore !== "boolean") return false
        var timestamps = ["firstReceivedUtcMs", "lastReceivedUtcMs", "minimumReceivedUtcMs", "maximumReceivedUtcMs"]
        for (var t = 0; t < timestamps.length; ++t) if (!integer(evidence[timestamps[t]], 1, 9007199254740991)) return false
        if (evidence.minimumReceivedUtcMs > evidence.firstReceivedUtcMs || evidence.minimumReceivedUtcMs > evidence.lastReceivedUtcMs
                || evidence.maximumReceivedUtcMs < evidence.firstReceivedUtcMs || evidence.maximumReceivedUtcMs < evidence.lastReceivedUtcMs) return false
        var counts = ["temperatureKnownCount", "temperatureUnknownCount", "temperatureOutOfRangeCount", "currentKnownCount", "currentUnknownCount", "chargingCount"]
        for (var c = 0; c < counts.length; ++c) if (!integer(evidence[counts[c]], 0, evidence.sampleCount)) return false
        if (evidence.temperatureKnownCount + evidence.temperatureUnknownCount + evidence.temperatureOutOfRangeCount !== evidence.sampleCount
                || evidence.currentKnownCount + evidence.currentUnknownCount + evidence.chargingCount !== evidence.sampleCount) return false
        var names = Object.keys(value.metrics)
        for (var m = 0; m < names.length; ++m) {
            var metric = value.metrics[names[m]]
            if (typeof metric !== "number" || !isFinite(metric)) return false
            if (names[m] === "temperatureC") { if (metric < -100 || metric > 300 || evidence.temperatureKnownCount === 0) return false }
            else if (names[m] === "currentAmps") { if (metric < 0 || metric > 327.67 || evidence.currentKnownCount === 0) return false }
            else return false
        }
        return Object.prototype.hasOwnProperty.call(value.metrics, "temperatureC") === (evidence.temperatureKnownCount > 0)
            && Object.prototype.hasOwnProperty.call(value.metrics, "currentAmps") === (evidence.currentKnownCount > 0)
            && (value.appendable ? names.length > 0 && value.reason === ""
                : names.length === 0 ? value.reason === "maintenance-history-no-usable-metrics"
                    : value.reason === "maintenance-history-timestamp-unrepresentable")
    }
    function advance() {
        var guard = lifetime
        if (!guard.alive) return false
        if (epoch >= 9007199254740991) { exhausted = true; return false }
        epoch += 1
        return guard.alive
    }
    function invalidate() {
        var guard = lifetime, prior = guard.ticket, barrier = prior || {}
        if (!guard.alive) return
        if (!prior) guard.ticket = barrier
        try {
            if (!advance() || !guard.alive || guard.ticket !== barrier) return
            previewConfirmed = false
            if (!guard.alive || guard.ticket !== barrier) return
            appendConfirmed = false
            if (!guard.alive || guard.ticket !== barrier) return
            savedPreview = null
        } finally {
            if (!prior && guard.ticket === barrier) guard.ticket = null
        }
    }
    function consentChanged() {
        if (consentEpoch >= 9007199254740991) exhausted = true
        else consentEpoch += 1
    }
    function sourceChanged() {
        var guard = lifetime, prior = guard.ticket, barrier = prior || {}
        if (!guard.alive) return
        if (!prior) guard.ticket = barrier
        try {
            savedSources = null
            if (!guard.alive || guard.ticket !== barrier) return
            selectedIndex = -1
            if (!guard.alive || guard.ticket !== barrier) return
            invalidate()
        } finally {
            if (!prior && guard.ticket === barrier) guard.ticket = null
        }
    }
    function snapshot(action) {
        if (["sources", "prepare", "append", "discard"].indexOf(action) < 0
                || !ready || callBusy || lifetime.ticket) return null
        if (action === "prepare" && (!selectedSource || !previewConfirmed)) return null
        if (action === "append" && (!previewCurrent || !appendConfirmed || savedPreview.dto.appendable !== true)) return null
        if (action === "discard" && !previewCurrent) return null
        return {action: action, owner: archive, pageStamp: pageStamp, epoch: epoch, consentEpoch: consentEpoch,
            selection: selectionKey(selectedSource), candidate: copy(selectedSource),
            queryId: page.queryId, previewId: savedPreview ? savedPreview.dto.previewId : ""}
    }
    function currentTicket(ticket) {
        return lifetime.alive && ready && ticket.owner === archive && ticket.pageStamp === pageStamp
            && ticket.epoch === epoch && ticket.consentEpoch === consentEpoch
            && ticket.selection === selectionKey(selectedSource)
    }
    function showFailure(message, contextCheck) {
        var guard = lifetime
        if (!guard.alive || !contextCheck()) return
        feedback = message
        if (!guard.alive || !contextCheck() || !active || !visible || !enabled) return
        feedbackLabel.forceActiveFocus()
    }
    function act(ticket) {
        if (!ticket || callBusy || lifetime.ticket || !currentTicket(ticket)) return
        var fresh = snapshot(ticket.action)
        if (!fresh || fresh.previewId !== ticket.previewId || fresh.selection !== ticket.selection
                || JSON.stringify(fresh.candidate) !== JSON.stringify(ticket.candidate)) return
        var guard = lifetime, owner = ticket.owner, currentPage = copy(page), result
        var operation = {}, expectedEpoch = ticket.epoch, expectedConsent = ticket.consentEpoch
        var expectedSelection = ticket.selection, expectedSources = savedSources, expectedPreview = savedPreview
        var expectedNativePreview = null
        function current() {
            return guard.alive && lifetime === guard && guard.ticket === operation && ready
                && ticket.owner === archive && ticket.pageStamp === pageStamp && ticket.queryId === page.queryId
                && expectedEpoch === epoch && expectedConsent === consentEpoch
                && expectedSelection === selectionKey(selectedSource)
                && savedSources === expectedSources && savedPreview === expectedPreview
                && (expectedNativePreview === null || (nativePreview.current === true
                    && nativePreview.previewId === expectedNativePreview.previewId
                    && JSON.stringify(nativePreview) === JSON.stringify(expectedNativePreview)))
        }
        function consumeConsents() {
            // Permit only our exact false assignment, not a false->true ABA
            // from a notification observer or a newly frozen confirmation.
            if (previewConfirmed) expectedConsent += 1
            previewConfirmed = false
            if (!current() || previewConfirmed) return false
            if (appendConfirmed) expectedConsent += 1
            appendConfirmed = false
            return current() && !previewConfirmed && !appendConfirmed
        }
        function clearLocalPreview() {
            expectedPreview = null
            savedPreview = null
            return current()
        }
        guard.ticket = operation
        try {
            callBusy = true
            if (!current() || !consumeConsents()) return
            if (ticket.action === "sources") {
                // Retire the previous local qualification before asking for
                // replacement candidates. Do not discard any native preview.
                expectedEpoch += 1
                if (!advance() || !current() || !clearLocalPreview()) return
                expectedSelection = ""
                if (selectedIndex !== -1) {
                    // onSelectedIndexChanged performs one local invalidation.
                    expectedEpoch += 1
                    selectedIndex = -1
                }
                if (!current() || previewConfirmed || appendConfirmed) return
            } else if (ticket.action === "prepare") {
                if (!clearLocalPreview()) return
            } else if (!previewCurrent || !savedPreview || savedPreview.dto.previewId !== ticket.previewId) return
            if (!current()) return
            if (ticket.action === "sources") result = owner.maintenanceSources(ticket.queryId, true)
            else if (ticket.action === "prepare") result = owner.prepareMaintenance({queryId: ticket.queryId,
                sourceEpoch: ticket.candidate.sourceEpoch, vehicleId: ticket.candidate.vehicleId,
                componentId: ticket.candidate.componentId, batteryId: ticket.candidate.batteryId}, true)
            else if (ticket.action === "append") result = owner.appendMaintenance(ticket.previewId, true)
            else if (ticket.action === "discard") result = owner.discardMaintenance(ticket.previewId)
            // Native may already have committed the append. Losing this
            // presentation never means rollback, and never triggers a retry.
            if (!current() || previewConfirmed || appendConfirmed) return
            if (ticket.action === "sources") {
                if (!candidatesKnown(result, ticket.queryId)) { showFailure(qsTr("来源结果不可核对，请重新读取当前页来源。"), current); return }
                expectedSources = {owner: owner, pageStamp: ticket.pageStamp, dto: copy(result)}
                savedSources = expectedSources
                if (!current() || previewConfirmed || appendConfirmed) return
                feedback = result.reason || qsTr("本页找到%1个历史电池来源；请选择一个，不会自动选取。").arg(result.candidates.length)
            } else if (ticket.action === "prepare") {
                if (!previewKnown(result, ticket.candidate, currentPage)) { showFailure(qsTr("预览未生成或已失效；请重新选择并预览当前页。"), current); return }
                var frozenPreview = copy(result)
                if (!frozenPreview) { showFailure(qsTr("预览返回不可保存为本页显示；不会写入或自动重试。"), current); return }
                expectedNativePreview = frozenPreview
                expectedPreview = {owner: owner, pageStamp: ticket.pageStamp, selection: ticket.selection, dto: frozenPreview}
                savedPreview = expectedPreview
                if (!current() || previewConfirmed || appendConfirmed) return
                if (result.appendable) feedback = qsTr("预览已冻结。请核对样本、来源及局限后另行确认写入。")
                else showFailure((result.reason === "maintenance-history-timestamp-unrepresentable"
                    ? qsTr("已保留本页指标诊断，但历史时间无法表示，不能写入；不会修造时间。")
                    : qsTr("本页此来源没有可写入的温度或放电电流；未知和充电不当作零值。"))
                    + "\n" + qsTr("原生原因：%1").arg(result.reason), current)
            } else if (ticket.action === "append") {
                if (!map(result) || Object.keys(result).length !== 5 || result.previewId !== ticket.previewId
                        || typeof result.error !== "string" || !map(result.assessment) || typeof result.historyScopeId !== "string"
                        || (result.appendAccepted !== true && result.appendAccepted !== false)) {
                    showFailure(qsTr("写入结果未知。请人工核对护照记录；不会自动重试或声称回滚。"), current)
                    return
                }
                if (result.appendAccepted && (result.error !== "" || !digest(result.historyScopeId)
                        || result.assessment.historyScopeId !== result.historyScopeId
                        || result.assessment.vehicleId !== ticket.candidate.vehicleId
                        || result.assessment.historicalOnly !== true || result.assessment.historyComplete !== false
                        || result.assessment.authenticated !== false || result.assessment.executionPermit !== false
                        || result.assessment.flightCommandReleased !== false
                        || JSON.stringify(result.assessment.historicalTelemetrySource) !== JSON.stringify(expectedPreview.dto.source))) {
                    showFailure(qsTr("写入回执的历史来源组不可核对。请人工检查记录；不会自动重试或声称回滚。"), current)
                    return
                }
                var frozenReceipt = copy(result)
                if (!frozenReceipt) { showFailure(qsTr("写入回执无法显示。请人工核对记录；不会自动重试或声称回滚。"), current); return }
                // A rejected later attempt must not erase the last actual
                // committed receipt, which remains explicitly historical.
                if (result.appendAccepted) appendReceipt = frozenReceipt
                if (!current() || previewConfirmed || appendConfirmed) return
                if (result.appendAccepted) feedback = qsTr("已追加本页历史证据并更新维护评估。不是当前健康认证或飞行放行。")
                else showFailure(qsTr("未追加：%1。请核对重复区间、容量或重新生成预览。").arg(result.error), current)
            } else {
                if (!clearLocalPreview()) return
                expectedEpoch += 1
                if (!advance() || !current()) return
                if (result === true) feedback = qsTr("已撤销本次准确预览；已写入的历史记录不会被删除。")
                else showFailure(qsTr("本次预览未撤销或已被替换，不会清理新的预览。"), current)
            }
        } catch (error) {
            if (current()) showFailure(ticket.action === "append"
                ? qsTr("写入结果未知。请人工核对护照记录；不会自动重试或声称回滚。")
                : qsTr("本次操作结果未知；不会自动续作、写入或清理其它预览。"), current)
        } finally {
            // Keep the plain ticket through callBusy=false notifications.
            if (guard.alive && guard.ticket === operation) callBusy = false
            if (guard.ticket === operation) guard.ticket = null
        }
    }
    function labels() {
        var list = [qsTr("请选择历史来源（不认证实物）")]
        for (var i = 0; i < candidates.length; ++i) {
            var source = candidates[i]
            list.push(qsTr("机号%1 / 组件%2 / 电池槽%3 · %4帧 · 来源%5")
                .arg(source.vehicleId).arg(source.componentId).arg(source.batteryId).arg(source.sampleCount).arg(source.sourceEpoch))
        }
        return list
    }
    function metricsText() {
        if (!savedPreview) return qsTr("尚未生成预览；不读取实时Fact补齐历史。")
        var value = savedPreview.dto, e = value.evidence
        return qsTr("仅本页报告峰值：温度 %1 °C；非负放电电流 %2 A\n样本%3；温度有效/未知/越界：%4/%5/%6；电流有效/未知/充电：%7/%8/%9\n选中序号：%10–%11；本页游标：%12→%13\n原本地收包UTC首/末(ms)：%14 / %15；UTC回退：%16\nFooter：%17；部分尾行：%18；还有下一页：%19；历史完整：否\n预览仍可用：%20")
            .arg(Object.prototype.hasOwnProperty.call(value.metrics, "temperatureC") ? value.metrics.temperatureC.toFixed(2) : qsTr("未知"))
            .arg(Object.prototype.hasOwnProperty.call(value.metrics, "currentAmps") ? value.metrics.currentAmps.toFixed(2) : qsTr("未知"))
            .arg(e.sampleCount).arg(e.temperatureKnownCount).arg(e.temperatureUnknownCount).arg(e.temperatureOutOfRangeCount)
            .arg(e.currentKnownCount).arg(e.currentUnknownCount).arg(e.chargingCount)
            .arg(e.firstSequence).arg(e.lastSequence).arg(e.pageAfterSequence).arg(e.pageNextSequence)
            .arg(e.firstReceivedUtcMs).arg(e.lastReceivedUtcMs).arg(e.utcRegressionObserved ? qsTr("是") : qsTr("否"))
            .arg(e.footerPresent ? qsTr("有") : qsTr("无")).arg(e.partialTail ? qsTr("有") : qsTr("无"))
            .arg(e.hasMore ? qsTr("是") : qsTr("否")).arg(previewCurrent ? qsTr("是") : qsTr("否 / 仅留显示"))
            + "\n" + qsTr("原本地收包UTC最小 / 最大(ms)：%1 / %2").arg(e.minimumReceivedUtcMs).arg(e.maximumReceivedUtcMs)
            + (value.appendable ? "" : "\n" + qsTr("原生不可追加原因：%1").arg(value.reason))
    }
    onPageStampChanged: sourceChanged()
    onArchiveChanged: sourceChanged()
    onSelectedIndexChanged: invalidate()
    onActiveChanged: invalidate()
    onVisibleChanged: invalidate()
    onEnabledChanged: invalidate()
    onPreviewConfirmedChanged: consentChanged()
    onAppendConfirmedChanged: consentChanged()
    Component.onDestruction: lifetime.alive = false

    ColumnLayout {
        id: form
        width: root.width
        spacing: root.pad
        QGCLabel { Layout.fillWidth: true; font.bold: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("本页电池证据 → 历史维护护照") }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("只提取本页BATTERY_STATUS报告温度和非负放电电流。不是整次飞行峰值、物理电池身份或当前健康；不推算振动RMS、寿命、飞行次数或成功率。写入仅在本进程保存，由既有护照文件接口另行导出；此处不自动落盘。") }
        QGCLabel { Layout.fillWidth: true; visible: !root.ready; color: pal.warningText; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("需要本面板当前已完成的原生历史页；换页、取消、隐藏或改选择后须重新确认。") }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("列出本页电池维护来源"); enabled: root.ready && !root.callBusy; Accessible.name: text; property var press: null; onPressed: press = root.snapshot("sources"); onCanceled: press = null; onClicked: { var guard = root.lifetime, value = press; press = null; if (guard.alive) root.act(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
        Controls.ComboBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; model: root.labels(); currentIndex: root.selectedIndex + 1; enabled: root.ready && !root.callBusy; Accessible.name: qsTr("完整档案历史来源，不认证同一架飞机或电池"); onActivated: root.selectedIndex = index - 1 }
        Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("确认预览此来源"); checked: root.previewConfirmed; enabled: root.ready && !root.callBusy && !!root.selectedSource; onToggled: root.previewConfirmed = checked; Accessible.name: qsTr("确认预览所选本页历史来源，不写入护照") }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("生成本页维护证据预览"); enabled: root.ready && !root.callBusy && !!root.selectedSource && root.previewConfirmed; Accessible.name: text; property var press: null; onPressed: press = root.snapshot("prepare"); onCanceled: press = null; onClicked: { var guard = root.lifetime, value = press; press = null; if (guard.alive) root.act(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
        QGCLabel { Layout.fillWidth: true; activeFocusOnTab: true; Accessible.role: Accessible.StaticText; Accessible.name: text; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.metricsText() }
        QGCLabel { Layout.fillWidth: true; visible: !!root.savedPreview; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.savedPreview ? qsTr("完整历史来源组（不认证实物）：\nRun %1\n头摘要 %2\n文件摘要 %3\n来源epoch %4\n机号%5 / 组件%6 / 电池槽%7").arg(root.savedPreview.dto.source.runId).arg(root.savedPreview.dto.source.headerHash).arg(root.savedPreview.dto.source.fileHash).arg(root.savedPreview.dto.source.sourceEpoch).arg(root.savedPreview.dto.source.vehicleId).arg(root.savedPreview.dto.source.componentId).arg(root.savedPreview.dto.source.batteryId) : "" }
        QGCLabel { Layout.fillWidth: true; textFormat: Text.PlainText; wrapMode: Text.WordWrap; text: qsTr("以下另一次确认仅追加此预览的历史证据，不认证飞机、不授飞行权限，也不自动保存文件。") }
        Controls.CheckBox { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; text: qsTr("确认追加历史证据"); checked: root.appendConfirmed; enabled: root.previewCurrent && !root.callBusy && root.savedPreview.dto.appendable === true; onToggled: root.appendConfirmed = checked; Accessible.name: qsTr("另行确认仅追加此预览的历史证据，不认证飞机、不授飞行权限") }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("写入历史维护护照"); enabled: root.previewCurrent && !root.callBusy && root.appendConfirmed && root.savedPreview.dto.appendable === true; Accessible.name: text; property var press: null; onPressed: press = root.snapshot("append"); onCanceled: press = null; onClicked: { var guard = root.lifetime, value = press; press = null; if (guard.alive) root.act(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
        QGCButton { Layout.fillWidth: true; Layout.minimumHeight: root.rowHeight; focusPolicy: Qt.StrongFocus; text: qsTr("撤销本次预览（不删除已写记录）"); enabled: root.previewCurrent && !root.callBusy; Accessible.name: text; property var press: null; onPressed: press = root.snapshot("discard"); onCanceled: press = null; onClicked: { var guard = root.lifetime, value = press; press = null; if (guard.alive) root.act(value) }; Keys.onReturnPressed: event.accepted = true; Keys.onEnterPressed: event.accepted = true }
        QGCLabel { id: feedbackLabel; Layout.fillWidth: true; activeFocusOnTab: true; Accessible.role: Accessible.StaticText; Accessible.name: text; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.feedback || qsTr("尚未写入。重复或重叠序号区间将拒绝，不会补成新的飞行记录。") }
        QGCLabel { Layout.fillWidth: true; visible: !!root.appendReceipt && root.appendReceipt.appendAccepted === true; textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere; text: root.appendReceipt && root.appendReceipt.appendAccepted === true ? qsTr("上次实际写入回执（不随换页变成当前健康）：\n来源组 %1\n状态 %2；风险仅估算；证据缺口：%3\n通用阈值原因：%4\n建议：%5").arg(root.appendReceipt.historyScopeId).arg(root.appendReceipt.assessment.maintenanceStatus || qsTr("未知")).arg((root.appendReceipt.assessment.evidenceGaps || []).join(" / ")).arg((root.appendReceipt.assessment.thresholdReasons || []).join(" / ")).arg((root.appendReceipt.assessment.recommendedChecks || []).join(" / ")) : "" }
    }
}
