#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include "sys_ios.h"

typedef enum { IOS_CLASS_A12, IOS_CLASS_A14, IOS_CLASS_A17 } iosClass_t;	// Apple5/6, Apple7/8, Apple9+

static iosClass_t Sys_IOS_Class( void ) {
	id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
	if ( [dev supportsFamily:MTLGPUFamilyApple9] ) return IOS_CLASS_A17;
	if ( [dev supportsFamily:MTLGPUFamilyApple7] ) return IOS_CLASS_A14;
	return IOS_CLASS_A12;
}

// The 540 px floor is applied by the caller (GLimp_IOS_RenderScale).
float Sys_IOS_DefaultRenderScale( qboolean highRefresh ) {
	float s = 1.0f;
	switch ( Sys_IOS_Class() ) {
	case IOS_CLASS_A12: s = 0.6f; break;
	case IOS_CLASS_A14: s = 0.75f; break;
	case IOS_CLASS_A17: s = 1.0f; break;
	}
	if ( highRefresh ) s *= 0.8f;
	return s < 0.5f ? 0.5f : s > 1.0f ? 1.0f : s;
}

qboolean Sys_IOS_IsPhone( void ) {
	return UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPhone ? qtrue : qfalse;
}

const char *Sys_IOS_DeviceDefaults( void ) {
	static char text[512];
	text[0] = '\0';
	// 32-bit: 16-bit gives 4444 alpha banding; opaque textures upload as RGBA8 (T0c.2b)
	Q_strcat( text, sizeof( text ), "seta r_texturebits 32\n" );
	if ( [NSProcessInfo processInfo].physicalMemory <= 3500ull * 1024 * 1024 )
		Q_strcat( text, sizeof( text ), "seta r_picmip 1\n" );
	if ( Sys_IOS_Class() == IOS_CLASS_A12 )
		Q_strcat( text, sizeof( text ), "seta r_surfaceSprites 0\n" );
	if ( Sys_IOS_MaxFramesPerSecond() >= 120 )
		Q_strcat( text, sizeof( text ), "seta com_maxfps 120\n" );
	if ( Sys_IOS_IsPhone() )
		Q_strcat( text, sizeof( text ), "seta cg_thirdPersonRange 130\nseta cg_fovViewmodel 100\nseta cg_gunX 5\n" );
	return text;
}
