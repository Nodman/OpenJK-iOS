#include "sys_ios.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "qcommon/qcommon.h"
#import <UIKit/UIKit.h>
#import <GameController/GameController.h>

static UIView *Sys_IOS_View( SDL_Window *window ) {
	if ( void *metal = GLimp_IOS_MetalView( window ) ) return (__bridge UIView *)metal;
	SDL_SysWMinfo info;
	SDL_VERSION( &info.version );
	if ( !window || !SDL_GetWindowWMInfo( window, &info ) || info.subsystem != SDL_SYSWM_UIKIT )
		return nil;
	return info.info.uikit.window.rootViewController.view;
}

void Sys_IOS_GetDrawableSize( SDL_Window *window, int *width, int *height ) {
	if ( GLimp_IOS_MetalView( window ) ) SDL_Metal_GetDrawableSize( window, width, height );
	else SDL_GL_GetDrawableSize( window, width, height );
}

void Sys_IOS_ConfigureMetalView( SDL_Window *window, int drawables ) {
	UIView *view = Sys_IOS_View( window );
	CAMetalLayer *layer = (CAMetalLayer *)view.layer;
	layer.opaque = YES;
	layer.maximumDrawableCount = drawables;
	Sys_IOS_SetDrawableScale( window, 1.0f );
}

float Sys_IOS_GetDrawableScale( SDL_Window *window ) {
	return (float)Sys_IOS_View( window ).contentScaleFactor;
}

int Sys_IOS_MetalDrawableCount( SDL_Window *window ) {
	if ( !GLimp_IOS_MetalView( window ) ) return 0;
	CAMetalLayer *layer = (CAMetalLayer *)Sys_IOS_View( window ).layer;
	return (int)layer.maximumDrawableCount;
}

void Sys_IOS_SetDrawableScale( SDL_Window *window, float scale ) {
	UIView *view = Sys_IOS_View( window );
	if ( !view ) return;
	CGFloat target = Sys_IOS_GetNativeScale( window ) * scale;
	bool metal = GLimp_IOS_MetalView( window ) != nullptr;
	if ( fabs( view.contentScaleFactor - target ) < 0.001 &&
		( !metal || fabs( view.layer.contentsScale - target ) < 0.001 ) ) return;
	view.contentScaleFactor = target;
	if ( metal ) view.layer.contentsScale = target;
	[view setNeedsLayout];
	[view layoutIfNeeded];
}

void Sys_IOS_LogView( SDL_Window *window ) {
	UIView *view = Sys_IOS_View( window );
	if ( !view ) return;
	Com_Printf( "iOS view: bounds=%.3fx%.3f nativeScale=%.3f viewScale=%.3f layerScale=%.3f\n",
		(float)view.bounds.size.width, (float)view.bounds.size.height, Sys_IOS_GetNativeScale( window ),
		(float)view.contentScaleFactor, (float)view.layer.contentsScale );
	if ( GLimp_IOS_MetalView( window ) ) {
		CAMetalLayer *layer = (CAMetalLayer *)view.layer;
		Com_Printf( "iOS Metal: opaque=%d framebufferOnly=%d drawables=%d drawable=%.0fx%.0f\n",
			(int)layer.opaque, (int)layer.framebufferOnly, (int)layer.maximumDrawableCount,
			(double)layer.drawableSize.width, (double)layer.drawableSize.height );
	}
}

void Sys_IOS_GetViewPointSize( SDL_Window *window, float *width, float *height ) {
	UIView *view = Sys_IOS_View( window );
	*width = view ? view.bounds.size.width : 0.0f;
	*height = view ? view.bounds.size.height : 0.0f;
}

touchLayoutProfile_t Sys_IOS_TouchLayoutProfile( SDL_Window *window ) {
	UIView *view = Sys_IOS_View( window );
	return view && view.traitCollection.userInterfaceIdiom == UIUserInterfaceIdiomPhone ? TOUCH_LAYOUT_PHONE : TOUCH_LAYOUT_TABLET;
}

float Sys_IOS_GetNativeScale( SDL_Window *window ) {
	UIView *view = Sys_IOS_View( window );
	return view.window ? (float)view.window.screen.nativeScale : 1.0f;
}

void Sys_IOS_GetSafeAreaPixels( SDL_Window *window, float *left, float *top, float *right, float *bottom ) {
	*left = *top = *right = *bottom = 0.0f;
	UIView *view = Sys_IOS_View( window );
	if ( !view || !view.window || view.bounds.size.width <= 0.0 || view.bounds.size.height <= 0.0 )
		return;
	UIEdgeInsets in = view.safeAreaInsets;
	CGFloat s = view.contentScaleFactor;
	int width, height;
	Sys_IOS_GetDrawableSize( window, &width, &height );
	*left = fmax( round( in.left * s ), 0.0 );
	*top = fmax( round( in.top * s ), 0.0 );
	*right = fmax( round( in.right * s ), 0.0 );
	*bottom = fmax( round( in.bottom * s ), 0.0 );
	if ( *left + *right >= width )
		*left = *right = 0.0f;
	if ( *top + *bottom >= height )
		*top = *bottom = 0.0f;
}

qboolean Sys_IOS_HardwareKeyboardConnected( void ) {
	return [GCKeyboard coalescedKeyboard] != nil ? qtrue : qfalse;
}

qboolean Sys_IOS_PointerConnected( void ) {
	return [GCMouse mice].count > 0 ? qtrue : qfalse;
}
