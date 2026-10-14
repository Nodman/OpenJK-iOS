#pragma once

#include <math.h>

#if defined(_MSC_VER) && !defined(__cplusplus)
#define ASPECT_INLINE static __inline
#else
#define ASPECT_INLINE static inline
#endif

typedef enum {
	ASPECT_LEGACY,
	ASPECT_CORRECTED
} aspectBranch_t;

typedef struct {
	double x, y;
} aspectPoint_t;

typedef struct {
	double x, y, w, h;
} aspectRect_t;

typedef struct {
	aspectBranch_t branch;
	int W, H;
	int enabled;
	float minRatio;
	int safeL, safeT, safeR, safeB;
	double s, sx, sy;
	int ox, oy;
	double left, right, top, bottom;
	aspectRect_t fullRect;
	double leftTranslation, rightTranslation, topTranslation, bottomTranslation;
} aspectLayout_t;

ASPECT_INLINE int AspectLayout_IsIdentity( int W, int H )
{
	return W > 0 && H > 0 && (long long)W * 3 == (long long)H * 4;
}

ASPECT_INLINE aspectLayout_t AspectLayout_Build( int W, int H, int enabled, float minRatio,
	int L, int T, int R, int B )
{
	aspectLayout_t layout;
	layout.branch = ASPECT_LEGACY;
	layout.W = W;
	layout.H = H;
	layout.enabled = enabled;
	layout.minRatio = minRatio;
	layout.safeL = L;
	layout.safeT = T;
	layout.safeR = R;
	layout.safeB = B;
	layout.sx = W / 640.0;
	layout.sy = H / 480.0;
	layout.s = W > 0 && H > 0 ? fmin( layout.sx, layout.sy ) : 1.0;
	layout.ox = 0;
	layout.oy = 0;
	layout.left = 0;
	layout.right = 640;
	layout.top = 0;
	layout.bottom = 480;
	layout.fullRect.x = 0;
	layout.fullRect.y = 0;
	layout.fullRect.w = 640;
	layout.fullRect.h = 480;
	layout.leftTranslation = 0;
	layout.rightTranslation = 0;
	layout.topTranslation = 0;
	layout.bottomTranslation = 0;
	if ( W <= 0 || H <= 0 || !enabled || AspectLayout_IsIdentity( W, H ) ) {
		return layout;
	}
	if ( !( (long long)W * 1000 >= (long long)H * llround( (double)minRatio * 1000.0 ) ) ) {
		return layout;
	}
	layout.branch = ASPECT_CORRECTED;
	// Integer products keep half ties exact when the scale is not dyadic.
	if ( layout.sy <= layout.sx ) {
		layout.ox = (int)floor( (double)( (long long)W * 480 - (long long)H * 640 ) / 960 + 0.5 );
	} else {
		layout.oy = (int)floor( (double)( (long long)H * 640 - (long long)W * 480 ) / 1280 + 0.5 );
	}
	layout.left = -layout.ox / layout.s;
	layout.right = ( W - layout.ox ) / layout.s;
	layout.top = -layout.oy / layout.s;
	layout.bottom = ( H - layout.oy ) / layout.s;
	layout.fullRect.x = layout.left;
	layout.fullRect.y = layout.top;
	layout.fullRect.w = W / layout.s;
	layout.fullRect.h = H / layout.s;
	layout.leftTranslation = ( (double)L - layout.ox ) / layout.s;
	layout.rightTranslation = ( (double)W - R - 640 * layout.s - layout.ox ) / layout.s;
	layout.topTranslation = ( (double)T - layout.oy ) / layout.s;
	layout.bottomTranslation = ( (double)H - B - 480 * layout.s - layout.oy ) / layout.s;
	return layout;
}

ASPECT_INLINE int AspectLayout_GutterRects( const aspectLayout_t *layout, aspectRect_t out[4] )
{
	const aspectRect_t *full = &layout->fullRect;
	const double right = full->x + full->w;
	const double bottom = full->y + full->h;
	const double innerLeft = fmax( 0, full->x );
	const double innerRight = fmin( 640, right );
	const aspectRect_t gutters[4] = {
		{ full->x, full->y, fmin( 0, right ) - full->x, full->h },
		{ fmax( 640, full->x ), full->y, right - fmax( 640, full->x ), full->h },
		{ innerLeft, full->y, innerRight - innerLeft, fmin( 0, bottom ) - full->y },
		{ innerLeft, fmax( 480, full->y ), innerRight - innerLeft, bottom - fmax( 480, full->y ) }
	};
	int count = 0;
	int i;
	if ( layout->branch != ASPECT_CORRECTED ) {
		return 0;
	}
	for ( i = 0; i < 4; ++i ) {
		if ( gutters[i].w > 0 && gutters[i].h > 0 ) {
			out[count++] = gutters[i];
		}
	}
	return count;
}

ASPECT_INLINE aspectPoint_t AspectLayout_ToPixels( const aspectLayout_t *layout, double x, double y )
{
	aspectPoint_t point;
	point.x = layout->ox + x * ( layout->branch == ASPECT_CORRECTED ? layout->s : layout->sx );
	point.y = layout->oy + y * ( layout->branch == ASPECT_CORRECTED ? layout->s : layout->sy );
	return point;
}

ASPECT_INLINE aspectPoint_t AspectLayout_SizeToPixels( const aspectLayout_t *layout, double w, double h )
{
	aspectPoint_t size;
	size.x = w * ( layout->branch == ASPECT_CORRECTED ? layout->s : layout->sx );
	size.y = h * ( layout->branch == ASPECT_CORRECTED ? layout->s : layout->sy );
	return size;
}

ASPECT_INLINE aspectPoint_t AspectLayout_SizeToVirtual( const aspectLayout_t *layout, double w, double h )
{
	aspectPoint_t size;
	const double sx = layout->branch == ASPECT_CORRECTED ? layout->s : layout->sx;
	const double sy = layout->branch == ASPECT_CORRECTED ? layout->s : layout->sy;
	size.x = sx != 0 ? w / sx : 0;
	size.y = sy != 0 ? h / sy : 0;
	return size;
}

ASPECT_INLINE aspectPoint_t AspectLayout_ToVirtual( const aspectLayout_t *layout, double px, double py )
{
	aspectPoint_t point;
	const double sx = layout->branch == ASPECT_CORRECTED ? layout->s : layout->sx;
	const double sy = layout->branch == ASPECT_CORRECTED ? layout->s : layout->sy;
	point.x = sx != 0 ? ( px - layout->ox ) / sx : 0;
	point.y = sy != 0 ? ( py - layout->oy ) / sy : 0;
	return point;
}

ASPECT_INLINE aspectPoint_t AspectLayout_ToVirtualCursor( const aspectLayout_t *layout, double px, double py )
{
	aspectPoint_t point = AspectLayout_ToVirtual( layout, px, py );
	point.x = fmin( 640, fmax( 0, point.x ) );
	point.y = fmin( 480, fmax( 0, point.y ) );
	return point;
}

ASPECT_INLINE double AspectLayout_HorizontalFov( double h4, int refdefW, int refdefH )
{
	double radians, aspect;
	const double pi = 3.14159265358979323846;
	if ( refdefW <= 0 || refdefH <= 0 || AspectLayout_IsIdentity( refdefW, refdefH ) ) {
		return h4;
	}
	radians = h4 * pi / 180;
	aspect = (double)refdefW / refdefH;
	return 2 * atan( tan( radians / 2 ) * ( aspect / ( 4.0 / 3.0 ) ) ) * 180 / pi;
}

ASPECT_INLINE double AspectLayout_VerticalFov( double h, int refdefW, int refdefH )
{
	double radians, aspect;
	const double pi = 3.14159265358979323846;
	if ( refdefW <= 0 || refdefH <= 0 ) {
		return h;
	}
	radians = h * pi / 180;
	aspect = (double)refdefW / refdefH;
	return 2 * atan( tan( radians / 2 ) / aspect ) * 180 / pi;
}
