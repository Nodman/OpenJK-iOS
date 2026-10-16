#pragma once
#include "ios_touch_layout_edit.h"

IOS_TOUCH_INLINE int TouchEdit_LockVisible( int active, int hidden, int ui, int cinematic )
{
	return active && !hidden && !ui && !cinematic;
}

typedef enum { TOUCH_EDIT_ENTER, TOUCH_EDIT_EXIT, TOUCH_EDIT_DOWN, TOUCH_EDIT_MOVE, TOUCH_EDIT_UP } touchEditEvent_t;
typedef struct {
	int active, dragging, button, moved;
	long long finger;
	float ox, oy, dx, dy;
} touchEditState_t;
typedef struct {
	int event, button;
	long long finger;
	float x, y, cx, cy, slop;
} touchEditInput_t;
typedef struct { int release, zero, save, move, button, tap; float x, y; } touchEditOutput_t;

IOS_TOUCH_INLINE touchEditOutput_t TouchEdit_State( touchEditState_t *s, touchEditInput_t in )
{
	touchEditOutput_t out = {0,0,0,0,0,0,0,0};
	if ( in.event == TOUCH_EDIT_ENTER && !s->active ) {
		s->active=1; s->dragging=0; s->button=-1; out.release=out.zero=1;
	} else if ( in.event == TOUCH_EDIT_EXIT && s->active ) {
		s->active=s->dragging=0; s->button=-1; out.save=out.release=out.zero=1;
	} else if ( s->active ) {
		if ( in.event == TOUCH_EDIT_DOWN && !s->dragging && TouchEdit_Editable(in.button) ) {
			s->dragging=1; s->button=in.button; s->finger=in.finger; s->moved=0;
			s->ox=in.x; s->oy=in.y; s->dx=in.cx-in.x; s->dy=in.cy-in.y;
		} else if ( s->dragging && s->finger==in.finger ) {
			if ( in.event == TOUCH_EDIT_MOVE ) {
				const float x=in.x-s->ox,y=in.y-s->oy;
				if ( x*x+y*y > in.slop*in.slop ) s->moved=1;
				out.move=s->moved; out.button=s->button; out.x=in.x+s->dx; out.y=in.y+s->dy;
			} else if ( in.event == TOUCH_EDIT_UP ) {
				const float x=in.x-s->ox,y=in.y-s->oy;
				if(x*x+y*y>in.slop*in.slop) s->moved=1;
				if(!s->moved && in.button==s->button && s->button>=TB_FORCE_SLOT1 && s->button<=TB_FORCE_SLOT4) {
					out.tap=1;out.button=s->button;
				}
				if ( s->button==TB_LOCK && !s->moved && in.button==s->button ) {
					s->active=0; out.save=out.release=out.zero=1;
				}
				s->dragging=0; s->button=-1;
			}
		}
	}
	return out;
}
