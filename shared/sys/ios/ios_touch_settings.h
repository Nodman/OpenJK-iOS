#pragma once
#include "ios_touch_layout.h"

IOS_TOUCH_INLINE touchRect_t TouchSettings_BarRect( float W, float H, float L, float T, float R, float B, float p )
{
	touchRect_t r = { 0, 0, 0, 0 };
	if ( TouchLayout_Unit(W-L-R,H-T-B,p)<=0 ) return r;
	r.w = fminf( 288*p, W-L-R ); r.h = fminf( 32*p, H-T-B );
	r.x = fmaxf( L, fminf( (W-r.w)/2, W-R-r.w ) );
	r.y = fmaxf( T, fminf( (H-r.h)/2, H-B-r.h ) );
	return r;
}
