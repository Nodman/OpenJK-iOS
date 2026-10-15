#pragma once

#include "ios_touch_geometry.h"
#include "ios_touch_layout.h"

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

typedef enum { TOUCH_WEAPON_UNKNOWN, TOUCH_WEAPON_SABER, TOUCH_WEAPON_GUN, TOUCH_WEAPON_MELEE } touchWeaponClass_t;

IOS_TOUCH_INLINE int TouchCaps_WeaponClass( int weapon,int none,int saber,int melee,int baton,int count )
{
	if(weapon==none || weapon<0 || weapon>=count) return TOUCH_WEAPON_UNKNOWN;
	if(weapon==saber) return TOUCH_WEAPON_SABER;
	if(weapon==melee || weapon==baton) return TOUCH_WEAPON_MELEE;
	return TOUCH_WEAPON_GUN;
}

typedef struct {
	int saberOwned, legalStyleCount, manualBladeToggle;
	int selectableForceCount, ownedWeaponCount, chatAvailable;
	int style, valid, equipmentAllowed;
	int weaponClass;
} touchCapsFacts_t;

typedef struct {
	int menu, console, basic, sticks, saber, style, force, forceCycle, weapons, pad, chat;
	int styleColor, fallback, logMissing;
	int weaponClass;
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
	if(facts->weaponClass>=TOUCH_WEAPON_SABER && facts->weaponClass<=TOUCH_WEAPON_MELEE) mask |= (unsigned int)facts->weaponClass << 11;
	if ( facts->saberOwned ) mask |= TOUCH_CAP_SABER;
	if ( facts->legalStyleCount > 1 || facts->manualBladeToggle ) mask |= TOUCH_CAP_STYLE;
	if ( facts->selectableForceCount > 0 ) mask |= TOUCH_CAP_FORCE;
	if ( facts->selectableForceCount > 1 ) mask |= TOUCH_CAP_FORCE_CYCLE;
	if ( facts->ownedWeaponCount > 1 ) mask |= TOUCH_CAP_WEAPONS;
	if ( !multiplayer ) mask |= TOUCH_CAP_PAD;
	if ( multiplayer && facts->chatAvailable ) mask |= TOUCH_CAP_CHAT;
	if ( facts->equipmentAllowed ) mask |= TOUCH_CAP_EQUIPMENT;
	return mask;
}

IOS_TOUCH_INLINE touchVisibility_t TouchCaps_Visible( unsigned int mask, int multiplayer )
{
	touchVisibility_t v = { 0 };
	const int valid = ( mask & TOUCH_CAP_VALID ) != 0;
	const int equipment = valid && ( mask & TOUCH_CAP_EQUIPMENT ) != 0;
	v.weaponClass = valid ? (int)((mask>>11)&3u) : TOUCH_WEAPON_UNKNOWN;
	v.menu = v.console = 1;
	v.styleColor = valid ? (int)( ( mask >> 6 ) & 3 ) : TOUCH_STYLE_UNKNOWN;
	v.basic = v.sticks = valid;
	v.saber = equipment && ( mask & TOUCH_CAP_SABER ) != 0;
	v.style = v.saber && ( mask & TOUCH_CAP_STYLE ) != 0;
	v.force = equipment && ( mask & TOUCH_CAP_FORCE ) != 0;
	v.forceCycle = equipment && ( mask & TOUCH_CAP_FORCE_CYCLE ) != 0;
	v.weapons = equipment && ( mask & TOUCH_CAP_WEAPONS ) != 0;
	v.pad = valid;
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

IOS_TOUCH_INLINE int TouchCaps_EquipmentAllowed( int health, int spectator, int follow, int intermission )
{
	return health > 0 && !spectator && !follow && !intermission;
}

IOS_TOUCH_INLINE int TouchCaps_CountBits( unsigned int mask )
{
	int count = 0;
	while ( mask ) {
		mask &= mask - 1;
		++count;
	}
	return count;
}

IOS_TOUCH_INLINE int TouchCaps_MPStyleCount( int saber, int special, int offense, unsigned int forbidden, int first, int last )
{
	int style, count = 0;
	if ( !saber ) return 0;
	if ( special ) return 2;
	for ( style = first; style <= last && style <= offense; ++style )
		if ( style >= 0 && style < 32 && !( forbidden & ( 1u << style ) ) ) ++count;
	return count;
}

IOS_TOUCH_INLINE int TouchCaps_StyleColor( int stance, const int styles[7] )
{
	if ( stance == styles[0] || stance == styles[4] ) return TOUCH_STYLE_BLUE;
	if ( stance == styles[1] || stance == styles[5] || stance == styles[6] ) return TOUCH_STYLE_YELLOW;
	if ( stance == styles[2] || stance == styles[3] ) return TOUCH_STYLE_RED;
	return TOUCH_STYLE_UNKNOWN;
}

IOS_TOUCH_INLINE int TouchCaps_ShouldPublish( unsigned int mask, int previous, unsigned int bridge )
{
	return previous < 0 || mask != (unsigned int)previous || ( ( previous & TOUCH_CAP_VALID ) && bridge == 0 );
}

IOS_TOUCH_INLINE unsigned int TouchCaps_ButtonMask( const touchVisibility_t *v )
{
	unsigned int mask = 0;
	if ( v->menu ) mask |= 1u << TB_MENU;
	if ( v->console ) mask |= 1u << TB_CONSOLE;
	if ( v->basic ) mask |= ( 1u << TB_ATTACK ) | ( 1u << TB_ALTATTACK ) | ( 1u << TB_JUMP ) | ( 1u << TB_CROUCH ) | ( 1u << TB_USE );
	if ( v->saber ) mask |= 1u << TB_SABER_SELECT;
	if ( v->style ) mask |= 1u << TB_STYLE;
	if ( v->force ) mask |= 1u << TB_FORCE;
	if ( v->forceCycle ) mask |= 1u << TB_FORCENEXT;
	if ( v->weapons ) mask |= ( 1u << TB_WEAPPREV ) | ( 1u << TB_WEAPNEXT );
	if ( v->pad ) mask |= 1u << TB_PAD;
	if ( v->chat ) mask |= 1u << TB_CHAT;
	return mask;
}
