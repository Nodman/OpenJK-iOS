// Run with tests/ios/cvar_platform_defaults_host.py; each engine uses its real cvar.cpp.
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>

#ifdef TEST_MP
#include "../../codemp/qcommon/cvar.cpp"
#else
#include "../../code/qcommon/cvar.cpp"
#endif

static std::string savedConfig;
static std::string commandArgs[3];
int Cmd_Argc( void ) { return 3; }
char *Cmd_Argv( int arg ) { return const_cast<char *>( commandArgs[arg].c_str() ); }
char *Cmd_ArgsFrom( int arg ) { assert( arg == 2 ); return Cmd_Argv( arg ); }
qboolean com_errorEntered = qfalse;
char *CopyString( const char *s ) { return strdup( s ); }
#ifdef TEST_MP
void Z_Free( void *p ) { free( p ); }
qboolean CL_ConnectedToRemoteServer( void ) { return qfalse; }
#else
int Z_Free( void *p ) { free( p ); return 0; }
#endif
void QDECL Com_Printf( const char *, ... ) {}
void QDECL Com_DPrintf( const char *, ... ) {}
void QDECL Com_Error( int, const char *, ... ) { abort(); }
int QDECL Com_sprintf( char *dest, int size, const char *fmt, ... ) {
	va_list args;
	va_start( args, fmt );
	int n = vsnprintf( dest, size, fmt, args );
	va_end( args );
	return n;
}
int FS_Write( const void *buffer, int len, fileHandle_t ) {
	savedConfig.append( static_cast<const char *>( buffer ), len );
	return len;
}

static cvar_t *Seta( const char *name, const char *value ) {
#ifdef TEST_MP
	cvar_t *var = Cvar_Set2( name, value, CVAR_USER_CREATED, qfalse );
#else
	cvar_t *var = Cvar_Set2( name, value, qfalse );
#endif
	var->flags |= CVAR_ARCHIVE;
	return var;
}

static void Expect( const char *name, const char *value, const char *reset ) {
	cvar_t *var = Cvar_FindVar( name );
	if(!var || strcmp(var->string,value) || strcmp(var->resetString,reset))
		fprintf(stderr,"%s: expected %s/reset%s, got %s/reset%s\n",name,value,reset,var ? var->string : "missing",var ? var->resetString : "missing");
	assert( var && !strcmp( var->string, value ) && !strcmp( var->resetString, reset ) );
}

static void ReadSetas(std::istream &input) {
	std::string line;
	while(std::getline(input,line)) {
		std::istringstream fields(line);std::string command,name,value;
		if(!(fields>>command>>name) || command!="seta") continue;
		std::getline(fields>>std::ws,value);
		if(value.size()>=2 && value.front()=='"' && value.back()=='"') value=value.substr(1,value.size()-2);
		Seta(name.c_str(),value.c_str());
	}
}

#ifdef __APPLE__
// The complete device-default body is compiled as Objective-C++. Foundation's
// process-memory query is real; GPU classification and panel maximum are inputs.
#import <Foundation/Foundation.h>
static int hostPanelFPS = 60, hostGPUClass = 0;
static bool hostPhone = false;
enum { IOS_CLASS_A12, IOS_CLASS_A14, IOS_CLASS_A17 };
static int Sys_IOS_Class() { return hostGPUClass; }
int Sys_IOS_MaxFramesPerSecond() { return hostPanelFPS; }
qboolean Sys_IOS_IsPhone() { return hostPhone ? qtrue : qfalse; }
#include "device_defaults.inc"

// Config storage and token transport are bounded stubs. Startup/capture/order,
// command-line application and all cvar/reset/archive logic are production code.
static std::string hostSaved, hostAutoexec;
static int com_numConsoleLines;
static char hostConsoleLine[] = "set com_maxfps 120";
static char *com_consoleLines[] = {hostConsoleLine};
static bool hostSafeMode;
qboolean Com_SafeMode() { return hostSafeMode ? qtrue : qfalse; }
void IN_TouchRestoreDefaultBinds() {}
void Cbuf_Execute() {}
void Cbuf_ExecuteText(int, const char *text) {
	std::istringstream input(text);
	std::string command, file;
	input >> command >> file;
	if(command != "exec") { std::istringstream values(text); ReadSetas(values); return; }
	if(file == "ios_default.cfg") {
		std::ifstream cfg("shared/ios/ios_default.cfg"); assert(cfg.good()); ReadSetas(cfg);
	} else if(file == "autoexec.cfg" || file == "autoexec_sp.cfg") {
		std::istringstream cfg(hostAutoexec); ReadSetas(cfg);
	} else if(file != "default.cfg" && file != "mpdefault.cfg") {
		std::istringstream cfg(hostSaved); ReadSetas(cfg);
	}
}
void Cmd_TokenizeString(const char *text) {
	std::istringstream input(text);
	for(auto &token : commandArgs) input >> token;
}
#include "config_migration.inc"
#include "startup_defaults.inc"
#include "rate_registration.inc"

static void RatePlatformDefaults() {
	for(int panel : {60, 120}) for(int gpu : {IOS_CLASS_A12, IOS_CLASS_A14, IOS_CLASS_A17}) {
		hostPanelFPS = panel; hostGPUClass = gpu;
		auto startup = [&](const std::string &saved, const std::string &autoexec, bool optIn) {
			while(cvar_vars) Cvar_Unset(cvar_vars);
			cvar_cheats = Cvar_Get("host_cheats", "1", 0);
			hostSaved = saved; hostAutoexec = autoexec;
			com_numConsoleLines = optIn ? 1 : 0;
			Com_StartupVariable(NULL); // early +set, as in Com_Init
			Com_ExecuteCfg();
			Com_StartupVariable(NULL); // final command-line precedence
			RegisterRate(); // actual late Sys_Init registration
		};
		const char *fresh = panel >= 120 ? "120" : "60";
		startup("", "", false); Expect("com_maxfps", fresh, fresh);
		Expect("r_texturebits", "32", "32");
		if(gpu == IOS_CLASS_A12) Expect("r_surfaceSprites", "0", "0");
		for(const char *savedRate : {"60", "120"}) {
			startup(std::string("seta com_maxfps ") + savedRate + "\n", "", false);
			Expect("com_maxfps", savedRate, fresh);
			auto *fps = Cvar_FindVar("com_maxfps");
			assert((fps->flags & (CVAR_ARCHIVE | CVAR_PLATFORM_DEFAULT)) == (CVAR_ARCHIVE | CVAR_PLATFORM_DEFAULT));
			savedConfig.clear(); Cvar_WriteVariables(1);
			// The real Sys_Init registration archives both saved rates.
			assert(savedConfig.find(std::string("seta com_maxfps \"") + savedRate + "\"\n") != std::string::npos);
			const std::string archived = savedConfig;
			startup(archived, "", false); Expect("com_maxfps", savedRate, fresh);
			Cvar_Reset("com_maxfps"); Expect("com_maxfps", fresh, fresh);
			Seta("com_maxfps", savedRate); Cvar_Restart(qfalse); Expect("com_maxfps", fresh, fresh);
		}
		startup("seta com_maxfps 60\n", "seta com_maxfps 120\n", false);
		Expect("com_maxfps", "120", fresh);
		startup("seta com_maxfps 60\n", "seta com_maxfps 60\n", true);
		Expect("com_maxfps", "120", fresh);
		printf("PASS: actual device/startup defaults panel=%d GPU=%d fresh%s, saved60/120, archive/reload, reset/restart, autoexec120, command-line120\n",panel,gpu,fresh);
	}
	while(cvar_vars) Cvar_Unset(cvar_vars);
}

static void CameraPlatformDefaults() {
	const char *names[]={"cg_thirdPersonRange","cg_fovViewmodel","cg_gunX"};
	const char *engine[]={"80","0","0"}, *phone[]={"130","100","5"};
	auto startup=[&](const std::string &saved) {
		while(cvar_vars) Cvar_Unset(cvar_vars);
		cvar_cheats=Cvar_Get("host_cheats","1",0);
		hostSaved=saved; hostAutoexec=""; com_numConsoleLines=0;
		Com_ExecuteCfg();
		for(int i=0;i<3;++i) Cvar_Get(names[i],engine[i],CVAR_ARCHIVE); // cgame registration
	};
	for(bool isPhone : {true,false}) {
		hostPhone=isPhone; hostPanelFPS=60; hostGPUClass=IOS_CLASS_A17;
		const char **want=isPhone ? phone : engine;
		startup("");
		for(int i=0;i<3;++i) Expect(names[i],want[i],want[i]);
		Cvar_Restart(qfalse);
		for(int i=0;i<3;++i) Expect(names[i],want[i],want[i]);
		startup("seta cg_thirdPersonRange 100\n");
		Expect("cg_thirdPersonRange","100",want[0]);
		Cvar_Reset("cg_thirdPersonRange");
		Expect("cg_thirdPersonRange",want[0],want[0]);
		// 0.1.0 saved every archived cvar, also ones at the old engine default.
		const char *exact[]={"80","0","0"}, *decimal[]={"80.0","0.0","0"};
		for(const char **old : {exact,decimal}) {
			std::string saved;
			for(int i=0;i<3;++i) saved+=std::string("seta ")+names[i]+" "+old[i]+"\n";
			startup(saved);
			for(int i=0;i<3;++i) Expect(names[i],isPhone ? phone[i] : old[i],want[i]);
			Expect("ios_configVersion","1","0");
			savedConfig.clear(); Cvar_WriteVariables(1);
			assert(savedConfig.find("seta ios_configVersion \"1\"\n")!=std::string::npos);
		}
		startup("seta cg_thirdPersonRange 100\nseta cg_fovViewmodel 0\n");
		Expect("cg_thirdPersonRange","100",want[0]);
		Expect("cg_fovViewmodel",want[1],want[1]);
		startup("seta ios_configVersion 1\nseta cg_thirdPersonRange 80\nseta cg_gunX 0\n");
		Expect("cg_thirdPersonRange","80",want[0]);
		Expect("cg_gunX","0",want[2]);
		Expect("ios_configVersion","1","0");
		hostSafeMode=true;
		startup("seta cg_thirdPersonRange 80\n");
		Expect("cg_thirdPersonRange",want[0],want[0]);
		assert((unsigned)Cvar_Flags("ios_configVersion")==CVAR_NONEXISTENT);
		hostSafeMode=false;
	}
	hostPhone=false;
	while(cvar_vars) Cvar_Unset(cvar_vars);
	puts("PASS: camera defaults phone 130/100/5, tablet engine 80/0/0, cvar_restart, saved 100 then reset");
	puts("PASS: config migration saved 80/0/0 and 80.0/0.0 phone->130/100/5 tablet kept, changed kept, version 1 kept, safe mode skipped");
}

#endif

static void TouchPlatformDefaults(void) {
	const char *names[]={"in_touchAimMode","in_touchLookScale","in_touchLookScaleY","in_touchDragScale","in_touchDragScaleY"};
	const char *defaults[]={"1","1.0","0.6","3.0","1.8"};
	auto fresh=[&]() {
		while(cvar_vars) Cvar_Unset(cvar_vars);
		cvar_cheats=Cvar_Get("host_cheats","1",0);
		std::ifstream cfg("shared/ios/ios_default.cfg");assert(cfg.good());
		Cvar_BeginPlatformDefaults();ReadSetas(cfg);Cvar_EndPlatformDefaults();
		for(int i=0;i<5;++i) Expect(names[i],defaults[i],defaults[i]);
	};
	auto registration=[&]() {for(int i=0;i<5;++i) Cvar_Get(names[i],defaults[i],CVAR_ARCHIVE);};
	fresh();registration();
	Seta("in_touchAimMode","0");Seta("in_touchLookScale","1.17");Seta("in_touchDragScale","2.33");registration();
	Expect("in_touchAimMode","0","1");Expect("in_touchLookScale","1.17","1.0");Expect("in_touchDragScale","2.33","3.0");
	Expect("in_touchLookScaleY","0.6","0.6");Expect("in_touchDragScaleY","1.8","1.8");
	Seta("in_touchLookScaleY","0.73");Seta("in_touchDragScaleY","2.17");registration();
	savedConfig.clear();Cvar_WriteVariables(1);const std::string saved=savedConfig;
	fresh();std::istringstream input(saved);ReadSetas(input);registration();
	const char *custom[]={"0","1.17","0.73","2.33","2.17"};
	for(int i=0;i<5;++i) {
		Expect(names[i],custom[i],defaults[i]);
		assert(Cvar_FindVar(names[i])->flags & CVAR_ARCHIVE);
		assert(Cvar_FindVar(names[i])->flags & CVAR_PLATFORM_DEFAULT);
	}
	Cvar_Restart(qfalse);registration();for(int i=0;i<5;++i) Expect(names[i],defaults[i],defaults[i]);
	while(cvar_vars) Cvar_Unset(cvar_vars);
	puts("PASS: actual ios_default.cfg fresh touch mode/H/V, saved old H/mode and explicit new Y, archive/reload/custom Y, cvar_restart");
}

static cvar_t *ConsoleSet( const char *command, const char *name, const char *value ) {
	commandArgs[0] = command;
	commandArgs[1] = name;
	commandArgs[2] = value;
	Cvar_Set_f();
	return Cvar_FindVar( name );
}

static bool Writes( const char *name, const char *value ) {
	savedConfig.clear();
	Cvar_WriteVariables( 1 );
	return savedConfig.find( std::string("seta ") + name + " \"" + value + "\"\n" ) != std::string::npos;
}

static int archiveFailures;
static void CheckROMArchive( cvar_t *var, const char *attempt ) {
	const bool archived = (var->flags & CVAR_ARCHIVE) != 0;
	const bool modified = (cvar_modifiedFlags & CVAR_ARCHIVE) != 0;
	savedConfig.clear();
	Cvar_WriteVariables( 1 );
	const bool written = savedConfig.find( std::string("seta ") + var->name + " " ) != std::string::npos;
	if ( archived || modified || written ) {
		printf( "FAIL: %s %s value=%s archive=%d modifiedArchive=%d serialized=%d\n",
			var->name, attempt, var->string, archived, modified, written );
		++archiveFailures;
	}
}

static void ConsoleFlagRegressions( void ) {
	const char *actualName = "r_glesProviderActive";
	for ( const char *actual : {"eagl", "angle"} ) {
		for ( bool adopt : {false, true} ) {
			while ( cvar_vars ) Cvar_Unset( cvar_vars );
			cvar_cheats = Cvar_Get( "host_cheats", "1", 0 );
			if ( adopt ) {
				cvar_t *user = ConsoleSet( "seta", actualName, "user-supplied" );
				assert( (user->flags & (CVAR_USER_CREATED | CVAR_ARCHIVE)) == (CVAR_USER_CREATED | CVAR_ARCHIVE) );
				assert( Writes( actualName, "user-supplied" ) );
			}
			cvar_t *var = Cvar_Get( actualName, "", CVAR_ROM );
			var->flags = CVAR_ROM;
			Cvar_Set( actualName, "" );
			assert( !strcmp(var->string, "") && var->flags == CVAR_ROM );
			Cvar_Set( actualName, actual );
			assert( !strcmp(var->string, actual) );
			const char *different = !strcmp(actual, "eagl") ? "angle" : "eagl";
			for ( const char *value : {different, actual, different, actual} ) {
				// Isolate each attempt from prior legitimate archive writes.
				cvar_modifiedFlags = 0;
				ConsoleSet( "seta", actualName, value );
				assert( !strcmp(var->string, actual) && (var->flags & CVAR_ROM) );
				CheckROMArchive( var, value );
			}
			Cvar_Set( actualName, different );
			assert( !strcmp(var->string, different) && (var->flags & CVAR_ROM) );
		}
	}
	puts( "CHECKED: ROM eagl/angle, different/same/repeated seta, exact adoption and engine force-set" );

	while ( cvar_vars ) Cvar_Unset( cvar_vars );
	cvar_cheats = Cvar_Get( "host_cheats", "1", 0 );
	for ( const char *name : {"host_console_new", "host_console_writable", "host_console_same", "host_console_archived"} ) {
		if ( strcmp(name, "host_console_new") )
			Cvar_Get( name, !strcmp(name, "host_console_same") ? "next" : "old",
				!strcmp(name, "host_console_archived") ? CVAR_ARCHIVE : 0 );
		cvar_modifiedFlags = 0;
		cvar_t *var = ConsoleSet( "seta", name, "next" );
		assert( !strcmp(var->string, "next") && (var->flags & CVAR_ARCHIVE) );
		assert( cvar_modifiedFlags & CVAR_ARCHIVE );
		assert( Writes(name, "next") );
		cvar_modifiedFlags = 0;
		ConsoleSet( "seta", name, "next" );
		assert( !strcmp(var->string, "next") && (var->flags & CVAR_ARCHIVE) );
		assert( !(cvar_modifiedFlags & CVAR_ARCHIVE) && Writes(name, "next") );
		ConsoleSet( "seta", name, "last" );
		assert( !strcmp(var->string, "last") && Writes(name, "last") );
	}
	cvar_t *requested = Cvar_Get( "r_glesProvider", "eagl", CVAR_ARCHIVE );
	for ( int i = 0; i < 3; ++i ) {
		ConsoleSet( "seta", "r_glesProvider", "angle" );
		assert( !strcmp(requested->string, "angle") && !requested->latchedString );
		assert( (requested->flags & (CVAR_ARCHIVE | CVAR_LATCH)) == CVAR_ARCHIVE );
		assert( Writes("r_glesProvider", "angle") );
	}
	puts( "PASS: console writable/new/same/already-archived seta and repeated requested-provider serialization" );

	for ( const char *command : {"setu", "sets"} ) {
		const int info = !strcmp(command, "setu") ? CVAR_USERINFO : CVAR_SERVERINFO;
		for ( bool rom : {false, true} ) {
			for ( bool same : {false, true} ) {
				const std::string name = std::string("host_") + command + (rom ? "_rom" : "_writable") + (same ? "_same" : "_different");
				cvar_t *var = Cvar_Get( name.c_str(), "old", rom ? CVAR_ROM : 0 );
				const char *value = same ? "old" : "next";
				cvar_modifiedFlags = 0;
				ConsoleSet( command, name.c_str(), value );
				assert( !strcmp(var->string, rom ? "old" : value) );
				assert( (var->flags & info) && (cvar_modifiedFlags & info) );
				assert( !(var->flags & CVAR_ARCHIVE) );
				cvar_modifiedFlags = 0;
				ConsoleSet( command, name.c_str(), var->string );
				assert( !(cvar_modifiedFlags & info) );
				cvar_modifiedFlags = 0;
				ConsoleSet( command, name.c_str(), "changed" );
				assert( (var->flags & info) && (cvar_modifiedFlags & info) );
				assert( !strcmp(var->string, rom ? "old" : "changed") );
				if ( rom ) {
					assert( var->flags & CVAR_ROM );
					for ( const char *attempt : {"old", "changed", "old"} ) {
						cvar_modifiedFlags = 0;
						ConsoleSet( "seta", name.c_str(), attempt );
						assert( !strcmp(var->string, "old") && (var->flags & CVAR_ROM) && (var->flags & info) );
						CheckROMArchive( var, attempt );
					}
				}
			}
		}
	}
	cvar_modifiedFlags = 0;
	cvar_t *plain = ConsoleSet( "set", "host_console_plain", "old" );
	assert( !(plain->flags & (CVAR_ARCHIVE | CVAR_USERINFO | CVAR_SERVERINFO)) );
	ConsoleSet( "set", "host_console_plain", "new" );
	ConsoleSet( "set", "host_console_plain", "new" );
	assert( !strcmp(plain->string, "new") && !Writes("host_console_plain", "new") );
	cvar_t *rom = Cvar_Get( "host_plain_rom", "old", CVAR_ROM );
	ConsoleSet( "set", "host_plain_rom", "new" );
	ConsoleSet( "set", "host_plain_rom", "old" );
	assert( !strcmp(rom->string, "old") && rom->flags == CVAR_ROM );
	puts( "PASS: writable/ROM same/different setu/sets info flags and modified bits; plain set" );
	while ( cvar_vars ) Cvar_Unset( cvar_vars );
}

int main() {
	cvar_cheats = Cvar_Get( "host_cheats", "1", 0 );
	Cvar_Get("ui_touchForceAPI","1",CVAR_ROM|CVAR_PLATFORM_DEFAULT);
	Cvar_Get("ui_touchForceAPI","0",CVAR_ROM|CVAR_VM_CREATED);
	Cvar_Restart(qtrue);
	Expect("ui_touchForceAPI","1","1");
	assert(Cvar_FindVar("ui_touchForceAPI")->flags & CVAR_PLATFORM_DEFAULT);

	// Desktop registration still adopts the code default after an ordinary seta.
	Seta( "host_desktop", "0" );
	cvar_t *desktop = Cvar_Get( "host_desktop", "1", CVAR_ARCHIVE_ND );
	Expect( "host_desktop", "0", "1" );
	assert( !(desktop->flags & (CVAR_USER_CREATED | CVAR_PLATFORM_DEFAULT)) );

	cvar_t *fps = Cvar_Get( "com_maxfps", "85", CVAR_ARCHIVE_ND );
	Cvar_BeginPlatformDefaults();
	cvar_t *flares = Seta( "r_flares", "0" );
	Seta( "com_maxfps", "60" );
	Seta( "com_maxfps", "120" );
	Seta( "host_empty", "" );
	cvar_t *same = Cvar_Get( "host_same", "0", CVAR_ARCHIVE_ND );
	Seta( "host_same", "0" );
	cvar_t *latched = Cvar_Get( "host_latched", "1", CVAR_ARCHIVE_ND | CVAR_LATCH );
	Seta( "host_latched", "0" );
	Seta( "host_latched", "0" );
	Cvar_EndPlatformDefaults();
	Cvar_Get( "r_flares", "1", CVAR_ARCHIVE_ND );
	Cvar_Get( "host_empty", "desktop", CVAR_ARCHIVE_ND );
	Cvar_Get( "host_latched", "1", CVAR_ARCHIVE_ND | CVAR_LATCH );
	Expect( "r_flares", "0", "0" );
	Expect( "com_maxfps", "120", "120" );
	Expect( "host_empty", "", "" );
	Expect( "host_latched", "0", "0" );
	assert( flares->flags & CVAR_PLATFORM_DEFAULT );
	assert( !(flares->flags & CVAR_USER_CREATED) );
	assert( fps->flags & CVAR_PLATFORM_DEFAULT );
	assert( same->flags & CVAR_PLATFORM_DEFAULT );
	assert( latched->flags & CVAR_PLATFORM_DEFAULT );

	Seta( "r_flares", "1" );
	Seta( "com_maxfps", "85" );
	Seta( "host_latched", "1" );
	Expect( "r_flares", "1", "0" );
	Expect( "com_maxfps", "85", "120" );
	savedConfig.clear();
	Cvar_WriteVariables( 1 );
	assert( savedConfig.find( "seta r_flares \"1\"\n" ) != std::string::npos );
	assert( savedConfig.find( "seta com_maxfps \"85\"\n" ) != std::string::npos );
	assert( savedConfig.find( "seta host_latched \"1\"\n" ) != std::string::npos );
	assert( savedConfig.find( "PLATFORM" ) == std::string::npos );
	const std::string userConfig = savedConfig;
	Cvar_Reset( "r_flares" );
	Expect( "r_flares", "0", "0" );
	Seta( "r_flares", "1" );
	Cvar_Restart( qfalse );
	Cvar_Get( "host_latched", "1", CVAR_ARCHIVE_ND | CVAR_LATCH );
	Expect( "r_flares", "0", "0" );
	Expect( "com_maxfps", "120", "120" );
	Expect( "host_latched", "0", "0" );
	Expect( "host_desktop", "1", "1" );
	savedConfig.clear();
	Cvar_WriteVariables( 1 );
	assert( savedConfig.find( "seta r_flares " ) == std::string::npos );
	assert( savedConfig.find( "seta com_maxfps " ) == std::string::npos );
	assert( savedConfig.find( "seta host_latched " ) == std::string::npos );
	while ( cvar_vars ) Cvar_Unset( cvar_vars );
	Cvar_BeginPlatformDefaults();
	Seta( "r_flares", "0" );
	Cvar_EndPlatformDefaults();
	char restored[16];
	size_t offset = userConfig.find( "seta r_flares " );
	assert( offset != std::string::npos );
	assert( sscanf( userConfig.c_str() + offset, "seta r_flares \"%15[^\"]\"", restored ) == 1 );
	Seta( "r_flares", restored );
	Cvar_Get( "r_flares", "1", CVAR_ARCHIVE_ND );
	Expect( "r_flares", "1", "0" );
	puts( "PASS: creation, late/early registration, unchanged/empty/latched defaults, outside-window seta, archive/relaunch, reset, restart, desktop control" );
	while ( cvar_vars ) Cvar_Unset( cvar_vars );
	TouchPlatformDefaults();
	#ifdef __APPLE__
	RatePlatformDefaults();
	CameraPlatformDefaults();
	#endif
	ConsoleFlagRegressions();
	printf( "%s: command-driven ROM archive regressions (%d failures)\n", archiveFailures ? "FAIL" : "PASS", archiveFailures );
	return archiveFailures ? 1 : 0;
}
