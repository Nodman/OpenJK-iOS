#pragma once

#include "ios_touch_geometry.h"

#define IOS_TOUCH_STICK_RADIUS .12f
#define IOS_TOUCH_AIM_DEADZONE .08f
#define IOS_TOUCH_AIM_EXPONENT 1.6f

IOS_TOUCH_INLINE int TouchStick_Finite( float value )
{
	return value >= -FLT_MAX && value <= FLT_MAX;
}

IOS_TOUCH_INLINE void TouchStick_Response( float dx, float dy, float radius, int curve, float *x, float *y )
{
	double length, magnitude;
	float r;
	*x = *y = 0;
	if ( !TouchStick_Finite( dx ) || !TouchStick_Finite( dy ) || !TouchStick_Finite( radius ) || radius <= 0 )
		return;
	length = sqrt( (double)dx * dx + (double)dy * dy );
	if ( length <= 0 )
		return;
	r = (float)fmin( length / radius, 1 );
	magnitude = curve ? pow( fmin( fmax( ( r - IOS_TOUCH_AIM_DEADZONE ) / ( 1 - IOS_TOUCH_AIM_DEADZONE ), 0 ), 1 ), IOS_TOUCH_AIM_EXPONENT ) : fmin( r, 1 );
	*x = (float)( dx / length * magnitude );
	*y = (float)( dy / length * magnitude );
}

IOS_TOUCH_INLINE void TouchStick_Curve( float dx, float dy, float radius, float *x, float *y )
{
	TouchStick_Response( dx, dy, radius, 1, x, y );
}

IOS_TOUCH_INLINE void TouchStick_Clamp( float dx, float dy, float radius, float *x, float *y )
{
	TouchStick_Response( dx, dy, radius, 0, x, y );
}

IOS_TOUCH_INLINE int TouchStick_Quantize( float value )
{
	if ( !TouchStick_Finite( value ) )
		return 0;
	return (int)lroundf( fmaxf( -1, fminf( value, 1 ) ) * 127 );
}

typedef struct { int x, y, touch; } touchAxisPair_t;

IOS_TOUCH_INLINE touchAxisPair_t TouchAxes_Select( int padX, int padY, int touchX, int touchY )
{
	touchAxisPair_t pair;
	const int pad = padX != 0 || padY != 0;
	pair.x = pad ? padX : touchX;
	pair.y = pad ? padY : touchY;
	pair.touch = !pad && ( touchX != 0 || touchY != 0 );
	return pair;
}
