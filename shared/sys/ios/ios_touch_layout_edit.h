#pragma once
#include "ios_touch_layout.h"
#include "ios_touch_edit_slider.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct touchLayoutOverrides_s {
	unsigned int mask;
	float cx[TB_COUNT], cy[TB_COUNT];
} touchLayoutOverrides_t;

IOS_TOUCH_INLINE int TouchEdit_Editable( int button )
{
	return button >= 0 && button < TB_COUNT && button != TB_CONSOLE;
}

IOS_TOUCH_INLINE void TouchEdit_Parse( const char *text, touchLayoutOverrides_t *out )
{
	memset( out, 0, sizeof(*out) );
	while ( text && *text ) {
		const char *end = strchr( text, ';' );
		const size_t length = end ? (size_t)(end - text) : strlen(text);
		if ( length < 96 ) {
			char entry[96], *p, *next;
			long id;
			double x, y;
			memcpy( entry, text, length ); entry[length] = 0;
			id = strtol( entry, &p, 10 );
			if ( p != entry && *p++ == ':' && id >= 0 && id < TB_COUNT && TouchEdit_Editable((int)id) ) {
				x = strtod( p, &next );
				if ( next != p && *next++ == ',' ) {
					p = next; y = strtod( p, &next );
					if ( next != p && !*next && x >= 0 && x <= 1000 && y >= 0 && y <= 1000 ) {
						out->mask |= 1u << id; out->cx[id] = (float)x; out->cy[id] = (float)y;
					}
				}
			}
		}
		text = end ? end + 1 : NULL;
	}
}

IOS_TOUCH_INLINE int TouchEdit_Serialize( const touchLayoutOverrides_t *o, char *out, size_t capacity )
{
	int button;
	size_t used = 0;
	if ( !out || !capacity ) return 0;
	out[0] = 0;
	for ( button = 0; button < TB_COUNT; ++button ) {
		int x, y, n;
		if ( !TouchEdit_Editable(button) || !(o->mask & (1u << button)) ) continue;
		if ( !(o->cx[button] >= 0 && o->cx[button] <= 1000 && o->cy[button] >= 0 && o->cy[button] <= 1000) ) continue;
		x = (int)lroundf(o->cx[button] * 1000); y = (int)lroundf(o->cy[button] * 1000);
		n = snprintf( out + used, capacity - used, "%d:%d.%03d,%d.%03d;", button, x/1000, x%1000, y/1000, y%1000 );
		if ( n < 0 || (size_t)n >= capacity - used ) { out[0] = 0; return 0; }
		used += (size_t)n;
	}
	return 1;
}

IOS_TOUCH_INLINE int TouchEdit_ClearCenter( float x, float y, float half, const touchRect_t *r )
{
	return r->w <= 0 || r->h <= 0 || x < r->x-half || x > r->x+r->w+half ||
		y < r->y-half || y > r->y+r->h+half;
}

IOS_TOUCH_INLINE int TouchEdit_ClampCenter( int button, float *x, float *y,
	float W, float H, float L, float T, float R, float B, float p )
{
	const float unit = TouchLayout_Unit(W-L-R,H-T-B,p);
	float half;
	if ( !TouchEdit_Editable(button) || unit <= 0 || !IOS_TouchFinite(*x) || !IOS_TouchFinite(*y) ) return 0;
	half = fminf(fmaxf(touchButtonDefs[button].size * unit,44*p),fminf(W-L-R,H-T-B)) / 2;
	*x = fmaxf(L+half,fminf(*x,W-R-half));
	*y = fmaxf(T+half,fminf(*y,H-B-half));
	if ( button == TB_LOCK ) {
		touchRect_t blocked[2];
		float xs[7], ys[7], bestX = *x, bestY = *y;
		double bestDistance = 0;
		int i, j, found = 0;
		// One storage quantum keeps saved rounding outside the hit regions.
		const float gap = .001f * unit + .01f;
		IOS_TouchConsoleRectPixels(W,H,L,T,R,B,&blocked[0].x,&blocked[0].y,&blocked[0].w,&blocked[0].h);
		blocked[1] = TouchEdit_SliderRect(W,H,L,T,R,B,p);
		if ( TouchEdit_ClearCenter(*x,*y,half,&blocked[0]) && TouchEdit_ClearCenter(*x,*y,half,&blocked[1]) ) return 1;
		xs[0] = *x; xs[1] = L+half; xs[2] = W-R-half;
		ys[0] = *y; ys[1] = T+half; ys[2] = H-B-half;
		for ( i = 0; i < 2; ++i ) {
			xs[3+2*i] = blocked[i].x-half-gap;
			xs[4+2*i] = blocked[i].x+blocked[i].w+half+gap;
			ys[3+2*i] = blocked[i].y-half-gap;
			ys[4+2*i] = blocked[i].y+blocked[i].h+half+gap;
		}
		for ( i = 0; i < 7; ++i ) for ( j = 0; j < 7; ++j ) {
			const float cx = fmaxf(L+half,fminf(xs[i],W-R-half));
			const float cy = fmaxf(T+half,fminf(ys[j],H-B-half));
			const double dx = (double)cx-*x, dy = (double)cy-*y;
			const double distance = dx*dx+dy*dy;
			if ( TouchEdit_ClearCenter(cx,cy,half,&blocked[0]) && TouchEdit_ClearCenter(cx,cy,half,&blocked[1]) &&
				(!found || distance < bestDistance) ) {
				found = 1; bestDistance = distance; bestX = cx; bestY = cy;
			}
		}
		if ( !found ) return 0;
		*x = bestX; *y = bestY;
	}
	return 1;
}

IOS_TOUCH_INLINE int TouchEdit_Drag( touchLayoutOverrides_t *o, int button, float x, float y,
	float W, float H, float L, float T, float R, float B, float p )
{
	const float unit = TouchLayout_Unit(W-L-R,H-T-B,p);
	const touchButtonDef_t *d;
	if ( !TouchEdit_ClampCenter(button,&x,&y,W,H,L,T,R,B,p) ) return 0;
	d = &touchButtonDefs[button];
	o->cx[button] = (d->anchor == TOUCH_ANCHOR_TR || d->anchor == TOUCH_ANCHOR_BR ? W-R-x : x-L) / unit;
	o->cy[button] = (d->anchor == TOUCH_ANCHOR_BL || d->anchor == TOUCH_ANCHOR_BR ? H-B-y : y-T) / unit;
	o->mask |= 1u << button;
	return 1;
}

IOS_TOUCH_INLINE int TouchEdit_RepairLock( touchLayoutOverrides_t *o,
	float W, float H, float L, float T, float R, float B, float p )
{
	const float unit = TouchLayout_Unit(W-L-R,H-T-B,p);
	float x, y, oldX, oldY;
	if ( !o || !(o->mask & (1u<<TB_LOCK)) || unit <= 0 ) return 0;
	oldX = o->cx[TB_LOCK]; oldY = o->cy[TB_LOCK];
	x = W-R-oldX*unit; y = T+oldY*unit;
	if ( !TouchEdit_Drag(o,TB_LOCK,x,y,W,H,L,T,R,B,p) ) return 0;
	return o->cx[TB_LOCK] != oldX || o->cy[TB_LOCK] != oldY;
}

IOS_TOUCH_INLINE void TouchEdit_Apply( const touchLayoutOverrides_t *o, touchButtonDef_t defs[TB_COUNT],
	float W, float H, float L, float T, float R, float B, float p )
{
	int button;
	touchLayoutOverrides_t repaired;
	if ( o ) {
		repaired = *o;
		TouchEdit_RepairLock(&repaired,W,H,L,T,R,B,p);
		o = &repaired;
	}
	for ( button = 0; button < TB_COUNT; ++button ) {
		defs[button] = touchButtonDefs[button];
		if ( o && TouchEdit_Editable(button) && (o->mask & (1u << button)) ) {
			defs[button].cx = o->cx[button]; defs[button].cy = o->cy[button];
		}
	}
}
