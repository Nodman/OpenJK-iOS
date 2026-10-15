#pragma once
#include "ios_touch_layout.h"
#include "ios_touch_settings.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define IOS_TOUCH_LAYOUT_TEXT 2048

typedef struct touchLayoutOverrides_s {
	unsigned int mask;
	int version;
	float cx[TB_COUNT], cy[TB_COUNT];
} touchLayoutOverrides_t;

IOS_TOUCH_INLINE int TouchEdit_Editable( int button ) { return button >= 0 && button < TB_COUNT; }
IOS_TOUCH_INLINE int TouchLegacy_Editable( int button ) { return button >= 0 && button < 16 && button != TB_CONSOLE; }

IOS_TOUCH_INLINE void TouchEdit_Parse( const char *text, touchLayoutOverrides_t *out )
{
	memset( out, 0, sizeof(*out) );
	out->version=1;
	if(text && !strncmp(text,"v2;",3)) { out->version=2; text+=3; }
	else if(text && text[0]=='v') return;
	while ( text && *text ) {
		const char *end = strchr( text, ';' );
		const size_t length = end ? (size_t)(end - text) : strlen(text);
		if ( length < 96 ) {
			char entry[96], *p, *next;
			long id;
			double x, y;
			memcpy( entry, text, length ); entry[length] = 0;
			id = strtol( entry, &p, 10 );
			if ( p != entry && *p++ == ':' && id >= 0 && id < TB_COUNT && (out->version==2 ? TouchEdit_Editable((int)id) : TouchLegacy_Editable((int)id)) ) {
				x = strtod( p, &next );
				if ( next != p && *next++ == ',' ) {
					p = next; y = strtod( p, &next );
					if ( next != p && !*next && x >= 0 && x <= (out->version==2?1:1000) && y >= 0 && y <= (out->version==2?1:1000) ) {
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
	if(o->version==2) { if(capacity<4) return 0; memcpy(out,"v2;",4); used=3; }
	for ( button = 0; button < TB_COUNT; ++button ) {
		int x, y, n;
		const int precision=o->version==2?1000000:1000;
		if ( !TouchEdit_Editable(button) || !(o->mask & (1u << button)) ) continue;
		if ( !(o->cx[button] >= 0 && o->cx[button] <= (o->version==2?1:1000) && o->cy[button] >= 0 && o->cy[button] <= (o->version==2?1:1000)) ) continue;
		x = (int)lroundf(o->cx[button] * precision); y = (int)lroundf(o->cy[button] * precision);
		n = snprintf( out + used, capacity - used, o->version==2?"%d:%d.%06d,%d.%06d;":"%d:%d.%03d,%d.%03d;", button, x/precision, x%precision, y/precision, y%precision );
		if ( n < 0 || (size_t)n >= capacity - used ) { out[0] = 0; return 0; }
		used += (size_t)n;
	}
	return 1;
}

IOS_TOUCH_INLINE touchRect_t TouchLegacy_SliderRect( float W,float H,float L,float T,float R,float B,float p )
{
	touchRect_t r={0,0,0,0};
	const float unit=TouchLayout_Unit(W-L-R,H-T-B,p);
	if(unit<=0) return r;
	r.w=.5f*(W-L-R); r.h=fminf(fmaxf(.12f*unit,44*p),fminf(r.w,H-T-B));
	r.x=L+.25f*(W-L-R); r.y=fmaxf(T,fminf(T+.42f*unit-r.h/2,H-B-r.h));
	return r;
}

IOS_TOUCH_INLINE int TouchLegacy_ClearCenter( float x, float y, float half, const touchRect_t *r )
{
	return r->w <= 0 || r->h <= 0 || x < r->x-half || x > r->x+r->w+half ||
		y < r->y-half || y > r->y+r->h+half;
}

IOS_TOUCH_INLINE int TouchLegacy_ClampCenter( int button, float *x, float *y,
	float W, float H, float L, float T, float R, float B, float p )
{
	const float unit = TouchLayout_Unit(W-L-R,H-T-B,p);
	float half;
	if ( !TouchLegacy_Editable(button) || unit <= 0 || !IOS_TouchFinite(*x) || !IOS_TouchFinite(*y) ) return 0;
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
		blocked[1] = TouchLegacy_SliderRect(W,H,L,T,R,B,p);
		if ( TouchLegacy_ClearCenter(*x,*y,half,&blocked[0]) && TouchLegacy_ClearCenter(*x,*y,half,&blocked[1]) ) return 1;
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
			if ( TouchLegacy_ClearCenter(cx,cy,half,&blocked[0]) && TouchLegacy_ClearCenter(cx,cy,half,&blocked[1]) &&
				(!found || distance < bestDistance) ) {
				found = 1; bestDistance = distance; bestX = cx; bestY = cy;
			}
		}
		if ( !found ) return 0;
		*x = bestX; *y = bestY;
	}
	return 1;
}

IOS_TOUCH_INLINE int TouchLegacy_Drag( touchLayoutOverrides_t *o, int button, float x, float y,
	float W, float H, float L, float T, float R, float B, float p )
{
	const float unit = TouchLayout_Unit(W-L-R,H-T-B,p);
	const touchButtonDef_t *d;
	if ( !TouchLegacy_ClampCenter(button,&x,&y,W,H,L,T,R,B,p) ) return 0;
	d = &touchButtonDefs[button];
	o->cx[button] = (d->anchor == TOUCH_ANCHOR_TR || d->anchor == TOUCH_ANCHOR_BR ? W-R-x : x-L) / unit;
	o->cy[button] = (d->anchor == TOUCH_ANCHOR_BL || d->anchor == TOUCH_ANCHOR_BR ? H-B-y : y-T) / unit;
	o->mask |= 1u << button;
	return 1;
}

IOS_TOUCH_INLINE int TouchLegacy_RepairLock( touchLayoutOverrides_t *o,
	float W, float H, float L, float T, float R, float B, float p )
{
	const float unit = TouchLayout_Unit(W-L-R,H-T-B,p);
	float x, y, oldX, oldY;
	if ( !o || !(o->mask & (1u<<TB_LOCK)) || unit <= 0 ) return 0;
	oldX = o->cx[TB_LOCK]; oldY = o->cy[TB_LOCK];
	x = W-R-oldX*unit; y = T+oldY*unit;
	if ( !TouchLegacy_Drag(o,TB_LOCK,x,y,W,H,L,T,R,B,p) ) return 0;
	return o->cx[TB_LOCK] != oldX || o->cy[TB_LOCK] != oldY;
}

IOS_TOUCH_INLINE void TouchLegacy_Apply( const touchLayoutOverrides_t *o, touchButtonDef_t defs[TB_COUNT],
	float W, float H, float L, float T, float R, float B, float p )
{
	int button;
	touchLayoutOverrides_t repaired;
	if ( o ) {
		repaired = *o;
		TouchLegacy_RepairLock(&repaired,W,H,L,T,R,B,p);
		o = &repaired;
	}
	for ( button = 0; button < TB_COUNT; ++button ) {
		defs[button] = touchButtonDefs[button];
		if ( o && TouchLegacy_Editable(button) && (o->mask & (1u << button)) ) {
			defs[button].cx = o->cx[button]; defs[button].cy = o->cy[button];
		}
	}
}

IOS_TOUCH_INLINE void TouchEdit_DefaultRect( int button, float W, float H, float L, float T, float R, float B, float p, touchRect_t *out )
{
	const touchButtonDef_t *d=&touchButtonDefs[button];
	memset(out,0,sizeof(*out));
	if(TouchLayout_Unit(W-L-R,H-T-B,p)<=0) return;
	if(button==TB_CONSOLE) IOS_TouchConsoleRectPixels(W,H,L,T,R,B,&out->x,&out->y,&out->w,&out->h);
	else TouchLayout_AnchorRect(W,H,L,T,R,B,p,d->anchor,d->cx,d->cy,button==TB_FORCE?touchButtonDefs[TB_ATTACK].size:d->size,&out->x,&out->y,&out->w,&out->h);
	TouchLayout_CompactRect(button,out);
}

IOS_TOUCH_INLINE int TouchEdit_Migrate( touchLayoutOverrides_t *o, float W, float H, float L, float T, float R, float B, float p )
{
	int button;
	touchButtonDef_t defs[TB_COUNT];
	if(!o || o->version==2 || TouchLayout_Unit(W-L-R,H-T-B,p)<=0) return 0;
	o->mask &= ((1u<<16)-1u) & ~(1u<<TB_CONSOLE);
	TouchLegacy_Apply(o,defs,W,H,L,T,R,B,p);
	for(button=0;button<16;++button) if(o->mask&(1u<<button)) {
		touchRect_t old;
		const touchButtonDef_t *d=&defs[button];
		TouchLayout_AnchorRect(W,H,L,T,R,B,p,d->anchor,d->cx,d->cy,d->size,&old.x,&old.y,&old.w,&old.h);
		o->cx[button]=(old.x+old.w/2)/W; o->cy[button]=(old.y+old.h/2)/H;
	}
	o->version=2;
	return o->mask!=0;
}

IOS_TOUCH_INLINE void TouchEdit_PositionRect( const touchLayoutOverrides_t *o, int button,
	float W,float H,float L,float T,float R,float B,float p,touchRect_t *rect )
{
	TouchEdit_DefaultRect(button,W,H,L,T,R,B,p,rect);
	if(o && o->version==2 && (o->mask&(1u<<button))) {
		rect->x=fmaxf(0,fminf(o->cx[button]*W-rect->w/2,W-rect->w));
		rect->y=fmaxf(0,fminf(o->cy[button]*H-rect->h/2,H-rect->h));
	}
}

IOS_TOUCH_INLINE int TouchEdit_RepairLock( touchLayoutOverrides_t *o, float W,float H,float L,float T,float R,float B,float p )
{
	touchRect_t lock,blocked[2];
	float xs[7],ys[7],half,x,y,bestX,bestY,gap;
	double bestDistance=0;
	int i,j,found=0;
	if(!o || o->version!=2 || TouchLayout_Unit(W-L-R,H-T-B,p)<=0) return 0;
	TouchEdit_PositionRect(o,TB_LOCK,W,H,L,T,R,B,p,&lock);
	TouchEdit_PositionRect(o,TB_CONSOLE,W,H,L,T,R,B,p,&blocked[0]);
	blocked[1]=TouchSettings_BarRect(W,H,L,T,R,B,p);
	half=lock.w/2; x=bestX=lock.x+half; y=bestY=lock.y+half;
	if(TouchLegacy_ClearCenter(x,y,half,&blocked[0]) && TouchLegacy_ClearCenter(x,y,half,&blocked[1])) return 0;
	gap=.000001f*fmaxf(W,H)+.01f;
	xs[0]=x; xs[1]=half; xs[2]=W-half;
	ys[0]=y; ys[1]=half; ys[2]=H-half;
	for(i=0;i<2;++i) {
		xs[3+2*i]=blocked[i].x-half-gap; xs[4+2*i]=blocked[i].x+blocked[i].w+half+gap;
		ys[3+2*i]=blocked[i].y-half-gap; ys[4+2*i]=blocked[i].y+blocked[i].h+half+gap;
	}
	for(i=0;i<7;++i) for(j=0;j<7;++j) {
		const float cx=fmaxf(half,fminf(xs[i],W-half)),cy=fmaxf(half,fminf(ys[j],H-half));
		const double distance=(double)(cx-x)*(cx-x)+(double)(cy-y)*(cy-y);
		if(TouchLegacy_ClearCenter(cx,cy,half,&blocked[0]) && TouchLegacy_ClearCenter(cx,cy,half,&blocked[1]) && (!found || distance<bestDistance)) {
			found=1; bestDistance=distance; bestX=cx; bestY=cy;
		}
	}
	if(!found) return 0;
	o->mask|=1u<<TB_LOCK; o->cx[TB_LOCK]=bestX/W; o->cy[TB_LOCK]=bestY/H;
	return 1;
}

IOS_TOUCH_INLINE int TouchEdit_ClampCenter( int button,float *x,float *y,float W,float H,float L,float T,float R,float B,float p )
{
	touchRect_t r;
	if(!TouchEdit_Editable(button) || !IOS_TouchFinite(*x) || !IOS_TouchFinite(*y)) return 0;
	TouchEdit_DefaultRect(button,W,H,L,T,R,B,p,&r);
	if(r.w<=0) return 0;
	*x=fmaxf(r.w/2,fminf(*x,W-r.w/2)); *y=fmaxf(r.h/2,fminf(*y,H-r.h/2));
	return 1;
}

IOS_TOUCH_INLINE int TouchEdit_Drag( touchLayoutOverrides_t *o,int button,float x,float y,float W,float H,float L,float T,float R,float B,float p )
{
	if(!o || !TouchEdit_ClampCenter(button,&x,&y,W,H,L,T,R,B,p)) return 0;
	TouchEdit_Migrate(o,W,H,L,T,R,B,p);
	o->version=2; o->mask|=1u<<button; o->cx[button]=x/W; o->cy[button]=y/H;
	TouchEdit_RepairLock(o,W,H,L,T,R,B,p);
	return 1;
}
