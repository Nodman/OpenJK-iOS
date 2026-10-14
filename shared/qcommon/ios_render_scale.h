#pragma once

#include <math.h>

#if defined(_MSC_VER) && !defined(__cplusplus)
#define IOS_RS_INLINE static __inline
#else
#define IOS_RS_INLINE static inline
#endif

typedef struct {
	float scale;
	int floorRaised;
	int width;
	int height;
} iosRenderScale_t;

IOS_RS_INLINE int IOS_RenderScale_HighRefresh( int maxfps )
{
	return maxfps > 60;
}

IOS_RS_INLINE float IOS_RenderScale_Clamp( float scale )
{
	return scale < 0.5f ? 0.5f : ( scale > 1.0f ? 1.0f : scale );
}

/* requested <= 0 is Auto; baseAuto excludes the high-refresh multiplier. */
IOS_RS_INLINE iosRenderScale_t IOS_RenderScale_Resolve( float requested, float baseAuto,
	int highRefresh, int nativeW, int nativeH )
{
	iosRenderScale_t result;
	result.scale = IOS_RenderScale_Clamp( requested > 0 ? requested : baseAuto * ( highRefresh ? 0.8f : 1.0f ) );
	result.floorRaised = 0;
	result.width = result.height = 0;
	if ( nativeW <= 0 || nativeH <= 0 ) {
		return result;
	}
	if ( nativeH * result.scale < 540.0f ) {
		result.scale = IOS_RenderScale_Clamp( 540.5f / nativeH );
		result.floorRaised = 1;
	}
	/* Predict UIKit drawable sizes by truncating positive pixel products. */
	result.width = (int)( nativeW * result.scale );
	result.height = (int)( nativeH * result.scale );
	return result;
}

IOS_RS_INLINE float IOS_RenderScale_ForPreset( int id )
{
	switch ( id ) {
	case 0: return 0;
	case 1: return 1.0f;
	case 2: return 0.75f;
	case 3: return 0.5f;
	default: return -1;
	}
}

IOS_RS_INLINE int IOS_RenderScale_PresetFromScale( float scale )
{
	int id;
	if ( scale <= 0 ) {
		return 0;
	}
	for ( id = 1; id < 4; ++id ) {
		if ( fabsf( scale - IOS_RenderScale_ForPreset( id ) ) <= 0.001f ) {
			return id;
		}
	}
	return 4;
}
