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
	{ "F>>",   TOUCH_ANCHOR_TR, .41f, .26f, .12f, TG_WEAPNEXT, 22, "forcenext" },
	{ "STYLE", TOUCH_ANCHOR_BR, .12f, .08f, .12f, TG_SABER_SELECT, 23, "saberAttackCycle" },
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

static void TouchLayout_ForceRow(float W,float H,float L,float T,float R,float B,float p,touchRect_t rects[TB_COUNT])
{
	const int row[7]={TB_FORCE_SLOT1,TB_FORCE_SLOT2,TB_FORCE_SLOT3,TB_FORCE_SLOT4,TB_FORCEPREV,TB_FORCE,TB_FORCENEXT};
	const float gap=4*p;
	touchRect_t *use=&rects[TB_FORCE];
	if(use->w<=0) return;
	rects[TB_FORCEPREV].x=fminf(rects[TB_FORCEPREV].x,use->x-gap-rects[TB_FORCEPREV].w);
	rects[TB_FORCENEXT].x=fmaxf(rects[TB_FORCENEXT].x,use->x+use->w+gap);
	float left=rects[TB_FORCEPREV].x;
	for(int i=3;i>=0;--i) {
		touchRect_t *slot=&rects[TB_FORCE_SLOT1+i];
		slot->x=left-gap-slot->w;left=slot->x;
	}
	if(rects[TB_FORCENEXT].x+rects[TB_FORCENEXT].w-left>W-L-R) {
		const float width=rects[TB_FORCEPREV].w+use->w+rects[TB_FORCENEXT].w+2*gap;
		const float x=fmaxf(L,fminf(rects[TB_FORCEPREV].x,W-R-width));
		rects[TB_FORCEPREV].x=x;use->x=x+rects[TB_FORCEPREV].w+gap;
		rects[TB_FORCENEXT].x=use->x+use->w+gap;
		const float side=rects[TB_FORCE_SLOT1].w;
		const int columns=(int)fmaxf(1,fminf(4,floorf((W-L-R+gap)/(side+gap))));
		const float start=fmaxf(L,fminf(x,W-R-columns*side-(columns-1)*gap));
		for(int i=0;i<4;++i) {
			touchRect_t *slot=&rects[TB_FORCE_SLOT1+i];
			slot->x=start+(i%columns)*(side+gap);
			slot->y=fmaxf(T,fminf(use->y+use->h+gap+(i/columns)*(side+gap),H-B-slot->h));
		}
	}
	float minX=W,minY=H,maxX=0,maxY=0;
	for(int i=0;i<7;++i) {
		const touchRect_t *r=&rects[row[i]];
		minX=fminf(minX,r->x);minY=fminf(minY,r->y);maxX=fmaxf(maxX,r->x+r->w);maxY=fmaxf(maxY,r->y+r->h);
	}
	float dx=fmaxf(L-minX,fminf(0,W-R-maxX)),dy=fmaxf(T-minY,fminf(0,H-B-maxY));
	const int combat[5]={TB_ATTACK,TB_ALTATTACK,TB_JUMP,TB_CROUCH,TB_STYLE};
	for(int i=0;i<5;++i) {
		const touchRect_t *r=&rects[combat[i]];
		if(minX+dx>=r->x+r->w || maxX+dx<=r->x || minY+dy>=r->y+r->h || maxY+dy<=r->y) continue;
		const float left=r->x-gap-maxX,above=r->y-gap-maxY,below=r->y+r->h+gap-minY;
		if(minX+left>=L) dx=fminf(dx,left);
		else if(minY+above>=T) dy=above;
		else if(maxY+below<=H-B) dy=below;
	}
	for(int i=0;i<7;++i) {
		touchRect_t *r=&rects[row[i]];
		r->x=fmaxf(L,fminf(r->x+dx,W-R-r->w));r->y=fmaxf(T,fminf(r->y+dy,H-B-r->h));
	}
}

void TouchLayout_Build( float viewW, float viewH, float safeL, float safeT, float safeR, float safeB, float p, touchRect_t out[TB_COUNT], const touchLayoutOverrides_t *overrides )
{
	touchLayoutOverrides_t positions = {};
	if ( overrides ) positions = *overrides;
	TouchEdit_Migrate( &positions, viewW, viewH, safeL, safeT, safeR, safeB, p );
	TouchEdit_RepairLock( &positions, viewW, viewH, safeL, safeT, safeR, safeB, p );
	for(int i=0;i<TB_COUNT;++i) TouchEdit_DefaultRect(i,viewW,viewH,safeL,safeT,safeR,safeB,p,&out[i]);
	TouchLayout_ForceRow(viewW,viewH,safeL,safeT,safeR,safeB,p,out);
	for ( int i = 0; i < TB_COUNT; ++i ) {
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
