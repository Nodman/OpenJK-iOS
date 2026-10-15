#pragma once

#include <string.h>

#define TOUCH_FORCE_API 1
#define TOUCH_FORCE_COUNT 14
#define TOUCH_FORCE_LEARNED 0x1ffeu

typedef struct { unsigned int learned; int selected, equipment; } touchForceReport_t;
typedef struct { unsigned int epoch; int outer; } touchForceToken_t;
typedef struct {
	unsigned int epoch, updateEpoch, fieldEpoch, completedAt;
	int updates, fields, hadField, tainted, producerSeen;
	touchForceReport_t pending, committed;
} touchForceBridge_t;

static inline touchForceReport_t TouchForce_Empty(void) {
	touchForceReport_t r = { 0, 0, 0 }; return r;
}
static inline void TouchForce_Reset(touchForceBridge_t *s) {
	++s->epoch; s->pending = s->committed = TouchForce_Empty(); s->producerSeen=0;
}
static inline touchForceToken_t TouchForce_BeginUpdate(touchForceBridge_t *s) {
	touchForceToken_t t = { s->epoch, ++s->updates == 1 };
	if(t.outer) { s->updateEpoch=s->epoch; s->hadField=0; s->tainted=0; }
	else s->tainted=1;
	return t;
}
static inline touchForceToken_t TouchForce_BeginField(touchForceBridge_t *s) {
	touchForceToken_t t = { s->epoch, ++s->fields==1 && s->updates==1 };
	if(t.outer) { s->fieldEpoch=s->epoch; s->pending=TouchForce_Empty(); }
	else s->tainted=1;
	return t;
}
static inline int TouchForce_InFrame(const touchForceBridge_t *s) {
	return s->updates==1 && s->fields==1 && !s->tainted && s->updateEpoch==s->epoch && s->fieldEpoch==s->epoch;
}
static inline void TouchForce_ClearPending(touchForceBridge_t *s) {
	if(TouchForce_InFrame(s)) s->pending=TouchForce_Empty();
}
static inline int TouchForce_ShouldCancel(const touchForceBridge_t *s,touchForceReport_t previous,touchForceReport_t current) {
	return s->producerSeen && (!current.equipment || !current.selected || current.learned!=previous.learned ||
		current.selected!=previous.selected || current.equipment!=previous.equipment);
}
static inline void TouchForce_Report(touchForceBridge_t *s,int version,int mask,int selected,int equipment) {
	if(!TouchForce_InFrame(s)) return;
	s->pending=TouchForce_Empty();
	if(version!=TOUCH_FORCE_API || mask<0 || ((unsigned int)mask & ~TOUCH_FORCE_LEARNED) || selected<0 || selected>TOUCH_FORCE_COUNT || (equipment!=0 && equipment!=1)) return;
	if(selected && selected<=12 && !((unsigned int)mask & (1u<<selected))) return;
	s->producerSeen=1;
	if(!equipment) return;
	s->pending.learned=(unsigned int)mask; s->pending.selected=selected; s->pending.equipment=equipment;
}
static inline void TouchForce_EndField(touchForceBridge_t *s,touchForceToken_t t,unsigned int now) {
	if(t.outer && t.epoch==s->epoch && s->updateEpoch==s->epoch) {
		s->committed=s->tainted ? TouchForce_Empty() : s->pending;
		s->completedAt=now; s->hadField=1;
	}
	if(s->fields>0) --s->fields;
}
static inline void TouchForce_EndUpdate(touchForceBridge_t *s,touchForceToken_t t,unsigned int now) {
	if(t.outer && t.epoch==s->epoch && (!s->hadField || s->tainted)) {
		s->committed=TouchForce_Empty(); s->completedAt=now;
	}
	if(s->updates>0) --s->updates;
}
static inline touchForceReport_t TouchForce_Current(touchForceBridge_t *s,unsigned int now) {
	if(now-s->completedAt>1000u) s->committed=TouchForce_Empty();
	return s->committed;
}
static inline const char *TouchForce_Icon(int id) {
	static const char *const names[TOUCH_FORCE_COUNT]={
		"gfx/mp/f_icon_push","gfx/mp/f_icon_pull","gfx/mp/f_icon_speed","gfx/mp/f_icon_sight",
		"gfx/mp/f_icon_lt_absorb","gfx/mp/f_icon_lt_heal","gfx/mp/f_icon_lt_telepathy","gfx/mp/f_icon_lt_protect",
		"gfx/mp/f_icon_dk_drain","gfx/mp/f_icon_dk_grip","gfx/mp/f_icon_dk_l1","gfx/mp/f_icon_dk_rage",
		"gfx/mp/f_icon_lt_healother","gfx/mp/f_icon_dk_forceother" };
	return id>0 && id<=TOUCH_FORCE_COUNT ? names[id-1] : 0;
}
