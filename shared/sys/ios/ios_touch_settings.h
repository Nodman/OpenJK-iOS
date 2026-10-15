#pragma once
#include "ios_touch_layout.h"
#include "ios_touch_force_slots.h"
#include <stdio.h>
#include <string.h>

#define IOS_TOUCH_SETTINGS_ITEMS 40
#define IOS_TOUCH_SETTINGS_FINGERS 10

typedef enum { TOUCH_SETTINGS_DOWN, TOUCH_SETTINGS_MOVE, TOUCH_SETTINGS_UP } touchSettingsEvent_t;
typedef enum { TOUCH_SETTINGS_NONE, TOUCH_SETTINGS_APPEARANCE, TOUCH_SETTINGS_AIM, TOUCH_SETTINGS_FORCE, TOUCH_SETTINGS_DONE,
	TOUCH_SETTINGS_SLOT = 40, TOUCH_SETTINGS_POWER = 50, TOUCH_SETTINGS_OPACITY = 10, TOUCH_SETTINGS_PERCENT = 20 } touchSettingsControl_t;
typedef struct { touchRect_t rect; int control, selected, force; char label[32]; } touchSettingsItem_t;
typedef struct {
	int picker, slots[4]; unsigned int learned;
	int tab, dropdown, capture[IOS_TOUCH_SETTINGS_FINGERS], control, moved;
	long long fingers[IOS_TOUCH_SETTINGS_FINGERS];
	float startX, startY, lastY, scroll;
} touchSettingsState_t;
typedef struct { touchRect_t bar, panel; touchSettingsItem_t items[IOS_TOUCH_SETTINGS_ITEMS]; int count; float maxScroll, p; } touchSettingsLayout_t;
typedef struct { int consumed, cancelDrag, done, opacity, assign, slot, power; } touchSettingsAction_t;

IOS_TOUCH_INLINE int TouchSettings_Contains( const touchRect_t *r, float x, float y )
{
	return r->w > 0 && r->h > 0 && x >= r->x && x <= r->x+r->w && y >= r->y && y <= r->y+r->h;
}

IOS_TOUCH_INLINE touchRect_t TouchSettings_BarRect( float W, float H, float L, float T, float R, float B, float p )
{
	touchRect_t r = { 0, 0, 0, 0 };
	if ( TouchLayout_Unit(W-L-R,H-T-B,p)<=0 ) return r;
	r.w = fminf( 288*p, W-L-R ); r.h = fminf( 32*p, H-T-B );
	r.x = fmaxf( L, fminf( (W-r.w)/2, W-R-r.w ) );
	r.y = fmaxf( T, fminf( (H-r.h)/2, H-B-r.h ) );
	return r;
}

IOS_TOUCH_INLINE void TouchSettings_Add( touchSettingsLayout_t *layout, touchRect_t rect, int control, int selected, const char *label )
{
	touchSettingsItem_t *item;
	if ( layout->count >= IOS_TOUCH_SETTINGS_ITEMS || rect.w <= 0 || rect.h <= 0 ) return;
	item = &layout->items[layout->count++];
	item->force=0;
	item->rect = rect; item->control = control; item->selected = selected;
	snprintf( item->label, sizeof(item->label), "%s", label );
}

IOS_TOUCH_INLINE void TouchSettings_Build( const touchSettingsState_t *state, float opacity,
	float W, float H, float L, float T, float R, float B, float p, touchSettingsLayout_t *out )
{
	static const char *tabs[4] = { "Appearance", "Aim", "Force", "Done" };
	float content, panelH, gap, groupH, scroll;
	int i;
	memset( out, 0, sizeof(*out) );
	out->p = p;
	out->bar = TouchSettings_BarRect( W,H,L,T,R,B,p );
	if ( out->bar.w <= 0 ) return;
	gap = state->tab ? fminf( 4*p, fmaxf( 0,H-T-B-out->bar.h ) ) : 0;
	content = state->tab ? 52*p : 0;
	if ( state->tab == TOUCH_SETTINGS_APPEARANCE && state->dropdown ) content += 132*p;
	if(state->tab==TOUCH_SETTINGS_FORCE) {
		int choices=1,id;
		for(id=1;id<=12;++id) if(state->learned & (1u<<id)) ++choices;
		content=(8+4*44+(state->picker?((choices+2)/3)*44+4:0))*p;
	}
	panelH = fminf( content, fmaxf( 0,H-T-B-out->bar.h-gap ) );
	groupH = out->bar.h + gap + panelH;
	out->bar.y = fmaxf( T, fminf( (H-groupH)/2, H-B-groupH ) );
	out->panel.x = out->bar.x; out->panel.y = out->bar.y + out->bar.h + gap;
	out->panel.w = out->bar.w; out->panel.h = panelH;
	out->maxScroll = fmaxf( 0,content-panelH );
	scroll = fmaxf( 0,fminf( state->scroll,out->maxScroll ) );
	TouchSettings_Add( out, out->panel, TOUCH_SETTINGS_NONE, 0, "" );
	for ( i=0;i<4;++i ) {
		touchRect_t r = { out->bar.x + i*out->bar.w/4, out->bar.y, out->bar.w/4, out->bar.h };
		TouchSettings_Add( out,r,i+1,state->tab==i+1,tabs[i] );
	}
	if ( state->tab == TOUCH_SETTINGS_APPEARANCE ) {
		char label[32];
		touchRect_t r = { out->panel.x+4*p, out->panel.y+4*p-scroll, out->panel.w-8*p, 44*p };
		if ( !(opacity >= 0) ) opacity=0;
		snprintf( label,sizeof(label),"Opacity %d%%",(int)lroundf(fminf(opacity,1)*100) );
		for ( i=-1;i<9;++i ) {
			touchRect_t clipped = r;
			int control=TOUCH_SETTINGS_OPACITY, selected=state->dropdown;
			if ( i >= 0 ) {
				if ( !state->dropdown ) break;
				r.x=out->panel.x+4*p+(i%3)*(out->panel.w-8*p)/3;
				r.y=out->panel.y+48*p+(i/3)*44*p-scroll;
				r.w=(out->panel.w-8*p)/3; r.h=44*p;
				clipped=r; control=TOUCH_SETTINGS_PERCENT+i;
				selected=fabsf(opacity-(i+2)*.1f)<.001f;
				snprintf(label,sizeof(label),"%d%%",20+i*10);
			}
			clipped.y=fmaxf(r.y,out->panel.y);
			clipped.h=fminf(r.y+r.h,out->panel.y+out->panel.h)-clipped.y;
			TouchSettings_Add(out,clipped,control,selected,clipped.h>=24*p?label:"");
		}
	}
	if(state->tab==TOUCH_SETTINGS_FORCE) {
		int index=0,id;
		for(i=0;i<4;++i) {
			char label[32];touchRect_t r={out->panel.x+4*p,out->panel.y+(4+i*44)*p-scroll,out->panel.w-8*p,44*p};
			const int power=state->slots[i];
			snprintf(label,sizeof(label),"Slot %d: %s",i+1,TouchForceSlots_Name(power));
			r.h=fminf(r.y+r.h,out->panel.y+out->panel.h)-fmaxf(r.y,out->panel.y);r.y=fmaxf(r.y,out->panel.y);
			const int before=out->count;
			TouchSettings_Add(out,r,TOUCH_SETTINGS_SLOT+i,state->picker==i+1,r.h>=24*p?label:"");
			if(out->count>before && r.h>=24*p) out->items[out->count-1].force=power;
		}
		if(state->picker) for(id=0;id<=12;++id) {
			touchRect_t r;
			if(id && !(state->learned & (1u<<id))) continue;
			r.x=out->panel.x+4*p+(index%3)*(out->panel.w-8*p)/3;
			r.y=out->panel.y+(184+(index/3)*44)*p-scroll;r.w=(out->panel.w-8*p)/3;r.h=44*p;++index;
			r.h=fminf(r.y+r.h,out->panel.y+out->panel.h)-fmaxf(r.y,out->panel.y);r.y=fmaxf(r.y,out->panel.y);
			const int before=out->count;
			TouchSettings_Add(out,r,TOUCH_SETTINGS_POWER+id,state->slots[state->picker-1]==id,r.h>=24*p?TouchForceSlots_Name(id):"");
			if(out->count>before && r.h>=24*p) out->items[out->count-1].force=id;
		}
	}
}

IOS_TOUCH_INLINE int TouchSettings_Hit( const touchSettingsLayout_t *layout, float x, float y )
{
	int i;
	for ( i=layout->count-1;i>=0;--i )
		if ( layout->items[i].control && TouchSettings_Contains(&layout->items[i].rect,x,y) ) return layout->items[i].control;
	return 0;
}

IOS_TOUCH_INLINE int TouchSettings_ModalContains( const touchSettingsLayout_t *layout, float x, float y )
{
	touchRect_t group=layout->bar;
	if(layout->panel.h>0) group.h=layout->panel.y+layout->panel.h-group.y;
	return TouchSettings_Contains(&group,x,y);
}

IOS_TOUCH_INLINE touchSettingsAction_t TouchSettings_Step( touchSettingsState_t *state,
	const touchSettingsLayout_t *layout, int event, long long finger, float x, float y )
{
	touchSettingsAction_t action = {0,0,0,0,0,0,0};
	int i,found=-1,freeSlot=-1,owned=0;
	for(i=0;i<IOS_TOUCH_SETTINGS_FINGERS;++i) {
		if(state->capture[i]) { if(state->fingers[i]==finger) found=i; ++owned; }
		else if(freeSlot<0) freeSlot=i;
	}
	if(event==TOUCH_SETTINGS_DOWN && found<0 &&
		(owned || TouchSettings_ModalContains(layout,x,y))) {
		action.consumed=action.cancelDrag=1;
		if(freeSlot<0) return action;
		state->capture[freeSlot]=owned?2:1; state->fingers[freeSlot]=finger;
		if(!owned) { state->control=TouchSettings_Hit(layout,x,y); state->startX=x; state->startY=state->lastY=y; state->moved=0; }
		return action;
	}
	if(found<0) return action;
	action.consumed=1;
	if(state->capture[found]==1) {
		if(event==TOUCH_SETTINGS_MOVE) {
			const float dx=x-state->startX,dy=y-state->startY;
			if(dx*dx+dy*dy>36*layout->p*layout->p) state->moved=1;
			if(state->moved && !TouchSettings_Contains(&layout->bar,state->startX,state->startY))
				state->scroll=fmaxf(0,fminf(state->scroll+state->lastY-y,layout->maxScroll));
			state->lastY=y;
		} else if(event==TOUCH_SETTINGS_UP && !state->moved && TouchSettings_Hit(layout,x,y)==state->control) {
			if(state->control>=1 && state->control<=3) { state->tab=state->tab==state->control?0:state->control; state->dropdown=state->picker=0; state->scroll=0; }
			else if(state->control>=TOUCH_SETTINGS_SLOT && state->control<TOUCH_SETTINGS_SLOT+4) {
				state->picker=state->picker==state->control-TOUCH_SETTINGS_SLOT+1?0:state->control-TOUCH_SETTINGS_SLOT+1;state->scroll=0;
			} else if(state->control>=TOUCH_SETTINGS_POWER && state->control<=TOUCH_SETTINGS_POWER+12 && state->picker) {
				action.assign=1;action.slot=state->picker-1;action.power=state->control-TOUCH_SETTINGS_POWER;state->picker=0;state->scroll=0;
			} else if(state->control==TOUCH_SETTINGS_DONE) action.done=1;
			else if(state->control==TOUCH_SETTINGS_OPACITY) { state->dropdown=!state->dropdown; state->scroll=0; }
			else if(state->control>=TOUCH_SETTINGS_PERCENT && state->control<TOUCH_SETTINGS_PERCENT+9) {
				action.opacity=20+(state->control-TOUCH_SETTINGS_PERCENT)*10; state->dropdown=0; state->scroll=0;
			}
		}
	}
	if(event==TOUCH_SETTINGS_UP) state->capture[found]=0;
	return action;
}
