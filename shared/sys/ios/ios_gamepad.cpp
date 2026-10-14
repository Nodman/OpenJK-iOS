#include "sys_ios.h"
#include "qcommon/qcommon.h"
#include "sys/sys_local.h"
#include "client/client.h"
#include <SDL.h>
#include <math.h>

static SDL_GameController *pad;
static SDL_JoystickID padId = -1;
static int heldKey[SDL_CONTROLLER_BUTTON_MAX];	// key sent on press, 0 = not held
static qboolean triggerHeld[2];
static int padAxis[4];	// side, forward, yaw, pitch; 0 in UI mode or without a pad
static int touchSide, touchFwd;
static qboolean axesActive;	// last combined value sent was non-zero
static float cursorX = 320.0f, cursorY = 240.0f;
static qboolean lastUIMode;
static cvar_t *j_deadzone, *j_lookExponent;

static void IN_GamepadOpen( int index ) {
	pad = SDL_GameControllerOpen( index );
	if ( !pad )
		return;
	padId = SDL_JoystickInstanceID( SDL_GameControllerGetJoystick( pad ) );
	Com_Printf( "Gamepad opened: %s\n", SDL_GameControllerName( pad ) );
}

// The touch stick reports here, so one place owns the engine's move/look axes.
void IN_GamepadSetTouchMove( int side, int fwd ) {
	touchSide = side;
	touchFwd = fwd;
}

// Resent every frame while non-zero: CL_ClearState zeroes the engine axes on a level change.
static void IN_GamepadSendAxes( void ) {
	static const int axes[4] = { AXIS_SIDE, AXIS_FORWARD, AXIS_YAW, AXIS_PITCH };
	qboolean padMove = ( padAxis[0] || padAxis[1] ) ? qtrue : qfalse;
	int v[4] = { padMove ? padAxis[0] : touchSide, padMove ? padAxis[1] : touchFwd, padAxis[2], padAxis[3] };
	qboolean active = ( v[0] || v[1] || v[2] || v[3] ) ? qtrue : qfalse;
	if ( !active && !axesActive )
		return;
	for ( int i = 0; i < 4; i++ )
		Sys_QueEvent( 0, SE_JOYSTICK_AXIS, axes[i], v[i], 0, NULL );
	axesActive = active;
}

static void IN_GamepadReleaseAll( void ) {
	for ( int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++ ) {
		if ( heldKey[i] ) {
			Sys_QueEvent( 0, SE_KEY, heldKey[i], qfalse, 0, NULL );
			heldKey[i] = 0;
		}
	}
	for ( int i = 0; i < 2; i++ ) {
		if ( triggerHeld[i] ) {
			Sys_QueEvent( 0, SE_KEY, A_JOY21 + i, qfalse, 0, NULL );
			triggerHeld[i] = qfalse;
		}
	}
	memset( padAxis, 0, sizeof( padAxis ) );
}

static void IN_GamepadClose( void ) {
	IN_GamepadReleaseAll();
	if ( pad ) {
		Com_Printf( "Gamepad closed\n" );
		SDL_GameControllerClose( pad );
	}
	pad = NULL;
	padId = -1;
}

void IN_GamepadInit( void ) {
	j_deadzone = Cvar_Get( "j_deadzone", "0.15", CVAR_ARCHIVE_ND );
	j_lookExponent = Cvar_Get( "j_lookExponent", "2.0", CVAR_ARCHIVE_ND );
	if ( !SDL_WasInit( SDL_INIT_GAMECONTROLLER ) && SDL_InitSubSystem( SDL_INIT_GAMECONTROLLER ) < 0 ) {
		Com_Printf( "SDL_INIT_GAMECONTROLLER failed: %s\n", SDL_GetError() );
		return;
	}
	lastUIMode = CL_TouchUIMode();
	for ( int i = 0; i < SDL_NumJoysticks() && !pad; i++ ) {
		if ( SDL_IsGameController( i ) )
			IN_GamepadOpen( i );
	}
}

void IN_GamepadShutdown( void ) {
	IN_GamepadClose();
	if ( SDL_WasInit( SDL_INIT_GAMECONTROLLER ) )
		SDL_QuitSubSystem( SDL_INIT_GAMECONTROLLER );
}

qboolean IN_GamepadActive( void ) {
	return pad ? qtrue : qfalse;
}

static int IN_GamepadButtonKey( int button ) {
	if ( button == SDL_CONTROLLER_BUTTON_START )
		return A_ESCAPE;
	if ( CL_TouchUIMode() ) {
		if ( button == SDL_CONTROLLER_BUTTON_A )
			return A_MOUSE1;
		if ( button == SDL_CONTROLLER_BUTTON_B )
			return A_ESCAPE;
	}
	return A_JOY0 + button;
}

void IN_GamepadHandleEvent( const SDL_Event *e ) {
	switch ( e->type ) {
	case SDL_CONTROLLERDEVICEADDED:
		if ( !pad && SDL_IsGameController( e->cdevice.which ) )
			IN_GamepadOpen( e->cdevice.which );
		break;
	case SDL_CONTROLLERDEVICEREMOVED:
		if ( pad && e->cdevice.which == padId )
			IN_GamepadClose();
		break;
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP: {
		int b = e->cbutton.button;
		if ( !pad || e->cbutton.which != padId || b < 0 || b >= SDL_CONTROLLER_BUTTON_MAX || b == SDL_CONTROLLER_BUTTON_GUIDE )
			break;
		if ( e->type == SDL_CONTROLLERBUTTONDOWN && !heldKey[b] ) {
			heldKey[b] = IN_GamepadButtonKey( b );
			Sys_QueEvent( 0, SE_KEY, heldKey[b], qtrue, 0, NULL );
		} else if ( e->type == SDL_CONTROLLERBUTTONUP && heldKey[b] ) {
			Sys_QueEvent( 0, SE_KEY, heldKey[b], qfalse, 0, NULL );
			heldKey[b] = 0;
		}
		break;
	}
	default:
		break;
	}
}

static float IN_GamepadAxis( SDL_GameControllerAxis axis ) {
	float v = SDL_GameControllerGetAxis( pad, axis ) / 32767.0f;
	return v < -1.0f ? -1.0f : ( v > 1.0f ? 1.0f : v );
}

// Radial deadzone; the result length is 0..1, raised to expo when expo > 0.
static void IN_GamepadStick( float *x, float *y, float dz, float expo ) {
	float mag = sqrtf( *x * *x + *y * *y );
	if ( mag < dz || mag <= 0.0f ) {
		*x = *y = 0.0f;
		return;
	}
	float scaled = ( mag - dz ) / ( 1.0f - dz );
	if ( scaled > 1.0f )
		scaled = 1.0f;
	if ( expo > 0.0f )
		scaled = powf( scaled, expo );
	*x *= scaled / mag;
	*y *= scaled / mag;
}

static void IN_GamepadPoll( qboolean uiMode ) {
	float dz = Com_Clamp( 0.0f, 0.9f, j_deadzone->value );
	float lx = IN_GamepadAxis( SDL_CONTROLLER_AXIS_LEFTX ), ly = IN_GamepadAxis( SDL_CONTROLLER_AXIS_LEFTY );
	float rx = IN_GamepadAxis( SDL_CONTROLLER_AXIS_RIGHTX ), ry = IN_GamepadAxis( SDL_CONTROLLER_AXIS_RIGHTY );
	IN_GamepadStick( &lx, &ly, dz, 0.0f );
	IN_GamepadStick( &rx, &ry, dz, j_lookExponent->value );

	const SDL_GameControllerAxis triggers[2] = { SDL_CONTROLLER_AXIS_TRIGGERLEFT, SDL_CONTROLLER_AXIS_TRIGGERRIGHT };
	for ( int i = 0; i < 2; i++ ) {
		float t = IN_GamepadAxis( triggers[i] );
		if ( !triggerHeld[i] && t > 0.5f ) {
			triggerHeld[i] = qtrue;
			Sys_QueEvent( 0, SE_KEY, A_JOY21 + i, qtrue, 0, NULL );
		} else if ( triggerHeld[i] && t < 0.3f ) {
			triggerHeld[i] = qfalse;
			Sys_QueEvent( 0, SE_KEY, A_JOY21 + i, qfalse, 0, NULL );
		}
	}

	if ( uiMode ) {
		// the left stick drives the cursor here, so it must not also move the player
		memset( padAxis, 0, sizeof( padAxis ) );
		if ( lx != 0.0f || ly != 0.0f ) {
			float step = 8.0f * cls.frametime / 16.0f;
			cursorX = Com_Clamp( 0.0f, 640.0f, cursorX + lx * step );
			cursorY = Com_Clamp( 0.0f, 480.0f, cursorY + ly * step );
			Sys_QueEvent( 0, SE_MOUSE_ABS, (int)cursorX, (int)cursorY, 0, NULL );
		}
		return;
	}

	padAxis[0] = (int)( lx * 127 );
	padAxis[1] = (int)( -ly * 127 );
	padAxis[2] = (int)( rx * 127 );
	padAxis[3] = (int)( ry * 127 );
}

void IN_GamepadFrame( void ) {
	qboolean uiMode = CL_TouchUIMode();
	if ( uiMode != lastUIMode ) {
		IN_GamepadReleaseAll();
		lastUIMode = uiMode;
	}
	if ( pad )
		IN_GamepadPoll( uiMode );
	IN_GamepadSendAxes();
}
