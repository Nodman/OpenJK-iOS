include(FetchContent)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(SDL2
	URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz
	URL_HASH SHA256=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165)

set(NOX11 ON CACHE BOOL "" FORCE)
set(NOEGL ON CACHE BOOL "" FORCE)
set(STATICLIB ON CACHE BOOL "" FORCE)
set(DEFAULT_ES 2 CACHE STRING "" FORCE)
# gl4es already sets both on Apple; explicit so init stays manual after a gl4es bump
set(NO_LOADER ON CACHE BOOL "" FORCE)
set(NO_INIT_CONSTRUCTOR ON CACHE BOOL "" FORCE)
set(_gl4es_patch "${CMAKE_CURRENT_LIST_DIR}/../ios/patches/gl4es-apple-build.patch")
set(_gl4es_fbo_patch "${CMAKE_CURRENT_LIST_DIR}/../ios/patches/gl4es-borrowed-fbo.patch")
# the hash is part of the patch step args, so editing a patch re-runs the step
file(SHA256 "${_gl4es_patch}" _gl4es_patch_hash)
file(SHA256 "${_gl4es_fbo_patch}" _gl4es_fbo_patch_hash)
FetchContent_Declare(gl4es
	GIT_REPOSITORY https://github.com/ptitSeb/gl4es.git
	GIT_TAG 201599684ada5bf76059b21be8c69361fa4b891b
	# checkout first so a re-run of the patch step stays clean
	PATCH_COMMAND git checkout -- .
		COMMAND git apply "${_gl4es_patch}"
		COMMAND git apply "${_gl4es_fbo_patch}"
		COMMAND ${CMAKE_COMMAND} -E echo "gl4es patches ${_gl4es_patch_hash} ${_gl4es_fbo_patch_hash}")

FetchContent_MakeAvailable(SDL2 gl4es)
# gl4es writes into ${CMAKE_SOURCE_DIR}/lib by default
set_target_properties(GL PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/gl4es")
target_compile_options(GL PRIVATE -fvisibility=hidden)
set(GL4ES_INCLUDE_DIR "${gl4es_SOURCE_DIR}/include")
