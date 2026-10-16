#include "touch_phone_fixture.h"
#include "touch_tablet_fixture.h"
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
#include "../../shared/sys/ios/ios_touch.cpp"
#define lastUIMode gamepadLastUIMode
#include "../../shared/sys/ios/ios_gamepad.cpp"
#undef lastUIMode

static cvar_t testLayoutVar,testSlotsVar,testControlsVar,testOpacityVar,testCapsVar;
static cvar_t aimVars[6];static std::string aimTexts[6];
static const char *aimNames[6]={"in_touchAimMode","in_touchAimCurve","in_touchLookScale","in_touchDragScale","in_touchLookScaleY","in_touchDragScaleY"};
static float drawableScale=1;
static int testPointWidth=844,testPointHeight=390;
static touchLayoutProfile_t testProfile=TOUCH_LAYOUT_TABLET;
touchLayoutProfile_t Sys_IOS_TouchLayoutProfile(SDL_Window *) {return testProfile;}
static std::string testLayoutText,testSlotsText,testOpacityText;
static cvar_t *SetTouchVar(const char *name,const char *value) {
	cvar_t *var=NULL;std::string *text=NULL;
	if(!strcmp(name,"in_touchOpacity")) {var=&testOpacityVar;text=&testOpacityText;}
	if(!strcmp(name,"in_touchLayout")) { var=&testLayoutVar;text=&testLayoutText; }
	if(!strcmp(name,"in_touchForceSlots")) { var=&testSlotsVar;text=&testSlotsText; }
	for(int i=0;i<6;++i) if(!strcmp(name,aimNames[i])) {var=&aimVars[i];text=&aimTexts[i];}
	if(var) { *text=value;var->string=const_cast<char *>(text->c_str());var->value=atof(value);var->integer=atoi(value);++var->modificationCount; }
	return var;
}
void SDL_GL_GetDrawableSize(SDL_Window *,int *w,int *h) { *w=testPointWidth*drawableScale;*h=testPointHeight*drawableScale; }
void Sys_IOS_GetSafeAreaPixels(SDL_Window *,float *l,float *t,float *r,float *b) { *l=*t=*r=*b=0; }
void Sys_IOS_GetViewPointSize(SDL_Window *,float *w,float *h) { *w=testPointWidth;*h=testPointHeight; }
qboolean Sys_IOS_HardwareKeyboardConnected(void) { return qfalse; }
qboolean Sys_IOS_PointerConnected(void) { return qfalse; }
void SDL_StopTextInput(void) {}
void SDL_StartTextInput(void) {}
SDL_bool SDL_IsTextInputActive(void) { return SDL_FALSE; }
static int testTime=100;
int com_frameTime;
Uint32 SDL_GetTicks(void) { return testTime; }
#ifdef TEST_MP
static const int centralButton=9;
#else
static const int centralButton=2;
#endif
static std::deque<sysEvent_t> events;
static unsigned int droppedEvents;
int Sys_Milliseconds(bool) { return testTime; }
#ifdef TEST_MP
uint32_t cvar_modifiedFlags;
#else
int cvar_modifiedFlags;
#endif
void Sys_QueEvent(int time,sysEventType_t type,int value,int value2,int length,void *ptr) {
	if(events.size()==256) {free(events.front().evPtr);events.pop_front();++droppedEvents;}
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
qboolean Cvar_Command(void) { return Cmd_Argc()==2 && SetTouchVar(Cmd_Argv(0),Cmd_Argv(1))?qtrue:qfalse; }
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
cvar_t *Cvar_SetValue(const char *name,float value) { char s[32];snprintf(s,sizeof(s),"%.8g",value);return SetTouchVar(name,s); }
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
void Cvar_Set(const char *name,const char *value) { SetTouchVar(name,value); }
float Cvar_VariableValue(const char *) { return 0; }
void Cvar_SetValue(const char *name,float value) { char s[32];snprintf(s,sizeof(s),"%.8g",value);SetTouchVar(name,s); }
void UI_SetActiveMenu(const char *,const char *) { assert(false); }
void SCR_StopCinematic(qboolean) { assert(false); }
int Cvar_VariableIntegerValue(const char *) { return 0; }
qboolean CL_IsRunningInGameCinematic(void) { return qfalse; }
#endif
static void ExpandEvents(void) {
	while(!events.empty()) {
		const auto e=events.front();events.pop_front();
		if(e.evType==SE_JOYSTICK_AXIS) CL_JoystickEvent(e.evValue,e.evValue2,e.evTime);
		else if(e.evType==SE_KEY) CL_KeyEvent(e.evValue,e.evValue2?qtrue:qfalse,e.evTime);
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
static qboolean testNoForce;
qboolean CGVM_NoUseableForce(void) { return testNoForce; }
void UIVM_SetActiveMenu(uiMenuCommand_t) {}
void Cvar_User_SetValue(const char *,float) {}
float Cvar_VariableValue(const char *) { return 0; }
int Cvar_VariableIntegerValue(const char *) { return 0; }
float AngleNormalize180(float x) { return x; }
float AngleNormalize360(float x) { return x; }
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
cvar_t *Cvar_Set(const char *name,const char *value) { return SetTouchVar(name,value); }
cvar_t *Cvar_Get(const char *,const char *,uint32_t,const char *) { static cvar_t var;return &var; }
float AngleSubtract(float a,float b) { return a-b; }
#endif
static void Reset(void) {
	assert(events.empty());Cbuf_Init();cmd_wait=0;commandsSeen.clear();
	kg.anykeydown=qfalse;kg.keyDownCount=0;
	for(auto &key:kg.keys) {key.down=qfalse;key.repeats=0;}
	memset(in_buttons,0,sizeof(in_buttons));memset(touchForcePulses,0,sizeof(touchForcePulses));
	memset(&in_up,0,sizeof(in_up));memset(&in_down,0,sizeof(in_down));memset(touchMovePulses,0,sizeof(touchMovePulses));
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
	Cmd_AddCommand("+moveup",IN_UpDown);Cmd_AddCommand("-moveup",IN_UpUp);
	Cmd_AddCommand("+movedown",IN_DownDown);Cmd_AddCommand("-movedown",IN_DownUp);
	Key_SetBinding(A_AUX18,"+moveup");Key_SetBinding(A_AUX19,"+movedown");
	#ifdef TEST_MP
	Cmd_AddCommand("+useforce",IN_Button9Down);Cmd_AddCommand("-useforce",IN_Button9Up);
#else
	Cmd_AddCommand("+useforce",IN_Button2Down);Cmd_AddCommand("-useforce",IN_Button2Up);
#endif
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
static unsigned int ActionButtons(void) {
#ifdef TEST_MP
	return Buttons() & ~BUTTON_ANY;
#else
	return Buttons();
#endif
}
static void FenceOverflowRecovery(void) {
	for(int central:{0,1}) for(int reset:{0,1}) for(int queuedDown:{0,1}) {
		Reset();droppedEvents=0;Key_SetBinding(A_AUX21,"+useforce");
		assert(CL_TouchForceReserve(central?0:10,central?-1:0));
		const unsigned int old=touchForceDispatch.generation;
		CL_TouchForceQueue(1);if(!queuedDown) Drain();CL_TouchForceQueue(0);
		for(int i=0;i<256;++i) Sys_QueEvent(0,SE_CONSOLE,0,0,1,CopyString(""));
		assert(droppedEvents && events.size()==256 && CL_TouchForceBusy());
		Cbuf_AddText("wait\n");
		if(!reset) {
			Sys_QueEvent(0,SE_KEY,A_AUX16,qtrue,0,NULL);
			Sys_QueEvent(0,SE_KEY,A_AUX16,qfalse,0,NULL);
		}
		if(reset) CL_TouchForceReset();else CL_TouchCancelAllSources();
		ExpandEvents();Cbuf_Execute();assert(CL_TouchForceBusy());
		Drain();
		assert(!CL_TouchForceBusy() && !ActionButtons() && !in_buttons[0].active);
		assert(CL_TouchForceReserve(10,0));const unsigned int next=touchForceDispatch.generation;
		assert(next!=old);CL_TouchQueueToken(old,0);CL_TouchQueueToken(old,1);CL_TouchQueueToken(old,2);
		Drain();assert(touchForceDispatch.generation==next && !touchForceDispatch.dispatched);
		CL_TouchForceQueue(1);Drain();assert(in_buttons[6].active);
		CL_TouchForceCancel();CL_TouchForceCancel();Drain();assert(!CL_TouchForceBusy() && !ActionButtons());
	}
	Reset();Cbuf_AddText("wait\n");Sys_QueEvent(0,SE_KEY,A_AUX18,qtrue,0,NULL);
	Sys_QueEvent(0,SE_KEY,A_AUX16,qtrue,0,NULL);
	Sys_QueEvent(0,SE_KEY,A_AUX18,qfalse,0,NULL);Sys_QueEvent(0,SE_KEY,A_AUX16,qfalse,0,NULL);ExpandEvents();Cbuf_Execute();
	assert(!in_up.active && !in_buttons[0].active);
	CL_TouchCancelAllSources();Drain();assert(!in_up.active && !in_buttons[0].active && !ActionButtons());
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
		CL_TouchForceCancel();Drain();assert(!in_buttons[centralButton].active);
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
static void DemoForceRecovery(void) {
	for(int power:{0,1,10}) {
		Reset();clc.demoplaying=qtrue;assert(!CL_TouchForceReserve(power,power?0:-1));
		assert(!CL_TouchForceBusy());clc.demoplaying=qfalse;
		assert(CL_TouchForceReserve(power,power?0:-1));CL_TouchForceQueue(1);
		clc.demoplaying=qtrue;CL_TouchForceQueue(0);Drain();Drain();
		assert(!CL_TouchForceBusy() && !cl.gcmdSendValue && commandsSeen.empty() && !Buttons());
		clc.demoplaying=qfalse;assert(CL_TouchForceReserve(power,power?0:-1));
		CL_TouchForceCancel();Drain();assert(!CL_TouchForceBusy());
	}
	for(const char *binding:{"force_heal","+useforce; force_heal"}) {
		Reset();Key_SetBinding(A_AUX21,binding);assert(CL_TouchForceReserve(0,-1));CL_TouchForceQueue(1);
		clc.demoplaying=qtrue;CL_TouchForceQueue(0);Drain();
		assert(commandsSeen.empty() && !cl.gcmdSendValue && !CL_TouchForceBusy());
		clc.demoplaying=qfalse;
		Reset();Key_SetBinding(A_AUX21,binding);assert(CL_TouchForceReserve(0,-1));CL_TouchForceQueue(1);
		Cbuf_AddText("wait\n");ExpandEvents();Cbuf_Execute();
		clc.demoplaying=qtrue;CL_TouchForceQueue(0);Drain();
		assert(commandsSeen.size()==1 && commandsSeen[0]=="force_heal");
		assert(!CL_TouchForceBusy() && !in_buttons[centralButton].active && cl.gcmdSendValue);
		clc.demoplaying=qfalse;
		usercmd_t command={};CL_FinishMove(&command);cl.cmdNumber=1;cl.cmds[1]=command;CL_WritePacket();
		assert(!cl.gcmdSendValue && CL_TouchForceReserve(1,0));CL_TouchForceCancel();Drain();
	}
	Key_SetBinding(A_AUX21,"+useforce");
	Reset();assert(CL_TouchForceReserve(10,0));CL_TouchForceQueue(1);Drain();assert(in_buttons[6].active);
	clc.demoplaying=qtrue;CL_TouchForceQueue(0);Drain();assert(!in_buttons[6].active && !CL_TouchForceBusy());
	clc.demoplaying=qfalse;
}
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
static void SDLFixture(void) {
	Reset();cls.realtime=100;cls.cgameStarted=qfalse;drawableScale=1;testPointWidth=844;testPointHeight=390;testProfile=layoutProfile=TOUCH_LAYOUT_LEGACY;
	memset(&axesState,0,sizeof(axesState));dragX=dragY=0;
	for(int i=0;i<6;++i) {SetTouchVar(aimNames[i],i==1?"2":i==0?"0":"1");aimRevisions[i]=aimVars[i].modificationCount;}
	touchAimMode=&aimVars[0];touchAimCurve=&aimVars[1];touchLookScale=in_touchLookScale=&aimVars[2];touchDragScale=&aimVars[3];
	touchLookScaleY=in_touchLookScaleY=&aimVars[4];touchDragScaleY=&aimVars[5];
	memset(&testLayoutVar,0,sizeof(testLayoutVar));memset(&testSlotsVar,0,sizeof(testSlotsVar));
	SetTouchVar("in_touchLayout","");SetTouchVar("in_touchForceSlots","v1:1,2,3,4");
	testControlsVar.integer=2;testOpacityVar.value=1;
	testCapsVar.integer=TOUCH_CAP_VALID|TOUCH_CAP_EQUIPMENT|TOUCH_CAP_FORCE|TOUCH_CAP_FORCE_CYCLE;
	in_touchControls=&testControlsVar;in_touchOpacity=&testOpacityVar;ui_touchState=&testCapsVar;
	in_touchLayout=&testLayoutVar;in_touchForceSlots=&testSlotsVar;
	TouchForceSlots_Default(&forceSlots);forceSlotsModification=testSlotsVar.modificationCount;
	memset(fingers,0,sizeof(fingers));TouchGesture_Reset(&gestures);memset(&editState,0,sizeof(editState));memset(&settingsState,0,sizeof(settingsState));
	memset(&layoutOverrides,0,sizeof(layoutOverrides));TouchBridge_Reset(&bridgeState);
	viewW=844;viewH=390;safeL=safeT=safeR=safeB=0;safeH=390;pixelsPerPoint=1;
	layoutModification=testLayoutVar.modificationCount;touchUnit=TouchLayout_Unit(844,390,1);layoutReady=qtrue;
	touchWindow=reinterpret_cast<SDL_Window *>(1);lastHidden=lastUIMode=lastCinematic=qfalse;
	touchLayout=AspectLayout_Build(844,390,0,1.5f,0,0,0,0);
	TouchLayout_Build(844,390,0,0,0,0,1,rects,&layoutOverrides,layoutProfile);
	memset(&forceBridge,0,sizeof(forceBridge));
	const auto update=IN_TouchBeginForceUpdate(),frame=IN_TouchBeginForceFrame();
	IN_TouchReportForce(TOUCH_FORCE_API,TOUCH_FORCE_LEARNED,1,1);IN_TouchEndForceFrame(frame);IN_TouchEndForceUpdate(update);
	lastForceReport=IN_TouchGetForceReport();
	Key_SetBinding(A_AUX21,"+useforce");Key_SetBinding(A_AUX16,"+attack");
}
static void SDLEvent(Uint32 type,long long finger,float x,float y) {
	SDL_Event event={};event.type=type;event.tfinger.type=type;event.tfinger.fingerId=finger;
	event.tfinger.timestamp=testTime;event.tfinger.x=x/viewW;event.tfinger.y=y/viewH;IN_TouchHandleEvent(&event);
}
static void SDLButton(Uint32 type,long long finger,int button) {
	SDLEvent(type,finger,rects[button].x+rects[button].w/2,rects[button].y+rects[button].h/2);
}
static void SDLNormalLiftAndCancellation(void) {
	SDLFixture();SDLButton(SDL_FINGERDOWN,1,TB_FORCE_SLOT1);SDLButton(SDL_FINGERUP,1,TB_FORCE_SLOT1);Drain();
	assert(commandsSeen.size()==1 && commandsSeen[0]=="force_throw");
	SDLFixture();SDLButton(SDL_FINGERDOWN,2,TB_FORCE_SLOT1);IN_TouchReleaseAll();Drain();assert(commandsSeen.empty() && !Buttons());
	SDLFixture();SDLButton(SDL_FINGERDOWN,3,TB_FORCE);SDLButton(SDL_FINGERUP,3,TB_FORCE);Drain();
	assert(!in_buttons[centralButton].active && in_buttons[centralButton].wasPressed);
#ifdef TEST_MP
	assert(Buttons() & BUTTON_FORCEPOWER);
#else
	assert(Buttons() & (1u<<centralButton));
#endif
	SDLFixture();SDLButton(SDL_FINGERDOWN,4,TB_FORCE);IN_TouchReleaseAll();Drain();assert(!Buttons());
	SDLFixture();SDLButton(SDL_FINGERDOWN,5,TB_ATTACK);SDLButton(SDL_FINGERUP,5,TB_ATTACK);Drain();
	assert(!in_buttons[0].active && (Buttons() & 1u));
	SDLFixture();SDLButton(SDL_FINGERDOWN,6,TB_ATTACK);IN_TouchReleaseAll();Drain();assert(!Buttons());
#ifdef TEST_MP
	SDLFixture();cls.cgameStarted=qtrue;testNoForce=qtrue;
	SDLButton(SDL_FINGERDOWN,7,TB_FORCE);SDLButton(SDL_FINGERUP,7,TB_FORCE);Drain();
	const unsigned int buttons=Buttons();assert(!(buttons & BUTTON_FORCEPOWER) && (buttons & BUTTON_USE_HOLDABLE));testNoForce=qfalse;
#endif
}
static void SDLEditorSlotRouting(void) {
	SDLFixture();editState.active=1;
	const touchRect_t slot=rects[TB_FORCE_SLOT2];
	SDLButton(SDL_FINGERDOWN,10,TB_FORCE_SLOT2);SDLEvent(SDL_FINGERMOTION,10,slot.x+slot.w/2+2,slot.y+slot.h/2+1);
	SDLEvent(SDL_FINGERUP,10,slot.x+slot.w/2+2,slot.y+slot.h/2+1);
	assert(settingsState.tab==TOUCH_SETTINGS_FORCE && settingsState.picker==2 && !editState.dragging && !layoutOverrides.mask && events.empty());
	touchSettingsLayout_t panel;IN_TouchSettingsForce();TouchSettings_Build(&settingsState,1,844,390,0,0,0,0,1,&panel);
	touchRect_t empty={};for(int i=0;i<panel.count;++i) if(panel.items[i].control==TOUCH_SETTINGS_POWER) empty=panel.items[i].rect;
	assert(empty.w>0);SDLEvent(SDL_FINGERDOWN,11,empty.x+empty.w/2,empty.y+empty.h/2);SDLEvent(SDL_FINGERUP,11,empty.x+empty.w/2,empty.y+empty.h/2);
	assert(forceSlots.powers[1]==0 && editState.active && !layoutOverrides.mask && events.empty());
	SDLFixture();editState.active=1;const touchRect_t original=rects[TB_FORCE_SLOT1];
	SDLButton(SDL_FINGERDOWN,12,TB_FORCE_SLOT1);SDLEvent(SDL_FINGERMOTION,12,original.x+original.w/2+20,original.y+original.h/2+20);
	SDLEvent(SDL_FINGERUP,12,original.x+original.w/2+20,original.y+original.h/2+20);
	assert(!settingsState.picker && !settingsState.tab && (layoutOverrides.mask & (1u<<TB_FORCE_SLOT1)) && events.empty());
	SDLFixture();editState.active=1;
	SDLButton(SDL_FINGERDOWN,13,TB_FORCE_SLOT1);SDLButton(SDL_FINGERDOWN,14,TB_FORCE_SLOT3);
	SDLButton(SDL_FINGERUP,13,TB_FORCE_SLOT1);SDLButton(SDL_FINGERUP,14,TB_FORCE_SLOT3);
	assert(settingsState.picker==1 && !layoutOverrides.mask && events.empty());
	SDLFixture();editState.active=1;const touchRect_t edge=rects[TB_FORCE_SLOT1];
	SDLEvent(SDL_FINGERDOWN,15,edge.x+edge.w-1,edge.y+edge.h/2);SDLEvent(SDL_FINGERUP,15,edge.x+edge.w+2,edge.y+edge.h/2);
	assert(!settingsState.picker && !layoutOverrides.mask && events.empty());
	SDLFixture();editState.active=1;forceBridge.committed=lastForceReport=TouchForce_Empty();
	SDLButton(SDL_FINGERDOWN,16,TB_FORCE_SLOT4);SDLButton(SDL_FINGERUP,16,TB_FORCE_SLOT4);
	assert(settingsState.picker==4 && !settingsState.learned && !layoutOverrides.mask && events.empty());
	touchWindow=NULL;
}
static cvar_t inputZero,inputOne,inputSpeed,inputJoystick;
cvar_t *m_filter,*m_yaw,*m_pitch,*m_forward,*m_side,*cl_sensitivity,*cl_mouseAccel,*cl_showMouseRate;
#ifdef TEST_MP
cvar_t *cl_mouseAccelOffset,*cl_mouseAccelStyle,*m_pitchVeh;
#endif
cvar_t *in_joystick=&inputJoystick;
#ifndef TEST_MP
cvar_t *cl_debugMove=&inputZero,*cl_freelook=&inputOne;
#endif
void SCR_DebugGraph(float,int) {}
static void AimFixture(int mode=1) {
	SDLFixture();testTime=100;SetTouchVar("in_touchAimMode",mode?"1":"0");IN_TouchSyncAim();Drain();
	inputOne.value=1;inputOne.integer=1;inputSpeed.value=140;inputJoystick.integer=1;
	cl_anglespeedkey=cl_run=j_twinStick=j_lookSpeed=&inputOne;
	cl_yawspeed=cl_pitchspeed=&inputSpeed;
	cl_forwardspeed=cl_sidespeed=cl_upspeed=&inputSpeed;
	m_filter=m_yaw=m_pitch=m_forward=m_side=cl_sensitivity=&inputOne;cl_mouseAccel=cl_showMouseRate=&inputZero;
	cl.cgameSensitivity=1;cls.frametime=16;frame_msec=16;com_frameTime=116;
}
static void CommandFrame(int ms=16) {
	cls.frametime=ms;frame_msec=ms;com_frameTime+=ms;testTime=com_frameTime;
	IN_TouchFrame();IN_GamepadSendAxes();Drain();CL_CreateCmd();
}
static void SDLAimCommandRouting(void) {
	for(int enabled : {0,1}) {
		AimFixture(0);j_twinStick=&inputZero;inputJoystick.integer=enabled;
		SDLEvent(SDL_FINGERDOWN,50,500,300);SDLEvent(SDL_FINGERMOTION,50,500+IOS_TOUCH_STICK_RADIUS*touchUnit,300);
		CommandFrame(10);assert(fabsf(cl.viewangles[YAW]+2.1f)<.001f);IN_TouchReleaseAll();Drain();
	}
	for(int frames : {1,6,12}) {
		AimFixture();SDLEvent(SDL_FINGERDOWN,50,500,300);
		for(int i=1;i<=frames;++i) {SDLEvent(SDL_FINGERMOTION,50,500+36.f*i/frames,300+18.f*i/frames);CommandFrame(96/frames);}
		SDLEvent(SDL_FINGERUP,50,536,318);CommandFrame();
		assert(fabsf(cl.viewangles[YAW]+7.2f)<.001f && fabsf(cl.viewangles[PITCH]-3.6f)<.001f);
	}
	for(int frames : {6,12}) {
		AimFixture(0);SDLEvent(SDL_FINGERDOWN,50,500,300);SDLEvent(SDL_FINGERMOTION,50,500+IOS_TOUCH_STICK_RADIUS*touchUnit,300);
		for(int i=0;i<frames;++i) CommandFrame(96/frames);
		assert(fabsf(cl.viewangles[YAW]+20.16f)<.001f);IN_TouchReleaseAll();Drain();
	}
	AimFixture();SDLEvent(SDL_FINGERDOWN,50,500,300);SDLEvent(SDL_FINGERMOTION,50,520,310);SDLEvent(SDL_FINGERUP,50,520,310);
	CommandFrame();assert(fabsf(cl.viewangles[YAW]+4)<.001f && fabsf(cl.viewangles[PITCH]-2)<.001f);
	CommandFrame();assert(fabsf(cl.viewangles[YAW]+4)<.001f);
	AimFixture();SDLEvent(SDL_FINGERDOWN,50,500,300);
	for(int i=1;i<=4;++i) {SDLEvent(SDL_FINGERMOTION,50,500+i*5,300+i*2.5f);CommandFrame(4);}
	assert(fabsf(cl.viewangles[YAW]+4)<.001f && fabsf(cl.viewangles[PITCH]-2)<.001f);
	SDLEvent(SDL_FINGERUP,50,520,310);CommandFrame();
	AimFixture();drawableScale=3;IN_TouchUpdateLayout();Drain();
	SDLEvent(SDL_FINGERDOWN,50,1500,900);SDLEvent(SDL_FINGERMOTION,50,1560,930);SDLEvent(SDL_FINGERUP,50,1560,930);CommandFrame(33);
	assert(fabsf(cl.viewangles[YAW]+4)<.001f && fabsf(cl.viewangles[PITCH]-2)<.001f);
	AimFixture();SDLButton(SDL_FINGERDOWN,51,TB_ATTACK);Drain();assert(in_buttons[0].active);
	const auto fire=rects[TB_ATTACK];const float x=fire.x+fire.w/2,y=fire.y+fire.h/2;
	SDLEvent(SDL_FINGERMOTION,51,x+30,y);CommandFrame();assert(!cl.viewangles[YAW]);
	SDLEvent(SDL_FINGERMOTION,51,x+40,y);SDLButton(SDL_FINGERUP,51,TB_ATTACK);CommandFrame();assert(fabsf(cl.viewangles[YAW]+2)<.001f && !in_buttons[0].active);
	AimFixture();SDLEvent(SDL_FINGERDOWN,50,500,300);SDLEvent(SDL_FINGERMOTION,50,520,310);
	axesState.pad[2]=127;CommandFrame();assert(fabsf(cl.viewangles[YAW]+2.24f)<.001f);assert(!dragX && axesState.pad[2]==127 && !axesState.touch[2]);
	const float hardwareYaw=cl.viewangles[YAW];axesState.pad[2]=0;CommandFrame();assert(cl.viewangles[YAW]==hardwareYaw);
	SDLEvent(SDL_FINGERMOTION,50,530,310);IN_TouchReleaseAll();CommandFrame();assert(cl.viewangles[YAW]==hardwareYaw);
	AimFixture();axesState.pad[0]=64;axesState.pad[2]=127;Hardware("+attack",77,100);Cbuf_Execute();
	SetTouchVar("in_touchDragScale","2");IN_TouchSyncAim();Drain();assert(axesState.pad[0]==64 && axesState.pad[2]==127 && in_buttons[0].active && in_buttons[0].down[0]==77);
	Hardware("-attack",77,120);Cbuf_Execute();
	AimFixture(0);SDLEvent(SDL_FINGERDOWN,50,500,300);SDLEvent(SDL_FINGERMOTION,50,500+IOS_TOUCH_STICK_RADIUS*touchUnit,300);
	j_lookSpeed=&inputZero;inputSpeed.value=999;CommandFrame(10);assert(fabsf(cl.viewangles[YAW]+2.1f)<.001f);
	CommandFrame(10);assert(fabsf(cl.viewangles[YAW]+4.2f)<.001f);
	IN_TouchReleaseAll();Drain();
	AimFixture(0);SDLEvent(SDL_FINGERDOWN,50,500,300);SDLEvent(SDL_FINGERMOTION,50,500+IOS_TOUCH_STICK_RADIUS*touchUnit,300);
	CommandFrame(20);assert(fabsf(cl.viewangles[YAW]+4.2f)<.001f);
	SetTouchVar("in_touchAimMode","1");CommandFrame();assert(fabsf(cl.viewangles[YAW]+4.2f)<.001f && !axesState.touch[2] && gestures.lookOwner<0);
	SDLEvent(SDL_FINGERMOTION,50,600,320);CommandFrame();assert(fabsf(cl.viewangles[YAW]+4.2f)<.001f);
	AimFixture();SDLEvent(SDL_FINGERDOWN,50,500,300);SDLEvent(SDL_FINGERMOTION,50,520,310);
	SetTouchVar("in_touchAimCurve","1");CommandFrame();assert(!cl.viewangles[YAW] && !cl.viewangles[PITCH]);
	AimFixture();SDLEvent(SDL_FINGERDOWN,50,500,300);SDLEvent(SDL_FINGERDOWN,51,550,300);
	SDLEvent(SDL_FINGERMOTION,51,570,310);CommandFrame();assert(!cl.viewangles[YAW]);
	SDLEvent(SDL_FINGERUP,50,500,300);SDLEvent(SDL_FINGERMOTION,51,580,310);CommandFrame();assert(!cl.viewangles[YAW]);
	SDLEvent(SDL_FINGERUP,51,580,310);
	AimFixture();SDLEvent(SDL_FINGERDOWN,50,500,300);SDLEvent(SDL_FINGERMOTION,50,520,310);
	SDLButton(SDL_FINGERDOWN,52,TB_LOCK);SDLButton(SDL_FINGERUP,52,TB_LOCK);CommandFrame();assert(editState.active && !cl.viewangles[YAW]);
	IN_TouchEndEdit();Drain();touchWindow=NULL;
}
static void SDLPanelTap(int control,long long id=80) {
	IN_TouchSettingsForce();touchSettingsLayout_t layout;
	TouchSettings_Build(&settingsState,1,viewW,viewH,safeL,safeT,safeR,safeB,pixelsPerPoint,&layout);
	for(int i=0;i<layout.count;++i) if(layout.items[i].control==control) {
		const auto r=layout.items[i].rect;SDLEvent(SDL_FINGERDOWN,id,r.x+r.w/2,r.y+r.h/2);SDLEvent(SDL_FINGERUP,id,r.x+r.w/2,r.y+r.h/2);return;
	}
	assert(false);
}
static void SDLAimSettingsRouting(void) {
	AimFixture();editState.active=1;axesState.pad[0]=64;axesState.pad[2]=127;
	Hardware("+attack",77,100);Cbuf_Execute();
	SetTouchVar("in_touchLookScale","1.17");SetTouchVar("in_touchDragScale","2.33");IN_TouchSyncAim();Drain();
	SDLPanelTap(TOUCH_SETTINGS_AIM);
	assert(fabsf(touchDragScale->value-2.33f)<.001f && fabsf(touchLookScale->value-1.17f)<.001f);
	IN_TouchSettingsForce();touchSettingsLayout_t panel;
	TouchSettings_Build(&settingsState,1,viewW,viewH,safeL,safeT,safeR,safeB,pixelsPerPoint,&panel);
	touchRect_t r={};for(int i=0;i<panel.count;++i) if(panel.items[i].control==TOUCH_SETTINGS_SENSITIVITY) r=panel.items[i].rect;
	assert(r.w>0);const float y=r.y+r.h/2;
	SDLEvent(SDL_FINGERDOWN,81,r.x+r.w/3,y);assert(fabsf(touchDragScale->value-1.5f)<.001f && settingsState.capture[0]);
	SDLEvent(SDL_FINGERDOWN,82,r.x,y);SDLEvent(SDL_FINGERMOTION,81,r.x+r.w,y-200);assert(touchDragScale->value==4 && settingsState.capture[0]);
	SDLEvent(SDL_FINGERMOTION,81,r.x-100,y+200);assert(touchDragScale->value==.25f);
	SDLEvent(SDL_FINGERUP,81,r.x+r.w/3,y);SDLEvent(SDL_FINGERMOTION,82,r.x+r.w,y);SDLEvent(SDL_FINGERUP,82,r.x+r.w,y);Drain();
	assert(fabsf(touchDragScale->value-1.5f)<.001f && fabsf(touchLookScale->value-1.17f)<.001f && editState.active && !settingsState.scroll);
	SDLPanelTap(TOUCH_SETTINGS_MODE);SDLPanelTap(TOUCH_SETTINGS_MODE_CHOICE);Drain();assert(touchAimMode->integer==0);
	SDLPanelTap(TOUCH_SETTINGS_CURVE);SDLPanelTap(TOUCH_SETTINGS_CURVE_CHOICE+4);Drain();assert(touchAimCurve->integer==4);
	IN_TouchSettingsForce();TouchSettings_Build(&settingsState,1,viewW,viewH,safeL,safeT,safeR,safeB,pixelsPerPoint,&panel);
	for(int i=0;i<panel.count;++i) if(panel.items[i].control==TOUCH_SETTINGS_SENSITIVITY) r=panel.items[i].rect;
	SDLEvent(SDL_FINGERDOWN,84,r.x+r.w*.2f,r.y+r.h/2);assert(fabsf(touchLookScale->value-1)<.001f);
	Cbuf_AddText("in_touchAimMode 1\n");Cbuf_Execute();
	SDLEvent(SDL_FINGERMOTION,84,r.x+r.w,r.y);SDLEvent(SDL_FINGERUP,84,r.x+r.w,r.y);
	assert(fabsf(touchLookScale->value-1)<.001f && fabsf(touchDragScale->value-1.5f)<.001f);

	assert(axesState.pad[0]==64 && axesState.pad[2]==127 && in_buttons[0].active && in_buttons[0].down[0]==77);
	assert(!layoutOverrides.mask && !settingsState.picker && !CL_TouchForceBusy() && commandsSeen.empty());
	SDLPanelTap(TOUCH_SETTINGS_APPEARANCE);IN_TouchSettingsForce();
	TouchSettings_Build(&settingsState,.55f,viewW,viewH,safeL,safeT,safeR,safeB,pixelsPerPoint,&panel);
	for(int i=0;i<panel.count;++i) if(panel.items[i].control==TOUCH_SETTINGS_OPACITY) r=panel.items[i].rect;
	SDLEvent(SDL_FINGERDOWN,83,r.x,r.y+r.h/2);assert(in_touchOpacity->value==0 && IN_TouchGetOpacity()==0);
	SDLEvent(SDL_FINGERMOTION,83,r.x+r.w*.43f,r.y+r.h/2);assert(fabsf(in_touchOpacity->value-.43f)<.001f);
	SDLEvent(SDL_FINGERUP,83,r.x+r.w+100,r.y+r.h);assert(in_touchOpacity->value==1);
	SDLEvent(SDL_FINGERDOWN,85,r.x+r.w*.4f,r.y+r.h/2);const float savedOpacity=in_touchOpacity->value;
	IN_TouchReleaseAll();Drain();assert(!editState.active);
	SDLEvent(SDL_FINGERMOTION,85,r.x+r.w,r.y);SDLEvent(SDL_FINGERUP,85,r.x+r.w,r.y);
	assert(in_touchOpacity->value==savedOpacity && !layoutOverrides.mask);
	editState.active=1;SDLPanelTap(TOUCH_SETTINGS_DONE);Drain();assert(!editState.active);
	Hardware("-attack",77,120);Cbuf_Execute();touchWindow=NULL;
}
static void LateCbufAimSettings(void) {
	AimFixture();SDLEvent(SDL_FINGERDOWN,90,500,300);SDLEvent(SDL_FINGERMOTION,90,520,310);SDLEvent(SDL_FINGERUP,90,520,310);
	IN_TouchFrame();IN_GamepadSendAxes();ExpandEvents();Cbuf_AddText("in_touchDragScale 3\n");Cbuf_Execute();
	assert(dragX>0);CL_CreateCmd();assert(!cl.viewangles[YAW] && !cl.viewangles[PITCH] && !dragX);Drain();
	IN_TouchReleaseAll();Drain();touchWindow=NULL;
	for(int mode : {0,1}) for(int setting=0;setting<6;++setting) for(int hardware : {0,1}) for(int padLook : {0,1}) {
		AimFixture(mode);
		if(hardware) Hardware("+attack",77,100);
		SDLEvent(SDL_FINGERDOWN,90,500,300);SDLButton(SDL_FINGERDOWN,91,TB_ATTACK);
		SDLEvent(SDL_FINGERDOWN,92,80,300);SDLEvent(SDL_FINGERMOTION,92,100,290);
		SDLEvent(SDL_FINGERMOTION,90,500+(mode?20:IOS_TOUCH_STICK_RADIUS*touchUnit),310);
		axesState.pad[0]=padLook?64:0;axesState.pad[1]=padLook?-32:0;
		axesState.pad[2]=padLook?64:0;axesState.pad[3]=padLook?-127:0;
		IN_TouchFrame();IN_GamepadSendAxes();ExpandEvents();
		assert(gestures.lookOwner>=0 && gestures.moveOwner>=0);
		if(mode) assert(dragX>0);else assert(cl.joystickAxis[AXIS_YAW]!=0);
		char command[96];snprintf(command,sizeof(command),"%s %d\n",aimNames[setting],setting==0?!mode:setting==1?1:3);
		Cbuf_AddText(command);Cbuf_Execute();
		assert(in_buttons[0].active && gestures.lookOwner>=0);
		if(mode) assert(dragX>0);
		const usercmd_t cmd=CL_CreateCmd();
		assert(!!(cmd.buttons&1u)==hardware && !!in_buttons[0].active==hardware);
		assert(cmd.rightmove==axesState.pad[0] && cmd.forwardmove==axesState.pad[1]);
		if(hardware) assert(in_buttons[0].down[0]==77 || in_buttons[0].down[1]==77);
		assert(gestures.lookOwner<0 && gestures.moveOwner<0 && !dragX && !dragY && !IN_GamepadTouchLookActive());
		for(int i=0;i<4;++i) {assert(!axesState.touch[i]);assert(cl.joystickAxis[gamepadAxes[i]]==axesState.pad[i]);}
		const float yaw=padLook?-.016f*140*64/127:0,pitch=padLook?-.016f*140:0;
		assert(fabsf(cl.viewangles[YAW]-yaw)<.001f && fabsf(cl.viewangles[PITCH]-pitch)<.001f);
		assert(!events.empty());
		CL_CreateCmd();assert(fabsf(cl.viewangles[YAW]-2*yaw)<.001f && fabsf(cl.viewangles[PITCH]-2*pitch)<.001f);
		Drain();assert(!!in_buttons[0].active==hardware);
		SDLEvent(SDL_FINGERMOTION,90,550,320);assert(gestures.lookOwner<0 && !dragX);
		if(hardware) {Hardware("-attack",77,120);Cbuf_Execute();}
		IN_TouchReleaseAll();Drain();touchWindow=NULL;
	}
}
static kbutton_t *ActionButton(int button) {return button==TB_FORCE?&in_buttons[centralButton]:button==TB_JUMP?&in_up:&in_down;}
static void SDLHeldActionAim(void) {
	for(int mode:{0,1}) for(int button:{TB_FORCE,TB_CROUCH,TB_JUMP}) {
		AimFixture(mode);const auto r=rects[button];const float x=r.x+r.w/2,y=r.y+r.h/2;
		SDLButton(SDL_FINGERDOWN,110,button);Drain();assert(ActionButton(button)->active);
		SDLEvent(SDL_FINGERMOTION,110,x+30,y);CommandFrame();
		assert(gestures.lookOwner>=0 && ActionButton(button)->active);
		if(mode) assert(!cl.viewangles[YAW]);else assert(cl.viewangles[YAW]<0);
		const float angle=cl.viewangles[YAW];SDLEvent(SDL_FINGERMOTION,110,x+40,y);SDLButton(SDL_FINGERUP,110,button);CommandFrame();
		assert(!ActionButton(button)->active && gestures.lookOwner<0);
		if(mode) assert(fabsf(cl.viewangles[YAW]-angle+2)<.001f);
		AimFixture(mode);SDLEvent(SDL_FINGERDOWN,111,500,300);SDLButton(SDL_FINGERDOWN,112,button);
		SDLEvent(SDL_FINGERMOTION,112,x+30,y);SDLEvent(SDL_FINGERUP,111,500,300);SDLEvent(SDL_FINGERMOTION,112,x+40,y);CommandFrame();
		assert(gestures.lookOwner<0 && !cl.viewangles[YAW] && ActionButton(button)->active);
		IN_TouchReleaseAll();Drain();assert(!ActionButton(button)->active);
	}
	for(int button:{TB_CROUCH,TB_JUMP}) for(int hardware:{0,1,2}) {
		AimFixture();const char *down=button==TB_JUMP?"+moveup":"+movedown",*up=button==TB_JUMP?"-moveup":"-movedown";
		SDLButton(SDL_FINGERDOWN,113,button);
		if(hardware) {Hardware(down,77,101);if(hardware==2) Hardware(up,77,105);}
		IN_TouchFrame();IN_GamepadSendAxes();ExpandEvents();Cbuf_AddText("in_touchDragScale 2\n");Cbuf_Execute();
		const usercmd_t command=CL_CreateCmd();assert(!!ActionButton(button)->active==(hardware==1));
		assert(hardware?button==TB_JUMP?command.upmove>0:command.upmove<0:command.upmove==0);
		assert(!events.empty());Drain();assert(!!ActionButton(button)->active==(hardware==1));
		if(hardware==1) {Hardware(up,77,120);Cbuf_Execute();}
		IN_TouchReleaseAll();Drain();
		AimFixture();Hardware(down,77,100);CommandFrame();
		SDLButton(SDL_FINGERDOWN,119,button);Drain();testTime+=5;Hardware(up,77,testTime);Cbuf_Execute();
		IN_TouchReleaseAll();assert(!ActionButton(button)->active);
		const auto interval=CL_CreateCmd();assert(button==TB_JUMP?interval.upmove>0:interval.upmove<0);
		Drain();assert(CL_CreateCmd().upmove==0);
		SDLButton(SDL_FINGERDOWN,120,button);Drain();IN_TouchReleaseAll();assert(CL_CreateCmd().upmove==0);Drain();
		AimFixture();SDLButton(SDL_FINGERDOWN,114,button);testTime+=8;SDLButton(SDL_FINGERUP,114,button);Drain();
		assert(!ActionButton(button)->active);const auto tap=CL_CreateCmd();assert(button==TB_JUMP?tap.upmove>0:tap.upmove<0);assert(CL_CreateCmd().upmove==0);
		AimFixture();Cbuf_AddText("wait\n");SDLButton(SDL_FINGERDOWN,115,button);ExpandEvents();Cbuf_Execute();IN_TouchReleaseAll();Drain();assert(CL_CreateCmd().upmove==0);
	}
	AimFixture();SDLButton(SDL_FINGERDOWN,116,TB_FORCE_SLOT1);SDLButton(SDL_FINGERDOWN,117,TB_FORCE);
	const auto r=rects[TB_FORCE];SDLEvent(SDL_FINGERMOTION,117,r.x+r.w/2+30,r.y+r.h/2);assert(gestures.lookOwner<0 && !TouchGesture_HoldsButton(&gestures,117,TB_FORCE));
	IN_TouchReleaseAll();Drain();
	AimFixture();SDLButton(SDL_FINGERDOWN,118,TB_FORCE);SDLEvent(SDL_FINGERMOTION,118,r.x+r.w/2+30,r.y+r.h/2);Drain();assert(gestures.lookOwner>=0);
	const auto update=IN_TouchBeginForceUpdate(),frame=IN_TouchBeginForceFrame();IN_TouchReportForce(TOUCH_FORCE_API,0,0,0);IN_TouchEndForceFrame(frame);IN_TouchEndForceUpdate(update);
	CommandFrame();assert(!in_buttons[centralButton].active && gestures.lookOwner<0 && !dragX);
	IN_TouchReleaseAll();Drain();touchWindow=NULL;
}
static void SDLPhoneProfileDefaults(void) {
	SDLFixture();const touchRect_t oldConsole=rects[TB_CONSOLE];
	const int revision=testLayoutVar.modificationCount;
	testProfile=TOUCH_LAYOUT_PHONE;IN_TouchUpdateLayout();
	assert(layoutProfile==TOUCH_LAYOUT_PHONE && rects[TB_CONSOLE].x<viewW/2 && oldConsole.x>viewW/2);
	assert(testLayoutVar.modificationCount==revision && testLayoutText.empty());
	touchRect_t expected[TB_COUNT];TouchLayout_Build(844,390,0,0,0,0,1,expected,NULL,TOUCH_LAYOUT_PHONE);
	assert(!memcmp(expected,rects,sizeof(rects)));
	SDLButton(SDL_FINGERDOWN,201,TB_CONSOLE);assert(IN_TouchFind(201) && IN_TouchFind(201)->role==ROLE_CONSOLE);
	IN_TouchReleaseAll();Drain();assert(commandsSeen.empty());
	const float consoleX=rects[TB_CONSOLE].x+rects[TB_CONSOLE].w/2,consoleY=rects[TB_CONSOLE].y+rects[TB_CONSOLE].h/2;
	char saved[128];snprintf(saved,sizeof(saved),"v2;15:%.6f,%.6f;0:0.7,0.6;",consoleX/viewW,consoleY/viewH);
	SetTouchVar("in_touchLayout",saved);const int savedRevision=testLayoutVar.modificationCount;IN_TouchUpdateLayout();
	assert(testLayoutText==saved && testLayoutVar.modificationCount==savedRevision);
	assert(fabs(rects[TB_LOCK].x+rects[TB_LOCK].w/2-consoleX)<.001f);
	assert(fabs(rects[TB_LOCK].y+rects[TB_LOCK].h/2-consoleY)<.001f);
	testProfile=TOUCH_LAYOUT_TABLET;IN_TouchUpdateLayout();testProfile=TOUCH_LAYOUT_PHONE;IN_TouchUpdateLayout();
	assert(testLayoutText==saved && testLayoutVar.modificationCount==savedRevision);
	assert(fabs(rects[TB_ATTACK].x+rects[TB_ATTACK].w/2-.7f*viewW)<.001f);
	SetTouchVar("in_touchLayout","");IN_TouchUpdateLayout();assert(testLayoutText.empty());
	assert(!memcmp(expected,rects,sizeof(rects)));
	const float lockX=rects[TB_LOCK].x+rects[TB_LOCK].w/2,lockY=rects[TB_LOCK].y+rects[TB_LOCK].h/2;
	snprintf(saved,sizeof(saved),"v2;11:%.6f,%.6f;",lockX/viewW,lockY/viewH);
	SetTouchVar("in_touchLayout",saved);const int consoleRevision=testLayoutVar.modificationCount;IN_TouchUpdateLayout();
	assert(testLayoutText==saved && testLayoutVar.modificationCount==consoleRevision && !(layoutOverrides.mask & (1u<<TB_LOCK)));
	testProfile=TOUCH_LAYOUT_TABLET;IN_TouchUpdateLayout();testProfile=TOUCH_LAYOUT_PHONE;IN_TouchUpdateLayout();
	assert(testLayoutText==saved && testLayoutVar.modificationCount==consoleRevision && !(layoutOverrides.mask & (1u<<TB_LOCK)));
	SetTouchVar("in_touchLayout","");IN_TouchUpdateLayout();
	editState.active=1;SDLButton(SDL_FINGERDOWN,202,TB_LOCK);
	SDLEvent(SDL_FINGERMOTION,202,consoleX,consoleY);SDLEvent(SDL_FINGERUP,202,consoleX,consoleY);
	assert(editState.active && (layoutOverrides.mask & (1u<<TB_LOCK)));
	const auto &lock=rects[TB_LOCK],&console=rects[TB_CONSOLE];
	assert(lock.x+lock.w<=console.x || console.x+console.w<=lock.x || lock.y+lock.h<=console.y || console.y+console.h<=lock.y);
	IN_TouchEndEdit();Drain();touchWindow=NULL;
}

static touchRect_t SDLPanelControl(int control) {
	IN_TouchSettingsForce();touchSettingsLayout_t panel;
	TouchSettings_Build(&settingsState,IN_TouchGetOpacity(),viewW,viewH,safeL,safeT,safeR,safeB,pixelsPerPoint,&panel);
	for(int i=0;i<panel.count;++i) if(panel.items[i].control==control) return panel.items[i].rect;
	assert(false);return {};
}
static void ResetLayoutFixture(void) {
	AimFixture();testProfile=TOUCH_LAYOUT_PHONE;IN_TouchUpdateLayout();
	SetTouchVar("in_touchLayout",touchPhoneLayoutFixture);IN_TouchUpdateLayout();
	SetTouchVar("in_touchOpacity","0.37");SetTouchVar("in_touchLookScale","1.17");SetTouchVar("in_touchDragScale","2.33");
	SetTouchVar("in_touchLookScaleY","0.71");SetTouchVar("in_touchDragScaleY","1.91");
	SetTouchVar("in_touchForceSlots","v1:9,3,2,11");IN_TouchSyncAim();Drain();
	editState.active=1;settingsState.tab=TOUCH_SETTINGS_APPEARANCE;
}
static void AssertResetDefaults(void) {
	touchRect_t defaults[TB_COUNT];TouchLayout_Build(viewW,viewH,safeL,safeT,safeR,safeB,pixelsPerPoint,defaults,NULL,layoutProfile);
	assert(!layoutOverrides.mask && !memcmp(rects,defaults,sizeof(rects)));
	assert(testOpacityText=="0.37" && aimTexts[2]=="1.17" && aimTexts[3]=="2.33" && aimTexts[0]=="1" && aimTexts[1]=="2" && aimTexts[4]=="0.71" && aimTexts[5]=="1.91");
	assert(testSlotsText=="v1:9,3,2,11" && !strcmp(Key_GetBinding(A_AUX16),"+attack"));
	assert(testControlsVar.integer==2 && commandsSeen.empty() && gestures.lookOwner<0 && gestures.moveOwner<0);
}
static void SDLResetLayoutRouting(void) {
	ResetLayoutFixture();const auto reset=SDLPanelControl(TOUCH_SETTINGS_RESET_LAYOUT);
	const float x=reset.x+reset.w/2,y=reset.y+reset.h/2;
	SDLEvent(SDL_FINGERDOWN,210,x,y);assert(layoutOverrides.mask);
	SDLEvent(SDL_FINGERDOWN,211,x,y);SDLEvent(SDL_FINGERUP,210,x,y);Drain();
	assert(editState.active && settingsState.tab==TOUCH_SETTINGS_APPEARANCE && testLayoutText.empty());
	AssertResetDefaults();const int revision=testLayoutVar.modificationCount;
	SDLEvent(SDL_FINGERMOTION,211,700,300);SDLEvent(SDL_FINGERUP,211,700,300);Drain();
	assert(testLayoutVar.modificationCount==revision && !Buttons() && CL_CreateCmd().upmove==0 && gestures.lookOwner<0 && gestures.moveOwner<0);
	SDLPanelTap(TOUCH_SETTINGS_RESET_LAYOUT,212);Drain();AssertResetDefaults();
	SDLPanelTap(TOUCH_SETTINGS_DONE,213);Drain();assert(!editState.active && testLayoutText=="v2;");
	memset(&layoutOverrides,0,sizeof(layoutOverrides));layoutModification=-1;IN_TouchUpdateLayout();Drain();AssertResetDefaults();
	editState.active=1;settingsState.tab=TOUCH_SETTINGS_APPEARANCE;AssertResetDefaults();
	IN_TouchLayoutReset();Drain();assert(!editState.active && testLayoutText.empty());AssertResetDefaults();

	for(int motion=0;motion<2;++motion) {
		ResetLayoutFixture();const auto row=SDLPanelControl(TOUCH_SETTINGS_RESET_LAYOUT);const auto saved=testLayoutText;
		const float cx=row.x+row.w/2,cy=row.y+row.h/2;
		SDLEvent(SDL_FINGERDOWN,214,cx,cy);
		if(motion) SDLEvent(SDL_FINGERMOTION,214,cx+20,cy);
		SDLEvent(SDL_FINGERUP,214,cx+20,cy);Drain();assert(testLayoutText==saved && layoutOverrides.mask);
		SDLEvent(SDL_FINGERDOWN,215,cx,cy);SDLEvent(SDL_FINGERUP,215,0,0);Drain();assert(testLayoutText==saved);
	}
	ResetLayoutFixture();const auto before=SDLPanelControl(TOUCH_SETTINGS_RESET_LAYOUT);
	SDLEvent(SDL_FINGERDOWN,216,before.x+before.w/2,before.y+before.h/2);
	testPointWidth=390;testPointHeight=844;IN_TouchFrame();Drain();assert(!editState.active);
	const auto resized=testLayoutText;SDLEvent(SDL_FINGERUP,216,200,200);Drain();assert(testLayoutText==resized && layoutOverrides.mask && !Buttons());
	ResetLayoutFixture();const auto canceled=SDLPanelControl(TOUCH_SETTINGS_RESET_LAYOUT);
	SDLEvent(SDL_FINGERDOWN,217,canceled.x+canceled.w/2,canceled.y+canceled.h/2);IN_TouchReleaseAll();Drain();
	const auto saved=testLayoutText;SDLEvent(SDL_FINGERUP,217,400,230);Drain();assert(testLayoutText==saved && layoutOverrides.mask && !Buttons());

	ResetLayoutFixture();const auto jump=rects[TB_JUMP];
	SDLButton(SDL_FINGERDOWN,218,TB_JUMP);SDLEvent(SDL_FINGERMOTION,218,jump.x+jump.w/2+20,jump.y+jump.h/2-20);assert(editState.dragging);
	SDLPanelTap(TOUCH_SETTINGS_RESET_LAYOUT,219);Drain();AssertResetDefaults();assert(editState.active && !editState.dragging);
	SDLEvent(SDL_FINGERUP,218,700,300);Drain();AssertResetDefaults();
	SDLPanelTap(TOUCH_SETTINGS_DONE,220);Drain();
	ResetLayoutFixture();const auto held=rects[TB_JUMP];
	SDLButton(SDL_FINGERDOWN,230,TB_JUMP);SDLEvent(SDL_FINGERMOTION,230,held.x+held.w/2+20,held.y+held.h/2-20);
	SDLPanelTap(TOUCH_SETTINGS_RESET_LAYOUT,231);Drain();AssertResetDefaults();
	SDLPanelTap(TOUCH_SETTINGS_DONE,232);Drain();const auto cleared=testLayoutText;
	SDLEvent(SDL_FINGERUP,230,700,300);Drain();assert(testLayoutText==cleared && !Buttons());
	ResetLayoutFixture();SDLPanelTap(TOUCH_SETTINGS_RESET_LAYOUT,222);Drain();
	const auto position=rects[TB_JUMP];const float movedX=position.x+position.w/2-15,movedY=position.y+position.h/2-15;
	SDLButton(SDL_FINGERDOWN,223,TB_JUMP);SDLEvent(SDL_FINGERMOTION,223,movedX,movedY);SDLEvent(SDL_FINGERUP,223,movedX,movedY);
	assert(layoutOverrides.mask==(1u<<TB_JUMP));SDLPanelTap(TOUCH_SETTINGS_DONE,224);Drain();
	const auto dragged=testLayoutText;memset(&layoutOverrides,0,sizeof(layoutOverrides));layoutModification=-1;IN_TouchUpdateLayout();Drain();
	assert(testLayoutText==dragged && layoutOverrides.mask==(1u<<TB_JUMP));
	assert(fabsf(rects[TB_JUMP].x+rects[TB_JUMP].w/2-movedX)<.002f && fabsf(rects[TB_JUMP].y+rects[TB_JUMP].h/2-movedY)<.002f);

	ResetLayoutFixture();SDLPanelTap(TOUCH_SETTINGS_RESET_LAYOUT,225);Drain();
	testPointWidth=932;testPointHeight=430;drawableScale=3;IN_TouchFrame();Drain();AssertResetDefaults();
	testPointWidth=844;testPointHeight=390;drawableScale=1;IN_TouchFrame();Drain();AssertResetDefaults();

	ResetLayoutFixture();testPointWidth=400;testPointHeight=120;IN_TouchFrame();Drain();
	editState.active=1;settingsState.tab=TOUCH_SETTINGS_APPEARANCE;touchSettingsLayout_t panel;
	TouchSettings_Build(&settingsState,IN_TouchGetOpacity(),viewW,viewH,safeL,safeT,safeR,safeB,pixelsPerPoint,&panel);assert(panel.maxScroll>0);
	SDLEvent(SDL_FINGERDOWN,226,panel.panel.x+1,panel.panel.y+panel.panel.h-2);
	SDLEvent(SDL_FINGERMOTION,226,panel.panel.x+1,panel.panel.y+panel.panel.h-32);
	SDLEvent(SDL_FINGERUP,226,panel.panel.x+1,panel.panel.y+panel.panel.h-32);assert(settingsState.scroll>0 && layoutOverrides.mask);
	SDLPanelTap(TOUCH_SETTINGS_RESET_LAYOUT,227);Drain();AssertResetDefaults();assert(editState.active);

	AimFixture();testProfile=TOUCH_LAYOUT_PHONE;IN_TouchUpdateLayout();
	SDLButton(SDL_FINGERDOWN,228,TB_ATTACK);SDLEvent(SDL_FINGERDOWN,229,200,300);SDLEvent(SDL_FINGERMOTION,229,230,300);Drain();
	assert(in_buttons[0].active && gestures.moveOwner>=0);
	Cmd_AddCommand("touchLayoutReset",IN_TouchLayoutReset);Cmd_ExecuteString("touchLayoutReset");Drain();
	assert(!in_buttons[0].active && gestures.moveOwner<0 && gestures.lookOwner<0 && !CL_CreateCmd().buttons);
	SDLEvent(SDL_FINGERUP,228,700,300);SDLEvent(SDL_FINGERUP,229,230,300);Drain();assert(!Buttons());
	ResetLayoutFixture();Hardware("+attack",77,100);Cbuf_Execute();axesState.pad[0]=64;
	SDLPanelTap(TOUCH_SETTINGS_RESET_LAYOUT,221);Drain();assert(in_buttons[0].active && in_buttons[0].down[0]==77 && axesState.pad[0]==64);
	Hardware("-attack",77,120);Cbuf_Execute();IN_TouchEndEdit();Drain();touchWindow=NULL;
}

static void SDLTabletProfileDefaults(void) {
	SDLFixture();testPointWidth=1194;testPointHeight=834;drawableScale=2;testProfile=TOUCH_LAYOUT_TABLET;
	IN_TouchUpdateLayout();Drain();assert(layoutProfile==TOUCH_LAYOUT_TABLET && testLayoutText.empty());
	touchRect_t defaults[TB_COUNT];memcpy(defaults,rects,sizeof(rects));
	assert(rects[TB_CONSOLE].w==rects[TB_MENU].w && rects[TB_CONSOLE].h==rects[TB_PAD].h);
	const auto console=rects[TB_CONSOLE];
	assert(IN_TouchOnConsole(console.x+1,console.y+1));
	assert(!IN_TouchOnConsole(console.x+console.w+1,console.y+console.h/2));
	SDLButton(SDL_FINGERDOWN,240,TB_CONSOLE);assert(IN_TouchFind(240) && IN_TouchFind(240)->role==ROLE_CONSOLE);
	IN_TouchReleaseAll();Drain();assert(commandsSeen.empty());
	SetTouchVar("in_touchLayout",touchTabletLayoutFixture);const int savedRevision=testLayoutVar.modificationCount;
	IN_TouchUpdateLayout();Drain();assert(testLayoutText==touchTabletLayoutFixture && testLayoutVar.modificationCount==savedRevision);
	touchLayoutOverrides_t source;TouchEdit_Parse(touchTabletLayoutFixture,&source);
	for(int i=0;i<TB_COUNT;++i) {
		assert(fabsf(rects[i].x+rects[i].w/2-source.cx[i]*viewW)<.002f);
		assert(fabsf(rects[i].y+rects[i].h/2-source.cy[i]*viewH)<.002f);
	}
	editState.active=1;touchDrawItem_t draw[IOS_TOUCH_MAX_DRAW_ITEMS];const int count=IN_TouchGetDrawList(draw,IOS_TOUCH_MAX_DRAW_ITEMS);
	int seen=0;for(int i=0;i<count;++i) if(draw[i].kind==TOUCH_DRAW_BUTTON && draw[i].button==TB_CONSOLE) {
		const auto expected=TouchLayout_RectToVirtual(&touchLayout,rects[TB_CONSOLE]);
		assert(draw[i].x==expected.x && draw[i].y==expected.y && draw[i].w==expected.w && draw[i].h==expected.h);++seen;
	}
	assert(seen==1 && testLayoutVar.modificationCount==savedRevision);
	const auto start=rects[TB_CONSOLE];const float cx=start.x+start.w/2,cy=start.y+start.h/2;
	SDLEvent(SDL_FINGERDOWN,241,start.x+1,start.y+1);assert(editState.dragging && editState.button==TB_CONSOLE);
	SDLEvent(SDL_FINGERMOTION,241,start.x-19,start.y+1);SDLEvent(SDL_FINGERUP,241,start.x-19,start.y+1);
	assert(fabsf(rects[TB_CONSOLE].x+rects[TB_CONSOLE].w/2-(cx-20))<.002f);
	assert(fabsf(rects[TB_CONSOLE].y+rects[TB_CONSOLE].h/2-cy)<.002f && rects[TB_CONSOLE].w==rects[TB_MENU].w);
	IN_TouchEndEdit();Drain();
	SetTouchVar("in_touchLayout",touchTabletLayoutFixture);IN_TouchUpdateLayout();
	testPointWidth=834;testPointHeight=1194;IN_TouchUpdateLayout();
	testProfile=TOUCH_LAYOUT_PHONE;IN_TouchUpdateLayout();testProfile=TOUCH_LAYOUT_TABLET;IN_TouchUpdateLayout();
	assert(testLayoutText==touchTabletLayoutFixture);
	testPointWidth=1194;testPointHeight=834;drawableScale=1;IN_TouchUpdateLayout();drawableScale=2;IN_TouchUpdateLayout();
	assert(testLayoutText==touchTabletLayoutFixture);
	memset(&layoutOverrides,0,sizeof(layoutOverrides));layoutModification=-1;IN_TouchUpdateLayout();assert(testLayoutText==touchTabletLayoutFixture);
	editState.active=1;settingsState.tab=TOUCH_SETTINGS_APPEARANCE;
	SDLPanelTap(TOUCH_SETTINGS_RESET_LAYOUT,242);Drain();assert(editState.active && testLayoutText.empty());
	assert(!memcmp(defaults,rects,sizeof(rects)));
	SDLPanelTap(TOUCH_SETTINGS_DONE,243);Drain();assert(!editState.active && testLayoutText=="v2;");
	memset(&layoutOverrides,0,sizeof(layoutOverrides));layoutModification=-1;IN_TouchUpdateLayout();
	assert(!memcmp(defaults,rects,sizeof(rects)));
	for(const char *text:{"15:.1,.08;0:.4,.6;","15:.5,.5;10:0,0;","0:.1,.1;15:.8,.7;"}) {
		touchLayoutOverrides_t migrated;TouchEdit_Parse(text,&migrated);
		TouchEdit_Migrate(&migrated,viewW,viewH,0,0,0,0,pixelsPerPoint);
		TouchEdit_RepairLock(&migrated,viewW,viewH,0,0,0,0,pixelsPerPoint,TOUCH_LAYOUT_LEGACY);
		char expected[IOS_TOUCH_LAYOUT_TEXT];assert(TouchEdit_Serialize(&migrated,expected,sizeof(expected)));
		SetTouchVar("in_touchLayout",text);IN_TouchUpdateLayout();assert(testLayoutText==expected);
	}
	IN_TouchReleaseAll();Drain();touchWindow=NULL;
}

static void SDLPerAxisAim(void) {
	for(auto profile:{TOUCH_LAYOUT_PHONE,TOUCH_LAYOUT_TABLET}) for(int mode:{0,1}) for(int axis:{0,1,2}) for(int sign:{-1,1}) for(float p:{1.f,1.5f,2.f}) for(int ms:{8,16,32}) {
		AimFixture(mode);testProfile=profile;testPointWidth=profile==TOUCH_LAYOUT_PHONE?844:1194;testPointHeight=profile==TOUCH_LAYOUT_PHONE?390:834;drawableScale=p;IN_TouchUpdateLayout();
		SetTouchVar("in_touchLookScale","1.7");SetTouchVar("in_touchLookScaleY","0.4");SetTouchVar("in_touchDragScale","3");SetTouchVar("in_touchDragScaleY","1.8");IN_TouchSyncAim();Drain();
		const float x=.55f*viewW,y=.68f*viewH;
		const float dx=axis==1?0:mode?20*p:(axis==2?.6f:1)*IOS_TOUCH_STICK_RADIUS*touchUnit;
		const float dy=axis==0?0:mode?10*p:(axis==2?.8f:1)*IOS_TOUCH_STICK_RADIUS*touchUnit;
		SDLEvent(SDL_FINGERDOWN,250,x,y);SDLEvent(SDL_FINGERMOTION,250,x+sign*dx,y+sign*dy);CommandFrame(ms);
		const float yaw=axis==1?0:mode?-12*sign:-.001f*ms*210*1.7f*sign*(axis==2?76.f/127:1);
		const float pitch=axis==0?0:mode?3.6f*sign:.001f*ms*210*.4f*sign*(axis==2?102.f/127:1);
		assert(fabsf(cl.viewangles[YAW]-yaw)<.002f && fabsf(cl.viewangles[PITCH]-pitch)<.002f);
		if(mode) {CommandFrame(ms);assert(fabsf(cl.viewangles[YAW]-yaw)<.002f && fabsf(cl.viewangles[PITCH]-pitch)<.002f && !dragX && !dragY);}
		IN_TouchReleaseAll();Drain();touchWindow=NULL;
	}
	for(int button:{TB_ATTACK,TB_ALTATTACK}) {
		AimFixture();SetTouchVar("in_touchDragScale","3");SetTouchVar("in_touchDragScaleY","1.8");IN_TouchSyncAim();Drain();
		const float x=rects[button].x+rects[button].w/2,y=rects[button].y+rects[button].h/2;
		SDLButton(SDL_FINGERDOWN,251,button);SDLEvent(SDL_FINGERMOTION,251,x+30,y);CommandFrame();assert(!cl.viewangles[YAW] && !cl.viewangles[PITCH]);
		SDLEvent(SDL_FINGERMOTION,251,x+40,y+5);SDLButton(SDL_FINGERUP,251,button);CommandFrame();
		assert(fabsf(cl.viewangles[YAW]+6)<.001f && fabsf(cl.viewangles[PITCH]-1.8f)<.001f && !Buttons());
		IN_TouchReleaseAll();Drain();touchWindow=NULL;
	}
	AimFixture(0);SetTouchVar("in_touchLookScaleY","0.6");IN_TouchSyncAim();Drain();
	SDLEvent(SDL_FINGERDOWN,256,500,300);SDLEvent(SDL_FINGERMOTION,256,500,300+IOS_TOUCH_STICK_RADIUS*touchUnit);CommandFrame(16);
	assert(!cl.viewangles[YAW] && fabsf(cl.viewangles[PITCH]-2.016f)<.001f);IN_TouchReleaseAll();Drain();touchWindow=NULL;
	AimFixture();SetTouchVar("in_touchDragScale","nan");SetTouchVar("in_touchDragScaleY","11");IN_TouchSyncAim();Drain();
	dragX=10;dragY=-5;float yaw,pitch;IN_TouchConsumeLookDelta(&yaw,&pitch);assert(yaw==2 && pitch==-10);
	IN_TouchConsumeLookDelta(&yaw,&pitch);assert(!yaw && !pitch);
	IN_TouchReleaseAll();Drain();touchWindow=NULL;
}

static void SDLZoomAim(void) {
	for(int mode:{0,1}) for(float zoom:{1.f,.25f}) for(int pad:{0,1}) {
		AimFixture(mode);cl.cgameSensitivity=zoom;
		SetTouchVar("in_touchLookScale","1.7");SetTouchVar("in_touchLookScaleY","0.4");
		SetTouchVar("in_touchDragScale","3");SetTouchVar("in_touchDragScaleY","1.8");IN_TouchSyncAim();Drain();
		SDLEvent(SDL_FINGERDOWN,260,500,300);
		const float dx=mode?20:.6f*IOS_TOUCH_STICK_RADIUS*touchUnit;
		const float dy=mode?10:.8f*IOS_TOUCH_STICK_RADIUS*touchUnit;
		SDLEvent(SDL_FINGERMOTION,260,500+dx,300+dy);
		if(pad) {axesState.pad[2]=127;axesState.pad[3]=64;}
		CommandFrame();
		const float yaw=pad?-2.24f:zoom*(mode?-12:-.016f*210*1.7f*76/127);
		const float pitch=pad?2.24f*64/127:zoom*(mode?3.6f:.016f*210*.4f*102/127);
		assert(fabsf(cl.viewangles[YAW]-yaw)<.002f && fabsf(cl.viewangles[PITCH]-pitch)<.002f);
		if(mode) {CommandFrame();assert(fabsf(cl.viewangles[YAW]-yaw*(pad?2:1))<.002f && fabsf(cl.viewangles[PITCH]-pitch*(pad?2:1))<.002f);}
		IN_TouchReleaseAll();Drain();touchWindow=NULL;
	}
}
static void SDLPerAxisSettings(void) {
	for(int mode:{0,1}) for(int axis:{0,1}) {
		AimFixture(mode);SetTouchVar("in_touchLookScale","1.17");SetTouchVar("in_touchDragScale","2.33");SetTouchVar("in_touchLookScaleY","0.73");SetTouchVar("in_touchDragScaleY","1.93");IN_TouchSyncAim();Drain();
		const int selected=axis?(mode?5:4):(mode?3:2);
		std::string original[6];for(int i=0;i<6;++i) original[i]=aimTexts[i];
		editState.active=1;settingsState.tab=TOUCH_SETTINGS_AIM;
		const int control=axis?TOUCH_SETTINGS_SENSITIVITY_Y:TOUCH_SETTINGS_SENSITIVITY;
		const auto r=SDLPanelControl(control);const float y=r.y+r.h/2;
		SDLEvent(SDL_FINGERDOWN,252,r.x+r.w*.2f,y);assert(fabsf(aimVars[selected].value-1)<.001f && settingsState.sliderAxis==axis && settingsState.sliderMode==mode);
		SDLEvent(SDL_FINGERDOWN,253,r.x,y);SDLEvent(SDL_FINGERMOTION,252,r.x+r.w,y+88);assert(aimVars[selected].value==4 && settingsState.capture[0]);
		SDLEvent(SDL_FINGERMOTION,252,r.x-100,y-200);assert(aimVars[selected].value==.25f);
		SDLEvent(SDL_FINGERUP,252,r.x+r.w*.42f,y);const std::string saved=aimTexts[selected];
		SDLEvent(SDL_FINGERMOTION,253,r.x,y);SDLEvent(SDL_FINGERUP,253,r.x,y);assert(aimTexts[selected]==saved);
		for(int i=0;i<6;++i) if(i!=selected) assert(aimTexts[i]==original[i]);
		SDLEvent(SDL_FINGERDOWN,254,r.x+r.w*.3f,y);const std::string beforeMode=aimTexts[selected];
		SetTouchVar("in_touchAimMode",mode?"0":"1");
		SDLEvent(SDL_FINGERMOTION,254,r.x+r.w,y);SDLEvent(SDL_FINGERUP,254,r.x+r.w,y);assert(aimTexts[selected]==beforeMode);
		SetTouchVar("in_touchAimMode",mode?"1":"0");IN_TouchSyncAim();
		SDLEvent(SDL_FINGERDOWN,255,r.x+r.w*.3f,y);const std::string beforeResize=aimTexts[selected];
		testPointWidth=932;testPointHeight=430;IN_TouchFrame();assert(!editState.active);
		SDLEvent(SDL_FINGERMOTION,255,r.x+r.w,y);SDLEvent(SDL_FINGERUP,255,r.x+r.w,y);assert(aimTexts[selected]==beforeResize);
		assert(commandsSeen.empty() && !Buttons() && gestures.lookOwner<0 && gestures.moveOwner<0);
		IN_TouchReleaseAll();Drain();touchWindow=NULL;
	}
}

int main(int argc,char **argv) {
	cls.state=CA_ACTIVE;Cbuf_Init();Register();
	if(argc==2) {
		if(!strcmp(argv[1],"zoom")) SDLZoomAim();
		else if(!strcmp(argv[1],"overflow")) FenceOverflowRecovery();
#ifdef TEST_MP
		else if(!strcmp(argv[1],"demo")) DemoForceRecovery();
#endif
		else assert(false);
		return 0;
	}
	FenceOverflowRecovery();SDLZoomAim();
	CommandNamesAndTap();CancellationAndSourcePulses();SelectionAndAssignment();CentralAndReset();
#ifdef TEST_MP
	DemoForceRecovery();PacketTiming();
#endif
	SDLNormalLiftAndCancellation();SDLEditorSlotRouting();SDLAimCommandRouting();SDLAimSettingsRouting();LateCbufAimSettings();SDLHeldActionAim();SDLPhoneProfileDefaults();SDLResetLayoutRouting();SDLTabletProfileDefaults();SDLPerAxisAim();SDLPerAxisSettings();
	puts("PASS: zoom 1/.25, bounded 256-event fence recovery, demo reserve/dispatch and custom-bind limits; actual phone/tablet SP/MP per-axis drag/joystick/timing/scales/fire and captured mode/axis settings; tablet current Console draw/hit/drag/native migration/saved center/reset; actual Appearance Reset layout and held-finger cancellation; native profile cache/Console/editor/saved v2; late Cbuf settings before command with touch/hardware/pad; actual SDL aim→gamepad merge→CL_CreateCmd, drag/rate/scales/pad/cancel; SDL normal lift/cancel and editor slot routing; actual cl_input, CL_KeyEvent/CL_ParseBinding and Cbuf; deferred taps, pulses, custom binds, reset/fence and packet timing");
}
