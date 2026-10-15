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
static touchSettingsState_t settingsState;
static int layoutModification = -1;
static touchLayoutProfile_t layoutProfile = TOUCH_LAYOUT_TABLET;
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
static touchForceBridge_t forceBridge;
static touchForceReport_t lastForceReport;
static cvar_t *in_touchForceSlots;
static touchForceSlots_t forceSlots;
static int forceSlotsModification=-1;
static cvar_t *touchAimMode,*touchAimCurve,*touchLookScale,*touchDragScale;
static int aimRevisions[4]={-1,-1,-1,-1};
static float dragX,dragY;

void IN_TouchConsumeLookDelta(float *yaw,float *pitch) {
	const float scale=IOS_TOUCH_DRAG_DEGREES_PER_POINT*TouchAim_Sensitivity(touchDragScale?touchDragScale->value:1);
	*yaw=dragX*scale;*pitch=dragY*scale;
	if(!TouchStick_Finite(*yaw) || !TouchStick_Finite(*pitch)) *yaw=*pitch=0;
	dragX=dragY=0;
}

static void IN_TouchRelease(touchFinger_t *f,int event);

static int IN_TouchSyncAim(void) {
	cvar_t *vars[4]={touchAimMode,touchAimCurve,touchLookScale,touchDragScale};
	int changed=0;
	for(int i=0;i<4;++i) if(vars[i] && aimRevisions[i]!=vars[i]->modificationCount) {
		aimRevisions[i]=vars[i]->modificationCount;changed=1;
	}
	if(!changed) return 0;
	editState.dragging=0;editState.button=-1;
	CL_TouchCancelAllSources();
	for(int i=0;i<MAX_TOUCH_FINGERS;++i) IN_TouchRelease(&fingers[i],TOUCH_CANCEL);
	TouchGesture_Reset(&gestures);
	dragX=dragY=0;
	IN_GamepadZeroTouchAxes();
	return 1;
}

void IN_TouchPrepareCommand(void) {
	if(IN_TouchSyncAim()) IN_GamepadSyncAxesForCommand();
}

void IN_TouchReportForce(int version,int mask,int selected,int equipment) {
	TouchForce_Report(&forceBridge,version,mask,selected,equipment);
}
void IN_TouchClearForceFrame(void) { TouchForce_ClearPending(&forceBridge); }
touchForceToken_t IN_TouchBeginForceUpdate(void) { return TouchForce_BeginUpdate(&forceBridge); }
touchForceToken_t IN_TouchBeginForceFrame(void) { return TouchForce_BeginField(&forceBridge); }
touchForceReport_t IN_TouchGetForceReport(void) { return TouchForce_Current(&forceBridge,(unsigned int)cls.realtime); }
void IN_TouchEndForceUpdate(touchForceToken_t token) { TouchForce_EndUpdate(&forceBridge,token,(unsigned int)cls.realtime); }
void IN_TouchEndForceFrame(touchForceToken_t token) { TouchForce_EndField(&forceBridge,token,(unsigned int)cls.realtime); }


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
	const touchLayoutProfile_t profile = Sys_IOS_TouchLayoutProfile( touchWindow );
	touchLayout = AspectLayout_Build( w, h, (int)Cvar_VariableValue( "r_aspectCorrect" ) != 0,
		Cvar_VariableValue( "r_aspectCorrectMinRatio" ), (int)l, (int)t, (int)r, (int)b );
	if ( w == viewW && h == viewH && l == safeL && t == safeT && r == safeR && b == safeB && p == pixelsPerPoint && profile == layoutProfile && in_touchLayout->modificationCount == layoutModification )
		return;
	IN_TouchReleaseAll();
	TouchEdit_Parse( in_touchLayout->string, &layoutOverrides );
	const int legacy=layoutOverrides.version!=2;
	int repaired=TouchEdit_Migrate(&layoutOverrides,(float)w,(float)h,l,t,r,b,p);
	if(legacy) repaired |= TouchEdit_RepairLock(&layoutOverrides,(float)w,(float)h,l,t,r,b,p,TOUCH_LAYOUT_LEGACY);
	if ( repaired ) {
		char corrected[IOS_TOUCH_LAYOUT_TEXT];
		if ( TouchEdit_Serialize(&layoutOverrides,corrected,sizeof(corrected)) ) Cvar_Set("in_touchLayout",corrected);
	}
	layoutModification = in_touchLayout->modificationCount;
	TouchLayout_Build( (float)w, (float)h, l, t, r, b, p, rects, &layoutOverrides, profile );
	viewW = w;
	viewH = h;
	safeL = l;
	safeT = t;
	safeR = r;
	safeB = b;
	safeH = h - t - b;
	pixelsPerPoint = p;
	layoutProfile = profile;
	touchUnit = TouchLayout_Unit( w - l - r, safeH, p );
	layoutReady = touchUnit > 0 ? qtrue : qfalse;
}

static int IN_TouchButtonKey( int button ) {
	return TouchLayout_Key( button, A_AUX16, A_ESCAPE, A_TAB );
}

static void IN_TouchCancelForceFingers(void);

static void IN_TouchSyncForceSlots(void) {
	if(!in_touchForceSlots || forceSlotsModification==in_touchForceSlots->modificationCount) return;
	// Cancellation reenters visibility, so publish the revision first.
	forceSlotsModification=in_touchForceSlots->modificationCount;
	IN_TouchCancelForceFingers();
	TouchForceSlots_Parse(in_touchForceSlots->string,&forceSlots);
}
static void IN_TouchSettingsForce(void) {
	IN_TouchSyncAim();
	settingsState.aimMode=touchAimMode?touchAimMode->integer==1:0;
	settingsState.aimCurve=touchAimCurve?touchAimCurve->integer:2;
	settingsState.lookScale=TouchAim_Sensitivity(touchLookScale?touchLookScale->value:1);
	settingsState.dragScale=TouchAim_Sensitivity(touchDragScale?touchDragScale->value:1);
	IN_TouchSyncForceSlots();
	settingsState.learned=IN_TouchGetForceReport().learned;
	for(int i=0;i<4;++i) settingsState.slots[i]=TouchForceSlots_Visible(&forceSlots,i,settingsState.learned);
}
static unsigned int IN_TouchVisibleButtons( void ) {
	unsigned int mask = TouchCaps_ButtonMask( &visibility );
	IN_TouchSyncForceSlots();
	const touchForceReport_t report=IN_TouchGetForceReport();
	if(cls.state==CA_ACTIVE && report.equipment && !lastHidden && !CL_TouchUIMode() && !CL_TouchCinematic())
		for(int i=0;i<4;++i) if(TouchForceSlots_Visible(&forceSlots,i,report.learned)) mask|=1u<<(TB_FORCE_SLOT1+i);
	if ( TouchEdit_LockVisible(cls.state==CA_ACTIVE,lastHidden,CL_TouchUIMode(),CL_TouchCinematic()) ) mask |= 1u << TB_LOCK;
	return mask;
}

static void IN_TouchStep( int event, SDL_FingerID id, int target, int button, float x, float y ) {
	touchGestureInput_t input = {};
	input.event = event;
	input.aimMode=touchAimMode?touchAimMode->integer==1:0;
	input.aimCurve=touchAimCurve?touchAimCurve->integer:2;
	if(input.aimCurve<0 || input.aimCurve>5 || (touchAimCurve && !TouchStick_Finite(touchAimCurve->value))) input.aimCurve=-2;
	input.finger = id;
	input.target = target;
	input.button = button;
	input.x = x;
	input.y = y;
	input.radius = IOS_TOUCH_STICK_RADIUS * touchUnit;
	input.slop = TouchLayout_FireSlop( touchUnit, pixelsPerPoint );
	input.visibleButtons = IN_TouchVisibleButtons();
	input.moveEnabled = input.lookEnabled = layoutReady && visibility.sticks;
	const int force=button==TB_FORCE || (button>=TB_FORCE_SLOT1 && button<=TB_FORCE_SLOT4);
	int reserved=0;
	if(event==TOUCH_DOWN && (input.target==TOUCH_TARGET_BUTTON || input.target==TOUCH_TARGET_FIRE)) {
		if((button==TB_FORCEPREV || button==TB_FORCENEXT) && CL_TouchForceBusy()) input.target=TOUCH_TARGET_IGNORE;
		if(force) {
			const int slot=button==TB_FORCE?-1:button-TB_FORCE_SLOT1;
			const touchForceReport_t report=IN_TouchGetForceReport();
			const int power=slot<0?report.selected:TouchForceSlots_Visible(&forceSlots,slot,report.learned);
			reserved=CL_TouchForceReserve(power,slot);
			if(!reserved) input.target=TOUCH_TARGET_IGNORE;
		}
	}
	const touchGestureActions_t actions = TouchGesture_Step( &gestures, input );
	if(reserved && !TouchGesture_HoldsButton(&gestures,id,button)) CL_TouchForceCancel();
	for ( int i = 0; i < actions.keyCount; ++i ) {
		const int pressed=actions.keys[i].button,down=actions.keys[i].down;
		if(pressed==TB_FORCE || (pressed>=TB_FORCE_SLOT1 && pressed<=TB_FORCE_SLOT4)) {
			if(!down && event!=TOUCH_UP) CL_TouchForceCancel();else CL_TouchForceQueue(down);
		} else {
			const int key=IN_TouchButtonKey(pressed);
			Sys_QueEvent(0,SE_KEY,key,down?qtrue:qfalse,0,NULL);
			if(!down && event!=TOUCH_UP) CL_TouchCancelSource(key);
		}
	}
	if(actions.cancelLook || event==TOUCH_CANCEL_ALL) {
		dragX=dragY=0;
		IN_GamepadZeroTouchAxes();
	} else if(pixelsPerPoint>0) {
		dragX+=actions.deltaX/pixelsPerPoint;dragY+=actions.deltaY/pixelsPerPoint;
		if(!TouchStick_Finite(dragX) || !TouchStick_Finite(dragY)) dragX=dragY=0;
	}
	IN_GamepadSetTouchMove( actions.moveX, -actions.moveY );
	IN_GamepadSetTouchLook( actions.lookX, actions.lookY );
}

static void IN_TouchRelease( touchFinger_t *f, int event=TOUCH_CANCEL ) {
	switch ( f->role ) {
	case ROLE_GAMEPLAY:
		IN_TouchStep( event, f->id, TOUCH_TARGET_IGNORE, 0, f->x, f->y );
		break;
	case ROLE_UITAP:
		Sys_QueEvent( 0, SE_KEY, A_MOUSE1, qfalse, 0, NULL );
		break;
	default:
		break;
	}
	f->role = ROLE_NONE;
}

static void IN_TouchCancelForceFingers(void) {
	CL_TouchForceCancel();
	for(int i=0;i<MAX_TOUCH_FINGERS;++i) {
		if(fingers[i].role!=ROLE_GAMEPLAY) continue;
		for(int button=TB_FORCE;button<=TB_FORCE_SLOT4;++button)
			if((button==TB_FORCE || button>=TB_FORCE_SLOT1) && TouchGesture_HoldsButton(&gestures,fingers[i].id,button)) {
				IN_TouchRelease(&fingers[i]);break;
			}
	}
}
static void IN_TouchCancelUnavailableForce(void) {
	const touchForceReport_t report=IN_TouchGetForceReport();
	const int cancel=TouchForce_ShouldCancelPower(&forceBridge,lastForceReport,report,CL_TouchForcePower());
	lastForceReport=report;
	if(!cancel) return;
	IN_TouchCancelForceFingers();
}

static void IN_TouchSaveEdit( void ) {
	if ( !editState.active ) return;
	touchEditInput_t input = {}; input.event=TOUCH_EDIT_EXIT;
	TouchEdit_State(&editState,input);
	memset(&settingsState,0,sizeof(settingsState));
	char text[IOS_TOUCH_LAYOUT_TEXT];
	if ( in_touchLayout && TouchEdit_Serialize(&layoutOverrides,text,sizeof(text)) ) {
		Cvar_Set("in_touchLayout",text);
		layoutModification=in_touchLayout->modificationCount;
	}
}

void IN_TouchReleaseAll( void ) {
	CL_TouchCancelAllSources();
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
	IN_TouchSyncAim();
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
	IN_TouchCancelUnavailableForce();
	if ( visibility.logMissing )
		Com_Printf( "Touch controls: no cgame state for 1000ms; using full controls\n" );
	if ( !editState.active ) IN_TouchStep( TOUCH_REFRESH, 0, TOUCH_TARGET_IGNORE, 0, 0, 0 );
}

void IN_TouchResetState( void ) {
	CL_TouchForceReset();
	TouchForce_Reset(&forceBridge);
	lastForceReport=TouchForce_Empty();
	SCR_ResetTouchForceMedia();
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

static void IN_TouchResetPositions( void ) {
	memset(&layoutOverrides,0,sizeof(layoutOverrides));
	layoutOverrides.version=2;
	Cvar_Set( "in_touchLayout", "" );
	layoutModification=in_touchLayout->modificationCount;
	if(layoutReady) TouchLayout_Build((float)viewW,(float)viewH,safeL,safeT,safeR,safeB,pixelsPerPoint,rects,&layoutOverrides,layoutProfile);
}

static void IN_TouchLayoutReset( void ) {
	IN_TouchReleaseAll();
	IN_GamepadZeroTouchAxes();
	if(touchWindow) IN_TouchUpdateLayout();
	IN_TouchResetPositions();
}

void IN_TouchInit( SDL_Window *window ) {
	in_touchForceSlots=Cvar_Get("in_touchForceSlots","v1:1,2,3,4",CVAR_ARCHIVE);
	IN_TouchSyncForceSlots();
	touchAimMode=Cvar_Get("in_touchAimMode","0",CVAR_ARCHIVE);
	touchAimCurve=Cvar_Get("in_touchAimCurve","2",CVAR_ARCHIVE);
	touchLookScale=Cvar_Get("in_touchLookScale","1.0",CVAR_ARCHIVE);
	touchDragScale=Cvar_Get("in_touchDragScale","1.0",CVAR_ARCHIVE);
	IN_TouchSyncAim();
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
			target = hit == TB_ATTACK || hit == TB_ALTATTACK || hit == TB_FORCE || hit == TB_CROUCH || hit == TB_JUMP ? TOUCH_TARGET_FIRE : TOUCH_TARGET_BUTTON;
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
	touchSettingsLayout_t panel;
	IN_TouchSettingsForce();
	TouchSettings_Build(&settingsState,IN_TouchGetOpacity(),(float)viewW,(float)viewH,safeL,safeT,safeR,safeB,pixelsPerPoint,&panel);
	const int panelEvent=event==TOUCH_EDIT_DOWN?TOUCH_SETTINGS_DOWN:event==TOUCH_EDIT_MOVE?TOUCH_SETTINGS_MOVE:TOUCH_SETTINGS_UP;
	const touchSettingsAction_t action=TouchSettings_Step(&settingsState,&panel,panelEvent,tf->fingerId,tf->x*viewW,tf->y*viewH);
	if(action.cancelDrag) { editState.dragging=0; editState.button=-1; }
	if(action.assign) {
		IN_TouchCancelForceFingers();
		char text[64];
		TouchForceSlots_Assign(&forceSlots,action.slot,action.power,IN_TouchGetForceReport().learned);
		TouchForceSlots_Save(&forceSlots,text,sizeof(text));Cvar_Set("in_touchForceSlots",text);
		forceSlotsModification=in_touchForceSlots->modificationCount;
	}
	if(action.aimSetting) {
		const char *name=action.aimSetting==1?"in_touchAimMode":action.aimSetting==2?(action.aimMode?"in_touchDragScale":"in_touchLookScale"):"in_touchAimCurve";
		Cvar_SetValue(name,action.aimValue);
		IN_TouchSyncAim();
	}
	if(action.setOpacity) Cvar_SetValue("in_touchOpacity",action.opacity);
	if(action.resetLayout) {
		editState.dragging=0;editState.button=-1;
		CL_TouchCancelAllSources();
		for(int i=0;i<MAX_TOUCH_FINGERS;++i) IN_TouchRelease(&fingers[i],TOUCH_CANCEL);
		IN_TouchStep(TOUCH_CANCEL_ALL,0,TOUCH_TARGET_IGNORE,0,0,0);
		IN_GamepadZeroTouchAxes();
		IN_TouchResetPositions();
		return;
	}
	if(action.done) { IN_TouchEndEdit(); return; }
	if(action.consumed) return;

	touchEditInput_t input = {};
	input.event=event; input.finger=tf->fingerId; input.x=tf->x*viewW; input.y=tf->y*viewH;
	input.button=TouchLayout_Hit(rects,input.x,input.y,IOS_TOUCH_ALL_BUTTONS);
	if(TouchLayout_Hit(rects,input.x,input.y,1u<<TB_LOCK)==TB_LOCK) input.button=TB_LOCK;
	input.slop=6*pixelsPerPoint;
	if(input.button>=0) {
		const touchRect_t *r=&rects[input.button]; input.cx=r->x+r->w/2; input.cy=r->y+r->h/2;
	}
	const touchEditOutput_t out=TouchEdit_State(&editState,input);
	if(out.tap) {
		settingsState.tab=TOUCH_SETTINGS_FORCE;settingsState.picker=out.button-TB_FORCE_SLOT1+1;
		settingsState.dropdown=0;settingsState.scroll=0;
	}
	if(out.move && TouchEdit_Drag(&layoutOverrides,out.button,out.x,out.y,(float)viewW,(float)viewH,safeL,safeT,safeR,safeB,pixelsPerPoint,layoutProfile))
		TouchLayout_Build((float)viewW,(float)viewH,safeL,safeT,safeR,safeB,pixelsPerPoint,rects,&layoutOverrides,layoutProfile);
	if(out.save) {
		char text[IOS_TOUCH_LAYOUT_TEXT];
		if(TouchEdit_Serialize(&layoutOverrides,text,sizeof(text))) Cvar_Set("in_touchLayout",text);
		layoutModification=in_touchLayout->modificationCount;
		memset(&settingsState,0,sizeof(settingsState));
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
			IN_TouchRelease( f, TOUCH_UP );
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
	touchGestureState_t drawingGestures=gestures;
	if(touchAimMode && touchAimMode->integer==1) drawingGestures.lookOwner=-1;
	input.gestures = &drawingGestures;
	input.visibleButtons = IN_TouchVisibleButtons();
	input.sticks = visibility.sticks;
	const touchForceReport_t report=IN_TouchGetForceReport();
	input.selectedForce = cls.state==CA_ACTIVE && !editState.active ? report.selected : 0;
	for(int i=0;i<4;++i) input.quickForces[i]=TouchForceSlots_Visible(&forceSlots,i,report.learned);
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

int IN_TouchGetSettingsDrawList( touchSettingsItem_t *items, int capacity, float *pointScale ) {
	if(!editState.active || !layoutReady || !items || capacity<=0) return 0;
	touchSettingsLayout_t panel;
	IN_TouchSettingsForce();
	TouchSettings_Build(&settingsState,IN_TouchGetOpacity(),(float)viewW,(float)viewH,safeL,safeT,safeR,safeB,pixelsPerPoint,&panel);
	const int count=panel.count<capacity?panel.count:capacity;
	memcpy(items,panel.items,count*sizeof(*items));
	*pointScale=pixelsPerPoint;
	return count;
}
