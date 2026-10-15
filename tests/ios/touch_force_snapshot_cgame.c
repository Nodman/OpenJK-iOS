#define Com_Printf SnapshotCG_Printf
#define Com_Error SnapshotCG_Error
#include "../../codemp/cgame/cg_main.c"
#undef Com_Printf
#undef Com_Error

void SnapshotCG_Init(cgameImport_t *imports)
{
	trap=imports;
	trap->Cvar_Register(&cg_touchState,"ui_touchState","0",CVAR_ROM);
	trap->Cvar_Register(&cg_touchForceAPI,"ui_touchForceAPI","0",CVAR_ROM);
	cg.clientNum=0;
	cgs.clientinfo[0].infoValid=qtrue;
	cgs.clientinfo[0].team=TEAM_FREE;
}

void SnapshotCG_Report(const playerState_t *ps,int selected,qboolean valid)
{
	static snapshot_t snapshot;
	snapshot.ps=*ps;
	cg.snap=&snapshot;
	cg.predictedPlayerState=*ps;
	cg.forceSelect=selected;
	CG_UpdateTouchState(valid);
}

int SnapshotCG_StartFrame(const playerState_t *ps,const playerState_t *prediction,int selected,int clientValid,int team,int flags,int client,int intermission)
{
	static snapshot_t snapshot;
	snapshot.ps=*ps;
	snapshot.snapFlags=flags;
	cg.snap=&snapshot;
	cg.clientNum=client;
	cg.intermissionStarted=intermission;
	cg.predictedPlayerState=*prediction;
	cg.forceSelect=selected;
	cg.time=1000;
	cg.forceSelectTime=0;
	cgs.clientinfo[0].infoValid=clientValid;
	cgs.clientinfo[0].team=team;
	trap->SetUserCmdValue(0,1,0,0,0,CG_TouchForceSelection(),-1,qfalse);
	CG_UpdateTouchState(qtrue);
	return cg.forceSelect;
}

int SnapshotCG_Selected(void) {return cg.snap->ps.fd.forcePowerSelected;}
