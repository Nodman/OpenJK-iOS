#pragma once

#include "ios_touch_math.h"

#define IOS_TOUCH_GESTURE_FINGERS 10

typedef enum {
	TOUCH_DOWN, TOUCH_MOTION, TOUCH_UP, TOUCH_CANCEL, TOUCH_CANCEL_ALL, TOUCH_REFRESH
} touchGestureEvent_t;

typedef enum {
	TOUCH_TARGET_IGNORE, TOUCH_TARGET_MOVE, TOUCH_TARGET_AIM, TOUCH_TARGET_BUTTON, TOUCH_TARGET_FIRE
} touchGestureTarget_t;

typedef enum {
	TOUCH_IDLE, TOUCH_IGNORED, TOUCH_MOVE, TOUCH_AIM, TOUCH_BUTTON, TOUCH_FIRE_HELD, TOUCH_FIRE_AIM
} touchGestureRole_t;

typedef struct {
	long long id;
	int role, button, slopCrossed;
	float ox, oy, x, y, radius, slop;
} touchGestureFinger_t;

typedef struct {
	touchGestureFinger_t fingers[IOS_TOUCH_GESTURE_FINGERS];
	int moveOwner, lookOwner;
} touchGestureState_t;

typedef struct {
	int event, target, button;
	long long finger;
	float x, y, radius, slop;
	unsigned int visibleButtons;
	int moveEnabled, lookEnabled;
} touchGestureInput_t;

typedef struct { int button, down; } touchKeyAction_t;

typedef struct {
	touchKeyAction_t keys[IOS_TOUCH_GESTURE_FINGERS];
	int keyCount;
	int moveX, moveY, lookX, lookY;
} touchGestureActions_t;

IOS_TOUCH_INLINE void TouchGesture_Reset( touchGestureState_t *state )
{
	int i;
	for ( i = 0; i < IOS_TOUCH_GESTURE_FINGERS; ++i ) {
		touchGestureFinger_t empty = { 0 };
		state->fingers[i] = empty;
	}
	state->moveOwner = state->lookOwner = -1;
}

IOS_TOUCH_INLINE int TouchGesture_IsButton( int role )
{
	return role == TOUCH_BUTTON || role == TOUCH_FIRE_HELD || role == TOUCH_FIRE_AIM;
}

IOS_TOUCH_INLINE void TouchGesture_Release( touchGestureState_t *state, int index, touchGestureActions_t *out, int role )
{
	touchGestureFinger_t *f = &state->fingers[index];
	if ( TouchGesture_IsButton( f->role ) ) {
		out->keys[out->keyCount].button = f->button;
		out->keys[out->keyCount++].down = 0;
	}
	if ( state->moveOwner == index ) state->moveOwner = -1;
	if ( state->lookOwner == index ) state->lookOwner = -1;
	f->role = role;
}

IOS_TOUCH_INLINE touchGestureActions_t TouchGesture_Step( touchGestureState_t *state, touchGestureInput_t in )
{
	touchGestureActions_t out = { { { 0, 0 } }, 0, 0, 0, 0, 0 };
	int i, found = -1, freeSlot = -1;
	for ( i = 0; i < IOS_TOUCH_GESTURE_FINGERS; ++i ) {
		touchGestureFinger_t *f = &state->fingers[i];
		const int lostButton = TouchGesture_IsButton( f->role ) && !( in.visibleButtons & ( 1u << f->button ) );
		const int lostMove = f->role == TOUCH_MOVE && !in.moveEnabled;
		const int lostLook = ( f->role == TOUCH_AIM || f->role == TOUCH_FIRE_AIM ) && !in.lookEnabled;
		if ( in.event == TOUCH_CANCEL_ALL || lostButton || lostMove || lostLook )
			TouchGesture_Release( state, i, &out, in.event == TOUCH_CANCEL_ALL ? TOUCH_IDLE : TOUCH_IGNORED );
		if ( f->role != TOUCH_IDLE && f->id == in.finger ) found = i;
		if ( f->role == TOUCH_IDLE && freeSlot < 0 ) freeSlot = i;
	}
	if ( in.event == TOUCH_DOWN && found < 0 && freeSlot >= 0 ) {
		touchGestureFinger_t *f = &state->fingers[freeSlot];
		f->id = in.finger;
		f->role = TOUCH_IGNORED;
		f->button = in.button;
		f->slopCrossed = 0;
		f->ox = f->x = in.x;
		f->oy = f->y = in.y;
		f->radius = in.radius;
		f->slop = in.slop;
		if ( TouchStick_Finite( in.x ) && TouchStick_Finite( in.y ) ) {
			if ( in.target == TOUCH_TARGET_MOVE && in.moveEnabled && state->moveOwner < 0 ) {
				f->role = TOUCH_MOVE;
				state->moveOwner = freeSlot;
			} else if ( in.target == TOUCH_TARGET_AIM && in.lookEnabled && state->lookOwner < 0 ) {
				f->role = TOUCH_AIM;
				state->lookOwner = freeSlot;
			} else if ( ( in.target == TOUCH_TARGET_BUTTON || in.target == TOUCH_TARGET_FIRE ) &&
				in.button >= 0 && in.button < 32 && ( in.visibleButtons & ( 1u << in.button ) ) ) {
				int held = 0;
				for ( i = 0; i < IOS_TOUCH_GESTURE_FINGERS; ++i )
					if ( TouchGesture_IsButton( state->fingers[i].role ) && state->fingers[i].button == in.button ) held = 1;
				if ( !held ) {
					f->role = in.target == TOUCH_TARGET_FIRE ? TOUCH_FIRE_HELD : TOUCH_BUTTON;
					out.keys[out.keyCount].button = in.button;
					out.keys[out.keyCount++].down = 1;
				}
			}
		}
	} else if ( found >= 0 ) {
		touchGestureFinger_t *f = &state->fingers[found];
		if ( in.event == TOUCH_UP || in.event == TOUCH_CANCEL ) {
			TouchGesture_Release( state, found, &out, TOUCH_IDLE );
		} else if ( in.event == TOUCH_MOTION ) {
			f->x = in.x;
			f->y = in.y;
			if ( f->role == TOUCH_FIRE_HELD && !f->slopCrossed && TouchStick_Finite( in.x ) && TouchStick_Finite( in.y ) ) {
				const double dx = (double)in.x - f->ox, dy = (double)in.y - f->oy;
				if ( TouchStick_Finite( f->slop ) && f->slop >= 0 && dx * dx + dy * dy > (double)f->slop * f->slop ) {
					f->slopCrossed = 1;
					if ( in.lookEnabled && state->lookOwner < 0 ) {
						f->role = TOUCH_FIRE_AIM;
						state->lookOwner = found;
					}
				}
			}
		}
	}
	for ( i = 0; i < IOS_TOUCH_GESTURE_FINGERS; ++i ) {
		const touchGestureFinger_t *f = &state->fingers[i];
		float x, y;
		if ( state->moveOwner == i ) {
			TouchStick_Clamp( f->x - f->ox, f->y - f->oy, f->radius, &x, &y );
			out.moveX = TouchStick_Quantize( x );
			out.moveY = TouchStick_Quantize( y );
		}
		if ( state->lookOwner == i ) {
			TouchStick_Curve( f->x - f->ox, f->y - f->oy, f->radius, &x, &y );
			out.lookX = TouchStick_Quantize( x );
			out.lookY = TouchStick_Quantize( y );
		}
	}
	return out;
}
