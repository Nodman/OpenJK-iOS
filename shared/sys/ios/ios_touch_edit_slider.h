#pragma once
#include "ios_touch_layout.h"

IOS_TOUCH_INLINE touchRect_t TouchEdit_SliderRect( float W,float H,float L,float T,float R,float B,float p )
{
	touchRect_t r={0,0,0,0};
	const float unit=TouchLayout_Unit(W-L-R,H-T-B,p);
	if(unit<=0) return r;
	r.w=.5f*(W-L-R); r.h=fminf(fmaxf(.12f*unit,44*p),fminf(r.w,H-T-B));
	r.x=L+.25f*(W-L-R); r.y=fmaxf(T,fminf(T+.42f*unit-r.h/2,H-B-r.h));
	return r;
}

IOS_TOUCH_INLINE float TouchEdit_SliderValue( float x,const touchRect_t *r )
{
	float f;
	if(!IOS_TouchFinite(x) || r->w<=r->h) return .2f;
	f=(x-r->x-r->h/2)/(r->w-r->h);
	return .2f+.8f*fmaxf(0,fminf(f,1));
}

IOS_TOUCH_INLINE float TouchEdit_SliderX( float value,const touchRect_t *r )
{
	if(!IOS_TouchFinite(value)) value=.2f;
	return r->x+r->h/2+(r->w-r->h)*fmaxf(0,fminf((value-.2f)/.8f,1));
}
