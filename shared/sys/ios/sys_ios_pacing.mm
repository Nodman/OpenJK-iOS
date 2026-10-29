#import <UIKit/UIKit.h>
#include "sys_ios.h"
#include "qcommon/qcommon.h"
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mach/mach.h>
#include <os/proc.h>

#define FRAME_SAMPLES 1200

static qboolean framePaced;
static qboolean framePaused;
static int currentInterval;
static CFTimeInterval lastCallback;
static float samples[FRAME_SAMPLES];
static int sampleCount;
// per Com_Frame, paired by index: total, total minus present, present
static float cpuSamples[FRAME_SAMPLES], cpuNoPresent[FRAME_SAMPLES], presentSamples[FRAME_SAMPLES];
static int cpuCount;
static float framePresentMs;
static cvar_t *ios_frameStats;
static cvar_t *ios_frameStatsRun;

#define HIST_BINS 101	// 0.5 ms bins; last bin = 50 ms and above
static unsigned int runHist[HIST_BINS];
static unsigned int runCount, runLoads;
static double runSeconds;
static float memPeakMB, memLimitMB;	// limit = footprint + os_proc_available_memory() at the peak sample

static NSProcessInfoThermalState thermal = NSProcessInfoThermalStateNominal;
static float lastW, lastH;		// size of the last applied vid_restart
static float candW, candH;		// new size waiting to settle
static CFTimeInterval candSince;

static void Sys_IOS_SampleMemory( float *footMB, float *availMB ) {
	task_vm_info_data_t info;
	mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
	*footMB = task_info( mach_task_self(), TASK_VM_INFO, (task_info_t)&info, &count ) == KERN_SUCCESS
		? info.phys_footprint / 1048576.0f : -1.0f;
	*availMB = os_proc_available_memory() / 1048576.0f;
	if ( *footMB > memPeakMB ) {
		memPeakMB = *footMB;
		memLimitMB = *footMB + *availMB;
	}
}

qboolean Sys_FramePacedExternally( void ) { return framePaced; }
void Sys_IOS_SetFramePaused( qboolean paused ) { framePaused = paused; lastCallback = 0; }
int Sys_IOS_MaxFramesPerSecond( void ) { return (int)[UIScreen mainScreen].maximumFramesPerSecond; }
void Sys_IOS_NotePresent( float ms ) { framePresentMs += ms; }

static int Sys_IOS_TargetFPS( void ) {
	int maxFPS = Sys_IOS_MaxFramesPerSecond();
	int target = Cvar_VariableIntegerValue( "com_maxfps" );
	if ( target <= 0 || target > maxFPS ) target = maxFPS;
	if ( target < 30 ) target = 30;
	if ( thermal == NSProcessInfoThermalStateSerious && target > 60 ) target = 60;
	if ( thermal == NSProcessInfoThermalStateCritical ) target = 30;
	return target;
}

static int Sys_IOS_CompareFloat( const void *a, const void *b ) {
	float d = *(const float *)a - *(const float *)b;
	return d < 0 ? -1 : d > 0 ? 1 : 0;
}

static float Sys_IOS_SortedPct( float *v, int n, int pct ) {
	if ( n <= 0 )
		return 0.0f;
	qsort( v, n, sizeof( float ), Sys_IOS_CompareFloat );
	return v[n * pct / 100];
}

static float Sys_IOS_RunPercentile( float pct ) {
	unsigned int want = (unsigned int)( runCount * pct ), seen = 0;
	for ( int i = 0; i < HIST_BINS; i++ ) {
		seen += runHist[i];
		if ( seen > want )
			return ( i + 1 ) * 0.5f;	// bin upper edge
	}
	return HIST_BINS * 0.5f;
}

static void Sys_IOS_FrameStatsReset_f( void ) {
	memset( runHist, 0, sizeof( runHist ) );
	runCount = runLoads = 0;
	runSeconds = 0;
	sampleCount = cpuCount = 0;
	memPeakMB = memLimitMB = 0;
	lastCallback = 0;	// drops the interval and CPU sample of the frame running this command
	Cvar_SetValue( "ios_frameStatsRun", ios_frameStatsRun->integer + 1 );
	char stamp[32];
	time_t t = time( NULL );
	strftime( stamp, sizeof( stamp ), "%Y-%m-%dT%H:%M:%S", localtime( &t ) );
	Com_Printf( "frameStats: run-start %s\n", stamp );
}

static void Sys_IOS_RecordInterval( float ms ) {
	float memMB, availMB;
	if ( !ios_frameStats->integer )
		return;
	Sys_IOS_SampleMemory( &memMB, &availMB );	// task_info is a few microseconds
	if ( ms > 250.0f ) {	// stall; counted AND kept in the stats
		runLoads++;
		if ( ios_frameStats->integer )
			Com_Printf( "frameStats: load %.0f ms mem_mb=%.0f\n", ms, memMB );
	}
	int bin = (int)( ms / 0.5f );
	runHist[bin < HIST_BINS ? bin : HIST_BINS - 1]++;
	runCount++;
	runSeconds += ms / 1000.0;
	samples[sampleCount++] = ms;
	if ( sampleCount < FRAME_SAMPLES )
		return;
	sampleCount = 0;
	float cpu50 = Sys_IOS_SortedPct( cpuSamples, cpuCount, 50 ), cpu95 = Sys_IOS_SortedPct( cpuSamples, cpuCount, 95 );
	float np50 = Sys_IOS_SortedPct( cpuNoPresent, cpuCount, 50 ), np95 = Sys_IOS_SortedPct( cpuNoPresent, cpuCount, 95 );
	float present95 = Sys_IOS_SortedPct( presentSamples, cpuCount, 95 );
	cpuCount = 0;
	if ( !ios_frameStats->integer )
		return;
	int b9 = 0, b17 = 0, b34 = 0, bOver = 0;
	for ( int i = 0; i < FRAME_SAMPLES; i++ ) {
		if ( samples[i] <= 9.2f ) b9++; else if ( samples[i] <= 17.5f ) b17++; else if ( samples[i] <= 34.0f ) b34++; else bOver++;
	}
	qsort( samples, FRAME_SAMPLES, sizeof( float ), Sys_IOS_CompareFloat );
	Com_Printf( "frameStats: n=%d target=%d p50=%.2f p95=%.2f p99=%.2f le9=%d le17=%d le34=%d gt34=%d cpu_p50=%.2f cpu_p95=%.2f cpu_nopresent_p50=%.2f cpu_nopresent_p95=%.2f present_p95=%.2f run_n=%u run_cps=%.1f run_p99=%.1f run_loads=%u mem_mb=%.0f avail_mb=%.0f mem_peak_mb=%.0f limit_mb=%.0f\n",
		FRAME_SAMPLES, Sys_IOS_TargetFPS(), samples[FRAME_SAMPLES / 2], samples[FRAME_SAMPLES * 95 / 100],
		samples[FRAME_SAMPLES * 99 / 100], b9, b17, b34, bOver, cpu50, cpu95, np50, np95, present95,
		runCount, runSeconds > 0 ? runCount / runSeconds : 0.0, Sys_IOS_RunPercentile( 0.99f ), runLoads,
		memMB, availMB, memPeakMB, memLimitMB );
}

static void Sys_IOS_Frame( void * );

static void Sys_IOS_ApplyInterval( void ) {
	int interval = (int)( (float)Sys_IOS_MaxFramesPerSecond() / (float)Sys_IOS_TargetFPS() + 0.5f );
	if ( interval < 1 ) interval = 1;
	if ( interval != currentInterval ) {
		currentInterval = interval;
		// SDL 2.32: preferredFramesPerSecond = maximumFramesPerSecond / interval
		SDL_iPhoneSetAnimationCallback( GLimp_IOS_Window(), interval, Sys_IOS_Frame, NULL );
		Com_Printf( "iOS display: max_fps=%d interval=%d link_fps=%d low_power=%d\n", Sys_IOS_MaxFramesPerSecond(), interval,
			Sys_IOS_MaxFramesPerSecond() / interval, [NSProcessInfo processInfo].lowPowerModeEnabled ? 1 : 0 );
	}
}

// Only the frame-rate cap changes; no scale change and no vid_restart.
static void Sys_IOS_SetThermal( NSProcessInfoThermalState s ) {
	if ( s == NSProcessInfoThermalStateCritical && thermal != s )
		Com_Printf( "Device is very hot: capped at 30 fps. Lower r_renderScale to help.\n" );
	thermal = s;
}

static void Sys_IOS_CheckResize( void ) {
	float w, h;
	Sys_IOS_GetViewPointSize( GLimp_IOS_Window(), &w, &h );
	if ( w <= 0 || ( w == lastW && h == lastH ) ) { candW = candH = 0; return; }
	CFTimeInterval now = CACurrentMediaTime();
	if ( w != candW || h != candH ) {	// still changing: restart the 500 ms wait
		candW = w; candH = h; candSince = now;
		return;
	}
	if ( now - candSince < 0.5 )
		return;
	lastW = w; lastH = h; candW = candH = 0;
	Cbuf_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}

static void Sys_IOS_Frame( void * ) {
	if ( framePaused )
		return;
	CFTimeInterval now = CACurrentMediaTime();
	if ( lastCallback > 0 )
		Sys_IOS_RecordInterval( (float)( ( now - lastCallback ) * 1000.0 ) );
	lastCallback = now;
	framePresentMs = 0;
	CFTimeInterval frameStart = CACurrentMediaTime();	// Com_Frame only, not the stats work above
	Com_Frame();
	float cpuMs = (float)( ( CACurrentMediaTime() - frameStart ) * 1000.0 );
	if ( lastCallback > 0 && cpuCount < FRAME_SAMPLES ) {	// 0 if ios_frameStatsReset ran in this frame
		cpuSamples[cpuCount] = cpuMs;
		cpuNoPresent[cpuCount] = cpuMs - framePresentMs;
		presentSamples[cpuCount++] = framePresentMs;
	}
	Sys_IOS_ApplyInterval();
	Sys_IOS_CheckResize();
}

void Sys_IOS_StartFrameLoop( void ) {
	ios_frameStats = Cvar_Get( "ios_frameStats", "0", 0 );
	ios_frameStatsRun = Cvar_Get( "ios_frameStatsRun", "0", 0 );
	Cmd_AddCommand( "ios_frameStatsReset", Sys_IOS_FrameStatsReset_f );
	framePaced = qtrue;
	Sys_IOS_GetViewPointSize( GLimp_IOS_Window(), &lastW, &lastH );
	Sys_IOS_SetThermal( [NSProcessInfo processInfo].thermalState );	// warn once if already critical at launch
	// Runs on the main queue between display-link callbacks, never inside Com_Frame.
	[[NSNotificationCenter defaultCenter] addObserverForName:NSProcessInfoThermalStateDidChangeNotification
		object:nil queue:[NSOperationQueue mainQueue] usingBlock:^( NSNotification * ) {
			Sys_IOS_SetThermal( [NSProcessInfo processInfo].thermalState );
		}];
	Sys_IOS_ApplyInterval();
}
