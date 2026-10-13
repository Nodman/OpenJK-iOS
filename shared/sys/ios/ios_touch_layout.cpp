#include "ios_touch_layout.h"

#include <math.h>

#define TOUCH_MIN_LAYOUT_H 540.0f

const touchButtonDef_t touchButtonDefs[TB_COUNT] = {
	{ "ATK",   0.90f, 0.72f, 0.20f, 1 },
	{ "ALT",   0.78f, 0.82f, 0.15f, 1 },
	{ "JUMP",  0.90f, 0.45f, 0.15f, 1 },
	{ "DUCK",  0.78f, 0.58f, 0.13f, 1 },
	{ "USE",   0.66f, 0.82f, 0.13f, 1 },
	{ "FORCE", 0.78f, 0.33f, 0.13f, 1 },
	{ "F>>",   0.66f, 0.33f, 0.11f, 1 },
	{ "STYLE", 0.66f, 0.58f, 0.11f, 1 },
	{ "<W",    0.40f, 0.10f, 0.11f, 1 },
	{ "W>",    0.52f, 0.10f, 0.11f, 1 },
	{ "MENU",  0.05f, 0.08f, 0.11f, 1 },
	{ "CON",   0.95f, 0.08f, 0.11f, 0 }
};

void TouchLayout_Build( float viewW, float viewH, float safeL, float safeT, float safeR, float safeB, touchRect_t out[TB_COUNT] )
{
	float safeW = viewW - safeL - safeR;
	float safeH = viewH - safeT - safeB;
	float layoutH = fminf( fmaxf( safeH, TOUCH_MIN_LAYOUT_H ), safeW );
	for ( int i = 0; i < TB_COUNT; ++i ) {
		float s = touchButtonDefs[i].size * layoutH;
		float x = safeL + touchButtonDefs[i].cx * safeW - s / 2;
		float y = safeT + touchButtonDefs[i].cy * safeH - s / 2;
		out[i].x = fmaxf( safeL, fminf( x, viewW - safeR - s ) );
		out[i].y = fmaxf( safeT, fminf( y, viewH - safeB - s ) );
		out[i].w = out[i].h = s;
	}
}

int TouchLayout_Hit( const touchRect_t rects[TB_COUNT], float x, float y, int uiMode )
{
	for ( int i = 0; i < TB_COUNT; ++i ) {
		if ( uiMode && touchButtonDefs[i].gameplayOnly ) {
			continue;
		}
		if ( x >= rects[i].x && x <= rects[i].x + rects[i].w &&
			y >= rects[i].y && y <= rects[i].y + rects[i].h ) {
			return i;
		}
	}
	return -1;
}

void TouchLayout_Stick( float ox, float oy, float x, float y, float radius, float *outX, float *outY )
{
	if ( radius <= 0 ) {
		*outX = *outY = 0;
		return;
	}
	float dx = ( x - ox ) / radius;
	float dy = ( y - oy ) / radius;
	float length = sqrtf( dx * dx + dy * dy );
	if ( length > 1 ) {
		dx /= length;
		dy /= length;
	}
	*outX = dx;
	*outY = dy;
}

void TouchLayout_ToVirtual( float nx, float ny, int *vx, int *vy )
{
	// clamp before the cast: (int) of NaN or a huge float is undefined
	float cx = nx < 0 ? 0 : ( nx > 1 ? 1 : nx );
	float cy = ny < 0 ? 0 : ( ny > 1 ? 1 : ny );
	*vx = (int)( cx * 640 + 0.5f );
	*vy = (int)( cy * 480 + 0.5f );
}
