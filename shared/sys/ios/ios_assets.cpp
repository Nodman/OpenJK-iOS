#include "ios_assets.h"

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static char *IOS_AssetPath( const char *dir, const char *name )
{
	size_t dirLength = strlen( dir ), nameLength = strlen( name );
	if ( nameLength > SIZE_MAX - 2 || dirLength > SIZE_MAX - nameLength - 2 ) {
		return NULL;
	}
	size_t size = dirLength + nameLength + 2;
	char *path = (char *)malloc( size );
	if ( path ) {
		snprintf( path, size, "%s/%s", dir, name );
	}
	return path;
}

static int IOS_AssetFileExists( const char *path )
{
	DIR *dir = opendir( path );
	if ( dir ) {
		closedir( dir );
		return 0;
	}
	FILE *file = fopen( path, "rb" );
	if ( !file ) {
		return 0;
	}
	fclose( file );
	return 1;
}

int IOS_ResolveBaseDir( const char *documents, char *out, size_t outSize )
{
	if ( !documents || !out || !outSize ) {
		return 0;
	}
	DIR *dir = opendir( documents );
	if ( !dir ) {
		return 0;
	}
	char *source = NULL;
	int hasExact = 0;
	while ( dirent *entry = readdir( dir ) ) {
		if ( strcasecmp( entry->d_name, "base" ) ) {
			continue;
		}
		char *candidate = IOS_AssetPath( documents, entry->d_name );
		if ( !candidate ) {
			free( source );
			closedir( dir );
			return 0;
		}
		DIR *base = opendir( candidate );
		if ( base ) {
			closedir( base );
			if ( !strcmp( entry->d_name, "base" ) ) {
				hasExact = 1;
			} else if ( !source ) {
				source = candidate;
				candidate = NULL;
			}
		}
		free( candidate );
	}
	closedir( dir );
	char *destination = IOS_AssetPath( documents, "base" );
	int found = destination && strlen( destination ) < outSize &&
		( hasExact || ( source && !rename( source, destination ) ) );
	if ( found ) {
		memcpy( out, destination, strlen( destination ) + 1 );
	}
	free( source );
	free( destination );
	return found;
}

static unsigned int IOS_ZipU16( const unsigned char *bytes )
{
	return (unsigned int)bytes[0] | ( (unsigned int)bytes[1] << 8 );
}

static uint32_t IOS_ZipU32( const unsigned char *bytes )
{
	return (uint32_t)bytes[0] | ( (uint32_t)bytes[1] << 8 ) |
		( (uint32_t)bytes[2] << 16 ) | ( (uint32_t)bytes[3] << 24 );
}

static int IOS_ScanZip( FILE *file, const char *name )
{
	if ( fseek( file, 0, SEEK_END ) ) {
		return 0;
	}
	long size = ftell( file );
	if ( size < 22 ) {
		return 0;
	}
	size_t tailSize = size < 65557 ? (size_t)size : 65557;
	unsigned char *tail = (unsigned char *)malloc( tailSize );
	if ( !tail ) {
		return 0;
	}
	if ( fseek( file, size - (long)tailSize, SEEK_SET ) ||
		fread( tail, 1, tailSize, file ) != tailSize ) {
		free( tail );
		return 0;
	}
	long end = (long)tailSize - 22;
	while ( end >= 0 ) {
		if ( IOS_ZipU32( tail + end ) == 0x06054b50 ) {
			break;
		}
		--end;
	}
	if ( end < 0 ) {
		free( tail );
		return 0;
	}
	unsigned int count = IOS_ZipU16( tail + end + 10 );
	uint32_t offset = IOS_ZipU32( tail + end + 16 );
	uint32_t directorySize = IOS_ZipU32( tail + end + 12 );
	int singleDisk = !IOS_ZipU16( tail + end + 4 ) && !IOS_ZipU16( tail + end + 6 ) &&
		IOS_ZipU16( tail + end + 8 ) == count;
	long endOffset = size - (long)tailSize + end;
	free( tail );
	if ( !singleDisk || count == 0xffff || offset == 0xffffffff || directorySize == 0xffffffff ||
		(uint64_t)offset + directorySize > (uint64_t)endOffset ) {
		return 0;
	}
	size_t wantedLength = strlen( name );
	if ( wantedLength > 65535 || fseek( file, (long)offset, SEEK_SET ) ) {
		return 0;
	}
	char *entryName = (char *)malloc( wantedLength + 1 );
	if ( !entryName ) {
		return 0;
	}
	int found = 0;
	uint32_t remaining = directorySize;
	for ( unsigned int i = 0; i < count; ++i ) {
		unsigned char header[46];
		if ( remaining < sizeof( header ) || fread( header, 1, sizeof( header ), file ) != sizeof( header ) ||
			IOS_ZipU32( header ) != 0x02014b50 ) {
			break;
		}
		unsigned int nameLength = IOS_ZipU16( header + 28 );
		unsigned int extraLength = IOS_ZipU16( header + 30 );
		unsigned int commentLength = IOS_ZipU16( header + 32 );
		remaining -= sizeof( header );
		if ( nameLength + extraLength + commentLength > remaining ) {
			break;
		}
		if ( nameLength == wantedLength ) {
			if ( fread( entryName, 1, nameLength, file ) != nameLength ) {
				break;
			}
			entryName[nameLength] = '\0';
			if ( !strcmp( entryName, name ) ) {
				found = 1;
				break;
			}
		} else if ( fseek( file, nameLength, SEEK_CUR ) ) {
			break;
		}
		if ( fseek( file, extraLength + commentLength, SEEK_CUR ) ) {
			break;
		}
		remaining -= nameLength + extraLength + commentLength;
	}
	free( entryName );
	return found;
}

int IOS_ZipHasEntry( const char *zipPath, const char *name )
{
	if ( !zipPath || !name ) {
		return 0;
	}
	FILE *file = fopen( zipPath, "rb" );
	if ( !file ) {
		return 0;
	}
	int found = IOS_ScanZip( file, name );
	fclose( file );
	return found;
}

int IOS_ValidateAssets( const char *baseDir, const char *cfgName )
{
	if ( !baseDir || !cfgName ) {
		return 0;
	}
	DIR *dir = opendir( baseDir );
	if ( !dir ) {
		return 0;
	}
	char *cfgPath = IOS_AssetPath( baseDir, cfgName );
	int hasCfg = cfgPath && IOS_AssetFileExists( cfgPath );
	free( cfgPath );
	int hasAssets = 0;
	while ( dirent *entry = readdir( dir ) ) {
		size_t length = strlen( entry->d_name );
		if ( length < 4 || strcasecmp( entry->d_name + length - 4, ".pk3" ) ) {
			continue;
		}
		char *path = IOS_AssetPath( baseDir, entry->d_name );
		if ( !path ) {
			continue;
		}
		if ( !strcasecmp( entry->d_name, "assets0.pk3" ) && IOS_AssetFileExists( path ) ) {
			hasAssets = 1;
		}
		if ( !hasCfg && IOS_ZipHasEntry( path, cfgName ) ) {
			hasCfg = 1;
		}
		free( path );
		if ( hasAssets && hasCfg ) {
			break;
		}
	}
	closedir( dir );
	return hasAssets && hasCfg;
}
