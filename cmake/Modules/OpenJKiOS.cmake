if(CMAKE_VERSION VERSION_LESS 3.28)
	message(FATAL_ERROR "iOS builds need CMake 3.28 or newer")
endif()
if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
	message(FATAL_ERROR "iOS builds need the Xcode generator (use: cmake --preset ios)")
endif()
enable_language(OBJC OBJCXX)
set(OPENJK_GLES ON)
include(OpenJKiOSDeps)	# T0a.2: SDL2, gl4es, GL4ES_INCLUDE_DIR

# All iOS-only files live here; globbed so parallel phases never edit this list.
file(GLOB OPENJK_IOS_SOURCES CONFIGURE_DEPENDS
	"${SharedDir}/sys/ios/*.h" "${SharedDir}/sys/ios/*.cpp" "${SharedDir}/sys/ios/*.mm")

function(openjk_ios_configure_app target flavor)
	string(TOLOWER "${flavor}" flavor_lower)
	set_target_properties(${target} PROPERTIES
		OUTPUT_NAME "OpenJK-${flavor}"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/apps/$<CONFIG>$(EFFECTIVE_PLATFORM_NAME)"	# genex stops Xcode adding only $<CONFIG>
		MACOSX_BUNDLE_BUNDLE_NAME "OpenJK ${flavor}"
		MACOSX_BUNDLE_GUI_IDENTIFIER "org.openjk.ios.${flavor_lower}"
		XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.openjk.ios.${flavor_lower}"
		XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
		XCODE_ATTRIBUTE_LLVM_LTO NO
		XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)	# the .mm files are written ARC-style
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks")
	execute_process(COMMAND git rev-list --count HEAD
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE build_number
		OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
	if(NOT build_number)
		set(build_number 1)
	endif()
	set_target_properties(${target} PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/ios/Info.plist.in"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${OPENJK_IOS_VERSION}"
		MACOSX_BUNDLE_BUNDLE_VERSION "${build_number}")
	set(icons "${CMAKE_SOURCE_DIR}/cmake/ios/AppIcon-${flavor}.xcassets")
	target_sources(${target} PRIVATE "${icons}")
	set_source_files_properties("${icons}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
	set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon)
	target_sources(${target} PRIVATE "${CMAKE_SOURCE_DIR}/shared/ios/ios_default.cfg")
	set_source_files_properties("${CMAKE_SOURCE_DIR}/shared/ios/ios_default.cfg"
		PROPERTIES MACOSX_PACKAGE_LOCATION "base")
	target_link_libraries(${target} SDL2::SDL2main SDL2::SDL2-static
		"-framework OpenGLES" "-framework Metal" "-framework GameController"
		"-framework AVFoundation" "-framework UIKit" "-framework QuartzCore")
endfunction()

# Game modules and renderer: embedded dylibs in .app/Frameworks
function(openjk_ios_configure_module target)
	set_target_properties(${target} PROPERTIES
		INSTALL_NAME_DIR "@rpath"
		BUILD_WITH_INSTALL_NAME_DIR ON
		XCODE_ATTRIBUTE_LLVM_LTO NO)
endfunction()
