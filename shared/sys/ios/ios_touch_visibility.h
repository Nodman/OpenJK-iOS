#pragma once

#include "ios_touch_geometry.h"

typedef enum {
	TOUCH_CAP_SABER = 1u << 0,
	TOUCH_CAP_STYLE = 1u << 1,
	TOUCH_CAP_FORCE = 1u << 2,
	TOUCH_CAP_WEAPONS = 1u << 3,
	TOUCH_CAP_PAD = 1u << 4,
	TOUCH_CAP_CHAT = 1u << 5,
	TOUCH_CAP_VALID = 1u << 8,
	TOUCH_CAP_FORCE_CYCLE = 1u << 9,
	TOUCH_CAP_EQUIPMENT = 1u << 10
} touchCapability_t;

typedef enum {
	TOUCH_STYLE_BLUE, TOUCH_STYLE_YELLOW, TOUCH_STYLE_RED, TOUCH_STYLE_UNKNOWN
} touchStyle_t;

typedef struct {
	int saberOwned, legalStyleCount, manualBladeToggle;
	int selectableForceCount, ownedWeaponCount, chatAvailable;
	int style, valid, equipmentAllowed;
} touchCapsFacts_t;

typedef struct {
	int menu, console, basic, sticks, saber, style, force, forceCycle, weapons, pad, chat;
	int styleColor, fallback, logMissing;
} touchVisibility_t;

typedef struct {
	int missing, warned;
	unsigned int missingSince;
} touchBridgeState_t;

IOS_TOUCH_INLINE unsigned int TouchCaps_Pack( const touchCapsFacts_t *facts, int multiplayer )
{
	unsigned int mask;
	int style;
	if ( !facts->valid )
		return 0;
	style = facts->style >= TOUCH_STYLE_BLUE && facts->style <= TOUCH_STYLE_UNKNOWN ? facts->style : TOUCH_STYLE_UNKNOWN;
	mask = TOUCH_CAP_VALID | ( (unsigned int)style << 6 );
	if ( facts->saberOwned ) mask |= TOUCH_CAP_SABER;
	if ( facts->legalStyleCount > 1 || facts->manualBladeToggle ) mask |= TOUCH_CAP_STYLE;
	if ( facts->selectableForceCount > 0 ) mask |= TOUCH_CAP_FORCE;
	if ( facts->selectableForceCount > 1 ) mask |= TOUCH_CAP_FORCE_CYCLE;
	if ( facts->ownedWeaponCount > 1 ) mask |= TOUCH_CAP_WEAPONS;
	if ( !multiplayer && facts->selectableForceCount > 0 ) mask |= TOUCH_CAP_PAD;
	if ( multiplayer && facts->chatAvailable ) mask |= TOUCH_CAP_CHAT;
	if ( facts->equipmentAllowed ) mask |= TOUCH_CAP_EQUIPMENT;
	return mask;
}

IOS_TOUCH_INLINE touchVisibility_t TouchCaps_Visible( unsigned int mask, int multiplayer )
{
	touchVisibility_t v = { 0 };
	const int valid = ( mask & TOUCH_CAP_VALID ) != 0;
	const int equipment = valid && ( mask & TOUCH_CAP_EQUIPMENT ) != 0;
	v.menu = v.console = 1;
	v.styleColor = valid ? (int)( ( mask >> 6 ) & 3 ) : TOUCH_STYLE_UNKNOWN;
	v.basic = v.sticks = valid;
	v.saber = equipment && ( mask & TOUCH_CAP_SABER ) != 0;
	v.style = v.saber && ( mask & TOUCH_CAP_STYLE ) != 0;
	v.force = equipment && ( mask & TOUCH_CAP_FORCE ) != 0;
	v.forceCycle = equipment && ( mask & TOUCH_CAP_FORCE_CYCLE ) != 0;
	v.weapons = equipment && ( mask & TOUCH_CAP_WEAPONS ) != 0;
	v.pad = valid && ( multiplayer || ( mask & TOUCH_CAP_PAD ) != 0 );
	v.chat = valid && multiplayer && ( mask & TOUCH_CAP_CHAT ) != 0;
	return v;
}

IOS_TOUCH_INLINE void TouchBridge_Reset( touchBridgeState_t *state )
{
	state->missing = state->warned = 0;
	state->missingSince = 0;
}

IOS_TOUCH_INLINE touchVisibility_t TouchBridge_Step( touchBridgeState_t *state, unsigned int mask,
	int multiplayer, int active, int hidden, int ui, int cinematic, unsigned int now )
{
	touchVisibility_t v = TouchCaps_Visible( active ? mask : 0, multiplayer );
	if ( !active || ( mask & TOUCH_CAP_VALID ) ) {
		state->missing = 0;
	} else {
		if ( !state->missing ) {
			state->missing = 1;
			state->missingSince = now;
		}
		if ( now - state->missingSince >= 1000u ) {
			v.menu = v.console = v.basic = v.sticks = v.saber = v.style = v.force = v.forceCycle = v.weapons = v.pad = 1;
			v.chat = multiplayer != 0;
			v.styleColor = TOUCH_STYLE_UNKNOWN;
			v.fallback = 1;
		}
	}
	if ( cinematic || hidden || ui ) {
		touchVisibility_t gated = { 0 };
		gated.console = !cinematic;
		gated.styleColor = TOUCH_STYLE_UNKNOWN;
		return gated;
	}
	if ( v.fallback && !state->warned ) {
		v.logMissing = 1;
		state->warned = 1;
	}
	return v;
}
