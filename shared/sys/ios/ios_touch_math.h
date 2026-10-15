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

typedef struct {
	int pad[4], touch[4];
	int active, lookTouch, forceSend;
} touchAxesState_t;

typedef struct {
	int values[4];
	int send;
} touchAxesBatch_t;

IOS_TOUCH_INLINE touchAxesBatch_t TouchAxes_Send( touchAxesState_t *state )
{
	const touchAxisPair_t move = TouchAxes_Select( state->pad[0], state->pad[1], state->touch[0], state->touch[1] );
	const touchAxisPair_t look = TouchAxes_Select( state->pad[2], state->pad[3], state->touch[2], state->touch[3] );
	touchAxesBatch_t batch = { { move.x, move.y, look.x, look.y }, 0 };
	const int active = move.x || move.y || look.x || look.y;
	batch.send = active || state->active || state->forceSend;
	state->lookTouch = look.touch;
	state->active = active;
	state->forceSend = 0;
	return batch;
}

IOS_TOUCH_INLINE int TouchAxes_PadLookActive( const touchAxesState_t *state )
{
	return state->pad[2]!=0 || state->pad[3]!=0;
}

IOS_TOUCH_INLINE void TouchAxes_ZeroTouch( touchAxesState_t *state )
{
	int i;
	for(i=0;i<4;++i) state->touch[i]=0;
	state->lookTouch=0;
	state->forceSend=1;
}

IOS_TOUCH_INLINE void TouchAxes_Zero( touchAxesState_t *state )
{
	int i;
	for ( i = 0; i < 4; ++i ) state->pad[i] = state->touch[i] = 0;
	state->active = state->lookTouch = 0;
	state->forceSend = 1;
}

IOS_TOUCH_INLINE float TouchAxes_NormalizeLook( int value, int touch, float scale )
{
	return ( value / 127.0f ) * ( touch ? scale : 1.0f );
}
