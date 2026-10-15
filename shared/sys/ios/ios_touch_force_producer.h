#pragma once
#include "ios_touch_force.h"

static inline int TouchForce_Canonical(int power) {
	switch(power) {
	case FP_PUSH:return 1; case FP_PULL:return 2; case FP_SPEED:return 3; case FP_SEE:return 4;
	case FP_ABSORB:return 5; case FP_HEAL:return 6; case FP_TELEPATHY:return 7; case FP_PROTECT:return 8;
	case FP_DRAIN:return 9; case FP_GRIP:return 10; case FP_LIGHTNING:return 11; case FP_RAGE:return 12;
#ifndef _JK2EXE
	case FP_TEAM_HEAL:return 13; case FP_TEAM_FORCE:return 14;
#endif
	default:return 0;
	}
}
static inline int TouchForce_Known(unsigned int known,const int *levels,int power) {
	return power>=0 && power<NUM_FORCE_POWERS && (known & (1u<<power)) && levels[power]>0;
}
static inline touchForceReport_t TouchForce_Produce(unsigned int known,const int *levels,int selected,int fallback,int equipment) {
	touchForceReport_t r=TouchForce_Empty(); int power;
	if(!equipment) return r;
	r.equipment=1;
	for(power=0;power<NUM_FORCE_POWERS;++power) {
		const int id=TouchForce_Canonical(power);
		if(id>0 && id<=12 && TouchForce_Known(known,levels,power)) r.learned|=1u<<id;
	}
	if(!TouchForce_Known(known,levels,selected) || !TouchForce_Canonical(selected)) selected=fallback;
	if(TouchForce_Known(known,levels,selected)) r.selected=TouchForce_Canonical(selected);
	return r;
}

static inline int TouchForce_SelectionIndex(int index,const int *powers,int count) {
	return index>=0 && index<count ? powers[index] : -1;
}
