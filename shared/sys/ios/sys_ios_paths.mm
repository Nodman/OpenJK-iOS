#include "sys_ios.h"
#include "ios_assets.h"
#include <SDL.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#import <Foundation/Foundation.h>

#ifdef _JK2EXE
#	define IOS_APP_NAME "OpenJK SP"
#	define IOS_ASSET_CFG "default.cfg"
#else
#	define IOS_APP_NAME "OpenJK MP"
#	define IOS_ASSET_CFG "mpdefault.cfg"
#endif

#define IOS_DISCLAIMER "OpenJK is not affiliated with Disney, Lucasfilm or Raven Software. You need your own copy of the game."

const char *Sys_IOS_DocumentsPath( void ) {
	static char path[MAX_OSPATH];
	if ( !path[0] ) {
		const char *home = getenv( "HOME" );
		Com_sprintf( path, sizeof( path ), "%s/Documents", home ? home : "." );
	}
	return path;
}

static const char *Sys_IOS_BaseDir( void ) {
	static char path[MAX_OSPATH];
	if ( !IOS_ResolveBaseDir( Sys_IOS_DocumentsPath(), path, sizeof( path ) ) ) {
		Com_sprintf( path, sizeof( path ), "%s/base", Sys_IOS_DocumentsPath() );
		mkdir( path, 0755 );
	}
	return path;
}

static void Sys_IOS_ExcludeFromBackup( const char *base, const char *name ) {
	@autoreleasepool {
		NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:va( "%s/%s", base, name )]];
		NSNumber *excluded = nil;
		[url getResourceValue:&excluded forKey:NSURLIsExcludedFromBackupKey error:nil];
		if ( [excluded boolValue] ) {
			return;
		}
		// Com_DPrintf is silent before Com_Init (no com_developer yet), so log once per file instead.
		if ( [url setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:nil] ) {
			Com_Printf( "backup-excluded %s\n", name );
		}
	}
}

void Sys_IOS_PrepareDocuments( void ) {
	const char *base = Sys_IOS_BaseDir();

	const char *readme = va( "%s/README.txt", base );
	if ( access( readme, F_OK ) ) {
		FILE *f = fopen( readme, "w" );
		if ( f ) {
			fputs( "Copy assets0.pk3, assets1.pk3, assets2.pk3 and assets3.pk3 from your own copy of Jedi Academy (GameData/base) into this folder.\n\n"
				IOS_DISCLAIMER "\n", f );
			fclose( f );
		}
	}

	DIR *dir = opendir( base );
	if ( !dir ) {
		return;
	}
	while ( struct dirent *entry = readdir( dir ) ) {
		size_t len = strlen( entry->d_name );
		if ( len > 4 && !strcasecmp( entry->d_name + len - 4, ".pk3" ) && entry->d_type == DT_REG ) {
			Sys_IOS_ExcludeFromBackup( base, entry->d_name );
		}
	}
	closedir( dir );
}

void Sys_IOS_WaitForAssets( void ) {
	static const SDL_MessageBoxButtonData button = { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Check again" };
	SDL_MessageBoxData box = {};
	box.flags = SDL_MESSAGEBOX_INFORMATION;
	box.title = "Game files needed";
	box.message =
		"OpenJK needs the game files from your own copy of Star Wars Jedi Knight: Jedi Academy.\n\n"
		"Open the Files app → On My iPhone/iPad → " IOS_APP_NAME " → base, and copy assets0.pk3 to assets3.pk3 there. Then tap Check again.\n\n"
		"OpenJK SP and OpenJK MP each need their own copy. Extra .pk3 files in base can stop you joining pure (sv_pure) servers.\n\n"
		IOS_DISCLAIMER;
	box.numbuttons = 1;
	box.buttons = &button;

	while ( !IOS_ValidateAssets( Sys_IOS_BaseDir(), IOS_ASSET_CFG ) ) {
		Com_Printf( "Game files missing: assets0.pk3 or %s not found in %s\n", IOS_ASSET_CFG, Sys_IOS_BaseDir() );
		int buttonId;
		if ( SDL_ShowMessageBox( &box, &buttonId ) < 0 ) {
			Com_Printf( "SDL_ShowMessageBox failed: %s\n", SDL_GetError() );
			SDL_Delay( 1000 );
		}
		Sys_IOS_PrepareDocuments();
	}
}
