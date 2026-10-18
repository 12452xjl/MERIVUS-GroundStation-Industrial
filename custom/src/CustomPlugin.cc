/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 *   @brief Custom QGCCorePlugin Implementation
 *   @author Gus Grubba <gus@auterion.com>
 */

#include <QtQml>
#include <QQmlEngine>
#include <QDateTime>
#include "QGCSettings.h"
#include "MAVLinkLogManager.h"

#include "CustomPlugin.h"

#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppMessages.h"
#include "QmlComponentInfo.h"
#include "QGCPalette.h"
#include "AiAgentClient.h"
#include "AiServiceSupervisor.h"
#include "Diagnostics/MerivusLinkDiagnostics.h"
#include "Review/ReviewRangeController.h"
#include "Review/ReviewVideoController.h"
#include "CommandTransaction.h"
#include "FaultToleranceManager.h"
#include "FormationPlanner.h"
#include "FleetExtensionRegistry.h"
#include "FleetAutonomyPolicy.h"
#include "FleetCapabilityMatcher.h"
#include "FleetRegistry.h"
#include "FleetHealthCenter.h"
#include "FleetCapabilityDiscovery.h"
#include "FleetEventRuleEngine.h"
#include "FleetAirspaceConflictChecker.h"
#include "FleetMissionCompiler.h"
#include "FleetMissionPlanStore.h"
#include "FleetMissionDraft.h"
#include "FleetMissionDraftStore.h"
#include "FleetMissionDeployment.h"
#include "FleetMissionDeploymentLifecycle.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetMissionStartReview.h"
#include "FleetRoleAllocator.h"
#include "FleetEnergyScheduler.h"
#include "FleetMaintenancePassport.h"
#include "FleetLinkDegradationPolicy.h"
#include "FleetMissionRiskEvaluator.h"
#include "FleetMissionReportBuilder.h"
#include "FleetMissionExecutionReport.h"
#include "FleetMissionRemainderArchive.h"
#include "FleetRolePolicy.h"
#include "FleetOperatorSession.h"
#include "FleetOperationApproval.h"
#include "FleetMissionEvidenceRefresh.h"
#include "FleetMissionStartContentCheck.h"
#include "FleetMissionStartApproval.h"
#include "FleetMissionStartupAuthority.h"
#include "FleetMissionStartupTransaction.h"
#include "FleetMissionArmedGroundReview.h"
#include "FleetMissionGroundStartApproval.h"
#include "FleetMissionGroundStartupAuthority.h"
#include "FleetMissionStartupSeries.h"
#include "FleetMissionObservationClose.h"
#include "FleetTaskTemplateRegistry.h"
#include "FleetUserTaskTemplates.h"
#include "FleetMissionReportWorkspace.h"
#include "MissionHandoffManager.h"
#include "SwarmMissionOrchestrator.h"
#include "SwarmController.h"
#include "FleetEventBlackBox.h"
#include "FleetPersistentEventStore.h"
#include "FleetTelemetryArchive.h"
#include "FleetTimelineReplay.h"
#include "FleetMissionSimulator.h"
#include "FleetMissionDigitalTwin.h"
#include "FleetMissionEvidenceChain.h"
#include "FleetMissionContinuityPlanner.h"
#include "FleetMissionReviewCoordinator.h"
#include "FleetOperationalMemory.h"
#include "FleetIntentTask.h"
#include "FleetRiskRadar.h"

QGC_LOGGING_CATEGORY(CustomLog, "CustomLog")

CustomFlyViewOptions::CustomFlyViewOptions(CustomOptions* options, QObject* parent)
    : QGCFlyViewOptions(options, parent)
{

}

// MERIVUS uses the QGroundControl multi-vehicle model as the source of truth
// for fleet discovery and batch operations. Keep the standard vehicle list
// visible so operators can inspect and select individual members.
bool CustomFlyViewOptions::showMultiVehicleList(void) const
{
    return true;
}

// This custom build has it's own custom instrument panel. Don't show regular one.
bool CustomFlyViewOptions::showInstrumentPanel(void) const
{
    return false;
}

CustomOptions::CustomOptions(CustomPlugin*, QObject* parent)
    : QGCOptions(parent)
{
}

QGCFlyViewOptions* CustomOptions::flyViewOptions(void)
{
    if (!_flyViewOptions) {
        _flyViewOptions = new CustomFlyViewOptions(this, this);
    }
    return _flyViewOptions;
}

// Firmware recovery must remain available even when no vehicle is connected.
bool CustomOptions::showFirmwareUpgrade() const
{
    return true;
}

// Normal QGC needs to work with an ESP8266 WiFi thing which is remarkably crappy. This in turns causes PX4 Pro calibration to fail
// quite often. There is a warning in regular QGC about this. Overriding the and returning true means that your custom vehicle has
// a reliable WiFi connection so don't show that warning.
bool CustomOptions::wifiReliableForCalibration(void) const
{
    return true;
}

CustomPlugin::CustomPlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin(app, toolbox)
{
    qmlRegisterType<AiAgentClient>("Merivus", 1, 0, "AiAgentClient");
    qmlRegisterType<AiServiceSupervisor>("Merivus", 1, 0, "AiServiceSupervisor");
    qmlRegisterType<MerivusLinkDiagnostics>("Merivus", 1, 0, "MerivusLinkDiagnostics");
    qmlRegisterType<ReviewRangeController>("Merivus", 1, 0, "ReviewRangeController");
    qmlRegisterType<ReviewVideoController>("Merivus", 1, 0, "ReviewVideoController");
    // Mutable command/mission/health services are registered in a separate
    // internal URI. GuidedActionsController is the only QML owner that
    // imports this URI; public panels consume its read-only facades instead
    // of being able to instantiate a second command controller or overwrite
    // registry/approval state. This is a module boundary for the QML API,
    // not a substitute for C++ authentication or process isolation.
    qmlRegisterType<CommandTransaction>("MerivusInternal", 1, 0, "CommandTransaction");
    qmlRegisterType<FaultToleranceManager>("MerivusInternal", 1, 0, "FaultToleranceManager");
    qmlRegisterType<FormationPlanner>("MerivusInternal", 1, 0, "FormationPlanner");
    qmlRegisterType<FleetExtensionRegistry>("MerivusInternal", 1, 0, "FleetExtensionRegistry");
    qmlRegisterType<FleetAutonomyPolicy>("MerivusInternal", 1, 0, "FleetAutonomyPolicy");
    qmlRegisterType<FleetCapabilityMatcher>("MerivusInternal", 1, 0, "FleetCapabilityMatcher");
    qmlRegisterType<FleetRegistry>("MerivusInternal", 1, 0, "FleetRegistry");
    qmlRegisterType<FleetHealthCenter>("MerivusInternal", 1, 0, "FleetHealthCenter");
    qmlRegisterType<FleetCapabilityDiscovery>("MerivusInternal", 1, 0, "FleetCapabilityDiscovery");
    qmlRegisterType<FleetEventRuleEngine>("MerivusInternal", 1, 0, "FleetEventRuleEngine");
    qmlRegisterType<FleetAirspaceConflictChecker>("MerivusInternal", 1, 0, "FleetAirspaceConflictChecker");
    qmlRegisterType<FleetMissionCompiler>("MerivusInternal", 1, 0, "FleetMissionCompiler");
    qmlRegisterType<FleetMissionPlanStore>("MerivusInternal", 1, 0, "FleetMissionPlanStore");
    qmlRegisterType<FleetMissionDraft>("MerivusInternal", 1, 0, "FleetMissionDraft");
    qmlRegisterType<FleetMissionDraftStore>("MerivusInternal", 1, 0, "FleetMissionDraftStore");
    qmlRegisterType<FleetMissionDeployment>("MerivusInternal", 1, 0, "FleetMissionDeployment");
    qmlRegisterType<FleetMissionDeploymentLifecycle>("MerivusInternal", 1, 0, "FleetMissionDeploymentLifecycle");
    qmlRegisterUncreatableType<FleetMissionDeploymentEpisode>("MerivusInternal", 1, 0, "FleetMissionDeploymentEpisode",
        QStringLiteral("Deployment episodes are created and retained by FleetMissionDeploymentLifecycle."));
    qmlRegisterType<FleetMissionStartReview>("MerivusInternal", 1, 0, "FleetMissionStartReview");
    qmlRegisterType<FleetRoleAllocator>("MerivusInternal", 1, 0, "FleetRoleAllocator");
    qmlRegisterType<FleetEnergyScheduler>("MerivusInternal", 1, 0, "FleetEnergyScheduler");
    qmlRegisterType<FleetMaintenancePassport>("MerivusInternal", 1, 0, "FleetMaintenancePassport");
    qmlRegisterType<FleetLinkDegradationPolicy>("MerivusInternal", 1, 0, "FleetLinkDegradationPolicy");
    qmlRegisterType<FleetMissionRiskEvaluator>("MerivusInternal", 1, 0, "FleetMissionRiskEvaluator");
    qmlRegisterType<FleetMissionReportBuilder>("MerivusInternal", 1, 0, "FleetMissionReportBuilder");
    qmlRegisterType<FleetMissionExecutionReport>("MerivusInternal", 1, 0, "FleetMissionExecutionReport");
    qmlRegisterType<FleetMissionRemainderArchive>("MerivusInternal", 1, 0, "FleetMissionRemainderArchive");
    qmlRegisterType<FleetRolePolicy>("MerivusInternal", 1, 0, "FleetRolePolicy");
    qmlRegisterType<FleetOperatorSession>("MerivusInternal", 1, 0, "FleetOperatorSession");
    qmlRegisterType<FleetOperationApproval>("MerivusInternal", 1, 0, "FleetOperationApproval");
    qmlRegisterType<FleetMissionEvidenceRefresh>("MerivusInternal", 1, 0, "FleetMissionEvidenceRefresh");
    qmlRegisterType<FleetMissionStartContentCheck>("MerivusInternal", 1, 0, "FleetMissionStartContentCheck");
    qmlRegisterType<FleetMissionStartApproval>("MerivusInternal", 1, 0, "FleetMissionStartApproval");
    qmlRegisterType<FleetMissionStartupAuthority>("MerivusInternal", 1, 0, "FleetMissionStartupAuthority");
    qmlRegisterType<FleetMissionStartupTransaction>("MerivusInternal", 1, 0, "FleetMissionStartupTransaction");
    qmlRegisterType<FleetMissionArmedGroundReview>("MerivusInternal", 1, 0, "FleetMissionArmedGroundReview");
    qmlRegisterType<FleetMissionGroundStartApproval>("MerivusInternal", 1, 0, "FleetMissionGroundStartApproval");
    qmlRegisterType<FleetMissionGroundStartupAuthority>("MerivusInternal", 1, 0, "FleetMissionGroundStartupAuthority");
    qmlRegisterType<FleetMissionStartupSeries>("MerivusInternal", 1, 0, "FleetMissionStartupSeries");
    qmlRegisterType<FleetMissionObservationClose>("MerivusInternal", 1, 0, "FleetMissionObservationClose");
    qmlRegisterType<FleetTaskTemplateRegistry>("MerivusInternal", 1, 0, "FleetTaskTemplateRegistry");
    qmlRegisterType<FleetUserTaskTemplates>("MerivusInternal", 1, 0, "FleetUserTaskTemplates");
    qmlRegisterType<FleetMissionReportWorkspace>("MerivusInternal", 1, 0, "FleetMissionReportWorkspace");
    qmlRegisterType<MissionHandoffManager>("MerivusInternal", 1, 0, "MissionHandoffManager");
    qmlRegisterType<SwarmMissionOrchestrator>("MerivusInternal", 1, 0, "SwarmMissionOrchestrator");
    qmlRegisterType<SwarmController>("MerivusInternal", 1, 0, "SwarmController");
    qmlRegisterType<FleetEventBlackBox>("MerivusInternal", 1, 0, "FleetEventBlackBox");
    qmlRegisterType<FleetPersistentEventStore>("MerivusInternal", 1, 0, "FleetPersistentEventStore");
    qmlRegisterType<FleetTelemetryArchive>("MerivusInternal", 1, 0, "FleetTelemetryArchive");
    qmlRegisterType<FleetTimelineReplay>("MerivusInternal", 1, 0, "FleetTimelineReplay");
    qmlRegisterType<FleetMissionSimulator>("MerivusInternal", 1, 0, "FleetMissionSimulator");
    qmlRegisterType<FleetMissionDigitalTwin>("MerivusInternal", 1, 0, "FleetMissionDigitalTwin");
    qmlRegisterType<FleetMissionEvidenceChain>("MerivusInternal", 1, 0, "FleetMissionEvidenceChain");
    qmlRegisterType<FleetMissionContinuityPlanner>("MerivusInternal", 1, 0, "FleetMissionContinuityPlanner");
    qmlRegisterType<FleetMissionReviewCoordinator>("MerivusInternal", 1, 0, "FleetMissionReviewCoordinator");
    qmlRegisterType<FleetOperationalMemory>("MerivusInternal", 1, 0, "FleetOperationalMemory");
    qmlRegisterType<FleetIntentTask>("MerivusInternal", 1, 0, "FleetIntentTask");
    qmlRegisterType<FleetRiskRadar>("MerivusInternal", 1, 0, "FleetRiskRadar");
    _options = new CustomOptions(this, this);
    _showAdvancedUI = false;
}

CustomPlugin::~CustomPlugin()
{
}

void CustomPlugin::setToolbox(QGCToolbox* toolbox)
{
    QGCCorePlugin::setToolbox(toolbox);

    // Allows us to be notified when the user goes in/out out advanced mode
    connect(qgcApp()->toolbox()->corePlugin(), &QGCCorePlugin::showAdvancedUIChanged, this, &CustomPlugin::_advancedChanged);
}

void CustomPlugin::_advancedChanged(bool changed)
{
    // Firmware Upgrade page is only show in Advanced mode
    emit _options->showFirmwareUpgradeChanged(changed);
}

//-----------------------------------------------------------------------------
void CustomPlugin::_addSettingsEntry(const QString& title, const char* qmlFile, const char* iconFile)
{
    Q_CHECK_PTR(qmlFile);
    // 'this' instance will take ownership on the QmlComponentInfo instance
    _customSettingsList.append(QVariant::fromValue(
        new QmlComponentInfo(title,
                QUrl::fromUserInput(qmlFile),
                iconFile == nullptr ? QUrl() : QUrl::fromUserInput(iconFile),
                this)));
}

//-----------------------------------------------------------------------------
QVariantList&
CustomPlugin::settingsPages()
{
    if(_customSettingsList.isEmpty()) {
        _addSettingsEntry(tr("General"),     "qrc:/qml/GeneralSettings.qml",     "qrc:/res/gear-white.svg");
        _addSettingsEntry(tr("Comm Links"),  "qrc:/qml/LinkSettings.qml",        "qrc:/res/waves.svg");
        _addSettingsEntry(tr("Offline Maps"),"qrc:/qml/OfflineMap.qml",          "qrc:/res/waves.svg");
        _addSettingsEntry(tr("MAVLink"),     "qrc:/qml/MavlinkSettings.qml",     "qrc:/res/waves.svg");
        _addSettingsEntry(tr("Console"),     "qrc:/qml/QGroundControl/Controls/AppMessages.qml");
#if defined(QT_DEBUG)
        //-- These are always present on Debug builds
        _addSettingsEntry(tr("Mock Link"),   "qrc:/qml/MockLink.qml");
#endif
    }
    return _customSettingsList;
}

QGCOptions* CustomPlugin::options()
{
    return _options;
}

QString CustomPlugin::brandImageIndoor(void) const
{
    return QStringLiteral("/qmlimages/mrws4.png");
}

QString CustomPlugin::brandImageOutdoor(void) const
{
    return QStringLiteral("/qmlimages/mrws4.png");
}

bool CustomPlugin::overrideSettingsGroupVisibility(QString name)
{
    // We have set up our own specific brand imaging. Hide the brand image settings such that the end user
    // can't change it.
    if (name == BrandImageSettings::name) {
        return false;
    }
    return true;
}

// This allows you to override/hide QGC Application settings
bool CustomPlugin::adjustSettingMetaData(const QString& settingsGroup, FactMetaData& metaData)
{
    bool parentResult = QGCCorePlugin::adjustSettingMetaData(settingsGroup, metaData);

    if (settingsGroup == AppSettings::settingsGroup) {
        // This tells QGC than when you are creating Plans while not connected to a vehicle
        // the specific firmware/vehicle the plan is for.
        if (metaData.name() == AppSettings::offlineEditingFirmwareClassName) {
            metaData.setRawDefaultValue(QGCMAVLink::FirmwareClassPX4);
            return false;
        } else if (metaData.name() == AppSettings::offlineEditingVehicleClassName) {
            metaData.setRawDefaultValue(QGCMAVLink::VehicleClassMultiRotor);
            return false;
        }
    }

    return parentResult;
}

// This modifies QGC colors palette to match possible custom corporate branding
void CustomPlugin::paletteOverride(QString colorName, QGCPalette::PaletteColorInfo_t& colorInfo)
{
    const auto setBrandColors = [&colorInfo](const char* darkEnabled,
                                             const char* darkDisabled,
                                             const char* lightEnabled,
                                             const char* lightDisabled) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled] = QColor(darkEnabled);
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled] = QColor(darkDisabled);
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled] = QColor(lightEnabled);
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor(lightDisabled);
    };

    // MERIVUS brand palette: navy surfaces, blue structure and gold actions.
    // Colors not listed here keep QGC's semantic defaults.
    if (colorName == QStringLiteral("window")) {
        setBrandColors("#101827", "#101827", "#f4f7fb", "#eef2f7");
        return;
    } else if (colorName == QStringLiteral("windowShadeLight")) {
        setBrandColors("#26354b", "#202d40", "#e5ebf3", "#dce4ee");
        return;
    } else if (colorName == QStringLiteral("windowShade")) {
        setBrandColors("#1a2638", "#172234", "#dbe3ed", "#d1dae6");
        return;
    } else if (colorName == QStringLiteral("windowShadeDark")) {
        setBrandColors("#0b1220", "#0b1220", "#c8d3e0", "#bcc8d6");
        return;
    } else if (colorName == QStringLiteral("text")) {
        setBrandColors("#f5e9c9", "#7d8798", "#172338", "#8792a3");
        return;
    } else if (colorName == QStringLiteral("button")) {
        setBrandColors("#25344b", "#1c283a", "#ffffff", "#e6ebf1");
        return;
    } else if (colorName == QStringLiteral("buttonText")) {
        setBrandColors("#f4d78a", "#7d8798", "#172338", "#8792a3");
        return;
    } else if (colorName == QStringLiteral("buttonHighlight")) {
        setBrandColors("#d4ad55", "#3b4350", "#e0bc65", "#d8dde5");
        return;
    } else if (colorName == QStringLiteral("buttonHighlightText")) {
        setBrandColors("#101827", "#8f98a7", "#101827", "#7d8798");
        return;
    } else if (colorName == QStringLiteral("primaryButton")) {
        setBrandColors("#d4ad55", "#4b5360", "#c69a3e", "#9ea6b1");
        return;
    } else if (colorName == QStringLiteral("primaryButtonText")) {
        setBrandColors("#101827", "#c8ced7", "#101827", "#eef1f5");
        return;
    } else if (colorName == QStringLiteral("textField")) {
        setBrandColors("#162338", "#202c3d", "#ffffff", "#e8edf3");
        return;
    } else if (colorName == QStringLiteral("textFieldText")) {
        setBrandColors("#ffffff", "#7d8798", "#172338", "#8792a3");
        return;
    } else if (colorName == QStringLiteral("mapButton")) {
        setBrandColors("#101827", "#4b5360", "#172338", "#7d8798");
        return;
    } else if (colorName == QStringLiteral("mapButtonHighlight") ||
               colorName == QStringLiteral("mapIndicator")) {
        setBrandColors("#d4ad55", "#4b5360", "#c69a3e", "#9ea6b1");
        return;
    } else if (colorName == QStringLiteral("mapIndicatorChild")) {
        setBrandColors("#8d6a29", "#4b5360", "#8d6a29", "#9ea6b1");
        return;
    } else if (colorName == QStringLiteral("missionItemEditor")) {
        setBrandColors("#162338", "#0b1220", "#ffffff", "#dbe3ed");
        return;
    } else if (colorName == QStringLiteral("toolbarBackground")) {
        setBrandColors("#0b1220", "#0b1220", "#172338", "#172338");
        return;
    } else if (colorName == QStringLiteral("toolStripFGColor")) {
        setBrandColors("#f4d78a", "#7d8798", "#f4d78a", "#8792a3");
        return;
    } else if (colorName == QStringLiteral("toolStripHoverColor") ||
               colorName == QStringLiteral("hoverColor")) {
        setBrandColors("#d4ad55", "#3b4350", "#e0bc65", "#d8dde5");
        return;
    } else if (colorName == QStringLiteral("mapMissionTrajectory")) {
        setBrandColors("#d4ad55", "#d4ad55", "#c69a3e", "#c69a3e");
        return;
    } else if (colorName == QStringLiteral("brandingPurple")) {
        setBrandColors("#101827", "#101827", "#172338", "#172338");
        return;
    } else if (colorName == QStringLiteral("brandingBlue")) {
        setBrandColors("#d4ad55", "#8d6a29", "#c69a3e", "#8d6a29");
        return;
    } else if (colorName == QStringLiteral("colorBlue")) {
        setBrandColors("#5b9bd5", "#476f96", "#2f6fa7", "#7891a8");
        return;
    } else if (colorName == QStringLiteral("colorOrange")) {
        setBrandColors("#e4bd65", "#9b8148", "#b88424", "#a89670");
        return;
    }

    if (colorName == QStringLiteral("window")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#212529");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#212529");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#f8f9fa");
    }
    else if (colorName == QStringLiteral("windowShade")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#343a40");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#343a40");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#f1f3f5");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#d9d9d9");
    }
    else if (colorName == QStringLiteral("windowShadeDark")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#1a1c1f");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#1a1c1f");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#e9ecef");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#bdbdbd");
    }
    else if (colorName == QStringLiteral("text")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777c89");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#212529");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#9d9d9d");
    }
    else if (colorName == QStringLiteral("warningText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#e03131");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#e03131");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#cc0808");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#cc0808");
    }
    else if (colorName == QStringLiteral("button")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#495057");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#495057");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#ffffff");
    }
    else if (colorName == QStringLiteral("buttonText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777c89");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#212529");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#9d9d9d");
    }
    else if (colorName == QStringLiteral("buttonHighlight")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#07916d");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#495057");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#aeebd0");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#e4e4e4");
    }
    else if (colorName == QStringLiteral("buttonHighlightText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777c89");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#212529");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#2c2c2c");
    }
    else if (colorName == QStringLiteral("primaryButton")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#12b886");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#495057");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#aeebd0");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("primaryButtonText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#212529");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#cad0d0");
    }
    else if (colorName == QStringLiteral("textField")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#212529");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#495057");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#f1f3f5");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#ffffff");
    }
    else if (colorName == QStringLiteral("textFieldText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#777c89");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#212529");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#808080");
    }
    else if (colorName == QStringLiteral("mapButton")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#212529");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("mapButtonHighlight")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#07916d");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#be781c");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("mapIndicator")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#9dda4f");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#be781c");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("mapIndicatorChild")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#527942");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#585858");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#766043");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("colorGreen")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#27bf89");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#0ca678");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#009431");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#009431");
    }
    else if (colorName == QStringLiteral("colorOrange")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#f7b24a");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#f6921e");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#b95604");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#b95604");
    }
    else if (colorName == QStringLiteral("colorRed")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#e1544c");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#e03131");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#ed3939");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#ed3939");
    }
    else if (colorName == QStringLiteral("colorGrey")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#8b90a0");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#8b90a0");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#808080");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#808080");
    }
    else if (colorName == QStringLiteral("colorBlue")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#228be6");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#228be6");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#1a72ff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#1a72ff");
    }
    else if (colorName == QStringLiteral("alertBackground")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#d4b106");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#d4b106");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#fffb8f");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#b45d48");
    }
    else if (colorName == QStringLiteral("alertBorder")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#876800");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#876800");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#808080");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#808080");
    }
    else if (colorName == QStringLiteral("alertText")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#fff9ed");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#212529");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#fff9ed");
    }
    else if (colorName == QStringLiteral("missionItemEditor")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#212529");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#0b1420");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#585858");
    }
    else if (colorName == QStringLiteral("hoverColor")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#07916d");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#33c494");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#aeebd0");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#464f5a");
    }
    else if (colorName == QStringLiteral("mapWidgetBorderLight")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#ffffff");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#ffffff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#f1f3f5");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#ffffff");
    }
    else if (colorName == QStringLiteral("mapWidgetBorderDark")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#000000");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#000000");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#212529");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#000000");
    }
    else if (colorName == QStringLiteral("brandingPurple")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#4a2c6d");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#4a2c6d");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#4a2c6d");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#4a2c6d");
    }
    else if (colorName == QStringLiteral("brandingBlue")) {
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupEnabled]   = QColor("#6045c5");
        colorInfo[QGCPalette::Dark][QGCPalette::ColorGroupDisabled]  = QColor("#48d6ff");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupEnabled]  = QColor("#6045c5");
        colorInfo[QGCPalette::Light][QGCPalette::ColorGroupDisabled] = QColor("#48d6ff");
    }
}

// We override this so we can get access to QQmlApplicationEngine and use it to register our qml module
QQmlApplicationEngine* CustomPlugin::createQmlApplicationEngine(QObject* parent)
{
    QQmlApplicationEngine* qmlEngine = QGCCorePlugin::createQmlApplicationEngine(parent);
    qmlEngine->addImportPath("qrc:/Custom/Widgets");
    return qmlEngine;
}
