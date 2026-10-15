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
}
