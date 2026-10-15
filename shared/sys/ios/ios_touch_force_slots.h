#pragma once
#include "ios_touch_force.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct { int powers[4]; } touchForceSlots_t;
static inline void TouchForceSlots_Default(touchForceSlots_t *slots) {
	int i;for(i=0;i<4;++i) slots->powers[i]=i+1;
}
static inline void TouchForceSlots_Parse(const char *text,touchForceSlots_t *slots) {
	int i,j;const char *cursor;touchForceSlots_t parsed={{0,0,0,0}};
	TouchForceSlots_Default(slots);
	if(!text || strncmp(text,"v1:",3)) return;
	cursor=text+3;
	for(i=0;i<4;++i) {
		char *end;long value;
		if((*cursor<'0' || *cursor>'9') && *cursor!='-') return;
		value=strtol(cursor,&end,10);
		if(end==cursor || *end!=(i==3?'\0':',')) return;
		parsed.powers[i]=value>=0 && value<=12 ? (int)value : 0;
		for(j=0;j<i;++j) if(parsed.powers[i] && parsed.powers[j]==parsed.powers[i]) parsed.powers[i]=0;
		cursor=end+(i!=3);
	}
	*slots=parsed;
}
static inline void TouchForceSlots_Save(const touchForceSlots_t *slots,char *text,size_t capacity) {
	snprintf(text,capacity,"v1:%d,%d,%d,%d",slots->powers[0],slots->powers[1],slots->powers[2],slots->powers[3]);
}
static inline int TouchForceSlots_Visible(const touchForceSlots_t *slots,int slot,unsigned int learned) {
	const int power=slot>=0 && slot<4 ? slots->powers[slot] : 0;
	return power>0 && power<=12 && (learned & (1u<<power)) ? power : 0;
}
static inline void TouchForceSlots_Assign(touchForceSlots_t *slots,int slot,int power,unsigned int learned) {
	int i;
	if(slot<0 || slot>=4 || power<0 || power>12 || (power && !(learned & (1u<<power)))) return;
	for(i=0;i<4;++i) if(i!=slot && power && slots->powers[i]==power) slots->powers[i]=0;
	slots->powers[slot]=power;
}
static inline const char *TouchForceSlots_Name(int power) {
	static const char *const names[]={"Empty","Push","Pull","Speed","Sense","Absorb","Heal","Mind Trick","Protect","Drain","Grip","Lightning","Dark Rage"};
	return power>=0 && power<=12 ? names[power] : names[0];
}
