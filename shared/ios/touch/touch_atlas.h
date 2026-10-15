#pragma once

#include "sys/ios/ios_touch_geometry.h"
#include "sys/ios/ios_touch_layout.h"
#include "sys/ios/ios_touch_edit_slider.h"

#define IOS_TOUCH_ATLAS_NAME "*ios_touch_v2"
#define IOS_TOUCH_ATLAS_WIDTH 1024
#define IOS_TOUCH_ATLAS_HEIGHT 1280
#define IOS_TOUCH_ATLAS_CELL 256
#define IOS_TOUCH_ATLAS_INSET 8

typedef struct { float s1, t1, s2, t2; } touchAtlasUV_t;

IOS_TOUCH_INLINE touchAtlasUV_t TouchAtlas_UV( int glyph )
{
	touchAtlasUV_t uv = { 0, 0, 0, 0 };
	if ( glyph < 0 || glyph >= 20 )
		return uv;
	uv.s1 = ( ( glyph % 4 ) * 256 + 8.5f ) / 1024;
	uv.t1 = ( ( glyph / 4 ) * 256 + 8.5f ) / IOS_TOUCH_ATLAS_HEIGHT;
	uv.s2 = ( ( glyph % 4 + 1 ) * 256 - 8.5f ) / 1024;
	uv.t2 = ( ( glyph / 4 + 1 ) * 256 - 8.5f ) / IOS_TOUCH_ATLAS_HEIGHT;
	return uv;
}

#include "sys/ios/ios_touch_state.h"
#include "sys/ios/ios_touch_visibility.h"

#define IOS_TOUCH_MAX_DRAW_ITEMS 32

typedef enum {
	TOUCH_DRAW_BUTTON, TOUCH_DRAW_STICK_BASE, TOUCH_DRAW_STICK_KNOB, TOUCH_DRAW_DIM, TOUCH_DRAW_HIGHLIGHT, TOUCH_DRAW_SLIDER_TRACK, TOUCH_DRAW_SLIDER_KNOB
} touchDrawKind_t;

typedef struct {
	float x, y, w, h;
	int kind, glyph, pressed;
	float rgb[3];
} touchDrawItem_t;

typedef struct {
	const aspectLayout_t *layout;
	const touchRect_t *rects;
	const touchGestureState_t *gestures;
	unsigned int visibleButtons, pressedButtons;
	int sticks, styleColor, weaponClass;
	int editing, dragButton;
	const touchRect_t *slider;
	float opacity;
} touchDrawInput_t;

IOS_TOUCH_INLINE int TouchDraw_WeaponGlyph( int button,int weaponClass )
{
	if(button<0 || button>=TB_COUNT) return TG_PLATE;
	if(button==TB_ATTACK || button==TB_ALTATTACK) {
		if(weaponClass==TOUCH_WEAPON_GUN) return button==TB_ATTACK?TG_GUN_ATTACK:TG_GUN_ALT;
		if(weaponClass==TOUCH_WEAPON_MELEE) return TG_MELEE;
	}
	return touchButtonDefs[button].glyph;
}

IOS_TOUCH_INLINE float TouchDraw_Opacity( float value )
{
	if ( !( value >= 0 ) ) return 0;
	return value > 1 ? 1 : value;
}

IOS_TOUCH_INLINE void TouchDraw_StyleRGB( int style, float rgb[3] )
{
	rgb[0] = rgb[1] = rgb[2] = 1;
	if ( style == TOUCH_STYLE_BLUE ) {
		rgb[0] = .567f;
		rgb[1] = .685f;
	} else if ( style == TOUCH_STYLE_YELLOW ) {
		rgb[2] = 0;
	} else if ( style == TOUCH_STYLE_RED ) {
		rgb[0] = .835f;
		rgb[1] = rgb[2] = .015f;
	}
}

IOS_TOUCH_INLINE void TouchDraw_Color( const touchDrawItem_t *item, int glyphLayer, float opacity, float rgba[4] )
{
	if ( item->kind == TOUCH_DRAW_DIM || item->kind == TOUCH_DRAW_HIGHLIGHT ) {
		rgba[0]=rgba[1]=rgba[2]=item->kind==TOUCH_DRAW_DIM ? 0.0f : 1.0f;
		rgba[3]=item->kind==TOUCH_DRAW_DIM ? .35f : .75f;
		return;
	}
	if (item->kind==TOUCH_DRAW_SLIDER_TRACK || item->kind==TOUCH_DRAW_SLIDER_KNOB) {
		rgba[0]=rgba[1]=rgba[2]=1; rgba[3]=item->kind==TOUCH_DRAW_SLIDER_TRACK ? .5f : 1.0f; return;
	}
	const int glyph = item->kind == TOUCH_DRAW_BUTTON && glyphLayer;
	int i;
	for ( i = 0; i < 3; ++i ) rgba[i] = glyph ? item->rgb[i] : 1;
	if ( item->kind == TOUCH_DRAW_BUTTON )
		rgba[3] = glyph ? ( item->pressed ? 1.0f : .85f ) : ( item->pressed ? .40f : .20f );
	else
		rgba[3] = item->kind == TOUCH_DRAW_STICK_KNOB ? .40f : .20f;
	rgba[3] *= TouchDraw_Opacity( opacity );
}

IOS_TOUCH_INLINE void TouchDraw_Add( const touchDrawInput_t *input, touchDrawItem_t *items, int *count, int capacity,
	float x, float y, float w, float h, int kind, int glyph, int pressed, int style )
{
	aspectPoint_t origin, size;
	touchDrawItem_t *item;
	if ( *count >= capacity || w <= 0 || h <= 0 || !IOS_TouchFinite( x ) || !IOS_TouchFinite( y ) || !IOS_TouchFinite( w ) || !IOS_TouchFinite( h ) )
		return;
	origin = AspectLayout_ToVirtual( input->layout, x, y );
	size = AspectLayout_SizeToVirtual( input->layout, w, h );
	item = &items[*count];
	item->x = (float)origin.x;
	item->y = (float)origin.y;
	item->w = (float)size.x;
	item->h = (float)size.y;
	item->kind = kind;
	item->glyph = glyph;
	item->pressed = pressed;
	TouchDraw_StyleRGB( style, item->rgb );
	++*count;
}

IOS_TOUCH_INLINE int TouchDraw_Build( const touchDrawInput_t *input, touchDrawItem_t *items, int capacity )
{
	int count = 0, button, i;
	if ( !input || !items || capacity <= 0 || !input->layout || !input->rects || !input->gestures || input->layout->W <= 0 || input->layout->H <= 0 )
		return 0;
	if ( capacity > IOS_TOUCH_MAX_DRAW_ITEMS ) capacity = IOS_TOUCH_MAX_DRAW_ITEMS;
	if ( input->editing )
		TouchDraw_Add(input,items,&count,capacity,0,0,(float)input->layout->W,(float)input->layout->H,TOUCH_DRAW_DIM,TG_PLATE,0,TOUCH_STYLE_UNKNOWN);
	for ( button = 0; button < TB_COUNT; ++button ) {
		const touchRect_t *r = &input->rects[button];
		int pressed = ( input->pressedButtons & ( 1u << button ) ) != 0;
		if ( !input->editing && !( input->visibleButtons & ( 1u << button ) ) ) continue;
		if ( input->editing && button==TB_LOCK ) pressed=1;
		if ( input->editing && button==input->dragButton )
			TouchDraw_Add(input,items,&count,capacity,r->x-.04f*r->w,r->y-.04f*r->h,1.08f*r->w,1.08f*r->h,TOUCH_DRAW_HIGHLIGHT,TG_PLATE,0,TOUCH_STYLE_UNKNOWN);
		for ( i = 0; i < IOS_TOUCH_GESTURE_FINGERS; ++i ) {
			const touchGestureFinger_t *f = &input->gestures->fingers[i];
			if ( TouchGesture_IsButton( f->role ) && f->button == button ) pressed = 1;
		}
		TouchDraw_Add( input, items, &count, capacity, r->x, r->y, r->w, r->h, TOUCH_DRAW_BUTTON,
			TouchDraw_WeaponGlyph(button,input->weaponClass), pressed, button == TB_STYLE ? input->styleColor : TOUCH_STYLE_UNKNOWN );
	}
	if (input->editing && input->slider) {
		const touchRect_t *r=input->slider;
		TouchDraw_Add(input,items,&count,capacity,r->x+r->h/2,r->y+.45f*r->h,r->w-r->h,.1f*r->h,TOUCH_DRAW_SLIDER_TRACK,TG_PLATE,0,TOUCH_STYLE_UNKNOWN);
		TouchDraw_Add(input,items,&count,capacity,TouchEdit_SliderX(input->opacity,r)-r->h/2,r->y,r->h,r->h,TOUCH_DRAW_SLIDER_KNOB,TG_PLATE,1,TOUCH_STYLE_UNKNOWN);
	}
	if ( input->sticks && !input->editing ) {
		const int owners[2] = { input->gestures->moveOwner, input->gestures->lookOwner };
		for ( i = 0; i < 2; ++i ) {
			const touchGestureFinger_t *f;
			float r, sx, sy;
			if ( owners[i] < 0 || owners[i] >= IOS_TOUCH_GESTURE_FINGERS ) continue;
			f = &input->gestures->fingers[owners[i]];
			r = f->radius;
			if ( !IOS_TouchFinite( r ) || r <= 0 ) continue;
			TouchStick_Clamp( f->x - f->ox, f->y - f->oy, r, &sx, &sy );
			TouchDraw_Add( input, items, &count, capacity, f->ox - r, f->oy - r, 2 * r, 2 * r,
				TOUCH_DRAW_STICK_BASE, TG_PLATE, 0, TOUCH_STYLE_UNKNOWN );
			TouchDraw_Add( input, items, &count, capacity, f->ox + sx * r - .3f * r, f->oy + sy * r - .3f * r, .6f * r, .6f * r,
				TOUCH_DRAW_STICK_KNOB, TG_PLATE, 1, TOUCH_STYLE_UNKNOWN );
		}
	}
	return count;
}
