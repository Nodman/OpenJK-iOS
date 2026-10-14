#include "ios/touch/touch_atlas.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( ios )
BOOST_AUTO_TEST_SUITE( touch_atlas )

BOOST_AUTO_TEST_CASE( immutable_cells_and_inner_texel_centres )
{
	const int glyphs[] = { TG_MENU, TG_CHAT, TG_PAD, TG_WEAPPREV, TG_SABER_SELECT, TG_WEAPNEXT,
		TG_FORCENEXT, TG_FORCE, TG_CONSOLE, TG_ALTATTACK, TG_ATTACK, TG_JUMP, TG_CROUCH, TG_STYLE, TG_USE, TG_PLATE };
	for ( int i = 0; i < 16; ++i ) {
		BOOST_CHECK_EQUAL( glyphs[i], i );
		const touchAtlasUV_t uv = TouchAtlas_UV( i );
		BOOST_CHECK_EQUAL( uv.s1 * 1024, ( i % 4 ) * 256 + 8.5f );
		BOOST_CHECK_EQUAL( uv.t1 * 1024, ( i / 4 ) * 256 + 8.5f );
		BOOST_CHECK_EQUAL( uv.s2 * 1024, ( i % 4 + 1 ) * 256 - 8.5f );
		BOOST_CHECK_EQUAL( uv.t2 * 1024, ( i / 4 + 1 ) * 256 - 8.5f );
		BOOST_CHECK_LT( uv.s1, uv.s2 );
		BOOST_CHECK_LT( uv.t1, uv.t2 );
		BOOST_CHECK_GE( uv.s1, 0 );
		BOOST_CHECK_LE( uv.s2, 1 );
		BOOST_CHECK_GE( uv.t1, 0 );
		BOOST_CHECK_LE( uv.t2, 1 );
	}
	BOOST_CHECK_EQUAL( TouchAtlas_UV( -1 ).s2, 0 );
	BOOST_CHECK_EQUAL( TouchAtlas_UV( 16 ).t2, 0 );
	BOOST_CHECK_EQUAL( IOS_TOUCH_ATLAS_NAME, "*ios_touch_v2" );
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
