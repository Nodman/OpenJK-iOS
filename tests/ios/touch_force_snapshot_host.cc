#include "../../codemp/qcommon/msg.cpp"
#include "../../codemp/qcommon/cvar.cpp"
#include "sys/ios/sys_ios.h"
#include "cgame/cg_public.h"
#include "game/bg_public.h"
#include "sys/ios/ios_touch_force_slots.h"
#include "sys/ios/ios_touch_force_dispatch.h"
#include "client/client.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" void SnapshotCG_Init(cgameImport_t *);
extern "C" void SnapshotCG_Report(const playerState_t *,int,qboolean);
clientStatic_t cls;
qboolean com_errorEntered=qfalse;
cvar_t *cl_shownet;
char *CopyString(const char *s) {return strdup(s);}
void Z_Free(void *p) {free(p);}
qboolean CL_ConnectedToRemoteServer(void) {return qfalse;}
void QDECL Com_Printf(const char *,...) {}
void QDECL Com_DPrintf(const char *,...) {}
void QDECL Com_Error(int,const char *,...) {abort();}
int QDECL Com_sprintf(char *dst,int n,const char *format,...) {
	va_list args;va_start(args,format);const int result=vsnprintf(dst,n,format,args);va_end(args);return result;
}
static void ModuleSet(const char *name,const char *value) {Cvar_Set(name,value);}

static playerState_t Decode(const playerState_t &server,int vehicle)
{
	byte data[MAX_MSGLEN]={};msg_t msg={};msg.data=data;msg.maxsize=sizeof(data);
	MSG_WriteDeltaPlayerstate(&msg,NULL,const_cast<playerState_t *>(&server),vehicle?qtrue:qfalse);
	assert(!msg.overflowed);
	MSG_BeginReading(&msg);playerState_t client={};
	MSG_ReadDeltaPlayerstate(&msg,NULL,&client,vehicle?qtrue:qfalse);
	assert(msg.readcount<=msg.cursize);
	return client;
}
static touchForceReport_t Report(const playerState_t &ps,int selected,qboolean valid=qtrue)
{
	const auto update=IN_TouchBeginForceUpdate(),frame=IN_TouchBeginForceFrame();
	SnapshotCG_Report(&ps,selected,valid);IN_TouchEndForceFrame(frame);IN_TouchEndForceUpdate(update);
	return IN_TouchGetForceReport();
}
int main()
{
	MSG_initHuffman();cls.realtime=100;
	Cvar_Get("ui_touchForceAPI","1",CVAR_ROM|CVAR_PLATFORM_DEFAULT);
	cgameImport_t imports={};imports.Cvar_Register=Cvar_Register;imports.Cvar_Update=Cvar_Update;
	imports.Cvar_Set=ModuleSet;imports.TouchForceReport=IN_TouchReportForce;SnapshotCG_Init(&imports);
	const int powers[12]={FP_PUSH,FP_PULL,FP_SPEED,FP_SEE,FP_ABSORB,FP_HEAL,FP_TELEPATHY,FP_PROTECT,FP_DRAIN,FP_GRIP,FP_LIGHTNING,FP_RAGE};
	for(int pilot=0;pilot<2;++pilot) {
		playerState_t server={};server.clientNum=0;server.stats[STAT_HEALTH]=100;
		server.m_iVehicleNum=pilot;server.eFlags=pilot?EF_NODRAW:0;
		for(int power:powers) {server.fd.forcePowersKnown|=1u<<power;server.fd.forcePowerLevel[power]=3;}
		server.fd.forcePowerSelected=FP_PUSH;
		const auto decoded=Decode(server,0);
		assert(decoded.fd.forcePowersKnown==server.fd.forcePowersKnown);
		for(int power:powers) assert(decoded.fd.forcePowerLevel[power]==(power==FP_SEE?3:0));
		for(int i=0;i<12;++i) {
			const auto report=Report(decoded,powers[i]);
			assert(report.learned==TOUCH_FORCE_LEARNED && report.selected==i+1 && report.equipment);
			touchForceSlots_t slots;TouchForceSlots_Default(&slots);
			for(int slot=0;slot<4;++slot) assert(TouchForceSlots_Visible(&slots,slot,report.learned)==slot+1);
			touchForceDispatch_t dispatch={};
			assert(TouchForce_Reserve(&dispatch,report.selected,A_AUX21,1,100,0));
		}
		assert(Report(decoded,-1).selected==1);
		assert(Report(decoded,NUM_FORCE_POWERS).selected==1);
		auto state=decoded;state.fd.forcePowerSelected=FP_PULL;
		assert(Report(state,FP_LEVITATION).selected==2);
		state.fd.forcePowersKnown=0;auto report=Report(state,FP_PUSH);assert(!report.learned && !report.selected && report.equipment);
		state=decoded;state.stats[STAT_HEALTH]=0;assert(!Report(state,FP_PUSH).equipment);
		state=decoded;state.pm_type=PM_DEAD;assert(!Report(state,FP_PUSH).equipment);
		state=decoded;state.pm_type=PM_SPECTATOR;assert(!Report(state,FP_PUSH).equipment);
		state=decoded;state.pm_flags|=PMF_FOLLOW;assert(!Report(state,FP_PUSH).equipment);
		state=decoded;state.pm_type=PM_INTERMISSION;assert(!Report(state,FP_PUSH).equipment);
		assert(!Report(decoded,FP_PUSH,qfalse).equipment);
		state=decoded;state.fd.forcePowersKnown=(1u<<FP_LEVITATION)|(1u<<FP_SABER_OFFENSE)|(1u<<FP_SABER_DEFENSE)|(1u<<FP_SABERTHROW);
		report=Report(state,FP_LEVITATION);assert(!report.learned && !report.selected);
		state.fd.forcePowersKnown=1u<<FP_TEAM_HEAL;state.fd.forcePowerSelected=FP_TEAM_HEAL;
		report=Report(state,FP_TEAM_HEAL);assert(!report.learned && report.selected==13);
		Cvar_Restart(qtrue);SnapshotCG_Init(&imports);assert(Report(decoded,FP_PUSH).selected==1);
		Cvar_Set("ui_touchForceAPI","0");
		void *oldHost=malloc(offsetof(cgameImport_t,TouchForceReport));assert(oldHost);
		memcpy(oldHost,&imports,offsetof(cgameImport_t,TouchForceReport));
		SnapshotCG_Init(static_cast<cgameImport_t *>(oldHost));assert(!Report(decoded,FP_PUSH).equipment);free(oldHost);
		Cvar_Set("ui_touchForceAPI","1");SnapshotCG_Init(&imports);
		Report(decoded,FP_PUSH);cls.realtime+=1001;assert(!IN_TouchGetForceReport().equipment);
		const auto skipped=IN_TouchBeginForceUpdate();IN_TouchEndForceUpdate(skipped);assert(!IN_TouchGetForceReport().equipment);
	}
	puts("PASS: normal/pilot MP snapshot codec→actual CG_UpdateTouchState→native callback→committed Force report; all12, selection, equipment, capability/restart/old-host and expiry");
}
