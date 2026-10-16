#include "sys_ios.h"
#include <SDL.h>
#include <SDL_syswm.h>
#import <UIKit/UIKit.h>
#import <GameController/GameController.h>

// SDL's GL view is the root view controller's view after SDL_GL_CreateContext.
static UIView *Sys_IOS_GLView( SDL_Window *window ) {
	SDL_SysWMinfo info;
	SDL_VERSION( &info.version );
	if ( !window || !SDL_GetWindowWMInfo( window, &info ) || info.subsystem != SDL_SYSWM_UIKIT )
		return nil;
	return info.info.uikit.window.rootViewController.view;
}

void Sys_IOS_SetDrawableScale( SDL_Window *window, float scale ) {
	UIView *view = Sys_IOS_GLView( window );
	if ( !view )
		return;
	CGFloat target = view.window.screen.nativeScale * scale;
	if ( fabs( view.contentScaleFactor - target ) < 0.001 )
		return;
	view.contentScaleFactor = target;
	[view setNeedsLayout];
	[view layoutIfNeeded];	// SDL_uikitopenglview reallocates its renderbuffer here
}

void Sys_IOS_GetViewPointSize( SDL_Window *window, float *width, float *height ) {
	UIView *view = Sys_IOS_GLView( window );
	*width = view ? view.bounds.size.width : 0.0f;
	*height = view ? view.bounds.size.height : 0.0f;
}

touchLayoutProfile_t Sys_IOS_TouchLayoutProfile( SDL_Window *window ) {
	UIView *view = Sys_IOS_GLView( window );
	return view && view.traitCollection.userInterfaceIdiom == UIUserInterfaceIdiomPhone ? TOUCH_LAYOUT_PHONE : TOUCH_LAYOUT_TABLET;
}

float Sys_IOS_GetNativeScale( SDL_Window *window ) {
	UIView *view = Sys_IOS_GLView( window );
	return view ? (float)view.window.screen.nativeScale : 1.0f;
}

void Sys_IOS_GetSafeAreaPixels( SDL_Window *window, float *left, float *top, float *right, float *bottom ) {
	*left = *top = *right = *bottom = 0.0f;
	UIView *view = Sys_IOS_GLView( window );
	if ( !view || !view.window || view.bounds.size.width <= 0.0 || view.bounds.size.height <= 0.0 )
		return;
	UIEdgeInsets in = view.safeAreaInsets;
	CGFloat s = view.contentScaleFactor;
	int width = (int)( view.bounds.size.width * s );
	int height = (int)( view.bounds.size.height * s );
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
