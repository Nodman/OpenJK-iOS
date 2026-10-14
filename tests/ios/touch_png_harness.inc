#include TOUCH_PNG_SOURCE
#include "ios/touch/touch-v2.inc"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <vector>
#include <zlib.h>

refimport_t ri = {};
static std::set<void *> live;
static size_t allocation, failAt = (size_t)-1, fsCalls, checks, cases;

static void Require( bool pass ) {
	++checks;
	if ( !pass ) {
		std::fprintf( stderr, "Check %zu failed\n", checks );
		std::abort();
	}
}

static void *Allocate( int size, memtag_t, qboolean zero, int = 4 ) {
	if ( ++allocation == failAt ) return NULL;
	void *ptr = std::malloc( size );
	Require( ptr != NULL );
	if ( zero ) std::memset( ptr, 0, size );
	Require( live.insert( ptr ).second );
	return ptr;
}

static void Free( void *ptr ) {
	Require( live.erase( ptr ) == 1 );
	std::free( ptr );
}

#ifdef TOUCH_PNG_SP
void *R_Malloc( int size, memtag_t tag, qboolean zero ) { return Allocate( size, tag, zero ); }
void R_Free( void *ptr ) { Free( ptr ); }
#endif

static void Quiet( int, const char *, ... ) {}
static long NoRead( const char *, void ** ) { ++fsCalls; return -1; }
static void NoFreeFile( void * ) { ++fsCalls; Require( false ); }

static void Decode( const byte *input, size_t length, bool expected ) {
	++cases;
	byte *pixels = (byte *)1;
	int width = -1, height = -1;
	const int success = LoadPNGFromMemory( input, length, &pixels, &width, &height );
	Require( ( success != 0 ) == expected );
	Require( fsCalls == 0 );
	if ( expected ) {
		Require( width == 1024 && height == 1024 );
		Require( pixels != NULL && live.size() == 1 );
		Require( pixels[3] == 0 );
		Require( pixels[( 896 * 1024 + 896 ) * 4 + 3] == 255 );
		Free( pixels );
	} else {
		Require( pixels == NULL && width == 0 && height == 0 );
	}
	Require( live.empty() );
}

static void HeaderCRC( std::vector<byte> &bytes ) {
	const unsigned long crc = crc32( 0, bytes.data() + 12, 17 );
	for ( int i = 0; i < 4; ++i ) bytes[29 + i] = (byte)( crc >> ( 24 - i * 8 ) );
}

int main() {
	ri.Printf = Quiet;
	ri.FS_ReadFile = NoRead;
	ri.FS_FreeFile = NoFreeFile;
#ifndef TOUCH_PNG_SP
	ri.Z_Malloc = Allocate;
	ri.Z_Free = Free;
#endif
	Decode( iosTouchPNG, iosTouchPNGSize, true );
	const size_t lengths[] = { 0, 1, 7, 8, 16, 32, 100, iosTouchPNGSize / 2, iosTouchPNGSize - 1 };
	for ( size_t length : lengths ) Decode( iosTouchPNG, length, false );
	Decode( NULL, 100, false );
	std::vector<byte> bad( iosTouchPNG, iosTouchPNG + iosTouchPNGSize );
	bad[0] = 0;
	Decode( bad.data(), bad.size(), false );
	bad.assign( iosTouchPNG, iosTouchPNG + iosTouchPNGSize );
	bad[bad.size() / 2] ^= 1;
	Decode( bad.data(), bad.size(), false );
	for ( int field : { 16, 24, 25, 28 } ) {
		bad.assign( iosTouchPNG, iosTouchPNG + iosTouchPNGSize );
		bad[field] = field == 16 ? 0xff : field == 24 ? 16 : field == 25 ? 0 : 1;
		HeaderCRC( bad );
		Decode( bad.data(), bad.size(), false );
	}
	bad.assign( iosTouchPNG, iosTouchPNG + iosTouchPNGSize );
	std::memset( bad.data() + 16, 0, 4 );
	HeaderCRC( bad );
	Decode( bad.data(), bad.size(), false );
	for ( size_t failure = 1; failure <= 3; ++failure ) {
		allocation = 0;
		failAt = failure;
		Decode( iosTouchPNG, iosTouchPNGSize, false );
		failAt = (size_t)-1;
		Decode( iosTouchPNG, iosTouchPNGSize, true );
	}
	for ( int repeat = 0; repeat < 20; ++repeat ) Decode( iosTouchPNG, iosTouchPNGSize, true );
	int width = 1, height = 1;
	Require( !LoadPNGFromMemory( iosTouchPNG, iosTouchPNGSize, NULL, &width, &height ) );
	Require( width == 0 && height == 0 && live.empty() && fsCalls == 0 );
	std::printf( "Memory PNG: %zu cases, %zu checks, FS calls=%zu, live allocations=%zu; allocation failures 1/2/3 clean\n", cases, checks, fsCalls, live.size() );
}
