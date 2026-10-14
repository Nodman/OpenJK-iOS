#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>
#include "sys_ios.h"
#include "client/client.h"	// Key_ClearStates via keys.h; same header sdl_input.cpp uses
#include <SDL.h>
#include <time.h>

void SNDDMA_Activate( qboolean activate );	// shared/sdl/sdl_sound.cpp
void Com_WriteConfiguration( void );		// common.cpp; no header prototype

static qboolean inBackground;
static uint64_t backgroundStartNs;

static void Sys_IOS_EnterBackground( void ) {
	if ( inBackground )
		return;
	inBackground = qtrue;
	backgroundStartNs = clock_gettime_nsec_np( CLOCK_MONOTONIC );	// keeps counting while the device sleeps
	Com_Printf( "iOS lifecycle: background\n" );
	Sys_IOS_SetFramePaused( qtrue );	// no GL calls while in background
	// iOS kills apps with GL work in flight in background; gl4es has nothing pending (swap ran qglFlush)
	void ( *nativeFinish )( void ) = (void ( * )( void ))SDL_GL_GetProcAddress( "glFinish" );
	if ( nativeFinish )
		nativeFinish();
	SNDDMA_Activate( qfalse );
	IN_TouchReleaseAll();
	IN_GamepadReleaseAll();	// UIKit cancels touches, not pad buttons
	Key_ClearStates();
	Com_WriteConfiguration();
}

static void Sys_IOS_EnterForeground( void ) {
	if ( !inBackground )
		return;
	inBackground = qfalse;
	Com_Printf( "iOS lifecycle: foreground\n" );
#ifndef _JK2EXE
	// Com_ModifyMsec caps the resume frame at 5 s, so cl_timeout alone would fire ~200 s late
	double away = ( clock_gettime_nsec_np( CLOCK_MONOTONIC ) - backgroundStartNs ) / 1e9;
	if ( cls.state >= CA_CONNECTED && cls.state != CA_CINEMATIC && !clc.demoplaying && !com_sv_running->integer
		&& away > Cvar_VariableValue( "cl_timeout" ) ) {
		Com_Printf( "iOS lifecycle: background %.0f s, disconnecting\n", away );
		Cbuf_ExecuteText( EXEC_INSERT, "disconnect\n" );
	}
#endif
	SNDDMA_Activate( qtrue );
	Sys_IOS_SetFramePaused( qfalse );
}

static const char *Sys_IOS_AudioOutput( void ) {
	AVAudioSessionPortDescription *port = [AVAudioSession sharedInstance].currentRoute.outputs.firstObject;
	return port ? port.portType.UTF8String : "none";
}

// Interruptions stay with SDL's CoreAudio backend (pause on Began, restart on Ended and app-active).
void Sys_IOS_InitLifecycle( void ) {
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	// Not SDL_APP_* events: SDL drops its UIKit lifecycle observer once main() returns to the display-link loop.
	// queue:nil runs the block at once on the main thread, before iOS suspends the app.
	// DidEnterBackground too: no WillResignActive comes if a system alert already made the app inactive
	[center addObserverForName:UIApplicationWillResignActiveNotification object:nil queue:nil
		usingBlock:^( NSNotification * ) { Sys_IOS_EnterBackground(); }];
	[center addObserverForName:UIApplicationDidEnterBackgroundNotification object:nil queue:nil
		usingBlock:^( NSNotification * ) { Sys_IOS_EnterBackground(); }];
	[center addObserverForName:UIApplicationDidBecomeActiveNotification object:nil queue:nil
		usingBlock:^( NSNotification * ) { Sys_IOS_EnterForeground(); }];
	[center addObserverForName:UIApplicationDidReceiveMemoryWarningNotification object:nil queue:nil
		usingBlock:^( NSNotification * ) { Com_Printf( "iOS low memory warning\n" ); }];

	NSOperationQueue *mainQueue = [NSOperationQueue mainQueue];	// audio session posts from other threads
	[center addObserverForName:AVAudioSessionRouteChangeNotification object:nil queue:mainQueue
		usingBlock:^( NSNotification *note ) {
			NSNumber *reason = note.userInfo[AVAudioSessionRouteChangeReasonKey];
			if ( reason.unsignedIntegerValue == AVAudioSessionRouteChangeReasonOldDeviceUnavailable )
				Com_Printf( "Audio route changed, output %s\n", Sys_IOS_AudioOutput() );
		}];
	[center addObserverForName:AVAudioSessionMediaServicesWereResetNotification object:nil queue:mainQueue
		usingBlock:^( NSNotification * ) {
			Com_Printf( "iOS audio: media services reset, restarting sound\n" );
			Cbuf_ExecuteText( EXEC_APPEND, "snd_restart\n" );
		}];
}
