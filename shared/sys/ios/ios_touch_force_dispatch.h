#pragma once
#include <string.h>

#define IOS_TOUCH_FORCE_SOURCE_BASE 65536
#define TOUCH_FORCE_SOURCE_COUNT 17

typedef struct {
	unsigned int next, generation;
	int source, power, central, cancelled, lifted, dispatched, pendingPacket, upQueued, fenceQueued, fenceDone;
	unsigned int downTime, upTime;
} touchForceDispatch_t;
typedef struct { unsigned int touch; int other; } touchForcePulse_t;

static inline int TouchForce_SourceIndex(int source,int aux16) {
	if(source>=aux16 && source<aux16+13) return source-aux16;
	if(source>=IOS_TOUCH_FORCE_SOURCE_BASE && source<IOS_TOUCH_FORCE_SOURCE_BASE+4) return 13+source-IOS_TOUCH_FORCE_SOURCE_BASE;
	return -1;
}
static inline unsigned int TouchForce_Reserve(touchForceDispatch_t *s,int power,int source,int central,unsigned int time,int pending) {
	if(s->generation || pending || (!central && (power<1 || power>12))) return 0;
	if(!++s->next) ++s->next;
	s->generation=s->next;s->source=source;s->power=power;s->central=central;s->downTime=time;
	s->cancelled=s->lifted=s->dispatched=s->pendingPacket=s->upQueued=s->fenceQueued=s->fenceDone=0;
	return s->generation;
}
static inline void TouchForce_Retire(touchForceDispatch_t *s) {
	if(s->generation && s->lifted && s->fenceDone && (!s->pendingPacket || s->cancelled)) s->generation=0;
}
static inline void TouchForce_PacketConsumed(touchForceDispatch_t *s) {
	s->pendingPacket=0;TouchForce_Retire(s);
}
static inline void TouchForce_Cancel(touchForceDispatch_t *s) {
	if(s->generation) s->cancelled=s->lifted=1;
}
static inline int TouchForce_CentralAllowed(const touchForceDispatch_t *s) {
	return s->generation && s->central && !s->cancelled;
}
static inline const char *TouchForce_Command(int power,int multiplayer) {
	static const char *const commands[]={0,"force_throw","force_pull","force_speed",0,"force_absorb","force_heal","force_distract","force_protect","force_drain","force_grip","force_lightning","force_rage"};
	if(power<1 || power>12) return 0;
	return power==4 ? (multiplayer?"force_seeing":"force_sight") : commands[power];
}
static inline int TouchForce_Held(int power) { return power>=9 && power<=11; }
static inline void TouchForce_PulseDown(touchForcePulse_t *pulse,int source) {
	if(source>=0 && source<TOUCH_FORCE_SOURCE_COUNT) pulse->touch|=1u<<source;
	else pulse->other=1;
}
static inline int TouchForce_PulseCancel(touchForcePulse_t *pulse,int source) {
	if(source>=0 && source<TOUCH_FORCE_SOURCE_COUNT) pulse->touch&=~(1u<<source);
	return pulse->touch || pulse->other;
}
