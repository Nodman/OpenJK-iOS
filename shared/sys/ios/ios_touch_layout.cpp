#include "ios_touch_layout.h"
#include "ios_touch_geometry.h"
#include "ios_touch_layout_edit.h"

#include <math.h>

const touchButtonDef_t touchButtonDefs[TB_COUNT] = {
	{ "ATK",   TOUCH_ANCHOR_BR, .413f, .590f, .14f, TG_ATTACK, 16, "+attack" },
	{ "ALT",   TOUCH_ANCHOR_BR, .510f, .438f, .14f, TG_ALTATTACK, 17, "+altattack" },
	{ "JUMP",  TOUCH_ANCHOR_BR, .246f, .659f, .14f, TG_JUMP, 18, "+moveup" },
	{ "DUCK",  TOUCH_ANCHOR_BR, .40f, .08f, .12f, TG_CROUCH, 19, "+movedown" },
	{ "USE",   TOUCH_ANCHOR_BL, .12f, .08f, .12f, TG_USE, 20, "+use" },
	{ "FORCE", TOUCH_ANCHOR_TR, .49f, .26f, .12f, TG_FORCE, 21, "+useforce" },
	{ "F>>",   TOUCH_ANCHOR_TR, .41f, .26f, .12f, TG_FORCENEXT, 22, "forcenext" },
	{ "STYLE", TOUCH_ANCHOR_BR, .12f, .08f, .12f, TG_STYLE, 23, "saberAttackCycle" },
	{ "<W",    TOUCH_ANCHOR_TR, .65f, .10f, .12f, TG_WEAPPREV, 24, "weapprev" },
	{ "W>",    TOUCH_ANCHOR_TR, .33f, .10f, .12f, TG_WEAPNEXT, 25, "weapnext" },
	{ "MENU",  TOUCH_ANCHOR_TL, .12f, .10f, .12f, TG_MENU, TOUCH_KEY_ESCAPE, 0 },
	{ 0,    TOUCH_ANCHOR_TL, IOS_TOUCH_CONSOLE_CX, IOS_TOUCH_CONSOLE_CY, IOS_TOUCH_CONSOLE_SIZE, TG_CONSOLE, TOUCH_KEY_NONE, 0 },
	{ "SABER", TOUCH_ANCHOR_TR, .49f, .10f, .12f, TG_SABER_SELECT, 26, "weapon 1" },
	{ "CHAT",  TOUCH_ANCHOR_TL, .28f, .10f, .12f, TG_CHAT, TOUCH_KEY_CHAT, 0 },
	{ "PAD",   TOUCH_ANCHOR_TL, .12f, .26f, .12f, TG_PAD, TOUCH_KEY_TAB, 0 },
	{ 0, TOUCH_ANCHOR_TR, .11f, .34f, .12f, TG_LOCK, TOUCH_KEY_NONE, 0 },
	{ "F<<", TOUCH_ANCHOR_TR, .57f, .26f, .12f, TG_WEAPPREV, 27, "forceprev" },
	{ "1", TOUCH_ANCHOR_TR, 1.04f, .26f, .12f, -1, TOUCH_KEY_NONE, 0 },
	{ "2", TOUCH_ANCHOR_TR, .96f, .26f, .12f, -1, TOUCH_KEY_NONE, 0 },
	{ "3", TOUCH_ANCHOR_TR, .88f, .26f, .12f, -1, TOUCH_KEY_NONE, 0 },
	{ "4", TOUCH_ANCHOR_TR, .80f, .26f, .12f, -1, TOUCH_KEY_NONE, 0 }
};

void TouchLayout_Build( float viewW, float viewH, float safeL, float safeT, float safeR, float safeB, float p, touchRect_t out[TB_COUNT], const touchLayoutOverrides_t *overrides )
{
	touchLayoutOverrides_t positions = {};
	if ( overrides ) positions = *overrides;
	TouchEdit_Migrate( &positions, viewW, viewH, safeL, safeT, safeR, safeB, p );
	TouchEdit_RepairLock( &positions, viewW, viewH, safeL, safeT, safeR, safeB, p );
	for ( int i = 0; i < TB_COUNT; ++i ) {
		TouchEdit_DefaultRect( i, viewW, viewH, safeL, safeT, safeR, safeB, p, &out[i] );
		if ( out[i].w > 0 && ( positions.mask & ( 1u << i ) ) ) {
			out[i].x = fmaxf( 0, fminf( positions.cx[i] * viewW - out[i].w / 2, viewW - out[i].w ) );
			out[i].y = fmaxf( 0, fminf( positions.cy[i] * viewH - out[i].h / 2, viewH - out[i].h ) );
		}
	}
}

int TouchLayout_Hit( const touchRect_t rects[TB_COUNT], float x, float y, unsigned int visibleButtons )
{
	for ( int i = 0; i < TB_COUNT; ++i ) {
		if ( !( visibleButtons & ( 1u << i ) ) || rects[i].w <= 0 || rects[i].h <= 0 )
			continue;
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

void TouchLayout_ToVirtual( const aspectLayout_t *layout, float nx, float ny, int *vx, int *vy )
{
	if ( layout->branch == ASPECT_LEGACY ) {
		float cx = nx < 0 ? 0 : ( nx > 1 ? 1 : nx );
		float cy = ny < 0 ? 0 : ( ny > 1 ? 1 : ny );
		*vx = (int)( cx * 640 + 0.5f );
		*vy = (int)( cy * 480 + 0.5f );
		return;
	}
	const aspectPoint_t cursor = AspectLayout_ToVirtualCursor( layout, (double)nx * layout->W, (double)ny * layout->H );
	*vx = (int)( cursor.x + 0.5 );
	*vy = (int)( cursor.y + 0.5 );
}

touchRect_t TouchLayout_RectToVirtual( const aspectLayout_t *layout, const touchRect_t &rect )
{
	const aspectPoint_t origin = AspectLayout_ToVirtual( layout, rect.x, rect.y );
	const aspectPoint_t size = AspectLayout_SizeToVirtual( layout, rect.w, rect.h );
	return { (float)origin.x, (float)origin.y, (float)size.x, (float)size.y };
}
