add_subdirectory(APM)

add_library(FirmwarePlugin
	CameraMetaData.cc
	FirmwarePlugin.cc
	FirmwarePluginManager.cc

	APM/APMFirmwarePlugin.cc
	APM/APMFirmwarePluginFactory.cc
	APM/APMParameterMetaData.cc
	APM/ArduCopterFirmwarePlugin.cc
	APM/ArduPlaneFirmwarePlugin.cc
	APM/ArduRoverFirmwarePlugin.cc
	APM/ArduSubFirmwarePlugin.cc
	APM/APMResources.qrc

	PX4/PX4FirmwarePlugin.cc
	PX4/PX4FirmwarePluginFactory.cc
	PX4/PX4ParameterMetaData.cc
	PX4/PX4Resources.qrc
)

target_link_libraries(FirmwarePlugin
	PRIVATE
		qgc
)

# This target unconditionally includes APM. Export the same capability gate
# as qmake's APM block so all consumers (including header users) agree.
target_compile_definitions(FirmwarePlugin PUBLIC MERIVUS_HAS_APM_CONFIGURATION_OBSERVATION)

target_include_directories(FirmwarePlugin
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
                APM
	)

