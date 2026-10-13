#include "sys/ios/ios_assets.cpp"

#include <boost/test/unit_test.hpp>

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <string>
#include <vector>

static void WriteZip( const std::string &path, const std::vector<std::string> &names,
	const std::string &comment = "" )
{
	std::string cd;
	for ( const auto &name : names ) {
		char header[46] = {};
		header[0] = 0x50; header[1] = 0x4b; header[2] = 0x01; header[3] = 0x02;
		header[28] = (char)( name.size() & 0xff );
		header[29] = (char)( name.size() >> 8 );
		cd.append( header, sizeof( header ) );
		cd += name;
	}
	char end[22] = {};
	end[0] = 0x50; end[1] = 0x4b; end[2] = 0x05; end[3] = 0x06;
	end[8] = end[10] = (char)names.size();
	uint32_t size = (uint32_t)cd.size(), offset = 0;
	memcpy( end + 12, &size, 4 );
	memcpy( end + 16, &offset, 4 );
	end[20] = (char)( comment.size() & 0xff );
	end[21] = (char)( comment.size() >> 8 );
	std::ofstream output( path, std::ios::binary );
	output << cd << std::string( end, sizeof( end ) ) << comment;
	BOOST_REQUIRE( output.good() );
}

static void RemoveTree( const std::string &path )
{
	DIR *dir = opendir( path.c_str() );
	if ( !dir ) {
		unlink( path.c_str() );
		return;
	}
	while ( dirent *entry = readdir( dir ) ) {
		if ( strcmp( entry->d_name, "." ) && strcmp( entry->d_name, ".." ) ) {
			RemoveTree( path + "/" + entry->d_name );
		}
	}
	closedir( dir );
	rmdir( path.c_str() );
}

struct AssetFixture
{
	std::string documents;

	AssetFixture()
	{
		const char *tmpDir = getenv( "TMPDIR" );
		std::string name = std::string( tmpDir && *tmpDir ? tmpDir : "/tmp" ) + "/openjk-assets-XXXXXX";
		std::vector<char> pattern( name.begin(), name.end() );
		pattern.push_back( '\0' );
		char *dir = mkdtemp( pattern.data() );
		BOOST_REQUIRE( dir );
		documents = dir;
	}

	~AssetFixture()
	{
		RemoveTree( documents );
	}
};

BOOST_AUTO_TEST_SUITE( ios )
BOOST_FIXTURE_TEST_SUITE( assets, AssetFixture )

BOOST_AUTO_TEST_CASE( cfg_in_pk3 )
{
	std::string base = documents + "/base";
	BOOST_REQUIRE_EQUAL( mkdir( base.c_str(), 0700 ), 0 );
	WriteZip( base + "/assets0.pk3", { "textures/test", "mpdefault.cfg" } );
	BOOST_CHECK_EQUAL( IOS_ZipHasEntry( ( base + "/assets0.pk3" ).c_str(), "mpdefault.cfg" ), 1 );
	BOOST_CHECK_EQUAL( IOS_ValidateAssets( base.c_str(), "mpdefault.cfg" ), 1 );
	WriteZip( base + "/assets0.pk3", { "textures/test" } );
	WriteZip( base + "/custom.PK3", { "mpdefault.cfg", "default.cfg" } );
	BOOST_CHECK_EQUAL( IOS_ValidateAssets( base.c_str(), "mpdefault.cfg" ), 1 );
	BOOST_CHECK_EQUAL( IOS_ValidateAssets( base.c_str(), "default.cfg" ), 1 );
}

BOOST_AUTO_TEST_CASE( missing_zip_entry )
{
	std::string zip = documents + "/assets0.pk3";
	WriteZip( zip, { "nested/mpdefault.cfg", "MPDEFAULT.CFG" } );
	BOOST_CHECK_EQUAL( IOS_ZipHasEntry( zip.c_str(), "mpdefault.cfg" ), 0 );
	BOOST_CHECK_EQUAL( IOS_ValidateAssets( documents.c_str(), "mpdefault.cfg" ), 0 );
}

BOOST_AUTO_TEST_CASE( zip_with_trailing_bytes )
{
	std::string zip = documents + "/assets0.pk3";
	WriteZip( zip, { "mpdefault.cfg" }, "archive comment" );
	std::ofstream( zip, std::ios::binary | std::ios::app ) << "junkjunk";
	BOOST_CHECK_EQUAL( IOS_ZipHasEntry( zip.c_str(), "mpdefault.cfg" ), 1 );
}

BOOST_AUTO_TEST_CASE( non_zip_file )
{
	std::string zip = documents + "/assets0.pk3";
	std::ofstream( zip, std::ios::binary ) << std::string( 80, 'x' );
	BOOST_CHECK_EQUAL( IOS_ZipHasEntry( zip.c_str(), "mpdefault.cfg" ), 0 );
	BOOST_CHECK_EQUAL( IOS_ZipHasEntry( ( documents + "/missing.pk3" ).c_str(), "mpdefault.cfg" ), 0 );
	WriteZip( zip, { "mpdefault.cfg" } );
	std::fstream file( zip, std::ios::in | std::ios::out | std::ios::binary );
	file.seekp( -12, std::ios::end );
	file.write( "\xff\xff", 2 );
	file.close();
	BOOST_CHECK_EQUAL( IOS_ZipHasEntry( zip.c_str(), "mpdefault.cfg" ), 0 );
}

BOOST_AUTO_TEST_CASE( rename_base_directory )
{
	std::string variant = documents + "/Base";
	std::string expected = documents + "/base";
	BOOST_REQUIRE_EQUAL( mkdir( variant.c_str(), 0700 ), 0 );
	char resolved[1024] = {};
	BOOST_REQUIRE_EQUAL( IOS_ResolveBaseDir( documents.c_str(), resolved, sizeof( resolved ) ), 1 );
	BOOST_CHECK_EQUAL( resolved, expected );
	DIR *dir = opendir( resolved );
	BOOST_REQUIRE( dir );
	closedir( dir );
	DIR *parent = opendir( documents.c_str() );
	BOOST_REQUIRE( parent );
	bool foundBase = false;
	while ( dirent *entry = readdir( parent ) ) {
		BOOST_CHECK_NE( std::string( entry->d_name ), "Base" );
		foundBase |= strcmp( entry->d_name, "base" ) == 0;
	}
	closedir( parent );
	BOOST_CHECK( foundBase );
	BOOST_CHECK_EQUAL( IOS_ResolveBaseDir( documents.c_str(), resolved, sizeof( resolved ) ), 1 );
	char small[2] = {};
	BOOST_CHECK_EQUAL( IOS_ResolveBaseDir( documents.c_str(), small, sizeof( small ) ), 0 );
}

BOOST_AUTO_TEST_CASE( no_base_directory )
{
	char resolved[1024] = {};
	BOOST_CHECK_EQUAL( IOS_ResolveBaseDir( documents.c_str(), resolved, sizeof( resolved ) ), 0 );
	std::ofstream( documents + "/base" ) << "not a directory";
	BOOST_CHECK_EQUAL( IOS_ResolveBaseDir( documents.c_str(), resolved, sizeof( resolved ) ), 0 );
}

BOOST_AUTO_TEST_CASE( prefer_exact_base_directory )
{
	std::string variant = documents + "/Base";
	std::string exact = documents + "/base";
	BOOST_REQUIRE_EQUAL( mkdir( variant.c_str(), 0700 ), 0 );
	int result = mkdir( exact.c_str(), 0700 );
	if ( result && errno == EEXIST ) {
		BOOST_TEST_MESSAGE( "Skipped coexistence check: temporary filesystem is case-insensitive." );
		return;
	}
	BOOST_REQUIRE_EQUAL( result, 0 );
	struct stat variantBefore, exactBefore, variantAfter, exactAfter;
	BOOST_REQUIRE_EQUAL( stat( variant.c_str(), &variantBefore ), 0 );
	BOOST_REQUIRE_EQUAL( stat( exact.c_str(), &exactBefore ), 0 );
	BOOST_REQUIRE_NE( variantBefore.st_ino, exactBefore.st_ino );
	char resolved[1024] = {};
	BOOST_REQUIRE_EQUAL( IOS_ResolveBaseDir( documents.c_str(), resolved, sizeof( resolved ) ), 1 );
	BOOST_CHECK_EQUAL( resolved, exact );
	BOOST_REQUIRE_EQUAL( stat( variant.c_str(), &variantAfter ), 0 );
	BOOST_REQUIRE_EQUAL( stat( exact.c_str(), &exactAfter ), 0 );
	BOOST_CHECK_EQUAL( variantBefore.st_ino, variantAfter.st_ino );
	BOOST_CHECK_EQUAL( exactBefore.st_ino, exactAfter.st_ino );
}

BOOST_AUTO_TEST_CASE( uppercase_assets_and_loose_cfg )
{
	WriteZip( documents + "/ASSETS0.PK3", {} );
	std::ofstream( documents + "/mpdefault.cfg" ) << "seta test 1\n";
	BOOST_CHECK_EQUAL( IOS_ValidateAssets( documents.c_str(), "mpdefault.cfg" ), 1 );
	BOOST_CHECK_EQUAL( IOS_ValidateAssets( documents.c_str(), "default.cfg" ), 0 );
}

BOOST_AUTO_TEST_CASE( no_assets0 )
{
	WriteZip( documents + "/assets1.pk3", { "mpdefault.cfg" } );
	std::ofstream( documents + "/mpdefault.cfg" ) << "seta test 1\n";
	BOOST_CHECK_EQUAL( IOS_ValidateAssets( documents.c_str(), "mpdefault.cfg" ), 0 );
	BOOST_REQUIRE_EQUAL( mkdir( ( documents + "/assets0.pk3" ).c_str(), 0700 ), 0 );
	BOOST_CHECK_EQUAL( IOS_ValidateAssets( documents.c_str(), "mpdefault.cfg" ), 0 );
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
