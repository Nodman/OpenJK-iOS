#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>
#ifdef TEST_MP
#include "../../codemp/client/cl_input.cpp"
#include "../../codemp/qcommon/cmd.cpp"
#include "../../codemp/client/cl_keys.cpp"
#else
#include "../../code/client/cl_input.cpp"
#include "../../code/qcommon/cmd.cpp"
#include "../../code/client/cl_keys.cpp"
#endif

static int testTime=100;
static std::deque<sysEvent_t> events;
int Sys_Milliseconds(bool) { return testTime; }
#ifdef TEST_MP
uint32_t cvar_modifiedFlags;
#else
int cvar_modifiedFlags;
#endif
void Sys_QueEvent(int time,sysEventType_t type,int value,int value2,int length,void *ptr) {
	sysEvent_t e={};e.evTime=time?time:testTime;e.evType=type;e.evValue=value;e.evValue2=value2;e.evPtrLength=length;e.evPtr=ptr;events.push_back(e);
}
char *CopyString(const char *text) { return strdup(text); }
#ifdef TEST_MP
void *Z_Malloc(int size) { return calloc(1,size); }
void Z_Free(void *ptr) { free(ptr); }
#else
void *Z_Malloc(int size,memtag_t,qboolean,int) { return calloc(1,size); }
int Z_Free(void *ptr) { free(ptr);return 0; }
#endif
void *S_Malloc(int size) { return malloc(size); }
void QDECL Com_Printf(const char *,...) {}
void QDECL Com_DPrintf(const char *,...) {}
void QDECL Com_Error(int,const char *,...) { abort(); }
int QDECL Com_sprintf(char *dest,int size,const char *format,...) {
	va_list args;va_start(args,format);int n=vsnprintf(dest,size,format,args);va_end(args);return n;
}
clientStatic_t cls;
clientActive_t cl;
clientConnection_t clc;
cvar_t *com_cl_running,*com_sv_running;
qboolean Cvar_Command(void) { return qfalse; }
qboolean CL_GameCommand(void) { return qfalse; }
qboolean SV_GameCommand(void) { return qfalse; }
qboolean UI_GameCommand(void) { return qfalse; }
#ifdef TEST_MP
void CL_ForwardCommandToServer(const char *text) { fprintf(stderr,"forward: %s\n",text);abort(); }
static cvar_t camera;
cvar_t *com_cameraMode=&camera;
void CGVM_KeyEvent(int,qboolean) { assert(false); }
void UIVM_KeyEvent(int,qboolean) { assert(false); }
void CGVM_EventHandling(int) { assert(false); }
void CL_CursorButton(int) { assert(false); }
void CL_AddReliableCommand(const char *,qboolean) { assert(false); }
cvar_t *Cvar_SetValue(const char *,float) { assert(false);return NULL; }
qboolean CGVM_IncomingConsoleCommand(void) { return qtrue; }
#else
void CL_ForwardCommandToServer(void) { fprintf(stderr,"forward: %s\n",Cmd_Argv(0));abort(); }
#endif
static cvar_t altEnter;
cvar_t *cl_allowAltEnter=&altEnter;
int g_console_field_width=80;
void Con_Bottom(void) { assert(false); }
void Con_Top(void) { assert(false); }
void Con_PageUp(void) { assert(false); }
void Con_PageDown(void) { assert(false); }
void Field_Clear(field_t *field) { memset(field,0,sizeof(*field)); }
void Field_AutoComplete(field_t *) { assert(false); }
void Con_ToggleConsole_f(void) { assert(false); }
void CL_Disconnect_f(void) { assert(false); }
void SCR_UpdateScreen(void) { assert(false); }
char *Sys_GetClipboardData(void) { assert(false);return NULL; }
uint32_t ConvertUTF8ToUTF32(char *,char **) { assert(false);return 0; }
uint8_t ConvertUTF32ToExpectedCharset(uint32_t) { assert(false);return 0; }
#ifndef TEST_MP
void _UI_KeyEvent(int,qboolean) { assert(false); }
void Cvar_SetValue(const char *,float) { assert(false); }
void UI_SetActiveMenu(const char *,const char *) { assert(false); }
void SCR_StopCinematic(qboolean) { assert(false); }
int Cvar_VariableIntegerValue(const char *) { return 0; }
qboolean CL_IsRunningInGameCinematic(void) { return qfalse; }
#endif
static void ExpandEvents(void) {
	while(!events.empty()) {
		const auto e=events.front();events.pop_front();
		if(e.evType==SE_KEY) CL_KeyEvent(e.evValue,e.evValue2?qtrue:qfalse,e.evTime);
		else if(e.evType==SE_CONSOLE) { Cbuf_AddText((char *)e.evPtr);Cbuf_AddText("\n");free(e.evPtr); }
	}
}

static std::vector<std::string> commandsSeen;
static void OneShot(void) {
	const std::string name=Cmd_Argv(0);commandsSeen.push_back(name);
#ifdef TEST_MP
	if(name=="force_throw") IN_GenCMD20(); else if(name=="force_pull") IN_GenCMD5();
	else if(name=="force_speed") IN_GenCMD4();else if(name=="force_seeing") IN_GenCMD12();
	else if(name=="force_absorb") IN_GenCMD9();else if(name=="force_heal") IN_GenCMD3();
	else if(name=="force_distract") IN_GenCMD6();else if(name=="force_protect") IN_GenCMD8();
	else if(name=="force_rage") IN_GenCMD7();else assert(false);
#endif
}
#ifdef TEST_MP
static cvar_t packetdup,showSend,noDelta;
cvar_t *cl_nodelta=&noDelta,*cl_freelook=&noDelta,*cl_debugMove=&noDelta;
void CGVM_AutomapInput(void) {}
qboolean CGVM_NoUseableForce(void) { return qfalse; }
void UIVM_SetActiveMenu(uiMenuCommand_t) {}
void Cvar_User_SetValue(const char *,float) {}
float Cvar_VariableValue(const char *) { return 0; }
int Cvar_VariableIntegerValue(const char *) { return 0; }
float AngleNormalize180(float x) { return x; }
float AngleNormalize360(float x) { return x; }
void VectorCopy(const vec3_t a,vec3_t b) { memcpy(b,a,sizeof(vec3_t)); }
cvar_t *cl_packetdup=&packetdup,*cl_showSend=&showSend;
void MSG_Init(msg_t *msg,byte *data,int length) { memset(msg,0,sizeof(*msg));msg->data=data;msg->maxsize=length; }
void MSG_Bitstream(msg_t *) {}
void MSG_WriteLong(msg_t *,int) {}
void MSG_WriteByte(msg_t *,int) {}
void MSG_WriteString(msg_t *,const char *) {}
static int serialized;
void MSG_WriteDeltaUsercmdKey(msg_t *,int,usercmd_t *,usercmd_t *) { ++serialized; }
int Com_HashKey(char *,int) { return 0; }
void CL_Netchan_Transmit(netchan_t *,msg_t *) {}
void CL_Netchan_TransmitNextFragment(netchan_t *) { assert(false); }
cvar_t *Cvar_Set(const char *,const char *) { return NULL; }
cvar_t *Cvar_Get(const char *,const char *,uint32_t,const char *) { static cvar_t var;return &var; }
float AngleSubtract(float a,float b) { return a-b; }
#endif
static void Reset(void) {
	assert(events.empty());Cbuf_Init();cmd_wait=0;commandsSeen.clear();
	memset(in_buttons,0,sizeof(in_buttons));memset(touchForcePulses,0,sizeof(touchForcePulses));
	memset(&touchForceDispatch,0,sizeof(touchForceDispatch));memset(&cl,0,sizeof(cl));
	cls.state=CA_ACTIVE;
}
static void Drain(void) { ExpandEvents();Cbuf_Execute(); }
static unsigned int Buttons(void) { usercmd_t command={};CL_CmdButtons(&command);return command.buttons; }
static void Hardware(const char *command,int key,int time) {
	char text[96];snprintf(text,sizeof(text),"%s %d %d",command,key,time);Cbuf_AddText(text);Cbuf_AddText("\n");
}
static void Register(void) {
	CL_TouchForceInit();Cmd_AddCommand("wait",Cmd_Wait_f);
	Cmd_AddCommand("+useforce",IN_Button2Down);Cmd_AddCommand("-useforce",IN_Button2Up);
	Cmd_AddCommand("+attack",IN_Button0Down);Cmd_AddCommand("-attack",IN_Button0Up);
	Cmd_AddCommand("+force_grip",IN_Button6Down);Cmd_AddCommand("-force_grip",IN_Button6Up);
#ifdef TEST_MP
	Cmd_AddCommand("+force_lightning",IN_Button10Down);Cmd_AddCommand("-force_lightning",IN_Button10Up);
	Cmd_AddCommand("+force_drain",IN_Button11Down);Cmd_AddCommand("-force_drain",IN_Button11Up);
#else
	Cmd_AddCommand("+force_lightning",IN_Button1Down);Cmd_AddCommand("-force_lightning",IN_Button1Up);
	Cmd_AddCommand("+force_drain",IN_Button3Down);Cmd_AddCommand("-force_drain",IN_Button3Up);
#endif
	const char *names[]={"force_throw","force_pull","force_speed","force_sight","force_seeing","force_absorb","force_heal","force_distract","force_protect","force_rage"};
	for(const auto name:names) { Cmd_RemoveCommand(name);Cmd_AddCommand(name,OneShot); }
	Key_SetBinding(A_AUX21,"+useforce");Key_SetBinding(A_AUX16,"+attack");
}
static void CommandNamesAndTap(void) {
	const char *expected[]={0,"force_throw","force_pull","force_speed",
#ifdef TEST_MP
	"force_seeing",
#else
	"force_sight",
#endif
	"force_absorb","force_heal","force_distract","force_protect",0,0,0,"force_rage"};
	for(int power=1;power<=12;++power) {
		Reset();assert(CL_TouchForceReserve(power,0));CL_TouchForceQueue(1);CL_TouchForceQueue(0);
		assert(!CL_TouchForceReserve(2,1));assert(commandsSeen.empty());ExpandEvents();assert(commandsSeen.empty());Cbuf_Execute();
		if(expected[power]) {
			assert(commandsSeen.size()==1 && commandsSeen[0]==expected[power]);
#ifdef TEST_MP
			assert(cl.gcmdSendValue && touchForceDispatch.pendingPacket && CL_TouchForceBusy());
#endif
		} else {
			const int index=power==10?6:
#ifdef TEST_MP
				power==11?10:11;
#else
				power==11?1:3;
#endif
			assert(!in_buttons[index].active && in_buttons[index].wasPressed);
			assert(Buttons() & (1u<<index));assert(!(Buttons() & (1u<<index)));
			assert(!CL_TouchForceBusy());
		}
	}
}
static void CancellationAndSourcePulses(void) {
	Reset();assert(CL_TouchForceReserve(10,0));const unsigned int old=touchForceDispatch.generation;
	CL_TouchForceQueue(1);CL_TouchForceCancel();assert(!CL_TouchForceReserve(10,0));Drain();assert(!Buttons() && !CL_TouchForceBusy());
	assert(CL_TouchForceReserve(10,0));const unsigned int next=touchForceDispatch.generation;
	CL_TouchQueueToken(old,1);CL_TouchQueueToken(old,2);CL_TouchQueueToken(old,0);Drain();
	assert(touchForceDispatch.generation==next && !touchForceDispatch.dispatched);CL_TouchForceCancel();Drain();
	Reset();assert(CL_TouchForceReserve(10,0));CL_TouchForceQueue(1);CL_TouchForceQueue(0);
	assert(!CL_TouchForceReserve(1,1));CL_TouchForceCancel();Drain();assert(!Buttons() && !CL_TouchForceBusy());
	Reset();assert(CL_TouchForceReserve(10,0));CL_TouchForceQueue(1);Drain();assert(in_buttons[6].active);
	Hardware("+force_grip",77,101);Hardware("-force_grip",77,102);Cbuf_Execute();
	assert(in_buttons[6].active && touchForcePulses[6].other);
	CL_TouchForceCancel();assert(!in_buttons[6].active);Drain();assert(Buttons() & (1u<<6));assert(!Buttons());
	Reset();Hardware("+force_grip",77,100);Cbuf_Execute();
	assert(CL_TouchForceReserve(10,0));CL_TouchForceQueue(1);Drain();CL_TouchForceQueue(0);Drain();
	assert(in_buttons[6].active && in_buttons[6].down[0]==77);Hardware("-force_grip",77,105);Cbuf_Execute();
	Reset();Sys_QueEvent(0,SE_KEY,A_AUX16,qtrue,0,NULL);Sys_QueEvent(0,SE_KEY,A_AUX16,qfalse,0,NULL);Drain();
	CL_TouchCancelAllSources();Drain();assert(!Buttons());
	Reset();Hardware("+attack",A_AUX16,100);Cbuf_Execute();Hardware("+attack",77,101);Hardware("-attack",77,102);Cbuf_Execute();
	CL_TouchCancelAllSources();Hardware("-attack",A_AUX16,103);Drain();assert(Buttons() & 1u);
	Reset();Cbuf_AddText("wait\n");Sys_QueEvent(0,SE_KEY,A_AUX16,qtrue,0,NULL);ExpandEvents();Cbuf_Execute();
	Sys_QueEvent(0,SE_KEY,A_AUX16,qfalse,0,NULL);CL_TouchCancelAllSources();ExpandEvents();Cbuf_Execute();assert(!Buttons());
}
static void SelectionAndAssignment(void) {
	Reset();touchForceBridge_t bridge={};bridge.producerSeen=1;
	touchForceReport_t report={1u<<10,0,1};
	assert(CL_TouchForceReserve(10,0));CL_TouchForceQueue(1);Drain();
	assert(!TouchForce_ShouldCancelPower(&bridge,report,report,CL_TouchForcePower()));
	assert(TouchForce_ShouldCancelPower(&bridge,report,report,0));
	touchForceSlots_t slots={{10,0,0,0}};
	CL_TouchForceCancel();TouchForceSlots_Assign(&slots,0,11,(1u<<10)|(1u<<11));Drain();
	assert(!in_buttons[6].active && !Buttons() && slots.powers[0]==11);
}
static void CentralAndReset(void) {
	const char *bindings[]={"+useforce","force_heal","+useforce; force_heal"};
	for(const auto binding:bindings) {
		Reset();Key_SetBinding(A_AUX21,binding);assert(CL_TouchForceReserve(0,-1));CL_TouchForceQueue(1);
		CL_TouchForceCancel();assert(!CL_TouchForceReserve(1,0));Drain();assert(commandsSeen.empty() && !Buttons() && !CL_TouchForceBusy());
		Reset();Key_SetBinding(A_AUX21,binding);assert(CL_TouchForceReserve(0,-1));CL_TouchForceQueue(1);ExpandEvents();
		CL_TouchForceCancel();Drain();assert(!in_buttons[2].active);
		assert(commandsSeen.size()==(strcmp(binding,"+useforce")?1u:0u));
	}
	Reset();Key_SetBinding(A_AUX21,"+useforce");assert(CL_TouchForceReserve(0,-1));CL_TouchForceQueue(1);
	Cbuf_AddText("wait\n");ExpandEvents();Cbuf_Execute();CL_TouchForceReset();
	const unsigned int old=touchForceDispatch.generation;assert(old && !CL_TouchForceReserve(0,-1));
	ExpandEvents();Cbuf_Execute();assert(!CL_TouchForceBusy() && !Buttons());
	assert(CL_TouchForceReserve(10,0));const unsigned int next=touchForceDispatch.generation;assert(next!=old);
	CL_TouchQueueToken(old,1);CL_TouchQueueToken(old,0);Drain();assert(touchForceDispatch.generation==next && !in_buttons[6].active);
	CL_TouchForceCancel();Drain();
	Reset();assert(CL_TouchForceReserve(0,-1));CL_TouchForceQueue(1);CL_TouchForceReset();
#ifdef TEST_MP
	cls.state=CA_DISCONNECTED;CL_ShutdownInput();
#endif
	Drain();assert(!CL_TouchForceBusy());
#ifdef TEST_MP
	CL_InitInput();Register();cls.state=CA_ACTIVE;
#endif
	CL_TouchForceInit();assert(CL_TouchForceReserve(0,-1));CL_TouchForceCancel();Drain();
}
#ifdef TEST_MP
static void PacketTiming(void) {
	Reset();cl_nodelta=&noDelta;assert(CL_TouchForceReserve(1,0));CL_TouchForceQueue(1);Drain();
	usercmd_t held={};CL_FinishMove(&held);cl.cmdNumber=1;cl.cmds[1]=held;CL_WritePacket();
	assert(CL_TouchForceBusy() && !cl.gcmdSendValue && !CL_TouchForceReserve(2,1));
	CL_TouchForceQueue(0);Drain();assert(!CL_TouchForceBusy());
	Reset();assert(CL_TouchForceReserve(1,0));CL_TouchForceQueue(1);Drain();CL_TouchForceCancel();Drain();
	assert(!CL_TouchForceBusy() && cl.gcmdSendValue && !CL_TouchForceReserve(2,1));
	Reset();cl_nodelta=&noDelta;assert(CL_TouchForceReserve(1,0));CL_TouchForceQueue(1);CL_TouchForceQueue(0);Drain();
	assert(cl.gcmdSendValue && CL_TouchForceBusy());usercmd_t command={};CL_FinishMove(&command);
	assert(cl.gcmdSendValue && CL_TouchForceBusy() && command.generic_cmd==GENCMD_FORCE_THROW);
	cl.cmdNumber=1;cl.cmds[1]=command;clc.demoplaying=qtrue;CL_WritePacket();assert(CL_TouchForceBusy());clc.demoplaying=qfalse;
	CL_WritePacket();assert(serialized && !cl.gcmdSendValue && !CL_TouchForceBusy());
	assert(CL_TouchForceReserve(2,1));CL_TouchForceCancel();Drain();
	Reset();cl.gcmdSendValue=qtrue;assert(!CL_TouchForceReserve(1,0));
	Reset();assert(CL_TouchForceReserve(1,0));CL_TouchForceQueue(1);
	Cbuf_AddText("force_heal\n");ExpandEvents();Cbuf_Execute();assert(commandsSeen.size()==1 && commandsSeen[0]=="force_heal");
	ExpandEvents();Cbuf_Execute();assert(CL_TouchForceBusy());CL_TouchForceQueue(0);Drain();assert(!CL_TouchForceBusy());
	assert(cl.gcmdValue==GENCMD_FORCE_HEAL && cl.gcmdSendValue);
	Reset();assert(CL_TouchForceReserve(1,0));CL_TouchForceQueue(1);cls.state=CA_DISCONNECTED;Drain();Drain();assert(!CL_TouchForceBusy() && commandsSeen.empty());
}
#endif
int main() {
	cls.state=CA_ACTIVE;Cbuf_Init();Register();
	CommandNamesAndTap();CancellationAndSourcePulses();SelectionAndAssignment();CentralAndReset();
#ifdef TEST_MP
	PacketTiming();
#endif
	puts("PASS: actual cl_input, CL_KeyEvent/CL_ParseBinding and Cbuf; deferred taps, pulses, custom binds, reset/fence and packet timing");
}
