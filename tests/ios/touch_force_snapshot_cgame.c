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

static int hudCenter, hudName;
void CG_DrawPic(float x,float y,float w,float h,qhandle_t shader) {
	if(w==60) hudCenter=shader;
}
void CG_DrawProportionalString(int x,int y,const char *text,int style,vec4_t color) {hudName=1;}
void CG_DrawForceSelect(void);
static qboolean SnapshotCG_String(const char *key,char *buffer,int size) {Q_strncpyz(buffer,key,size);return qtrue;}
static void SnapshotCG_Color(const float *color) {}

int SnapshotCG_DrawFrame(const playerState_t *ps,int selected,int expected,int frames) {
	static snapshot_t snapshot;
	int i;
	snapshot.ps=*ps;cg.snap=&snapshot;cg.forceSelect=selected;
	cg.time=1000;cg.forceSelectTime=950;
	for(i=0;i<NUM_FORCE_POWERS;++i) cgs.media.forcePowerIcons[i]=i+1;
	trap->R_SetColor=SnapshotCG_Color;trap->SE_GetStringTextString=SnapshotCG_String;
	for(i=0;i<frames;++i) {
		int local;
		hudCenter=hudName=0;
		local=CG_TouchForceSelection();
		trap->SetUserCmdValue(0,1,0,0,0,local,-1,qfalse);
		CG_DrawForceSelect();
		if(local!=expected || cg.forceSelect!=expected) return 0;
		if(expected==-1 && ps->fd.forcePowersKnown && ps->fd.forcePowersKnown!=(1u<<FP_LEVITATION)) {
			if(hudCenter!=ps->fd.forcePowerSelected+1 || !hudName) return 0;
		} else if(!ps->fd.forcePowersKnown || ps->fd.forcePowersKnown==(1u<<FP_LEVITATION)) {
			if(hudCenter || hudName) return 0;
		} else if(hudCenter!=expected+1 || !hudName) return 0;
		++cg.time;
	}
	return 1;
}

int SnapshotCG_DrawOnly(const playerState_t *ps,int selected) {
	static snapshot_t snapshot;
	snapshot.ps=*ps;cg.snap=&snapshot;cg.forceSelect=selected;
	cg.time=1000;cg.forceSelectTime=950;hudCenter=hudName=0;
	CG_DrawForceSelect();
	if(cg.forceSelect!=selected) return -1;
	return hudCenter;
}
