#pragma once

#include "sys/ios/ios_touch_geometry.h"
#include "sys/ios/ios_touch_layout.h"

#define IOS_TOUCH_ATLAS_NAME "*ios_touch_v2"
#define IOS_TOUCH_ATLAS_SIZE 1024
#define IOS_TOUCH_ATLAS_CELL 256
#define IOS_TOUCH_ATLAS_INSET 8

typedef struct { float s1, t1, s2, t2; } touchAtlasUV_t;

IOS_TOUCH_INLINE touchAtlasUV_t TouchAtlas_UV( int glyph )
{
	touchAtlasUV_t uv = { 0, 0, 0, 0 };
	if ( glyph < 0 || glyph >= 16 )
		return uv;
	uv.s1 = ( ( glyph % 4 ) * 256 + 8.5f ) / 1024;
	uv.t1 = ( ( glyph / 4 ) * 256 + 8.5f ) / 1024;
	uv.s2 = ( ( glyph % 4 + 1 ) * 256 - 8.5f ) / 1024;
	uv.t2 = ( ( glyph / 4 + 1 ) * 256 - 8.5f ) / 1024;
	return uv;
}
