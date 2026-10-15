#include "ios_touch_force_dispatch.h"
#include "sys/sys_local.h"

static touchForceDispatch_t touchForceDispatch;
static touchForcePulse_t touchForcePulses[sizeof(in_buttons)/sizeof(in_buttons[0])];
static touchForcePulse_t touchMovePulses[2];
static int CL_TouchMoveIndex(kbutton_t *button) {return button==&in_up?0:button==&in_down?1:-1;}
static void CL_TouchConsumeMove(kbutton_t *button) {
	const int index=CL_TouchMoveIndex(button);
	if(index<0) return;
	memset(&touchMovePulses[index],0,sizeof(touchMovePulses[index]));
	for(int i=0;i<2;++i) if(button->down[i])
		TouchForce_PulseDown(&touchMovePulses[index],TouchForce_SourceIndex(button->down[i],A_AUX16));
}
static int CL_TouchButtonIndex(kbutton_t *button) {
	for(unsigned int i=0;i<sizeof(in_buttons)/sizeof(in_buttons[0]);++i) if(button==&in_buttons[i]) return (int)i;
	return -1;
}
static void IN_ReleaseSource(kbutton_t *button,int source,unsigned int time) {
	if(button->down[0]==source) button->down[0]=0;
	else if(button->down[1]==source) button->down[1]=0;
	else return;
	if(button->down[0] || button->down[1]) return;
	button->active=qfalse;
	button->msec+=time ? time-button->downtime : frame_msec/2;
}
static void CL_TouchReleaseSource(int source,unsigned int time) {
	const int index=TouchForce_SourceIndex(source,A_AUX16);
	if(index<0) return;
	kbutton_t *movement[2]={&in_up,&in_down};
	for(int i=0;i<2;++i) {
		IN_ReleaseSource(movement[i],source,time);
		const int pending=TouchForce_PulseCancel(&touchMovePulses[i],index);
		if(!movement[i]->active && !pending) {movement[i]->msec=0;movement[i]->wasPressed=qfalse;}
	}
	for(unsigned int i=0;i<sizeof(in_buttons)/sizeof(in_buttons[0]);++i) {
		IN_ReleaseSource(&in_buttons[i],source,time);
		in_buttons[i].wasPressed=TouchForce_PulseCancel(&touchForcePulses[i],index)?qtrue:qfalse;
	}
}
static int CL_TouchKeyDown(kbutton_t *button,int source) {
	if(source==A_AUX21 && !TouchForce_CentralAllowed(&touchForceDispatch)) return 0;
	const int move=CL_TouchMoveIndex(button);
	if(move>=0) TouchForce_PulseDown(&touchMovePulses[move],TouchForce_SourceIndex(source,A_AUX16));
	const int index=CL_TouchButtonIndex(button);
	if(index>=0) {
		TouchForce_PulseDown(&touchForcePulses[index],TouchForce_SourceIndex(source,A_AUX16));
		button->wasPressed=qtrue;
	}
	return 1;
}
static void CL_TouchQueueToken(unsigned int generation,int operation) {
	char text[96];Com_sprintf(text,sizeof(text),"touchForceAction %u %d",generation,operation);
	Sys_QueEvent(0,SE_CONSOLE,0,0,(int)strlen(text)+1,CopyString(text));
}
static void CL_TouchFence(void) {
	if(!touchForceDispatch.generation || touchForceDispatch.fenceQueued) return;
	touchForceDispatch.fenceQueued=1;CL_TouchQueueToken(touchForceDispatch.generation,0);
}
static void CL_TouchForceToken(void) {
	const unsigned int generation=(unsigned int)strtoul(Cmd_Argv(1),NULL,10);
	const int operation=atoi(Cmd_Argv(2));
	touchForceDispatch_t *s=&touchForceDispatch;
	if(Cmd_Argc()!=3 || !generation || generation!=s->generation || operation<0 || operation>2) return;
	if(operation==0) { if(!s->fenceQueued) return;s->fenceDone=1;TouchForce_Retire(s);return; }
	if(s->cancelled) return;
	if(cls.state!=CA_ACTIVE) { CL_TouchForceCancel();return; }
	const int power=s->power,source=s->source;
	const unsigned int time=operation==1?s->downTime:s->upTime;
#ifdef _JK2EXE
	const int multiplayer=0;
#else
	const int multiplayer=1;
#endif
	if(operation==1) {
		if(s->dispatched) return;
		if(multiplayer && cl.gcmdSendValue) {
			const int lifted=s->lifted;CL_TouchForceCancel();s->lifted=lifted;return;
		}
		s->dispatched=1;
	} else if(!s->dispatched || !TouchForce_Held(power)) return;
	const char *command=TouchForce_Command(power,multiplayer);if(!command) return;
	char text[96];
	if(TouchForce_Held(power)) Com_sprintf(text,sizeof(text),"%c%s %d %u",operation==1?'+':'-',command,source,time);
	else Com_sprintf(text,sizeof(text),"%s",command);
	Cmd_ExecuteString(text);
	if(operation==1 && multiplayer && !TouchForce_Held(power)) s->pendingPacket=1;
}
static void CL_TouchSourceToken(void) {
	const int source=atoi(Cmd_Argv(1));CL_TouchReleaseSource(source,Sys_Milliseconds());
}
void CL_TouchForceInit(void) {
	static int registered;
	if(registered) return;registered=1;
	Cmd_AddCommand("touchForceAction",CL_TouchForceToken);
	Cmd_AddCommand("touchSourceCancel",CL_TouchSourceToken);
}
int CL_TouchForcePower(void) { return touchForceDispatch.generation && !touchForceDispatch.central ? touchForceDispatch.power : 0; }
int CL_TouchForceBusy(void) { return touchForceDispatch.generation!=0; }
int CL_TouchForceCentralAllowed(void) { return TouchForce_CentralAllowed(&touchForceDispatch); }
int CL_TouchForceReserve(int power,int slot) {
	if(cls.state!=CA_ACTIVE || slot< -1 || slot>3) return 0;
	const int central=slot<0;
#ifdef _JK2EXE
	const int pending=0;
#else
	const int pending=cl.gcmdSendValue;
#endif
	return TouchForce_Reserve(&touchForceDispatch,power,central?A_AUX21:IOS_TOUCH_FORCE_SOURCE_BASE+slot,central,Sys_Milliseconds(),pending)!=0;
}
void CL_TouchForceQueue(int down) {
	touchForceDispatch_t *s=&touchForceDispatch;if(!s->generation) return;
	if(down) {
		if(s->central) Sys_QueEvent(0,SE_KEY,A_AUX21,qtrue,0,NULL);
		else CL_TouchQueueToken(s->generation,1);
	} else {
		s->lifted=1;
		if(s->upQueued) { TouchForce_Retire(s);return; }
		s->upQueued=1;s->upTime=Sys_Milliseconds();
		if(s->central) Sys_QueEvent(0,SE_KEY,A_AUX21,qfalse,0,NULL);
		else CL_TouchQueueToken(s->generation,2);
		CL_TouchFence();
	}
}
void CL_TouchForceCancel(void) {
	touchForceDispatch_t *s=&touchForceDispatch;if(!s->generation) return;
	TouchForce_Cancel(s);CL_TouchReleaseSource(s->source,Sys_Milliseconds());
	CL_TouchForceQueue(0);CL_TouchFence();
}
void CL_TouchForceReset(void) {
	// Client/input resets leave queued keys and commands alive until their fence.
	CL_TouchForceCancel();if(!++touchForceDispatch.next) ++touchForceDispatch.next;
}
void CL_TouchCancelSource(int source) {
	CL_TouchReleaseSource(source,Sys_Milliseconds());
	char text[64];Com_sprintf(text,sizeof(text),"touchSourceCancel %d",source);
	Sys_QueEvent(0,SE_CONSOLE,0,0,(int)strlen(text)+1,CopyString(text));
}
void CL_TouchClearPulses(void) {
	CL_TouchForceReset();memset(touchForcePulses,0,sizeof(touchForcePulses));memset(touchMovePulses,0,sizeof(touchMovePulses));
	for(unsigned int i=0;i<sizeof(in_buttons)/sizeof(in_buttons[0]);++i) in_buttons[i].wasPressed=qfalse;
}

void CL_TouchCancelAllSources(void) {
	CL_TouchForceCancel();
	for(int i=0;i<12;++i) CL_TouchCancelSource(A_AUX16+i);
	for(int i=0;i<4;++i) CL_TouchCancelSource(IOS_TOUCH_FORCE_SOURCE_BASE+i);
}
