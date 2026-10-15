// Run with tests/ios/cvar_platform_defaults_host.py; each engine uses its real cvar.cpp.
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

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
}
