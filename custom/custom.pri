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
    $$PWD/src/Swarm/CommandTransaction.cc \
    $$PWD/src/Swarm/FaultToleranceManager.cc \
    $$PWD/src/Swarm/FormationPlanner.cc \
    $$PWD/src/Swarm/FleetRegistry.cc \
    $$PWD/src/Swarm/SwarmMissionOrchestrator.cc \
    $$PWD/src/Swarm/VehicleCapability.cc \
    $$PWD/src/Swarm/SwarmController.cc

HEADERS += \
    $$PWD/src/Ai/ActionProposal.h \
    $$PWD/src/Ai/AiAgentClient.h \
    $$PWD/src/Ai/AiAuditEvent.h \
    $$PWD/src/Ai/AiCommandPolicy.h \
    $$PWD/src/Ai/AiSchemaValidator.h \
    $$PWD/src/Ai/AiServiceSupervisor.h \
    $$PWD/src/CustomPlugin.h \
    $$PWD/src/Diagnostics/MerivusLinkDiagnostics.h \
    $$PWD/src/Swarm/CommandTransaction.h \
    $$PWD/src/Swarm/FaultToleranceManager.h \
    $$PWD/src/Swarm/FormationPlanner.h \
    $$PWD/src/Swarm/FleetRegistry.h \
    $$PWD/src/Swarm/SwarmMissionOrchestrator.h \
    $$PWD/src/Swarm/VehicleCapability.h \
    $$PWD/src/Swarm/SwarmController.h

INCLUDEPATH += \
    $$PWD/src \
    $$PWD/src/Ai \
    $$PWD/src/Diagnostics \
    $$PWD/src/Swarm

# Keep MSVC builds from failing on non-ASCII comments in upstream/source files.
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
