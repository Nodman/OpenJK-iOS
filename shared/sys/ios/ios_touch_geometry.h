#pragma once

#include <math.h>
#include <float.h>

#if defined(_MSC_VER) && !defined(__cplusplus)
#define IOS_TOUCH_INLINE static __inline
#else
#define IOS_TOUCH_INLINE static inline
#endif

#define IOS_TOUCH_MIN_LAYOUT_H 540.0f
#define IOS_TOUCH_CONSOLE_CX 0.95f
#define IOS_TOUCH_CONSOLE_CY 0.08f
#define IOS_TOUCH_CONSOLE_SIZE 0.11f

/* Touch layout inputs and outputs are drawable pixels, including the 540 px
 * minimum sizing height. UIKit safe insets are converted before this call. */
IOS_TOUCH_INLINE void IOS_TouchButtonRectPixels( float W, float H,
	float safeL, float safeT, float safeR, float safeB,
	float cx, float cy, float size, float *x, float *y, float *w, float *h )
{
	const float safeW = W - safeL - safeR;
	const float safeH = H - safeT - safeB;
	const float layoutH = fminf( fmaxf( safeH, IOS_TOUCH_MIN_LAYOUT_H ), safeW );
	const float s = size * layoutH;
	*x = fmaxf( safeL, fminf( safeL + cx * safeW - s / 2, W - safeR - s ) );
	*y = fmaxf( safeT, fminf( safeT + cy * safeH - s / 2, H - safeB - s ) );
	*w = *h = s;
}

IOS_TOUCH_INLINE void IOS_TouchConsoleRectPixels( float W, float H,
	float safeL, float safeT, float safeR, float safeB,
	float *x, float *y, float *w, float *h )
{
	IOS_TouchButtonRectPixels( W, H, safeL, safeT, safeR, safeB,
		IOS_TOUCH_CONSOLE_CX, IOS_TOUCH_CONSOLE_CY, IOS_TOUCH_CONSOLE_SIZE, x, y, w, h );
}

typedef enum {
	TOUCH_ANCHOR_TL, TOUCH_ANCHOR_TR, TOUCH_ANCHOR_BL, TOUCH_ANCHOR_BR
} touchAnchor_t;

typedef enum {
	TOUCH_ZONE_NONE, TOUCH_ZONE_MOVE, TOUCH_ZONE_AIM
} touchZone_t;

IOS_TOUCH_INLINE int IOS_TouchFinite( float value )
{
	return value >= -FLT_MAX && value <= FLT_MAX;
}

IOS_TOUCH_INLINE float TouchLayout_PixelsPerPoint( float W, float H, float pointW, float pointH )
{
	if ( !IOS_TouchFinite( W ) || !IOS_TouchFinite( H ) || !IOS_TouchFinite( pointW ) || !IOS_TouchFinite( pointH ) ||
		!( W > 0 && H > 0 && pointW > 0 && pointH > 0 ) )
		return 0;
	return fminf( W / pointW, H / pointH );
}

IOS_TOUCH_INLINE float TouchLayout_Unit( float safeW, float safeH, float p )
{
	if ( !IOS_TouchFinite( safeW ) || !IOS_TouchFinite( safeH ) || !IOS_TouchFinite( p ) || !( safeW > 0 && safeH > 0 && p > 0 ) )
		return 0;
	return fminf( fminf( fmaxf( safeH, IOS_TOUCH_MIN_LAYOUT_H ), safeW ), 440 * p );
}

IOS_TOUCH_INLINE float TouchLayout_FireSlop( float unit, float p )
{
	return fmaxf( .015f * unit, 6 * p );
}

IOS_TOUCH_INLINE void TouchLayout_AnchorRect( float W, float H,
	float L, float T, float R, float B, float p, int anchor,
	float cx, float cy, float size, float *x, float *y, float *w, float *h )
{
	const float safeW = W - L - R, safeH = H - T - B;
	const float unit = TouchLayout_Unit( safeW, safeH, p );
	float centerX, centerY, diameter;
	*x = *y = *w = *h = 0;
	if ( !( unit > 0 ) )
		return;
	centerX = anchor == TOUCH_ANCHOR_TR || anchor == TOUCH_ANCHOR_BR ? W - R - cx * unit : L + cx * unit;
	centerY = anchor == TOUCH_ANCHOR_BL || anchor == TOUCH_ANCHOR_BR ? H - B - cy * unit : T + cy * unit;
	diameter = fminf( fmaxf( size * unit, 44 * p ), fminf( safeW, safeH ) );
	*x = fmaxf( L, fminf( centerX - diameter / 2, W - R - diameter ) );
	*y = fmaxf( T, fminf( centerY - diameter / 2, H - B - diameter ) );
	*w = *h = diameter;
}

IOS_TOUCH_INLINE int TouchLayout_Zone( float W, float H, float L, float T, float R, float B, float x, float y )
{
	const float safeW = W - L - R, safeH = H - T - B;
	if ( !( safeW > 0 && safeH > 0 ) )
		return TOUCH_ZONE_NONE;
	// Insets only place the split; the zones reach the physical screen edges.
	if ( !( x >= 0 && x <= W && y >= 0 && y <= H ) )
		return TOUCH_ZONE_NONE;
	if ( x >= L + safeW * .5f )
		return TOUCH_ZONE_AIM;
	return y >= T + safeH * .32f ? TOUCH_ZONE_MOVE : TOUCH_ZONE_NONE;
}
