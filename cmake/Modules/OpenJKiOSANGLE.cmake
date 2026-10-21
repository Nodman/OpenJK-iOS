find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(OPENJK_IOS_ANGLE_LOCAL_ASSETS "" CACHE PATH "Explicit offline ANGLE release fixture (same pinned bytes)")
set(OPENJK_IOS_ANGLE_PREPARED "${CMAKE_BINARY_DIR}/angle-consumer")
set(_angle_consumer "${CMAKE_SOURCE_DIR}/scripts/ios/angle-consumer.py")
set(_angle_args prepare --destination "${OPENJK_IOS_ANGLE_PREPARED}")
if(OPENJK_IOS_ANGLE_LOCAL_ASSETS)
	list(APPEND _angle_args --local-assets "${OPENJK_IOS_ANGLE_LOCAL_ASSETS}")
endif()
execute_process(COMMAND "${Python3_EXECUTABLE}" "${_angle_consumer}" ${_angle_args}
	RESULT_VARIABLE _angle_result OUTPUT_VARIABLE _angle_output ERROR_VARIABLE _angle_error)
if(NOT _angle_result EQUAL 0)
	message(FATAL_ERROR "ANGLE dependency validation failed: ${_angle_output}${_angle_error}")
endif()
set(OPENJK_IOS_ANGLE_FRAMEWORKS
	"${OPENJK_IOS_ANGLE_PREPARED}/Frameworks/libEGL.framework"
	"${OPENJK_IOS_ANGLE_PREPARED}/Frameworks/libGLESv2.framework")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
	"${_angle_consumer}" "${CMAKE_SOURCE_DIR}/scripts/ios/angle-consumer.json")
