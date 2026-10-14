#include "sys_ios.h"
#include "ios_touch_layout.h"
#include "qcommon/qcommon.h"
#include "sys/sys_local.h"
#include "client/client.h"
#include <SDL.h>

enum { ROLE_NONE, ROLE_STICK, ROLE_LOOK, ROLE_BUTTON, ROLE_UITAP, ROLE_CONSOLE, ROLE_CINEMATIC_TAP, ROLE_IGNORED };
#define MAX_TOUCH_FINGERS 10
#define STICK_RADIUS_FRAC 0.12f
#define CONSOLE_HOLD_MS 500

void IN_GamepadSetTouchMove( int side, int fwd );	// ios_gamepad.cpp sends the combined axes

typedef struct {
	SDL_FingerID id;
	int role;
	int button;
	float ox, oy, x, y, remX, remY;
	Uint32 downTime;
	qboolean cancelled, longPressed;
} touchFinger_t;

static SDL_Window *touchWindow;
static touchFinger_t fingers[MAX_TOUCH_FINGERS];
static touchRect_t rects[TB_COUNT];
static int viewW, viewH;
static float safeL, safeT, safeR, safeB;
static aspectLayout_t touchLayout;
static float safeH;
static float pixelsPerPoint, touchUnit;
static qboolean layoutReady;
static qboolean lastUIMode, lastHidden, lastCinematic;
static int softKeyboardTarget;
static qboolean lastHwKeyboard, lastTextDemand;
static cvar_t *in_touchControls, *in_touchLookScale;

static qboolean IN_TouchHidden( void ) {
	return ( in_touchControls->integer == 0 || ( in_touchControls->integer == 1 &&
		( IN_GamepadActive() || ( Sys_IOS_HardwareKeyboardConnected() && Sys_IOS_PointerConnected() ) ) ) ) ? qtrue : qfalse;
}

static qboolean IN_TouchOnConsole( float x, float y ) {
	const touchRect_t *con = &rects[TB_CONSOLE];
	return ( x >= con->x && x <= con->x + con->w && y >= con->y && y <= con->y + con->h ) ? qtrue : qfalse;
}

static void IN_TouchUpdateLayout( void ) {
	int w, h;
	SDL_GL_GetDrawableSize( touchWindow, &w, &h );
	float l, t, r, b;
	Sys_IOS_GetSafeAreaPixels( touchWindow, &l, &t, &r, &b );
	float pointW, pointH;
	Sys_IOS_GetViewPointSize( touchWindow, &pointW, &pointH );
	const float p = TouchLayout_PixelsPerPoint( (float)w, (float)h, pointW, pointH );
	touchLayout = AspectLayout_Build( w, h, (int)Cvar_VariableValue( "r_aspectCorrect" ) != 0,
		Cvar_VariableValue( "r_aspectCorrectMinRatio" ), (int)l, (int)t, (int)r, (int)b );
	if ( w == viewW && h == viewH && l == safeL && t == safeT && r == safeR && b == safeB && p == pixelsPerPoint )
		return;
	IN_TouchReleaseAll();
	TouchLayout_Build( (float)w, (float)h, l, t, r, b, p, rects );
	viewW = w;
	viewH = h;
	safeL = l;
	safeT = t;
	safeR = r;
	safeB = b;
	safeH = h - t - b;
	pixelsPerPoint = p;
	touchUnit = TouchLayout_Unit( w - l - r, safeH, p );
	layoutReady = touchUnit > 0 ? qtrue : qfalse;
}

static int IN_TouchButtonKey( int button ) {
	return TouchLayout_Key( button, A_AUX16, A_ESCAPE, A_TAB );
}

static unsigned int IN_TouchVisibleButtons( void ) {
	if ( CL_TouchCinematic() )
		return 0;
	if ( lastHidden || CL_TouchUIMode() )
		return 1u << TB_CONSOLE;
	unsigned int visible = IOS_TOUCH_ALL_BUTTONS;
#ifdef _JK2EXE
	visible &= ~( 1u << TB_CHAT );
#endif
	return visible;
}

static void IN_TouchRelease( touchFinger_t *f ) {
	switch ( f->role ) {
	case ROLE_BUTTON:
		Sys_QueEvent( 0, SE_KEY, IN_TouchButtonKey( f->button ), qfalse, 0, NULL );
		break;
	case ROLE_UITAP:
		Sys_QueEvent( 0, SE_KEY, A_MOUSE1, qfalse, 0, NULL );
		break;
	case ROLE_STICK:
		IN_GamepadSetTouchMove( 0, 0 );
		break;
	default:
		break;
	}
	f->role = ROLE_NONE;
}

void IN_TouchReleaseAll( void ) {
	for ( int i = 0; i < MAX_TOUCH_FINGERS; i++ )
		IN_TouchRelease( &fingers[i] );
	IN_GamepadSetTouchMove( 0, 0 );
}

static void IN_TouchSyncMode( void ) {
	qboolean uiMode = CL_TouchUIMode(), hidden = IN_TouchHidden(), cinematic = CL_TouchCinematic();
	if ( uiMode != lastUIMode || hidden != lastHidden || cinematic != lastCinematic ) {
		IN_TouchReleaseAll();
		if ( cinematic && !lastCinematic ) {
			IN_GamepadZeroAxes();
			SDL_StopTextInput();
		}
		lastUIMode = uiMode;
		lastHidden = hidden;
		lastCinematic = cinematic;
	}
}

static void IN_TouchTextInput( void ) {
	int catcher = Key_GetCatcher();
	qboolean hwKeyboard = Sys_IOS_HardwareKeyboardConnected();
	qboolean disconnected = ( lastHwKeyboard && !hwKeyboard ) ? qtrue : qfalse;
	qboolean cinematic = CL_TouchCinematic();
	if ( softKeyboardTarget && !( catcher & softKeyboardTarget ) )
		softKeyboardTarget = 0;
	if ( disconnected && lastTextDemand && ( catcher & KEYCATCH_UI ) && !( catcher & KEYCATCH_CONSOLE ) && !cinematic )
		softKeyboardTarget = KEYCATCH_UI;
	qboolean demand = ( !cinematic && ( ( ( catcher & KEYCATCH_CONSOLE ) &&
		( hwKeyboard || softKeyboardTarget != KEYCATCH_CONSOLE ) ) ||
#ifndef _JK2EXE
		( catcher & KEYCATCH_MESSAGE ) ||
#endif
		( ( catcher & KEYCATCH_UI ) && ( hwKeyboard || softKeyboardTarget == KEYCATCH_UI ) ) ) ) ? qtrue : qfalse;
	if ( demand ) {
		if ( disconnected )
			SDL_StopTextInput();
		if ( !SDL_IsTextInputActive() )
			SDL_StartTextInput();
	} else if ( SDL_IsTextInputActive() ) {
		SDL_StopTextInput();
	}
	lastHwKeyboard = hwKeyboard;
	lastTextDemand = demand;
}

void IN_TouchInit( SDL_Window *window ) {
	in_touchControls = Cvar_Get( "in_touchControls", "1", CVAR_ARCHIVE );
	in_touchLookScale = Cvar_Get( "in_touchLookScale", "1.0", CVAR_ARCHIVE );
	IN_TouchReleaseAll();	// in_restart calls IN_Init without IN_Shutdown
	touchWindow = window;
	memset( fingers, 0, sizeof( fingers ) );
	viewW = viewH = 0;
	pixelsPerPoint = touchUnit = 0;
	layoutReady = qfalse;
	lastUIMode = CL_TouchUIMode();
	lastHidden = IN_TouchHidden();
	lastCinematic = CL_TouchCinematic();
	if ( lastCinematic )
		IN_GamepadZeroAxes();
	IN_TouchTextInput();
}

void IN_TouchShutdown( void ) {
	IN_TouchReleaseAll();
	touchWindow = NULL;
}

static touchFinger_t *IN_TouchFind( SDL_FingerID id ) {
	for ( int i = 0; i < MAX_TOUCH_FINGERS; i++ ) {
		if ( fingers[i].role != ROLE_NONE && fingers[i].id == id )
			return &fingers[i];
	}
	return NULL;
}

static qboolean IN_TouchButtonHeld( int button ) {
	for ( int i = 0; i < MAX_TOUCH_FINGERS; i++ ) {
		if ( fingers[i].role == ROLE_BUTTON && fingers[i].button == button )
			return qtrue;
	}
	return qfalse;
}

static qboolean IN_TouchHasRole( int role ) {
	for ( int i = 0; i < MAX_TOUCH_FINGERS; i++ ) {
		if ( fingers[i].role == role )
			return qtrue;
	}
	return qfalse;
}

static void IN_TouchConsoleAction( void ) {
	Cbuf_ExecuteText( EXEC_APPEND, "toggleconsole\n" );
}

static void IN_TouchKeyboardAction( void ) {
	int catcher = Key_GetCatcher();
	if ( !( catcher & ( KEYCATCH_CONSOLE | KEYCATCH_UI ) ) )
		return;
	if ( ( catcher & KEYCATCH_CONSOLE ) && Sys_IOS_HardwareKeyboardConnected() )
		return;
	int target = ( catcher & KEYCATCH_CONSOLE ) ? KEYCATCH_CONSOLE : KEYCATCH_UI;
	softKeyboardTarget = softKeyboardTarget == target ? 0 : target;
	IN_TouchTextInput();
}

static void IN_TouchFingerDown( const SDL_TouchFingerEvent *tf ) {
	if ( !layoutReady )
		return;
	float x = tf->x * viewW, y = tf->y * viewH;

	touchFinger_t *f = NULL;
	for ( int i = 0; i < MAX_TOUCH_FINGERS && !f; i++ ) {
		if ( fingers[i].role == ROLE_NONE )
			f = &fingers[i];
	}
	if ( !f || IN_TouchFind( tf->fingerId ) )
		return;

	if ( CL_TouchCinematic() ) {
		if ( IN_TouchHasRole( ROLE_CINEMATIC_TAP ) ) {
			for ( int i = 0; i < MAX_TOUCH_FINGERS; i++ ) {
				if ( fingers[i].role == ROLE_CINEMATIC_TAP )
					fingers[i].cancelled = qtrue;
			}
			return;
		}
		f->role = ROLE_CINEMATIC_TAP;
		f->downTime = tf->timestamp;
		f->cancelled = qfalse;
	} else if ( IN_TouchOnConsole( x, y ) ) {
		if ( IN_TouchHasRole( ROLE_CONSOLE ) )
			return;
		f->role = ROLE_CONSOLE;
		f->button = TB_CONSOLE;
		f->downTime = tf->timestamp;
		f->cancelled = f->longPressed = qfalse;
	} else if ( CL_TouchUIMode() ) {
		if ( IN_TouchHasRole( ROLE_UITAP ) )
			return;
		int vx, vy;
		TouchLayout_ToVirtual( &touchLayout, tf->x, tf->y, &vx, &vy );
		Sys_QueEvent( 0, SE_MOUSE_ABS, vx, vy, 0, NULL );
		Sys_QueEvent( 0, SE_KEY, A_MOUSE1, qtrue, 0, NULL );
		f->role = ROLE_UITAP;
	} else if ( lastHidden ) {
		return;
	} else {
		int hit = TouchLayout_Hit( rects, x, y, IN_TouchVisibleButtons() );
		const int zone = TouchLayout_Zone( (float)viewW, (float)viewH, safeL, safeT, safeR, safeB, x, y );
		f->role = ROLE_IGNORED;
		if ( hit >= 0 ) {
			if ( !IN_TouchButtonHeld( hit ) ) {
				Sys_QueEvent( 0, SE_KEY, IN_TouchButtonKey( hit ), qtrue, 0, NULL );
				f->role = ROLE_BUTTON;
				f->button = hit;
			}
		} else if ( zone == TOUCH_ZONE_MOVE && !IN_TouchHasRole( ROLE_STICK ) ) {
			f->role = ROLE_STICK;
		} else if ( zone == TOUCH_ZONE_AIM && !IN_TouchHasRole( ROLE_LOOK ) ) {
			f->role = ROLE_LOOK;
		}
	}
	f->id = tf->fingerId;
	f->ox = f->x = x;
	f->oy = f->y = y;
	f->remX = f->remY = 0.0f;
}

static void IN_TouchFingerMotion( const SDL_TouchFingerEvent *tf ) {
	touchFinger_t *f = IN_TouchFind( tf->fingerId );
	if ( !f )
		return;
	f->x = tf->x * viewW;
	f->y = tf->y * viewH;
	if ( f->role == ROLE_CONSOLE ) {
		if ( !IN_TouchOnConsole( f->x, f->y ) )
			f->cancelled = qtrue;
	} else if ( f->role == ROLE_CINEMATIC_TAP ) {
		float dx = f->x - f->ox, dy = f->y - f->oy;
		float slop = 0.02f * safeH;
		if ( dx * dx + dy * dy > slop * slop )
			f->cancelled = qtrue;
	} else if ( f->role == ROLE_UITAP ) {
		int vx, vy;
		TouchLayout_ToVirtual( &touchLayout, tf->x, tf->y, &vx, &vy );
		Sys_QueEvent( 0, SE_MOUSE_ABS, vx, vy, 0, NULL );
	} else if ( f->role == ROLE_LOOK ) {
		float dx = tf->dx * viewW * in_touchLookScale->value + f->remX;
		float dy = tf->dy * viewH * in_touchLookScale->value + f->remY;
		int ix = (int)dx, iy = (int)dy;
		f->remX = dx - ix;
		f->remY = dy - iy;
		if ( ix || iy )
			Sys_QueEvent( 0, SE_MOUSE, ix, iy, 0, NULL );
	}
}

void IN_TouchHandleEvent( const SDL_Event *e ) {
	if ( !touchWindow )
		return;
	IN_TouchUpdateLayout();
	IN_TouchSyncMode();
	switch ( e->type ) {
	case SDL_FINGERDOWN:
		IN_TouchFingerDown( &e->tfinger );
		break;
	case SDL_FINGERMOTION:
		IN_TouchFingerMotion( &e->tfinger );
		break;
	case SDL_FINGERUP: {
		touchFinger_t *f = IN_TouchFind( e->tfinger.fingerId );
		if ( f ) {
			if ( f->role == ROLE_CINEMATIC_TAP && !f->cancelled && e->tfinger.timestamp - f->downTime < CONSOLE_HOLD_MS ) {
				float dx = e->tfinger.x * viewW - f->ox, dy = e->tfinger.y * viewH - f->oy;
				float slop = 0.02f * safeH;
				if ( dx * dx + dy * dy <= slop * slop ) {
					Sys_QueEvent( 0, SE_KEY, A_ESCAPE, qtrue, 0, NULL );
					Sys_QueEvent( 0, SE_KEY, A_ESCAPE, qfalse, 0, NULL );
				}
			}
			if ( f->role == ROLE_CONSOLE && !f->cancelled && !f->longPressed &&
				IN_TouchOnConsole( e->tfinger.x * viewW, e->tfinger.y * viewH ) ) {
				if ( e->tfinger.timestamp - f->downTime >= CONSOLE_HOLD_MS )
					IN_TouchKeyboardAction();
				else
					IN_TouchConsoleAction();
			}
			IN_TouchRelease( f );
		}
		break;
	}
	default:
		break;
	}
}

void IN_TouchFrame( void ) {
	if ( !touchWindow )
		return;
	IN_TouchUpdateLayout();

	IN_TouchSyncMode();

	IN_TouchTextInput();

	for ( int i = 0; i < MAX_TOUCH_FINGERS; i++ ) {
		if ( fingers[i].role == ROLE_CONSOLE && !fingers[i].cancelled && !fingers[i].longPressed &&
			SDL_GetTicks() - fingers[i].downTime >= CONSOLE_HOLD_MS ) {
			IN_TouchKeyboardAction();
			fingers[i].longPressed = qtrue;
		}
		if ( fingers[i].role == ROLE_STICK ) {
			float sx, sy;
			TouchLayout_Stick( fingers[i].ox, fingers[i].oy, fingers[i].x, fingers[i].y, STICK_RADIUS_FRAC * touchUnit, &sx, &sy );
			IN_GamepadSetTouchMove( (int)( sx * 127 ), (int)( -sy * 127 ) );
		}
	}
}

static void IN_TouchAddItem( touchDrawItem_t *items, int *n, int maxItems, float x, float y, float w, float h, const char *label, int pressed ) {
	if ( *n >= maxItems )
		return;
	touchDrawItem_t *it = &items[( *n )++];
	const touchRect_t overlay = TouchLayout_RectToVirtual( &touchLayout, { x, y, w, h } );
	it->x = overlay.x;
	it->y = overlay.y;
	it->w = overlay.w;
	it->h = overlay.h;
	it->label = label;
	it->pressed = pressed;
}

int IN_TouchGetDrawList( touchDrawItem_t *items, int maxItems ) {
	if ( !touchWindow )
		return 0;
	IN_TouchUpdateLayout();
	IN_TouchSyncMode();
	if ( CL_TouchCinematic() )
		return 0;
	if ( !layoutReady )
		return 0;

	int n = 0;
	qboolean conOnly = ( lastHidden || CL_TouchUIMode() ) ? qtrue : qfalse;
	const unsigned int visible = IN_TouchVisibleButtons();
	for ( int b = 0; b < TB_COUNT; b++ ) {
		if ( !( visible & ( 1u << b ) ) )
			continue;
		int pressed = 0;
		for ( int i = 0; i < MAX_TOUCH_FINGERS; i++ ) {
			if ( ( fingers[i].role == ROLE_BUTTON || fingers[i].role == ROLE_CONSOLE ) && fingers[i].button == b )
				pressed = 1;
		}
		IN_TouchAddItem( items, &n, maxItems, rects[b].x, rects[b].y, rects[b].w, rects[b].h, touchButtonDefs[b].label, pressed );
	}
	if ( conOnly )
		return n;

	for ( int i = 0; i < MAX_TOUCH_FINGERS; i++ ) {
		if ( fingers[i].role != ROLE_STICK )
			continue;
		float r = STICK_RADIUS_FRAC * touchUnit, sx, sy;
		TouchLayout_Stick( fingers[i].ox, fingers[i].oy, fingers[i].x, fingers[i].y, r, &sx, &sy );
		float kx = fingers[i].ox + sx * r, ky = fingers[i].oy + sy * r;
		IN_TouchAddItem( items, &n, maxItems, fingers[i].ox - r, fingers[i].oy - r, 2 * r, 2 * r, NULL, 0 );
		IN_TouchAddItem( items, &n, maxItems, kx - 0.3f * r, ky - 0.3f * r, 0.6f * r, 0.6f * r, NULL, 1 );
		break;
	}
	return n;
}
