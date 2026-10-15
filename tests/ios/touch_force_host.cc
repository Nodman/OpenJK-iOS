#include "sys/ios/sys_ios.h"
#include "sys/ios/ios_touch_force_producer.h"
#include "sys/ios/ios_touch_force_media.h"
#include <cassert>
#include <cstdio>

static touchForceReport_t Produce(touchForceBridge_t *s,int selected,int fallback,int equipment) {
	int levels[NUM_FORCE_POWERS]={0};
	levels[FP_PUSH]=levels[FP_SEE]=2;
	touchForceReport_t report=TouchForce_Produce((1u<<FP_PUSH)|(1u<<FP_SEE),levels,selected,fallback,equipment);
	TouchForce_Report(s,TOUCH_FORCE_API,(int)report.learned,report.selected,report.equipment);
	return report;
}
static touchGestureActions_t RouteForceCancellation(touchGestureState_t *gestures,const touchForceBridge_t *bridge,
	touchForceReport_t previous,touchForceReport_t current,long long finger) {
	touchGestureActions_t actions={};
	if(TouchForce_ShouldCancel(bridge,previous,current) && TouchGesture_HoldsButton(gestures,finger,TB_FORCE)) {
		touchGestureInput_t in={};in.event=TOUCH_CANCEL;in.finger=finger;
		in.visibleButtons=~0u;in.moveEnabled=in.lookEnabled=1;
		actions=TouchGesture_Step(gestures,in);
	}
	return actions;
}
static void ForceRoutingAndLegacy(void) {
	touchForceBridge_t bridge={};
	touchGestureState_t gestures;TouchGesture_Reset(&gestures);
	touchGestureInput_t in={};in.event=TOUCH_DOWN;in.target=TOUCH_TARGET_BUTTON;in.button=TB_FORCE;
	in.finger=101;in.visibleButtons=~0u;in.moveEnabled=in.lookEnabled=1;in.radius=10;in.slop=1;
	touchGestureActions_t actions=TouchGesture_Step(&gestures,in);
	assert(actions.keyCount==1 && actions.keys[0].button==TB_FORCE && actions.keys[0].down);
	in.finger=102;actions=TouchGesture_Step(&gestures,in);assert(!actions.keyCount);
	in.finger=103;in.button=TB_ATTACK;actions=TouchGesture_Step(&gestures,in);assert(actions.keyCount==1);
	in.finger=104;in.target=TOUCH_TARGET_MOVE;TouchGesture_Step(&gestures,in);
	assert(TouchGesture_HoldsButton(&gestures,101,TB_FORCE));
	assert(!TouchGesture_HoldsButton(&gestures,102,TB_FORCE));
	assert(!TouchGesture_HoldsButton(&gestures,103,TB_FORCE));
	assert(!TouchGesture_HoldsButton(&gestures,104,TB_FORCE));
	assert(!TouchGesture_HoldsButton(&gestures,999,TB_FORCE));

	touchForceReport_t previous=TouchForce_Empty();
	touchForceToken_t u=TouchForce_BeginUpdate(&bridge),f=TouchForce_BeginField(&bridge);
	TouchForce_ClearPending(&bridge); // Host menu clear is not a cgame report.
	TouchForce_EndField(&bridge,f,1);TouchForce_EndUpdate(&bridge,u,1);
	assert(!bridge.producerSeen);
	u=TouchForce_BeginUpdate(&bridge);TouchForce_EndUpdate(&bridge,u,2);
	u=TouchForce_BeginUpdate(&bridge);f=TouchForce_BeginField(&bridge);
	TouchForce_EndField(&bridge,f,3);TouchForce_EndUpdate(&bridge,u,3);
	assert(!bridge.producerSeen);
	actions=RouteForceCancellation(&gestures,&bridge,previous,TouchForce_Current(&bridge,3),101);
	assert(!actions.keyCount && TouchGesture_HoldsButton(&gestures,101,TB_FORCE));

	u=TouchForce_BeginUpdate(&bridge);f=TouchForce_BeginField(&bridge);
	TouchForce_Report(&bridge,TOUCH_FORCE_API,0,0,0);
	TouchForce_EndField(&bridge,f,4);TouchForce_EndUpdate(&bridge,u,4);
	assert(bridge.producerSeen && !bridge.committed.equipment);
	u=TouchForce_BeginUpdate(&bridge);f=TouchForce_BeginField(&bridge);
	TouchForce_ClearPending(&bridge);
	TouchForce_EndField(&bridge,f,4);TouchForce_EndUpdate(&bridge,u,4);
	assert(bridge.producerSeen);
	assert(!RouteForceCancellation(&gestures,&bridge,previous,bridge.committed,102).keyCount);
	assert(!RouteForceCancellation(&gestures,&bridge,previous,bridge.committed,103).keyCount);
	actions=RouteForceCancellation(&gestures,&bridge,previous,bridge.committed,101);
	assert(actions.keyCount==1 && actions.keys[0].button==TB_FORCE && !actions.keys[0].down);
	assert(TouchGesture_HoldsButton(&gestures,103,TB_ATTACK) && gestures.moveOwner>=0);
	in.event=TOUCH_REFRESH;actions=TouchGesture_Step(&gestures,in);
	assert(!actions.keyCount && !TouchGesture_HoldsButton(&gestures,102,TB_FORCE));

	in.event=TOUCH_DOWN;in.target=TOUCH_TARGET_BUTTON;in.button=TB_FORCE;in.finger=105;
	actions=TouchGesture_Step(&gestures,in);assert(actions.keyCount==1);
	u=TouchForce_BeginUpdate(&bridge);f=TouchForce_BeginField(&bridge);
	Produce(&bridge,FP_PUSH,-1,1);
	TouchForce_EndField(&bridge,f,5);TouchForce_EndUpdate(&bridge,u,5);
	previous=bridge.committed;
	assert(!RouteForceCancellation(&gestures,&bridge,previous,bridge.committed,105).keyCount);
	u=TouchForce_BeginUpdate(&bridge);f=TouchForce_BeginField(&bridge);
	TouchForce_EndField(&bridge,f,6);TouchForce_EndUpdate(&bridge,u,6);
	assert(bridge.producerSeen);
	actions=RouteForceCancellation(&gestures,&bridge,previous,bridge.committed,105);
	assert(actions.keyCount==1 && !actions.keys[0].down && actions.keys[0].button==TB_FORCE);

	TouchForce_Reset(&bridge);assert(!bridge.producerSeen);
	in.finger=106;actions=TouchGesture_Step(&gestures,in);assert(actions.keyCount==1);
	u=TouchForce_BeginUpdate(&bridge);f=TouchForce_BeginField(&bridge);
	TouchForce_ClearPending(&bridge);
	TouchForce_EndField(&bridge,f,7);TouchForce_EndUpdate(&bridge,u,7);
	assert(!RouteForceCancellation(&gestures,&bridge,previous,bridge.committed,106).keyCount);
	assert(TouchGesture_HoldsButton(&gestures,106,TB_FORCE));
	u=TouchForce_BeginUpdate(&bridge);f=TouchForce_BeginField(&bridge);
	TouchForce_Report(&bridge,2,0,0,0);TouchForce_Report(&bridge,TOUCH_FORCE_API,-1,0,0);
	TouchForce_EndField(&bridge,f,8);TouchForce_EndUpdate(&bridge,u,8);assert(!bridge.producerSeen);
	assert(TouchGesture_HoldsButton(&gestures,103,TB_ATTACK) && gestures.moveOwner>=0);
}
static touchForceBridge_t scopes;
touchForceToken_t IN_TouchBeginForceUpdate(void) { return TouchForce_BeginUpdate(&scopes); }
touchForceToken_t IN_TouchBeginForceFrame(void) { return TouchForce_BeginField(&scopes); }
void IN_TouchEndForceUpdate(touchForceToken_t t) { TouchForce_EndUpdate(&scopes,t,20); }
void IN_TouchEndForceFrame(touchForceToken_t t) { TouchForce_EndField(&scopes,t,20); }
static void EarlyExit(int reset) {
	TouchForceUpdateScope update;
	TouchForceFrameScope frame;
	Produce(&scopes,FP_PUSH,-1,1);
	if(reset) { TouchForce_Reset(&scopes); return; }
	frame.Finish(); frame.Finish();
}
static int probes,registrations,available,shaderResult=17;
static int Readable(const char *,void *) { ++probes; return available; }
static int Register(const char *,void *) { ++registrations; return shaderResult; }
int main() {
	ForceRoutingAndLegacy();
	EarlyExit(0);assert(scopes.committed.selected==1 && !scopes.fields && !scopes.updates);
	EarlyExit(1);assert(!scopes.committed.equipment && !scopes.fields && !scopes.updates);
	EarlyExit(0);assert(scopes.committed.selected==1);
	{ TouchForceUpdateScope update; }
	assert(!scopes.committed.equipment && !scopes.fields && !scopes.updates);
	const int powers[]={FP_ABSORB,FP_HEAL,FP_PROTECT,FP_TELEPATHY,FP_SPEED,FP_PUSH,FP_PULL,FP_SEE,FP_DRAIN,FP_LIGHTNING,FP_RAGE,FP_GRIP};
	for(int i=-2;i<14;++i) assert(TouchForce_SelectionIndex(i,powers,12)==(i>=0 && i<12?powers[i]:-1));
	touchForceBridge_t s={};
	touchForceToken_t u=TouchForce_BeginUpdate(&s),f=TouchForce_BeginField(&s);
	touchForceReport_t report=Produce(&s,FP_PUSH,-1,1);
	assert(report.learned==((1u<<1)|(1u<<4)) && report.selected==1);
	assert(!TouchForce_Current(&s,5).selected);
	TouchForce_EndField(&s,f,5); TouchForce_EndUpdate(&s,u,5);
	assert(TouchForce_Current(&s,5).selected==1);
	assert(TouchForce_Current(&s,1005).selected==1);
	assert(!TouchForce_Current(&s,1006).selected);
	for(int power=-2;power<NUM_FORCE_POWERS+2;++power) {
		u=TouchForce_BeginUpdate(&s);f=TouchForce_BeginField(&s);
		report=Produce(&s,power,FP_SEE,1);
		assert(report.selected==(power==FP_PUSH?1:4));
		TouchForce_EndField(&s,f,10);TouchForce_EndUpdate(&s,u,10);
	}
	u=TouchForce_BeginUpdate(&s);f=TouchForce_BeginField(&s);
	Produce(&s,FP_PUSH,-1,1);
	touchForceToken_t nested=TouchForce_BeginUpdate(&s),nf=TouchForce_BeginField(&s);
	Produce(&s,FP_SEE,-1,1);
	TouchForce_EndField(&s,nf,11);TouchForce_EndUpdate(&s,nested,11);
	Produce(&s,FP_PUSH,-1,1);
	TouchForce_EndField(&s,f,11);TouchForce_EndUpdate(&s,u,11);
	assert(!s.committed.equipment && !s.updates && !s.fields);
	u=TouchForce_BeginUpdate(&s);f=TouchForce_BeginField(&s);Produce(&s,FP_PUSH,-1,1);
	TouchForce_Reset(&s);Produce(&s,FP_PUSH,-1,1);
	TouchForce_EndField(&s,f,12);TouchForce_EndUpdate(&s,u,12);
	assert(!s.committed.equipment && !s.updates && !s.fields);
	u=TouchForce_BeginUpdate(&s);f=TouchForce_BeginField(&s);Produce(&s,FP_SEE,-1,1);
	TouchForce_EndField(&s,f,13);TouchForce_EndUpdate(&s,u,13);assert(s.committed.selected==4);
	u=TouchForce_BeginUpdate(&s);TouchForce_EndUpdate(&s,u,14);assert(!s.committed.equipment);
	for(int invalid=0;invalid<4;++invalid) {
		u=TouchForce_BeginUpdate(&s);f=TouchForce_BeginField(&s);Produce(&s,FP_PUSH,-1,1);
		TouchForce_Report(&s,invalid==0?2:1,invalid==1?-1:2,invalid==2?15:1,invalid==3?2:1);
		TouchForce_EndField(&s,f,15);TouchForce_EndUpdate(&s,u,15);assert(!s.committed.equipment);
	}
	int levels[NUM_FORCE_POWERS]={0};
	const int expected[]={5,6,8,7,3,1,2,4,9,11,12,10};
	for(int i=0;i<NUM_FORCE_POWERS;++i) levels[i]=1;
	for(int i=0;i<12;++i) assert(TouchForce_Produce(~0u,levels,TouchForce_SelectionIndex(i,powers,12),-1,1).selected==expected[i]);
	assert(TouchForce_Produce(~0u,levels,FP_LEVITATION,-1,1).selected==0);
	assert(TouchForce_Produce(~0u,levels,FP_SABER_OFFENSE,-1,1).selected==0);
	assert(!TouchForce_Produce(~0u,levels,FP_PUSH,-1,0).equipment);
	memset(levels,0,sizeof(levels));
	assert(!TouchForce_Produce(~0u,levels,FP_PUSH,-1,1).learned);
#if !defined(_JK2EXE) && !defined(SP_GAME)
	levels[FP_TEAM_HEAL]=1;
	report=TouchForce_Produce(1u<<FP_TEAM_HEAL,levels,FP_TEAM_HEAL,-1,1);
	assert(report.selected==13 && !report.learned);
#endif
	touchForceMedia_t media={};
	assert(!TouchForceMedia_Resolve(&media,0,1,Readable,Register,0));
	assert(!TouchForceMedia_Resolve(&media,1,0,Readable,Register,0) && !probes);
	for(int id=1;id<=14;++id) {
		assert(TouchForce_Icon(id));
		assert(!TouchForceMedia_Resolve(&media,id,1,Readable,Register,0));
		assert(!TouchForceMedia_Resolve(&media,id,1,Readable,Register,0));
	}
	assert(probes==42 && !registrations);
	available=1;TouchForceMedia_Reset(&media);
	assert(TouchForceMedia_Resolve(&media,4,1,Readable,Register,0)==17);
	assert(TouchForceMedia_Resolve(&media,4,1,Readable,Register,0)==17);
	assert(probes==43 && registrations==1);
	TouchForceMedia_Reset(&media);shaderResult=0;
	assert(!TouchForceMedia_Resolve(&media,4,1,Readable,Register,0));
	assert(!TouchForceMedia_Resolve(&media,4,1,Readable,Register,0) && registrations==2);
	puts("PASS: engine enum producer -> pending -> committed; expiry, nested, reset, no-field, invalid payload, gesture identity/cancel, legacy/host provenance, cache bounds/negative/reset");
}
