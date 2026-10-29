#include "sys_ios.h"
#include "qcommon/qcommon.h"

void Sys_IOS_MigrateConfig( void ) {
	// Old engine defaults that 0.1.0 saved into every config.
	static const struct { const char *name, *oldDefault; } moved[] = {
		{ "cg_thirdPersonRange", "80" },
		{ "cg_fovViewmodel", "0" },
		{ "cg_gunX", "0" },
	};
	if ( Cvar_Get( "ios_configVersion", "0", CVAR_ARCHIVE | CVAR_NORESTART )->integer >= 1 )
		return;
	for ( const auto &cvar : moved ) {
		const unsigned flags = Cvar_Flags( cvar.name );
		if ( flags == CVAR_NONEXISTENT || !( flags & CVAR_PLATFORM_DEFAULT ) )
			continue;
		char value[MAX_CVAR_VALUE_STRING];
		Q_strncpyz( value, Cvar_VariableString( cvar.name ), sizeof( value ) );
		if ( Q_stricmp( value, cvar.oldDefault ) && !( Q_isanumber( value ) && atof( value ) == atof( cvar.oldDefault ) ) )
			continue;
		Cvar_Reset( cvar.name );
		Com_DPrintf( "ios_configVersion 1: %s %s -> %s\n", cvar.name, value, Cvar_VariableString( cvar.name ) );
	}
	Cvar_Set( "ios_configVersion", "1" );
}
