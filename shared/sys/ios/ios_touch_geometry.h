#pragma once

#include <math.h>

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
