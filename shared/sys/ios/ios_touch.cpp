#include "sys_ios.h"
#include "ios_touch_layout.h"
#include "qcommon/qcommon.h"
#include "sys/sys_local.h"
#include "client/client.h"
#include <SDL.h>

enum { ROLE_NONE, ROLE_STICK, ROLE_LOOK, ROLE_BUTTON, ROLE_UITAP };
#define MAX_TOUCH_FINGERS 10
#define STICK_RADIUS_FRAC 0.12f	// of the safe height

void IN_GamepadSetTouchMove( int side, int fwd );	// ios_gamepad.cpp sends the combined axes

typedef struct {
	SDL_FingerID id;
	int role;
	int button;
	float ox, oy, x, y, remX, remY;
} touchFinger_t;

static SDL_Window *touchWindow;
static touchFinger_t fingers[MAX_TOUCH_FINGERS];
static touchRect_t rects[TB_COUNT];
static int viewW, viewH;
static float safeH;
static qboolean lastUIMode, lastHidden;
static int keyboardOwner;	// catcher the CON button opened the soft keyboard for, 0 = none
static qboolean keyboardArmed;	// owner catcher seen; toggleconsole runs later via Cbuf
static qboolean lastHwKeyboard;
static cvar_t *in_touchControls, *in_touchLookScale;

static qboolean IN_TouchHidden( void ) {
	return ( in_touchControls->integer == 0 || ( in_touchControls->integer == 1 && IN_GamepadActive() ) ) ? qtrue : qfalse;
}

static void IN_TouchUpdateLayout( void ) {
	int w, h;
	SDL_GL_GetDrawableSize( touchWindow, &w, &h );
	if ( w == viewW && h == viewH )
		return;
	float l, t, r, b;
	Sys_IOS_GetSafeAreaPixels( touchWindow, &l, &t, &r, &b );
	TouchLayout_Build( (float)w, (float)h, l, t, r, b, rects );
	viewW = w;
	viewH = h;
	safeH = h - t - b;
}

static int IN_TouchButtonKey( int button ) {
	return button == TB_MENU ? A_ESCAPE : A_AUX16 + button;
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

void IN_TouchInit( SDL_Window *window ) {
	in_touchControls = Cvar_Get( "in_touchControls", "1", CVAR_ARCHIVE );
	in_touchLookScale = Cvar_Get( "in_touchLookScale", "1.0", CVAR_ARCHIVE );
	IN_TouchReleaseAll();	// in_restart calls IN_Init without IN_Shutdown
	touchWindow = window;
	memset( fingers, 0, sizeof( fingers ) );
	viewW = viewH = 0;
	lastUIMode = CL_TouchUIMode();
	lastHidden = IN_TouchHidden();
	lastHwKeyboard = Sys_IOS_HardwareKeyboardConnected();
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
	int catcher = Key_GetCatcher();
	qboolean hwKeyboard = Sys_IOS_HardwareKeyboardConnected();
	if ( ( catcher & KEYCATCH_CONSOLE ) || !( catcher & KEYCATCH_UI ) ) {
		qboolean opening = ( catcher & KEYCATCH_CONSOLE ) ? qfalse : qtrue;
		Cbuf_ExecuteText( EXEC_APPEND, "toggleconsole\n" );
		if ( opening ) {
			SDL_StartTextInput();
			keyboardOwner = KEYCATCH_CONSOLE;
			keyboardArmed = qfalse;
		} else {
			if ( !hwKeyboard )
				SDL_StopTextInput();
			keyboardOwner = 0;
		}
	} else if ( SDL_IsTextInputActive() && !hwKeyboard ) {
		SDL_StopTextInput();
		keyboardOwner = 0;
	} else {
		SDL_StartTextInput();
		keyboardOwner = KEYCATCH_UI;
		keyboardArmed = qfalse;
	}
}

static void IN_TouchFingerDown( const SDL_TouchFingerEvent *tf ) {
	float x = tf->x * viewW, y = tf->y * viewH;
	const touchRect_t *con = &rects[TB_CONSOLE];
	if ( x >= con->x && x <= con->x + con->w && y >= con->y && y <= con->y + con->h ) {
		IN_TouchConsoleAction();
		return;
	}

	touchFinger_t *f = NULL;
	for ( int i = 0; i < MAX_TOUCH_FINGERS && !f; i++ ) {
		if ( fingers[i].role == ROLE_NONE )
			f = &fingers[i];
	}
	if ( !f || IN_TouchFind( tf->fingerId ) )
		return;

	if ( CL_TouchUIMode() ) {
		if ( IN_TouchHasRole( ROLE_UITAP ) )
			return;
		int vx, vy;
		TouchLayout_ToVirtual( tf->x, tf->y, &vx, &vy );
		Sys_QueEvent( 0, SE_MOUSE_ABS, vx, vy, 0, NULL );
		Sys_QueEvent( 0, SE_KEY, A_MOUSE1, qtrue, 0, NULL );
		f->role = ROLE_UITAP;
	} else if ( IN_TouchHidden() ) {
		return;
	} else {
		int hit = TouchLayout_Hit( rects, x, y, 0 );
		if ( hit >= 0 ) {
			if ( IN_TouchButtonHeld( hit ) )
				return;	// one finger owns a button, so lifting a second one sends no early key-up
			Sys_QueEvent( 0, SE_KEY, IN_TouchButtonKey( hit ), qtrue, 0, NULL );
			f->role = ROLE_BUTTON;
			f->button = hit;
		} else if ( x < viewW / 2 && !IN_TouchHasRole( ROLE_STICK ) ) {
			f->role = ROLE_STICK;
		} else {
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
	if ( f->role == ROLE_UITAP ) {
		int vx, vy;
		TouchLayout_ToVirtual( tf->x, tf->y, &vx, &vy );
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
	switch ( e->type ) {
	case SDL_FINGERDOWN:
		IN_TouchFingerDown( &e->tfinger );
		break;
	case SDL_FINGERMOTION:
		IN_TouchFingerMotion( &e->tfinger );
		break;
	case SDL_FINGERUP: {
		touchFinger_t *f = IN_TouchFind( e->tfinger.fingerId );
		if ( f )
			IN_TouchRelease( f );
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

	qboolean uiMode = CL_TouchUIMode(), hidden = IN_TouchHidden();
	if ( uiMode != lastUIMode || hidden != lastHidden ) {
		IN_TouchReleaseAll();
		lastUIMode = uiMode;
		lastHidden = hidden;
	}

	// a hardware keyboard can come and go at any time; text input follows it
	qboolean hwKeyboard = Sys_IOS_HardwareKeyboardConnected();
	if ( hwKeyboard != lastHwKeyboard ) {
		lastHwKeyboard = hwKeyboard;
		if ( hwKeyboard ) {
			SDL_StartTextInput();
		} else if ( Key_GetCatcher() & KEYCATCH_CONSOLE ) {
			keyboardOwner = KEYCATCH_CONSOLE;	// keep typing on the soft keyboard
			keyboardArmed = qtrue;
		} else if ( !keyboardOwner ) {
			SDL_StopTextInput();
		}
	}

	// the console or menu closed some other way: drop the keyboard the CON button opened
	if ( keyboardOwner && ( Key_GetCatcher() & keyboardOwner ) ) {
		keyboardArmed = qtrue;
	} else if ( keyboardOwner && keyboardArmed ) {
		if ( !hwKeyboard )
			SDL_StopTextInput();
		keyboardOwner = 0;
	}

	for ( int i = 0; i < MAX_TOUCH_FINGERS; i++ ) {
		if ( fingers[i].role == ROLE_STICK ) {
			float sx, sy;
			TouchLayout_Stick( fingers[i].ox, fingers[i].oy, fingers[i].x, fingers[i].y, STICK_RADIUS_FRAC * safeH, &sx, &sy );
			IN_GamepadSetTouchMove( (int)( sx * 127 ), (int)( -sy * 127 ) );
			break;
		}
	}
}

static void IN_TouchAddItem( touchDrawItem_t *items, int *n, int maxItems, float x, float y, float w, float h, const char *label, int pressed ) {
	if ( *n >= maxItems )
		return;
	touchDrawItem_t *it = &items[( *n )++];
	it->x = x * SCREEN_WIDTH / viewW;
	it->y = y * SCREEN_HEIGHT / viewH;
	it->w = w * SCREEN_WIDTH / viewW;
	it->h = h * SCREEN_HEIGHT / viewH;
	it->label = label;
	it->pressed = pressed;
}

int IN_TouchGetDrawList( touchDrawItem_t *items, int maxItems ) {
	if ( !touchWindow )
		return 0;
	IN_TouchUpdateLayout();
	if ( viewW <= 0 || viewH <= 0 )
		return 0;

	int n = 0;
	qboolean conOnly = ( IN_TouchHidden() || CL_TouchUIMode() ) ? qtrue : qfalse;
	for ( int b = 0; b < TB_COUNT; b++ ) {
		if ( conOnly && b != TB_CONSOLE )
			continue;
		int pressed = 0;
		for ( int i = 0; i < MAX_TOUCH_FINGERS; i++ ) {
			if ( fingers[i].role == ROLE_BUTTON && fingers[i].button == b )
				pressed = 1;
		}
		IN_TouchAddItem( items, &n, maxItems, rects[b].x, rects[b].y, rects[b].w, rects[b].h, touchButtonDefs[b].label, pressed );
	}
	if ( conOnly )
		return n;

	for ( int i = 0; i < MAX_TOUCH_FINGERS; i++ ) {
		if ( fingers[i].role != ROLE_STICK )
			continue;
		float r = STICK_RADIUS_FRAC * safeH, sx, sy;
		TouchLayout_Stick( fingers[i].ox, fingers[i].oy, fingers[i].x, fingers[i].y, r, &sx, &sy );
		float kx = fingers[i].ox + sx * r, ky = fingers[i].oy + sy * r;
		IN_TouchAddItem( items, &n, maxItems, fingers[i].ox - r, fingers[i].oy - r, 2 * r, 2 * r, NULL, 0 );
		IN_TouchAddItem( items, &n, maxItems, kx - 0.3f * r, ky - 0.3f * r, 0.6f * r, 0.6f * r, NULL, 1 );
		break;
	}
	return n;
}
