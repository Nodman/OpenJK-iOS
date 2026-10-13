#pragma once

typedef enum {
	TB_ATTACK, TB_ALTATTACK, TB_JUMP, TB_CROUCH, TB_USE, TB_FORCE, TB_FORCENEXT,
	TB_SABER, TB_WEAPPREV, TB_WEAPNEXT, TB_MENU, TB_CONSOLE, TB_COUNT
} touchButton_t;

typedef struct { float x, y, w, h; } touchRect_t;

typedef struct {
	const char	*label;
	float		cx, cy, size;
	int			gameplayOnly;
} touchButtonDef_t;

extern const touchButtonDef_t touchButtonDefs[TB_COUNT];

void TouchLayout_Build( float viewW, float viewH, float safeL, float safeT, float safeR, float safeB, touchRect_t out[TB_COUNT] );
int TouchLayout_Hit( const touchRect_t rects[TB_COUNT], float x, float y, int uiMode );
void TouchLayout_Stick( float ox, float oy, float x, float y, float radius, float *outX, float *outY );
void TouchLayout_ToVirtual( float nx, float ny, int *vx, int *vy );
