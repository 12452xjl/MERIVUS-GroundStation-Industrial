message("Adding MERIVUS Custom Plugin")

CUSTOM_QGC_VERSION = 0.1.0

WindowsBuild {
    VERSION = 0.1.0.1
}

DEFINES -= DAILY_BUILD

DEFINES -= APP_VERSION_STR=\"\\\"$$APP_VERSION_STR\\\"\"
DEFINES += APP_VERSION_STR=\"\\\"$$CUSTOM_QGC_VERSION\\\"\"

DEFINES += CUSTOMHEADER=\"\\\"CustomPlugin.h\\\"\"
DEFINES += CUSTOMCLASS=CustomPlugin

TARGET   = MERIVUS
DEFINES += QGC_APPLICATION_NAME='"\\\"MERIVUS\\\""'
DEFINES += QGC_ORG_NAME=\"\\\"MERIVUS\\\"\"
DEFINES += QGC_ORG_DOMAIN=\"\\\"com.merivus\\\"\"

QGC_APP_NAME        = "MERIVUS"
QGC_BINARY_NAME     = "MERIVUS"
QGC_ORG_NAME        = "MERIVUS"
QGC_ORG_DOMAIN      = "com.merivus"
QGC_ANDROID_PACKAGE = "com.merivus.qgroundcontrol"
QGC_APP_DESCRIPTION = "MERIVUS Ground Control"
QGC_APP_COPYRIGHT   = "Copyright (C) 2026 MERIVUS. All rights reserved."

RESOURCES += \
    $$PWD/custom.qrc \
    $$PWD/merivus_ai_panel.qrc

# Qt 5.15 qmlcachegen crashes on the large MERIVUS AI panel after local edits;
# keep this panel interpreted while preserving the same qrc runtime path.
QTQUICK_COMPILER_SKIPPED_RESOURCES += $$PWD/merivus_ai_panel.qrc

QML_IMPORT_PATH += \
   $$PWD/res

SOURCES += \
    $$PWD/src/Ai/ActionProposal.cc \
    $$PWD/src/Ai/AiAgentClient.cc \
    $$PWD/src/Ai/AiAuditEvent.cc \
    $$PWD/src/Ai/AiCommandPolicy.cc \
    $$PWD/src/Ai/AiSchemaValidator.cc \
    $$PWD/src/Ai/AiServiceSupervisor.cc \
    $$PWD/src/CustomPlugin.cc \
    $$PWD/src/Diagnostics/MerivusLinkDiagnostics.cc \
    $$PWD/src/Review/ReviewRangeController.cc \
    $$PWD/src/Review/ReviewVideoController.cc \
    $$PWD/src/Swarm/CommandTransaction.cc \
    $$PWD/src/Swarm/FaultToleranceManager.cc \
    $$PWD/src/Swarm/FormationPlanner.cc \
    $$PWD/src/Swarm/FleetRegistry.cc \
    $$PWD/src/Swarm/FleetHealthCenter.cc \
    $$PWD/src/Swarm/FleetCapabilityDiscovery.cc \
    $$PWD/src/Swarm/FleetCapabilityEvidence.cc \
    $$PWD/src/Swarm/FleetReplacementMatcher.cc \
    $$PWD/src/Swarm/FleetNativeOccupancyEvidence.cc \
    $$PWD/src/Swarm/FleetFormationQueue.cc \
    $$PWD/src/Swarm/FleetOccupancyJournalCodec.cc \
    $$PWD/src/Swarm/FleetOccupancyJournalWorker.cc \
    $$PWD/src/Swarm/FleetOccupancyRecoveryEvidence.cc \
    $$PWD/src/Swarm/FleetOccupancyRecoveryReview.cc \
    $$PWD/src/Swarm/FleetEventRuleEngine.cc \
    $$PWD/src/Swarm/FleetAirspaceConflictChecker.cc \
    $$PWD/src/Swarm/FleetAreaCoveragePlanner.cc \
    $$PWD/src/Swarm/FleetMissionCompiler.cc \
    $$PWD/src/Swarm/FleetMissionPlanningWorker.cc \
    $$PWD/src/Swarm/FleetMissionStartupClearanceWorker.cc \
    $$PWD/src/Swarm/FleetMissionStartupDomainCover.cc \
    $$PWD/src/Swarm/FleetMissionStartupDomainReview.cc \
    $$PWD/src/Swarm/FleetMissionWaypointSwitchReview.cc \
    $$PWD/src/Swarm/FleetMissionInterVehicleEnvelopeReview.cc \
    $$PWD/src/Swarm/FleetMissionStartupClearanceWitness.cc \
    $$PWD/src/Swarm/FleetMissionExecutionProfile.cc \
    $$PWD/src/Swarm/FleetMissionVehicleEvidence.cc \
    $$PWD/src/Swarm/FleetMissionRateOverrideReview.cc \
    $$PWD/src/Swarm/FleetMissionActionPhaseReview.cc \
    $$PWD/src/Swarm/FleetMissionEvidenceRefresh.cc \
    $$PWD/src/Swarm/FleetMissionStartContentCheck.cc \
    $$PWD/src/Swarm/FleetMissionStartApproval.cc \
    $$PWD/src/Swarm/FleetMissionStartupAuthority.cc \
    $$PWD/src/Swarm/FleetMissionStartupTransaction.cc \
    $$PWD/src/Swarm/FleetMissionArmedGroundReview.cc \
    $$PWD/src/Swarm/FleetMissionGroundStartApproval.cc \
    $$PWD/src/Swarm/FleetMissionGroundStartupAuthority.cc \
    $$PWD/src/Swarm/FleetMissionStartupSeries.cc \
    $$PWD/src/Swarm/FleetMissionDeploymentEpisode.cc \
    $$PWD/src/Swarm/FleetMissionDeploymentLifecycle.cc \
    $$PWD/src/Swarm/FleetMissionStartupTransition.cc \
    $$PWD/src/Swarm/FleetMissionStartupMotion.cc \
    $$PWD/src/Swarm/FleetMissionStartupGeographicMotion.cc \
    $$PWD/src/Swarm/FleetMissionStartupIntegerDomain.cc \
    $$PWD/src/Swarm/FleetMissionStartupHome.cc \
    $$PWD/src/Swarm/FleetMissionStartupGlobalOrigin.cc \
    $$PWD/src/Swarm/FleetMissionStartupAltitude.cc \
    $$PWD/src/Swarm/FleetMissionAltitudeArithmetic.cc \
    $$PWD/src/Swarm/FleetMissionClearanceInput.cc \
    $$PWD/src/Swarm/FleetMissionClearanceSource.cc \
    $$PWD/src/Swarm/FleetMissionClearanceDatum.cc \
    $$PWD/src/Swarm/FleetNativeConfigurationObservation.cc \
    $$PWD/src/Swarm/FleetMissionClearanceGeometry.cc \
    $$PWD/src/Swarm/FleetMissionClearanceReview.cc \
    $$PWD/src/Swarm/FleetMissionContactModel.cc \
    $$PWD/src/Swarm/FleetMissionStartupEstimator.cc \
    $$PWD/src/Swarm/FleetMissionStartReadiness.cc \
    $$PWD/src/Swarm/FleetMissionStartReview.cc \
    $$PWD/src/Swarm/FleetMissionProgressObservation.cc \
    $$PWD/src/Swarm/FleetMissionExecutionObservation.cc \
    $$PWD/src/Swarm/FleetMissionModeExitObservation.cc \
    $$PWD/src/Swarm/FleetMissionObservationClose.cc \
    $$PWD/src/Swarm/FleetFirmwareAdapterCatalog.cc \
    $$PWD/src/Swarm/FleetFirmwareCatalogReload.cc \
    $$PWD/src/Swarm/FleetCapabilityReview.cc \
    $$PWD/src/Swarm/FleetOptionalTelemetryCapture.cc \
    $$PWD/src/Swarm/FleetOptionalTelemetryConfiguration.cc \
    $$PWD/src/Swarm/FleetLinkLatencyCapture.cc \
    $$PWD/src/Swarm/FleetCameraStreamCapture.cc \
    $$PWD/src/Swarm/FleetCameraStreamControl.cc \
    $$PWD/src/Swarm/FleetConfigurationDiff.cc \
    $$PWD/src/Swarm/FleetMissionPlanStore.cc \
    $$PWD/src/Swarm/FleetMissionDraft.cc \
    $$PWD/src/Swarm/FleetMissionDraftStore.cc \
    $$PWD/src/Swarm/FleetMissionPayloadBuilder.cc \
    $$PWD/src/Swarm/FleetMissionHeadingEncoding.cc \
    $$PWD/src/Swarm/FleetMissionDwellEncoding.cc \
    $$PWD/src/Swarm/FleetMissionReadbackDiff.cc \
    $$PWD/src/Swarm/FleetMissionReconciliation.cc \
    $$PWD/src/Swarm/FleetMissionDeployment.cc \
    $$PWD/src/Swarm/FleetRoleAllocator.cc \
    $$PWD/src/Swarm/FleetEnergyScheduler.cc \
    $$PWD/src/Swarm/FleetNativeBatteryEvidence.cc \
    $$PWD/src/Swarm/FleetReserveEnergyPlan.cc \
    $$PWD/src/Swarm/FleetMissionRemainderPlanner.cc \
    $$PWD/src/Swarm/FleetMissionGroupRemainderPlanner.cc \
    $$PWD/src/Swarm/FleetReserveDispatchPlanner.cc \
    $$PWD/src/Swarm/FleetMissionRemainderArchive.cc \
    $$PWD/src/Swarm/FleetMissionRemainderView.cc \
    $$PWD/src/Swarm/FleetMissionStartupSpatialReview.cc \
    $$PWD/src/Swarm/FleetMissionSpatialReview.cc \
    $$PWD/src/Swarm/FleetMissionJointSpatialReview.cc \
    $$PWD/src/Swarm/FleetMaintenancePassport.cc \
    $$PWD/src/Swarm/FleetMaintenanceWorkspaceWorker.cc \
    $$PWD/src/Swarm/FleetSnapshotFileIo.cc \
    $$PWD/src/Swarm/FleetGnssMemoryWorkspaceWorker.cc \
    $$PWD/src/Swarm/FleetDeploymentArchiveWorker.cc \
    $$PWD/src/Swarm/FleetLinkDegradationPolicy.cc \
    $$PWD/src/Swarm/FleetMissionRiskEvaluator.cc \
    $$PWD/src/Swarm/FleetMissionReportBuilder.cc \
    $$PWD/src/Swarm/FleetMissionExecutionReport.cc \
    $$PWD/src/Swarm/FleetCapabilityMatcher.cc \
    $$PWD/src/Swarm/FleetExtensionRegistry.cc \
    $$PWD/src/Swarm/FleetAutonomyPolicy.cc \
    $$PWD/src/Swarm/FleetRolePolicy.cc \
    $$PWD/src/Swarm/FleetOperatorSession.cc \
    $$PWD/src/Swarm/FleetOperationApproval.cc \
    $$PWD/src/Swarm/FleetTaskTemplateRegistry.cc \
    $$PWD/src/Swarm/FleetUserTaskTemplates.cc \
    $$PWD/src/Swarm/FleetMissionReportWorkspace.cc \
    $$PWD/src/Swarm/MissionHandoffManager.cc \
    $$PWD/src/Swarm/FleetIntentTask.cc \
    $$PWD/src/Swarm/FleetRiskRadar.cc \
    $$PWD/src/Swarm/SwarmMissionOrchestrator.cc \
    $$PWD/src/Swarm/VehicleCapability.cc \
    $$PWD/src/Swarm/SwarmController.cc \
    $$PWD/src/Swarm/FleetEventBlackBox.cc \
    $$PWD/src/Swarm/FleetArchiveDirectoryInventory.cc \
    $$PWD/src/Swarm/FleetEventArchiveCodec.cc \
    $$PWD/src/Swarm/FleetEventArchiveWorker.cc \
    $$PWD/src/Swarm/FleetEventArchiveSql.cc \
    $$PWD/src/Swarm/FleetEventArchiveHistoryReader.cc \
    $$PWD/src/Swarm/FleetArchiveTaskIndex.cc \
    $$PWD/src/Swarm/FleetEventArchiveMaintenanceWorker.cc \
    $$PWD/src/Swarm/FleetPersistentEventStore.cc \
    $$PWD/src/Swarm/FleetTelemetryArchiveCodec.cc \
    $$PWD/src/Swarm/FleetTelemetryArchiveWorker.cc \
    $$PWD/src/Swarm/FleetTelemetryArchiveHistoryReader.cc \
    $$PWD/src/Swarm/FleetTelemetryArchive.cc \
    $$PWD/src/Swarm/FleetTelemetryMotionReview.cc \
    $$PWD/src/Swarm/FleetTelemetryGnssReview.cc \
    $$PWD/src/Swarm/FleetTimelineReplay.cc \
    $$PWD/src/Swarm/FleetMissionSimulator.cc \
    $$PWD/src/Swarm/FleetMissionDigitalTwin.cc \
    $$PWD/src/Swarm/FleetMissionPreviewTimeline.cc \
    $$PWD/src/Swarm/FleetMissionEvidenceChain.cc \
    $$PWD/src/Swarm/FleetMissionContinuityPlanner.cc \
    $$PWD/src/Swarm/FleetOperationalMemory.cc \
    $$PWD/src/Swarm/FleetMissionReviewCoordinator.cc

HEADERS += \
    $$PWD/src/Ai/ActionProposal.h \
    $$PWD/src/Ai/AiAgentClient.h \
    $$PWD/src/Ai/AiAuditEvent.h \
    $$PWD/src/Ai/AiCommandPolicy.h \
    $$PWD/src/Ai/AiSchemaValidator.h \
    $$PWD/src/Ai/AiServiceSupervisor.h \
    $$PWD/src/CustomPlugin.h \
    $$PWD/src/Diagnostics/MerivusLinkDiagnostics.h \
    $$PWD/src/Review/ReviewRangeController.h \
    $$PWD/src/Review/ReviewRangeMath.h \
    $$PWD/src/Review/ReviewVideoController.h \
    $$PWD/src/Swarm/CommandTransaction.h \
    $$PWD/src/Swarm/FaultToleranceManager.h \
    $$PWD/src/Swarm/FormationPlanner.h \
    $$PWD/src/Swarm/FleetRegistry.h \
    $$PWD/src/Swarm/FleetHealthCenter.h \
    $$PWD/src/Swarm/FleetCapabilityDiscovery.h \
    $$PWD/src/Swarm/FleetCapabilityEvidence.h \
    $$PWD/src/Swarm/FleetReplacementMatcher.h \
    $$PWD/src/Swarm/FleetNativeOccupancyEvidence.h \
    $$PWD/src/Swarm/FleetFormationQueue.h \
    $$PWD/src/Swarm/FleetOccupancyJournalCodec.h \
    $$PWD/src/Swarm/FleetOccupancyJournalWorker.h \
    $$PWD/src/Swarm/FleetOccupancyRecoveryEvidence.h \
    $$PWD/src/Swarm/FleetOccupancyRecoveryReview.h \
    $$PWD/src/Swarm/FleetEventRuleEngine.h \
    $$PWD/src/Swarm/FleetAirspaceConflictChecker.h \
    $$PWD/src/Swarm/FleetAreaCoveragePlanner.h \
    $$PWD/src/Swarm/FleetMissionCompiler.h \
    $$PWD/src/Swarm/FleetMissionPlanningWorker.h \
    $$PWD/src/Swarm/FleetMissionStartupClearanceWorker.h \
    $$PWD/src/Swarm/FleetMissionStartupDomainCover.h \
    $$PWD/src/Swarm/FleetMissionStartupDomainReview.h \
    $$PWD/src/Swarm/FleetMissionWaypointSwitchReview.h \
    $$PWD/src/Swarm/FleetMissionInterVehicleEnvelopeReview.h \
    $$PWD/src/Swarm/FleetMissionStartupClearanceWitness.h \
    $$PWD/src/Swarm/FleetMissionExecutionProfile.h \
    $$PWD/src/Swarm/FleetMissionVehicleEvidence.h \
    $$PWD/src/Swarm/FleetMissionRateOverrideReview.h \
    $$PWD/src/Swarm/FleetMissionActionPhaseReview.h \
    $$PWD/src/Swarm/FleetMissionEvidenceRefresh.h \
    $$PWD/src/Swarm/FleetMissionStartContentCheck.h \
    $$PWD/src/Swarm/FleetMissionStartApproval.h \
    $$PWD/src/Swarm/FleetMissionStartupAuthority.h \
    $$PWD/src/Swarm/FleetMissionStartupTransaction.h \
    $$PWD/src/Swarm/FleetMissionArmedGroundReview.h \
    $$PWD/src/Swarm/FleetMissionGroundStartApproval.h \
    $$PWD/src/Swarm/FleetMissionGroundStartupAuthority.h \
    $$PWD/src/Swarm/FleetMissionStartupSeries.h \
    $$PWD/src/Swarm/FleetMissionDeploymentEpisode.h \
    $$PWD/src/Swarm/FleetMissionDeploymentLifecycle.h \
    $$PWD/src/Swarm/FleetMissionStartupTransition.h \
    $$PWD/src/Swarm/FleetMissionStartupMotion.h \
    $$PWD/src/Swarm/FleetMissionStartupGeographicMotion.h \
    $$PWD/src/Swarm/FleetMissionStartupIntegerDomain.h \
    $$PWD/src/Swarm/FleetMissionStartupHome.h \
    $$PWD/src/Swarm/FleetMissionStartupGlobalOrigin.h \
    $$PWD/src/Swarm/FleetMissionStartupAltitude.h \
    $$PWD/src/Swarm/FleetMissionAltitudeArithmetic.h \
    $$PWD/src/Swarm/FleetMissionClearanceInput.h \
    $$PWD/src/Swarm/FleetMissionClearanceSource.h \
    $$PWD/src/Swarm/FleetMissionClearanceDatum.h \
    $$PWD/src/Swarm/FleetNativeConfigurationObservation.h \
    $$PWD/src/Swarm/FleetMissionClearanceGeometry.h \
    $$PWD/src/Swarm/FleetMissionClearanceReview.h \
    $$PWD/src/Swarm/FleetMissionContactModel.h \
    $$PWD/src/Swarm/FleetMissionStartupEstimator.h \
    $$PWD/src/Swarm/FleetMissionStartReadiness.h \
    $$PWD/src/Swarm/FleetMissionStartReview.h \
    $$PWD/src/Swarm/FleetMissionProgressObservation.h \
    $$PWD/src/Swarm/FleetMissionExecutionObservation.h \
    $$PWD/src/Swarm/FleetMissionModeExitObservation.h \
    $$PWD/src/Swarm/FleetMissionObservationClose.h \
    $$PWD/src/Swarm/FleetFirmwareAdapterCatalog.h \
    $$PWD/src/Swarm/FleetFirmwareCatalogReload.h \
    $$PWD/src/Swarm/FleetCapabilityReview.h \
    $$PWD/src/Swarm/FleetOptionalTelemetryCapture.h \
    $$PWD/src/Swarm/FleetOptionalTelemetryConfiguration.h \
    $$PWD/src/Swarm/FleetLinkLatencyCapture.h \
    $$PWD/src/Swarm/FleetCameraStreamCapture.h \
    $$PWD/src/Swarm/FleetCameraStreamControl.h \
    $$PWD/src/Swarm/FleetConfigurationDiff.h \
    $$PWD/src/Swarm/FleetMissionPlanStore.h \
    $$PWD/src/Swarm/FleetMissionDraft.h \
    $$PWD/src/Swarm/FleetMissionDraftStore.h \
    $$PWD/src/Swarm/FleetMissionPayloadBuilder.h \
    $$PWD/src/Swarm/FleetMissionHeadingEncoding.h \
    $$PWD/src/Swarm/FleetMissionDwellEncoding.h \
    $$PWD/src/Swarm/FleetMissionReadbackDiff.h \
    $$PWD/src/Swarm/FleetMissionReconciliation.h \
    $$PWD/src/Swarm/FleetMissionDeployment.h \
    $$PWD/src/Swarm/FleetRoleAllocator.h \
    $$PWD/src/Swarm/FleetEnergyScheduler.h \
    $$PWD/src/Swarm/FleetNativeBatteryEvidence.h \
    $$PWD/src/Swarm/FleetReserveEnergyPlan.h \
    $$PWD/src/Swarm/FleetMissionRemainderPlanner.h \
    $$PWD/src/Swarm/FleetMissionGroupRemainderPlanner.h \
    $$PWD/src/Swarm/FleetReserveDispatchPlanner.h \
    $$PWD/src/Swarm/FleetMissionRemainderArchive.h \
    $$PWD/src/Swarm/FleetMissionRemainderView.h \
    $$PWD/src/Swarm/FleetMissionStartupSpatialReview.h \
    $$PWD/src/Swarm/FleetMissionSpatialReview.h \
    $$PWD/src/Swarm/FleetMissionJointSpatialReview.h \
    $$PWD/src/Swarm/FleetMaintenancePassport.h \
    $$PWD/src/Swarm/FleetMaintenanceWorkspaceWorker.h \
    $$PWD/src/Swarm/FleetSnapshotFileIo.h \
    $$PWD/src/Swarm/FleetGnssMemoryWorkspaceWorker.h \
    $$PWD/src/Swarm/FleetDeploymentArchiveWorker.h \
    $$PWD/src/Swarm/FleetLinkDegradationPolicy.h \
    $$PWD/src/Swarm/FleetMissionRiskEvaluator.h \
    $$PWD/src/Swarm/FleetMissionReportBuilder.h \
    $$PWD/src/Swarm/FleetMissionExecutionReport.h \
    $$PWD/src/Swarm/FleetCapabilityMatcher.h \
    $$PWD/src/Swarm/FleetExtensionRegistry.h \
    $$PWD/src/Swarm/FleetAutonomyPolicy.h \
    $$PWD/src/Swarm/FleetRolePolicy.h \
    $$PWD/src/Swarm/FleetOperatorSession.h \
    $$PWD/src/Swarm/FleetOperationApproval.h \
    $$PWD/src/Swarm/FleetTaskTemplateRegistry.h \
    $$PWD/src/Swarm/FleetUserTaskTemplates.h \
    $$PWD/src/Swarm/FleetMissionReportWorkspace.h \
    $$PWD/src/Swarm/MissionHandoffManager.h \
    $$PWD/src/Swarm/FleetIntentTask.h \
    $$PWD/src/Swarm/FleetRiskRadar.h \
    $$PWD/src/Swarm/SwarmMissionOrchestrator.h \
    $$PWD/src/Swarm/VehicleCapability.h \
    $$PWD/src/Swarm/SwarmController.h \
    $$PWD/src/Swarm/FleetEventBlackBox.h \
    $$PWD/src/Swarm/FleetArchiveDirectoryInventory.h \
    $$PWD/src/Swarm/FleetEventArchiveCodec.h \
    $$PWD/src/Swarm/FleetEventArchiveWorker.h \
    $$PWD/src/Swarm/FleetEventArchiveSql.h \
    $$PWD/src/Swarm/FleetEventArchiveHistoryReader.h \
    $$PWD/src/Swarm/FleetArchiveTaskIndex.h \
    $$PWD/src/Swarm/FleetEventArchiveMaintenanceWorker.h \
    $$PWD/src/Swarm/FleetPersistentEventStore.h \
    $$PWD/src/Swarm/FleetTelemetryArchiveCodec.h \
    $$PWD/src/Swarm/FleetTelemetryArchiveWorker.h \
    $$PWD/src/Swarm/FleetTelemetryArchiveHistoryReader.h \
    $$PWD/src/Swarm/FleetTelemetryArchive.h \
    $$PWD/src/Swarm/FleetTelemetryMotionReview.h \
    $$PWD/src/Swarm/FleetTelemetryGnssReview.h \
    $$PWD/src/Swarm/FleetTimelineReplay.h \
    $$PWD/src/Swarm/FleetMissionSimulator.h \
    $$PWD/src/Swarm/FleetMissionDigitalTwin.h \
    $$PWD/src/Swarm/FleetMissionPreviewTimeline.h \
    $$PWD/src/Swarm/FleetMissionEvidenceChain.h \
    $$PWD/src/Swarm/FleetMissionContinuityPlanner.h \
    $$PWD/src/Swarm/FleetOperationalMemory.h \
    $$PWD/src/Swarm/FleetMissionReviewCoordinator.h

INCLUDEPATH += \
    $$PWD/src \
    $$PWD/src/Ai \
    $$PWD/src/Diagnostics \
    $$PWD/src/Review \
    $$PWD/src/Swarm \
    $$PWD/src/FirmwarePlugin \
    $$PWD/src/AutoPilotPlugin

# Keep the custom Firmware/AutoPilot plugin factory in the target.  These
# files are part of the existing custom-build contract (see custom-example)
# and are required for the custom PX4 vehicle/plugin selection path.  They
# were previously present on disk but omitted from this project's source
# lists, which makes the registration depend on an unrelated build artifact.
HEADERS += \
    $$PWD/src/AutoPilotPlugin/CustomAutoPilotPlugin.h \
    $$PWD/src/FirmwarePlugin/CustomFirmwarePlugin.h \
    $$PWD/src/FirmwarePlugin/CustomFirmwarePluginFactory.h

SOURCES += \
    $$PWD/src/AutoPilotPlugin/CustomAutoPilotPlugin.cc \
    $$PWD/src/FirmwarePlugin/CustomFirmwarePlugin.cc \
    $$PWD/src/FirmwarePlugin/CustomFirmwarePluginFactory.cc

# Keep MSVC builds from failing on non-ASCII comments in upstream/source files.
win32 {
    # Explicit future local authentication; this source edit does not invoke it.
    LIBS += -lcredui -ladvapi32 -lnetapi32 -lole32 -lwtsapi32
}

win32-msvc {
    QMAKE_CXXFLAGS += /utf-8 /wd4819
    QMAKE_CFLAGS += /utf-8 /wd4819
    QMAKE_CXXFLAGS_WARN_ON -= /WX
    QMAKE_CFLAGS_WARN_ON -= /WX
    QMAKE_CXXFLAGS -= /WX
    QMAKE_CFLAGS -= /WX
    QMAKE_CXXFLAGS += /Zm500
    QMAKE_CFLAGS += /Zm500
}
