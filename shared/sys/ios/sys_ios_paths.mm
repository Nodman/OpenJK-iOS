#include "sys_ios.h"
#include <stdlib.h>
#include <sys/stat.h>

const char *Sys_IOS_DocumentsPath( void ) {
	static char path[MAX_OSPATH];
	if ( !path[0] ) {
		const char *home = getenv( "HOME" );
		Com_sprintf( path, sizeof( path ), "%s/Documents", home ? home : "." );
	}
	return path;
}

// Hook: P2 (T2.2) replaces both bodies.
void Sys_IOS_PrepareDocuments( void ) {
	mkdir( va( "%s/base", Sys_IOS_DocumentsPath() ), 0755 );
}
void Sys_IOS_WaitForAssets( void ) {
}
