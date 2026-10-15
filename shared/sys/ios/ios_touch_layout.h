#pragma once

#include "qcommon/aspect_layout.h"
#include "ios_touch_geometry.h"

typedef enum {
	TB_ATTACK, TB_ALTATTACK, TB_JUMP, TB_CROUCH, TB_USE, TB_FORCE, TB_FORCENEXT,
	TB_STYLE, TB_WEAPPREV, TB_WEAPNEXT, TB_MENU, TB_CONSOLE, TB_SABER_SELECT, TB_CHAT, TB_PAD, TB_LOCK, TB_FORCEPREV, TB_FORCE_SLOT1, TB_FORCE_SLOT2, TB_FORCE_SLOT3, TB_FORCE_SLOT4, TB_COUNT
} touchButton_t;

typedef enum {
	TG_MENU, TG_CHAT, TG_PAD, TG_WEAPPREV, TG_SABER_SELECT, TG_WEAPNEXT, TG_RING, TG_FORCE,
	TG_CONSOLE, TG_ALTATTACK, TG_ATTACK, TG_JUMP, TG_CROUCH, TG_STYLE_BLUE, TG_USE, TG_PLATE, TG_LOCK, TG_GUN_ATTACK, TG_STYLE_RED, TG_RECT_PLATE
} touchGlyph_t;

typedef enum {
	TOUCH_KEY_NONE = 0, TOUCH_KEY_ESCAPE = -1, TOUCH_KEY_TAB = -2, TOUCH_KEY_CHAT = -3
} touchSpecialKey_t;

#define IOS_TOUCH_ALL_BUTTONS ( ( 1u << TB_COUNT ) - 1u )

typedef struct { float x, y, w, h; } touchRect_t;

IOS_TOUCH_INLINE float TouchLayout_ButtonScale( int button )
{
	switch ( button ) {
	case TB_SABER_SELECT: case TB_STYLE:
	case TB_FORCE_SLOT1: case TB_FORCE_SLOT2: case TB_FORCE_SLOT3: case TB_FORCE_SLOT4:
		return .75f;
	case TB_FORCENEXT: case TB_WEAPPREV: case TB_WEAPNEXT:
	case TB_MENU: case TB_CONSOLE: case TB_CHAT: case TB_PAD: case TB_LOCK:
	case TB_FORCEPREV:
		return .5f;
	default:
		return 1;
	}
}

IOS_TOUCH_INLINE void TouchLayout_CompactRect( int button, touchRect_t *rect )
{
	const float scale = TouchLayout_ButtonScale( button );
	const float w = rect->w * scale, h = rect->h * scale;
	// Shrink after the legacy clamp to keep saved edge positions centred.
	rect->x += ( rect->w - w ) / 2;
	rect->y += ( rect->h - h ) / 2;
	rect->w = w;
	rect->h = h;
}

typedef struct {
	const char *label;
	int anchor;
	float cx, cy, size;
	int glyph, key;
	const char *defaultBind;
} touchButtonDef_t;

extern const touchButtonDef_t touchButtonDefs[TB_COUNT];

IOS_TOUCH_INLINE const char *TouchLayout_DefaultBind( int button, const char *binding )
{
	if ( button < 0 || button >= TB_COUNT || ( binding && binding[0] ) )
		return 0;
	return touchButtonDefs[button].defaultBind;
}

struct touchLayoutOverrides_s;

void TouchLayout_Build( float viewW, float viewH, float safeL, float safeT, float safeR, float safeB, float p, touchRect_t out[TB_COUNT], const struct touchLayoutOverrides_s *overrides
#ifdef __cplusplus
 = 0
#endif
 );
int TouchLayout_Hit( const touchRect_t rects[TB_COUNT], float x, float y, unsigned int visibleButtons );
IOS_TOUCH_INLINE int TouchLayout_Key( int button, int aux16, int escape, int tab )
{
	int key;
	if ( button < 0 || button >= TB_COUNT )
		return 0;
	key = touchButtonDefs[button].key;
	switch ( key ) {
	case TOUCH_KEY_ESCAPE: return escape;
	case TOUCH_KEY_TAB: return tab;
	case TOUCH_KEY_CHAT: return 'y';
	case TOUCH_KEY_NONE: return 0;
	default: return aux16 + key - 16;
	}
}
void TouchLayout_Stick( float ox, float oy, float x, float y, float radius, float *outX, float *outY );
void TouchLayout_ToVirtual( const aspectLayout_t *layout, float nx, float ny, int *vx, int *vy );
#ifdef __cplusplus
touchRect_t TouchLayout_RectToVirtual( const aspectLayout_t *layout, const touchRect_t &rect );
#endif
