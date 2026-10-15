#include "sys_ios.h"
#include "ios_touch_layout.h"
#include "ios_touch_layout_edit.h"
#include "ios_touch_edit_state.h"
#include "ios_touch_state.h"
#include "ios_touch_visibility.h"
#include "qcommon/qcommon.h"
#include "sys/sys_local.h"
#include "client/client.h"
#include <SDL.h>

enum { ROLE_NONE, ROLE_GAMEPLAY, ROLE_UITAP, ROLE_CONSOLE, ROLE_CINEMATIC_TAP, ROLE_EDIT_LOCK };
#define MAX_TOUCH_FINGERS 10
#define CONSOLE_HOLD_MS 500

typedef struct {
	SDL_FingerID id;
	int role;
	int button;
	float ox, oy, x, y;
	Uint32 downTime;
	qboolean cancelled, longPressed;
} touchFinger_t;

static SDL_Window *touchWindow;
static touchFinger_t fingers[MAX_TOUCH_FINGERS];
static touchGestureState_t gestures = { {}, -1, -1 };
static touchRect_t rects[TB_COUNT];
static touchLayoutOverrides_t layoutOverrides;
static touchEditState_t editState;
static int sliderDragging;
static SDL_FingerID sliderFinger;
static int layoutModification = -1;
static int viewW, viewH;
static float safeL, safeT, safeR, safeB;
static aspectLayout_t touchLayout;
static float safeH;
static float pixelsPerPoint, touchUnit;
static qboolean layoutReady;
static qboolean lastUIMode, lastHidden, lastCinematic;
static int softKeyboardTarget;
static qboolean lastHwKeyboard, lastTextDemand;
static cvar_t *in_touchControls, *in_touchOpacity, *ui_touchState, *in_touchLayout;
static touchBridgeState_t bridgeState;
static touchVisibility_t visibility;

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
	if ( w == viewW && h == viewH && l == safeL && t == safeT && r == safeR && b == safeB && p == pixelsPerPoint && in_touchLayout->modificationCount == layoutModification )
		return;
	IN_TouchReleaseAll();
	TouchEdit_Parse( in_touchLayout->string, &layoutOverrides );
	int repaired=TouchEdit_Migrate(&layoutOverrides,(float)w,(float)h,l,t,r,b,p);
	repaired |= TouchEdit_RepairLock(&layoutOverrides,(float)w,(float)h,l,t,r,b,p);
	if ( repaired ) {
		char corrected[IOS_TOUCH_LAYOUT_TEXT];
		if ( TouchEdit_Serialize(&layoutOverrides,corrected,sizeof(corrected)) ) Cvar_Set("in_touchLayout",corrected);
	}
	layoutModification = in_touchLayout->modificationCount;
	TouchLayout_Build( (float)w, (float)h, l, t, r, b, p, rects, &layoutOverrides );
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
	unsigned int mask = TouchCaps_ButtonMask( &visibility );
	if ( TouchEdit_LockVisible(cls.state==CA_ACTIVE,lastHidden,CL_TouchUIMode(),CL_TouchCinematic()) ) mask |= 1u << TB_LOCK;
	return mask;
}

static void IN_TouchStep( int event, SDL_FingerID id, int target, int button, float x, float y ) {
	touchGestureInput_t input = {};
	input.event = event;
	input.finger = id;
	input.target = target;
	input.button = button;
	input.x = x;
	input.y = y;
	input.radius = IOS_TOUCH_STICK_RADIUS * touchUnit;
	input.slop = TouchLayout_FireSlop( touchUnit, pixelsPerPoint );
	input.visibleButtons = IN_TouchVisibleButtons();
	input.moveEnabled = input.lookEnabled = layoutReady && visibility.sticks;
	const touchGestureActions_t actions = TouchGesture_Step( &gestures, input );
	for ( int i = 0; i < actions.keyCount; ++i )
		Sys_QueEvent( 0, SE_KEY, IN_TouchButtonKey( actions.keys[i].button ), actions.keys[i].down ? qtrue : qfalse, 0, NULL );
	IN_GamepadSetTouchMove( actions.moveX, -actions.moveY );
	IN_GamepadSetTouchLook( actions.lookX, actions.lookY );
}

static void IN_TouchRelease( touchFinger_t *f ) {
	switch ( f->role ) {
	case ROLE_GAMEPLAY:
		IN_TouchStep( TOUCH_CANCEL, f->id, TOUCH_TARGET_IGNORE, 0, f->x, f->y );
		break;
	case ROLE_UITAP:
		Sys_QueEvent( 0, SE_KEY, A_MOUSE1, qfalse, 0, NULL );
		break;
	default:
		break;
	}
	f->role = ROLE_NONE;
}

static void IN_TouchSaveEdit( void ) {
	if ( !editState.active ) return;
	touchEditInput_t input = {}; input.event=TOUCH_EDIT_EXIT;
	TouchEdit_State(&editState,input);
	sliderDragging=0;
	char text[IOS_TOUCH_LAYOUT_TEXT];
	if ( in_touchLayout && TouchEdit_Serialize(&layoutOverrides,text,sizeof(text)) ) {
		Cvar_Set("in_touchLayout",text);
		layoutModification=in_touchLayout->modificationCount;
	}
}

void IN_TouchReleaseAll( void ) {
	IN_TouchSaveEdit();
	for ( int i = 0; i < MAX_TOUCH_FINGERS; i++ )
		IN_TouchRelease( &fingers[i] );
	IN_TouchStep( TOUCH_CANCEL_ALL, 0, TOUCH_TARGET_IGNORE, 0, 0, 0 );
	IN_GamepadSendAxes();
}

qboolean IN_TouchIsEditing( void ) { return editState.active ? qtrue : qfalse; }

void IN_TouchEndEdit( void ) {
	if ( !editState.active ) return;
	IN_TouchReleaseAll();
	IN_GamepadZeroTouchAxes();
}

static void IN_TouchSyncMode( void ) {
	qboolean uiMode = CL_TouchUIMode(), hidden = IN_TouchHidden(), cinematic = CL_TouchCinematic();
	if ( editState.active && cls.state!=CA_ACTIVE ) IN_TouchEndEdit();
	if ( uiMode != lastUIMode || hidden != lastHidden || cinematic != lastCinematic ) {
		IN_TouchReleaseAll();
		if ( cinematic && !lastCinematic ) {
			IN_GamepadZeroTouchAxes();
			SDL_StopTextInput();
		}
		lastUIMode = uiMode;
		lastHidden = hidden;
		lastCinematic = cinematic;
	}
}

static void IN_TouchSyncVisibility( void ) {
#ifdef _JK2EXE
	const int multiplayer = 0;
#else
	const int multiplayer = 1;
#endif
	const unsigned int mask = ui_touchState ? (unsigned int)ui_touchState->integer : 0;
	visibility = TouchBridge_Step( &bridgeState, mask, multiplayer, cls.state == CA_ACTIVE,
		lastHidden, CL_TouchUIMode(), CL_TouchCinematic(), (unsigned int)cls.realtime );
	if ( visibility.logMissing )
		Com_Printf( "Touch controls: no cgame state for 1000ms; using full controls\n" );
	if ( !editState.active ) IN_TouchStep( TOUCH_REFRESH, 0, TOUCH_TARGET_IGNORE, 0, 0, 0 );
}

void IN_TouchResetState( void ) {
	if ( !ui_touchState )
		ui_touchState = Cvar_Get( "ui_touchState", "0", CVAR_ROM );
	Cvar_Set( "ui_touchState", "0" );
	TouchBridge_Reset( &bridgeState );
	visibility = {};
	IN_TouchReleaseAll();
	IN_GamepadZeroTouchAxes();
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

void IN_TouchRestoreDefaultBinds( void ) {
	for ( int button = 0; button < TB_COUNT; ++button ) {
		if ( !touchButtonDefs[button].defaultBind )
			continue;
		const int key = IN_TouchButtonKey( button );
		const char *command = TouchLayout_DefaultBind( button, Key_GetBinding( key ) );
		if ( command ) {
			Key_SetBinding( key, command );
			Com_DPrintf( "Touch: restored AUX%d bind to %s\n", touchButtonDefs[button].key, command );
		}
	}
}

static void IN_TouchLayoutReset( void ) {
	IN_TouchEndEdit();
	Cvar_Set( "in_touchLayout", "" );
	layoutModification = -1;
	if ( touchWindow ) IN_TouchUpdateLayout();
}

void IN_TouchInit( SDL_Window *window ) {
	in_touchLayout = Cvar_Get( "in_touchLayout", "", CVAR_ARCHIVE );
	Cmd_RemoveCommand( "touchLayoutReset" );
	Cmd_AddCommand( "touchLayoutReset", IN_TouchLayoutReset );
	in_touchControls = Cvar_Get( "in_touchControls", "1", CVAR_ARCHIVE );
	in_touchOpacity = Cvar_Get( "in_touchOpacity", "1.0", CVAR_ARCHIVE );
	ui_touchState = Cvar_Get( "ui_touchState", "0", CVAR_ROM );
	IN_TouchReleaseAll();	// in_restart calls IN_Init without IN_Shutdown
	touchWindow = window;
	memset( fingers, 0, sizeof( fingers ) );
	TouchGesture_Reset( &gestures );
	viewW = viewH = 0;
	pixelsPerPoint = touchUnit = 0;
	layoutReady = qfalse;
	lastUIMode = CL_TouchUIMode();
	lastHidden = IN_TouchHidden();
	lastCinematic = CL_TouchCinematic();
	if ( lastCinematic )
		IN_GamepadZeroTouchAxes();
	IN_TouchSyncVisibility();
	IN_TouchTextInput();
}

void IN_TouchShutdown( void ) {
	Cmd_RemoveCommand( "touchLayoutReset" );
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
	} else if ( TouchLayout_Hit(rects,x,y,IN_TouchVisibleButtons() & (1u<<TB_LOCK))==TB_LOCK ) {
		f->role=ROLE_EDIT_LOCK; f->cancelled=qfalse;
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
		if (TouchLayout_Hit(rects,x,y,IN_TouchVisibleButtons() & (1u<<TB_LOCK))==TB_LOCK) hit=TB_LOCK;
		const int zone = TouchLayout_Zone( (float)viewW, (float)viewH, safeL, safeT, safeR, safeB, x, y );
		int target = TOUCH_TARGET_IGNORE;
		if ( hit >= 0 )
			target = hit == TB_ATTACK || hit == TB_ALTATTACK ? TOUCH_TARGET_FIRE : TOUCH_TARGET_BUTTON;
		else if ( zone == TOUCH_ZONE_MOVE )
			target = TOUCH_TARGET_MOVE;
		else if ( zone == TOUCH_ZONE_AIM )
			target = TOUCH_TARGET_AIM;
		if (hit==TB_LOCK) {
			f->role=ROLE_EDIT_LOCK; f->cancelled=qfalse;
		} else {
		f->role = ROLE_GAMEPLAY;
		IN_TouchStep( TOUCH_DOWN, tf->fingerId, target, hit, x, y );
		}
	}
	f->id = tf->fingerId;
	f->ox = f->x = x;
	f->oy = f->y = y;
}

static void IN_TouchFingerMotion( const SDL_TouchFingerEvent *tf ) {
	touchFinger_t *f = IN_TouchFind( tf->fingerId );
	if ( !f )
		return;
	f->x = tf->x * viewW;
	f->y = tf->y * viewH;
	if (f->role==ROLE_EDIT_LOCK) {
		const float dx=f->x-f->ox,dy=f->y-f->oy,slop=6*pixelsPerPoint;
		if(dx*dx+dy*dy>slop*slop) f->cancelled=qtrue;
	} else if ( f->role == ROLE_CONSOLE ) {
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
	} else if ( f->role == ROLE_GAMEPLAY ) {
		IN_TouchStep( TOUCH_MOTION, f->id, TOUCH_TARGET_IGNORE, 0, f->x, f->y );
	}
}

static void IN_TouchEditEvent( const SDL_TouchFingerEvent *tf, int event ) {
	const touchRect_t slider=TouchEdit_SliderRect((float)viewW,(float)viewH,safeL,safeT,safeR,safeB,pixelsPerPoint);
	const float x=tf->x*viewW,y=tf->y*viewH;
	if (event==TOUCH_EDIT_DOWN && !editState.dragging && !sliderDragging &&
		x>=slider.x && x<=slider.x+slider.w && y>=slider.y && y<=slider.y+slider.h) {
		sliderDragging=1; sliderFinger=tf->fingerId;
	}
	if (sliderDragging) {
		if (sliderFinger==tf->fingerId) {
			Cvar_SetValue("in_touchOpacity",TouchEdit_SliderValue(x,&slider));
			if(event==TOUCH_EDIT_UP) sliderDragging=0;
		}
		return;
	}

	touchEditInput_t input = {};
	input.event=event; input.finger=tf->fingerId; input.x=tf->x*viewW; input.y=tf->y*viewH;
	input.button=TouchLayout_Hit(rects,input.x,input.y,IOS_TOUCH_ALL_BUTTONS);
	if (TouchLayout_Hit(rects,input.x,input.y,1u<<TB_LOCK)==TB_LOCK) input.button=TB_LOCK;
	input.slop=6*pixelsPerPoint;
	if ( input.button>=0 ) {
		const touchRect_t *r=&rects[input.button]; input.cx=r->x+r->w/2; input.cy=r->y+r->h/2;
	}
	const touchEditOutput_t out=TouchEdit_State(&editState,input);
	if ( out.move && TouchEdit_Drag(&layoutOverrides,out.button,out.x,out.y,(float)viewW,(float)viewH,safeL,safeT,safeR,safeB,pixelsPerPoint) )
		TouchLayout_Build((float)viewW,(float)viewH,safeL,safeT,safeR,safeB,pixelsPerPoint,rects,&layoutOverrides);
	if ( out.save ) {
		char text[IOS_TOUCH_LAYOUT_TEXT];
		if ( TouchEdit_Serialize(&layoutOverrides,text,sizeof(text)) ) Cvar_Set("in_touchLayout",text);
		layoutModification=in_touchLayout->modificationCount;
		IN_TouchReleaseAll(); IN_GamepadZeroTouchAxes();
	}
}

void IN_TouchHandleEvent( const SDL_Event *e ) {
	if ( !touchWindow )
		return;
	IN_TouchUpdateLayout();
	IN_TouchSyncMode();
	IN_TouchSyncVisibility();
	if ( editState.active ) {
		if (e->type==SDL_FINGERDOWN) IN_TouchEditEvent(&e->tfinger,TOUCH_EDIT_DOWN);
		else if (e->type==SDL_FINGERMOTION) IN_TouchEditEvent(&e->tfinger,TOUCH_EDIT_MOVE);
		else if (e->type==SDL_FINGERUP) IN_TouchEditEvent(&e->tfinger,TOUCH_EDIT_UP);
		return;
	}
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
			if ( f->role==ROLE_EDIT_LOCK && !f->cancelled &&
				TouchLayout_Hit(rects,e->tfinger.x*viewW,e->tfinger.y*viewH,IN_TouchVisibleButtons() & (1u<<TB_LOCK))==TB_LOCK ) {
				IN_TouchReleaseAll(); IN_GamepadZeroTouchAxes();
				touchEditInput_t input = {}; input.event=TOUCH_EDIT_ENTER;
				TouchEdit_State(&editState,input);
			}
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
	IN_TouchSyncVisibility();

	IN_TouchTextInput();
	if ( editState.active ) return;

	for ( int i = 0; i < MAX_TOUCH_FINGERS; i++ ) {
		if ( fingers[i].role == ROLE_CONSOLE && !fingers[i].cancelled && !fingers[i].longPressed &&
			SDL_GetTicks() - fingers[i].downTime >= CONSOLE_HOLD_MS ) {
			IN_TouchKeyboardAction();
			fingers[i].longPressed = qtrue;
		}
	}
	IN_TouchStep( TOUCH_REFRESH, 0, TOUCH_TARGET_IGNORE, 0, 0, 0 );
}

float IN_TouchGetOpacity( void ) {
	return TouchDraw_Opacity( in_touchOpacity ? in_touchOpacity->value : 1.0f );
}

int IN_TouchGetDrawList( touchDrawItem_t *items, int maxItems ) {
	if ( !touchWindow )
		return 0;
	IN_TouchUpdateLayout();
	IN_TouchSyncMode();
	IN_TouchSyncVisibility();
	if ( !layoutReady )
		return 0;
	touchDrawInput_t input = {};
	input.layout = &touchLayout;
	input.rects = rects;
	input.gestures = &gestures;
	input.visibleButtons = IN_TouchVisibleButtons();
	input.sticks = visibility.sticks;
	const touchRect_t slider=TouchEdit_SliderRect((float)viewW,(float)viewH,safeL,safeT,safeR,safeB,pixelsPerPoint);
	input.slider=&slider; input.opacity=IN_TouchGetOpacity();
	input.editing = editState.active;
	input.dragButton = editState.dragging ? editState.button : -1;
	input.styleColor = visibility.styleColor;
	input.weaponClass = visibility.weaponClass;
	for ( int i = 0; i < MAX_TOUCH_FINGERS; ++i ) {
		if ( fingers[i].role == ROLE_CONSOLE )
			input.pressedButtons |= 1u << TB_CONSOLE;
	}
	return TouchDraw_Build( &input, items, maxItems );
}
