#include "sys_ios_egl.h"
#include "qcommon/qcommon.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglext_angle.h>
#include <dlfcn.h>
#import <Foundation/Foundation.h>

static void *eglLibrary, *glesLibrary;
static EGLDisplay display = EGL_NO_DISPLAY;
static EGLConfig config;
static EGLContext context = EGL_NO_CONTEXT;
static EGLSurface surface = EGL_NO_SURFACE;
static bool initialized;

#define EGL_FUNCTIONS(X) \
	X(eglGetProcAddress) X(eglGetPlatformDisplay) X(eglGetError) X(eglInitialize) X(eglTerminate) \
	X(eglBindAPI) X(eglChooseConfig) X(eglGetConfigAttrib) X(eglCreateContext) \
	X(eglCreateWindowSurface) X(eglMakeCurrent) X(eglSwapBuffers) \
	X(eglQuerySurface) X(eglQueryString) X(eglDestroySurface) X(eglDestroyContext)
#define EGL_DECLARE(name) static decltype(&name) p_##name;
EGL_FUNCTIONS(EGL_DECLARE)
#undef EGL_DECLARE

void Sys_IOS_EGL_AbortInit( void ) {
	if ( initialized ) {
		p_eglMakeCurrent( display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
		if ( surface != EGL_NO_SURFACE ) p_eglDestroySurface( display, surface );
		if ( context != EGL_NO_CONTEXT ) p_eglDestroyContext( display, context );
		p_eglTerminate( display );
	}

	surface = EGL_NO_SURFACE;
	context = EGL_NO_CONTEXT;
	display = EGL_NO_DISPLAY;
	config = nullptr;
	initialized = false;
	if ( eglLibrary ) dlclose( eglLibrary );
	if ( glesLibrary ) dlclose( glesLibrary );
	eglLibrary = glesLibrary = nullptr;
#define EGL_CLEAR(name) p_##name = nullptr;
	EGL_FUNCTIONS(EGL_CLEAR)
#undef EGL_CLEAR
}

static bool EGL_InitFailed( const char *stage ) {
	Com_Printf( "iOS GLES: requested=angle failed stage=%s egl_error=0x%04x fallback=eagl\n",
		stage, p_eglGetError ? p_eglGetError() : EGL_SUCCESS );
	Sys_IOS_EGL_AbortInit();
	return false;
}

static void *EGL_LoadFramework( NSString *name ) {
	NSString *path = [[NSBundle mainBundle].privateFrameworksPath stringByAppendingPathComponent:
		[NSString stringWithFormat:@"%@.framework/%@", name, name]];
	if ( !path.isAbsolutePath ) return nullptr;
	void *handle = dlopen( path.fileSystemRepresentation, RTLD_NOW | RTLD_LOCAL );
	if ( !handle ) Com_Printf( "iOS ANGLE: dlopen %s: %s\n", path.fileSystemRepresentation, dlerror() );
	return handle;
}

static bool EGL_HasExtension( const char *extension ) {
	const char *list = p_eglQueryString( display, EGL_EXTENSIONS );
	size_t len = strlen( extension );
	for ( const char *p = list; p && ( p = strstr( p, extension ) ); p += len ) {
		if ( ( p == list || p[-1] == ' ' ) && ( p[len] == ' ' || p[len] == '\0' ) ) return true;
	}
	return false;
}

bool Sys_IOS_EGL_Probe( void ) {
	glesLibrary = EGL_LoadFramework( @"libGLESv2" );
	if ( !glesLibrary ) return EGL_InitFailed( "load-libGLESv2" );
	eglLibrary = EGL_LoadFramework( @"libEGL" );
	if ( !eglLibrary ) return EGL_InitFailed( "load-libEGL" );
#define EGL_RESOLVE(name) \
	p_##name = reinterpret_cast<decltype(p_##name)>( dlsym( eglLibrary, #name ) ); \
	if ( !p_##name ) return EGL_InitFailed( #name );
	EGL_FUNCTIONS(EGL_RESOLVE)
#undef EGL_RESOLVE
	const EGLAttrib platform[] = { EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE, EGL_NONE };
	display = p_eglGetPlatformDisplay( EGL_PLATFORM_ANGLE_ANGLE, nullptr, platform );
	if ( display == EGL_NO_DISPLAY ) return EGL_InitFailed( "display" );
	EGLint major, minor;
	if ( !p_eglInitialize( display, &major, &minor ) ) return EGL_InitFailed( "initialize" );
	initialized = true;
	if ( !EGL_HasExtension( "EGL_ANGLE_create_context_backwards_compatible" ) )
		return EGL_InitFailed( "missing-EGL_ANGLE_create_context_backwards_compatible" );
	if ( !p_eglBindAPI( EGL_OPENGL_ES_API ) ) return EGL_InitFailed( "bind-api" );
	const EGLint attributes[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8, EGL_SAMPLE_BUFFERS, 0, EGL_SAMPLES, 0, EGL_NONE
	};
	EGLint count = 0;
	if ( !p_eglChooseConfig( display, attributes, &config, 1, &count ) || count != 1 )
		return EGL_InitFailed( "config" );
	const EGLint bits[] = { EGL_RED_SIZE, EGL_GREEN_SIZE, EGL_BLUE_SIZE, EGL_ALPHA_SIZE,
		EGL_DEPTH_SIZE, EGL_STENCIL_SIZE, EGL_SAMPLE_BUFFERS, EGL_SAMPLES };
	const EGLint expected[] = { 8, 8, 8, 8, 24, 8, 0, 0 };
	for ( unsigned i = 0; i < sizeof(bits) / sizeof(bits[0]); ++i ) {
		EGLint value = -1;
		if ( !p_eglGetConfigAttrib( display, config, bits[i], &value ) || value != expected[i] )
			return EGL_InitFailed( "config-bits" );
	}
	Com_Printf( "iOS EGL: version=%d.%d vendor=%s version_string=%s APIs=%s config=RGBA8888/D24/S8 samples=0\n",
		major, minor, p_eglQueryString( display, EGL_VENDOR ), p_eglQueryString( display, EGL_VERSION ),
		p_eglQueryString( display, EGL_CLIENT_APIS ) );
	Com_Printf( "iOS EGL extensions: %s\n", p_eglQueryString( display, EGL_EXTENSIONS ) );
	return true;
}

void *Sys_IOS_EGL_GetProcAddress( const char *name ) {
	return p_eglGetProcAddress ? reinterpret_cast<void *>( p_eglGetProcAddress( name ) ) : nullptr;
}

bool Sys_IOS_EGL_Attach( void *metalLayer, float nativeScale ) {
	const EGLint attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_CONTEXT_OPENGL_BACKWARDS_COMPATIBLE_ANGLE, EGL_FALSE, EGL_NONE };
	context = p_eglCreateContext( display, config, EGL_NO_CONTEXT, attributes );
	if ( context == EGL_NO_CONTEXT ) return EGL_InitFailed( "context" );
	surface = p_eglCreateWindowSurface( display, config, (EGLNativeWindowType)metalLayer, nullptr );
	if ( surface == EGL_NO_SURFACE ) return EGL_InitFailed( "surface" );
	if ( !p_eglMakeCurrent( display, surface, surface, context ) ) return EGL_InitFailed( "make-current" );
	EGLint width = 0, height = 0;
	if ( !p_eglQuerySurface( display, surface, EGL_WIDTH, &width ) ||
		!p_eglQuerySurface( display, surface, EGL_HEIGHT, &height ) || width <= 0 || height <= 0 )
		return EGL_InitFailed( "surface-size" );
	auto bindFramebuffer = reinterpret_cast<void (*)(unsigned, unsigned)>( Sys_IOS_EGL_GetProcAddress( "glBindFramebuffer" ) );
	auto getString = reinterpret_cast<const unsigned char *(*)(unsigned)>( Sys_IOS_EGL_GetProcAddress( "glGetString" ) );
	auto getError = reinterpret_cast<unsigned (*)(void)>( Sys_IOS_EGL_GetProcAddress( "glGetError" ) );
	if ( !bindFramebuffer || !getString || !getError || !Sys_IOS_EGL_GetProcAddress( "glFinish" ) )
		return EGL_InitFailed( "native-procs" );
	bindFramebuffer( 0x8D40, 0 );
	const char *vendor = (const char *)getString( 0x1F00 );
	const char *renderer = (const char *)getString( 0x1F01 );
	const char *version = (const char *)getString( 0x1F02 );
	unsigned error = getError();
	Com_Printf( "iOS ANGLE native: vendor=%s renderer=%s version=%s nativeScale=%.3f gl_error=0x%04x egl_error=0x%04x\n",
		vendor ? vendor : "null", renderer ? renderer : "null", version ? version : "null",
		nativeScale, error, p_eglGetError() );
	int glMajor = 0, glMinor = 0;
	if ( !vendor || !renderer || !version ||
		sscanf( version, "OpenGL ES %d.%d", &glMajor, &glMinor ) != 2 ||
		glMajor != 2 || glMinor != 0 || error ) return EGL_InitFailed( "native-context" );
	Com_Printf( "iOS ANGLE native extensions: %s\n", (const char *)getString( 0x1F03 ) );
	return true;
}

bool Sys_IOS_EGL_MakeCurrent( void ) {
	if ( context == EGL_NO_CONTEXT ) return false;
	if ( p_eglMakeCurrent( display, surface, surface, context ) ) return true;
	Com_Printf( "iOS ANGLE: eglMakeCurrent failed egl_error=0x%04x; relaunch required\n", p_eglGetError() );
	return false;
}

bool Sys_IOS_EGL_Present( void ) {
	if ( p_eglSwapBuffers( display, surface ) ) return true;
	Com_Printf( "iOS ANGLE: eglSwapBuffers failed egl_error=0x%04x; relaunch required\n", p_eglGetError() );
	return false;
}

void Sys_IOS_EGL_CheckSize( int width, int height, bool afterSwap ) {
	EGLint eglWidth = 0, eglHeight = 0;
	if ( !p_eglQuerySurface( display, surface, EGL_WIDTH, &eglWidth ) ||
		!p_eglQuerySurface( display, surface, EGL_HEIGHT, &eglHeight ) ) {
		Com_Printf( "iOS EGL: query size failed egl_error=0x%04x\n", p_eglGetError() );
		Com_Error( ERR_FATAL, "ANGLE surface query failed; relaunch required" );
		return;
	}
	Com_Printf( "iOS EGL: SDL=%dx%d EGL=%dx%d match=%d\n", width, height, eglWidth, eglHeight,
		width == eglWidth && height == eglHeight );
	if ( afterSwap && ( width != eglWidth || height != eglHeight ) )
		Com_Error( ERR_FATAL, "ANGLE drawable size mismatch after swap; relaunch required" );
}
