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

static void TouchLayout_Center(touchRect_t *rect,float x,float y)
{
	rect->x=x-rect->w/2;rect->y=y-rect->h/2;
}

static void TouchLayout_FitGroup(float W,float H,touchRect_t rects[TB_COUNT],const int *ids,int count)
{
	float left=W,top=H,right=0,bottom=0;
	for(int i=0;i<count;++i) {
		const touchRect_t *r=&rects[ids[i]];
		left=fminf(left,r->x);top=fminf(top,r->y);right=fmaxf(right,r->x+r->w);bottom=fmaxf(bottom,r->y+r->h);
	}
	const float dx=fmaxf(-left,fminf(0,W-right)),dy=fmaxf(-top,fminf(0,H-bottom));
	for(int i=0;i<count;++i) {rects[ids[i]].x+=dx;rects[ids[i]].y+=dy;}
}

static void TouchLayout_Phone(float W,float H,touchRect_t rects[TB_COUNT])
{
	// Reference placements use full drawable height; safe insets only affect existing size rules.
	const float scale=H/440;
	const int fixed[7]={TB_ATTACK,TB_ALTATTACK,TB_JUMP,TB_CROUCH,TB_STYLE,TB_USE,TB_LOCK};
	const float offsets[7][2]={{210.583856f,225.467f},{271.331920f,133.626680f},{104.431528f,108.779880f},
		{58.789220f,38.266800f},{143.294840f,33.866800f},{104.169584f,41.266720f},{34.135892f,143.220880f}};
	for(int i=0;i<7;++i) TouchLayout_Center(&rects[fixed[i]],i<5?W-offsets[i][0]*scale:offsets[i][0]*scale,H-offsets[i][1]*scale);
	const int tools[4]={TB_CONSOLE,TB_MENU,TB_PAD,TB_CHAT};
	float cursor=45.816424f*scale;
	for(int i=0;i<4;++i) {
		touchRect_t *r=&rects[tools[i]];
		TouchLayout_Center(r,cursor+r->w/2,33.283250f*scale);cursor+=r->w+30.333944f*scale;
	}
	const int selectors[6]={TB_WEAPPREV,TB_SABER_SELECT,TB_WEAPNEXT,TB_FORCEPREV,TB_FORCE,TB_FORCENEXT};
	const float center=W-95.151955f*scale;
	for(int i=0;i<6;++i) TouchLayout_Center(&rects[selectors[i]],center+(i%3-1)*67.158761f*scale,(i<3?58.444467f:139.422360f)*scale);
	const int slots[4]={TB_FORCE_SLOT1,TB_FORCE_SLOT2,TB_FORCE_SLOT3,TB_FORCE_SLOT4};
	for(int i=0;i<4;++i) TouchLayout_Center(&rects[slots[i]],(111.695933f+(i%2)*80.606574f+(i/2)*61.212202f)*scale,(117.866540f+(i/2)*63.166840f)*scale);
	TouchLayout_FitGroup(W,H,rects,tools,4);TouchLayout_FitGroup(W,H,rects,selectors,6);TouchLayout_FitGroup(W,H,rects,slots,4);
	TouchLayout_FitGroup(W,H,rects,fixed,5);TouchLayout_FitGroup(W,H,rects,fixed+5,1);TouchLayout_FitGroup(W,H,rects,fixed+6,1);
}

void TouchLayout_Defaults(float W,float H,float L,float T,float R,float B,float p,touchRect_t out[TB_COUNT],touchLayoutProfile_t profile)
{
	for(int i=0;i<TB_COUNT;++i) TouchEdit_DefaultRect(i,W,H,L,T,R,B,p,&out[i]);
	if(TouchLayout_Unit(W-L-R,H-T-B,p)<=0) return;
	if(profile==TOUCH_LAYOUT_PHONE) TouchLayout_Phone(W,H,out);
	else TouchLayout_ForceRow(W,H,L,T,R,B,p,out);
}

void TouchLayout_Build( float viewW, float viewH, float safeL, float safeT, float safeR, float safeB, float p, touchRect_t out[TB_COUNT], const touchLayoutOverrides_t *overrides, touchLayoutProfile_t profile )
{
	touchLayoutOverrides_t positions = {};
	if ( overrides ) positions = *overrides;
	const int legacy=positions.version!=2;
	TouchEdit_Migrate( &positions, viewW, viewH, safeL, safeT, safeR, safeB, p );
	if(legacy) TouchEdit_RepairLock( &positions, viewW, viewH, safeL, safeT, safeR, safeB, p, TOUCH_LAYOUT_TABLET );
	if(!(positions.mask & (1u<<TB_LOCK))) TouchEdit_RepairLock( &positions, viewW, viewH, safeL, safeT, safeR, safeB, p, profile );
	TouchLayout_Defaults(viewW,viewH,safeL,safeT,safeR,safeB,p,out,profile);
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
