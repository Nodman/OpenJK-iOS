#pragma once
#include "ios_touch_layout.h"
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

IOS_TOUCH_INLINE void TouchEdit_Apply( const touchLayoutOverrides_t *o, touchButtonDef_t defs[TB_COUNT] )
{
	int button;
	for ( button = 0; button < TB_COUNT; ++button ) {
		defs[button] = touchButtonDefs[button];
		if ( o && TouchEdit_Editable(button) && (o->mask & (1u << button)) ) {
			defs[button].cx = o->cx[button]; defs[button].cy = o->cy[button];
		}
	}
}

IOS_TOUCH_INLINE int TouchEdit_Drag( touchLayoutOverrides_t *o, int button, float x, float y,
	float W, float H, float L, float T, float R, float B, float p )
{
	const float unit = TouchLayout_Unit(W-L-R,H-T-B,p);
	const touchButtonDef_t *d;
	float half;
	if ( !TouchEdit_Editable(button) || unit <= 0 || !IOS_TouchFinite(x) || !IOS_TouchFinite(y) ) return 0;
	d = &touchButtonDefs[button];
	half = fminf(fmaxf(d->size * unit,44*p),fminf(W-L-R,H-T-B)) / 2;
	x = fmaxf(L+half,fminf(x,W-R-half)); y = fmaxf(T+half,fminf(y,H-B-half));
	o->cx[button] = (d->anchor == TOUCH_ANCHOR_TR || d->anchor == TOUCH_ANCHOR_BR ? W-R-x : x-L) / unit;
	o->cy[button] = (d->anchor == TOUCH_ANCHOR_BL || d->anchor == TOUCH_ANCHOR_BR ? H-B-y : y-T) / unit;
	o->mask |= 1u << button;
	return 1;
}
