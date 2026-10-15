#include "sys/ios/ios_touch_layout_edit.h"
#include <boost/test/unit_test.hpp>
#include <cstring>

BOOST_AUTO_TEST_SUITE( ios )
BOOST_AUTO_TEST_SUITE( touch_edit )

BOOST_AUTO_TEST_CASE( override_round_trip_and_bad_entries )
{
	touchLayoutOverrides_t o;
	TouchEdit_Parse( "0:.25,.75;999:1,2;11:0,0;1:nan,1;2:1,2junk;3:-1,2;0:.4,.6;4:1e99,2;5:1,2,3", &o );
	BOOST_CHECK_EQUAL( o.mask, 1u );
	BOOST_CHECK_CLOSE( o.cx[0], .4f, .001f );
	char text[1024];
	BOOST_REQUIRE( TouchEdit_Serialize( &o, text, sizeof(text) ) );
	BOOST_CHECK_EQUAL( text, "0:0.400,0.600;" );
	touchLayoutOverrides_t copy;
	TouchEdit_Parse( text, &copy );
	BOOST_CHECK_EQUAL( copy.mask, o.mask );
	BOOST_CHECK_EQUAL( copy.cy[0], o.cy[0] );
	char shortText[3] = "xx";
	BOOST_CHECK( !TouchEdit_Serialize( &o, shortText, sizeof(shortText) ) );
	BOOST_CHECK_EQUAL( shortText[0], 0 );
	TouchEdit_Parse( "", &o );
	BOOST_CHECK_EQUAL( o.mask, 0 );
	TouchEdit_Parse( NULL, &o );
	BOOST_CHECK_EQUAL( o.mask, 0 );
}

BOOST_AUTO_TEST_CASE( anchored_drag_clamps_and_builds_without_moving_console )
{
	for ( int button = 0; button < TB_COUNT; ++button ) {
		if ( button == TB_CONSOLE ) continue;
		for ( const auto &f : { touchRect_t{2868,1320,3,0}, touchRect_t{2048,1536,2,0}, touchRect_t{1434,660,1.5f,0} } ) {
			touchLayoutOverrides_t o = {};
			BOOST_REQUIRE( TouchEdit_Drag( &o, button, -1000, 5000, f.x, f.y, 40, 10, 50, 30, f.w ) );
			touchRect_t rects[TB_COUNT], original[TB_COUNT];
			TouchLayout_Build( f.x, f.y, 40, 10, 50, 30, f.w, rects, &o );
			TouchLayout_Build( f.x, f.y, 40, 10, 50, 30, f.w, original );
			BOOST_CHECK_GE( rects[button].x, 40 );
			BOOST_CHECK_LE( rects[button].y + rects[button].h, f.y - 30 );
			BOOST_CHECK_EQUAL( rects[button].w, original[button].w );
			BOOST_CHECK_EQUAL( rects[TB_CONSOLE].x, original[TB_CONSOLE].x );
			char text[1024]; TouchEdit_Serialize( &o, text, sizeof(text) );
			touchLayoutOverrides_t parsed; TouchEdit_Parse( text, &parsed );
			BOOST_CHECK_SMALL( parsed.cx[button] - o.cx[button], .00051f );
			touchButtonDef_t defs[TB_COUNT]; TouchEdit_Apply( &parsed, defs );
			BOOST_CHECK_EQUAL( defs[button].anchor, touchButtonDefs[button].anchor );
		}
	}
	touchLayoutOverrides_t o = {};
	BOOST_CHECK( !TouchEdit_Drag( &o, TB_CONSOLE, 100, 100, 2048, 1536, 0,0,0,0,2 ) );
	BOOST_CHECK( !TouchEdit_Drag( &o, 0, 100, 100, 0,0,0,0,0,0,0 ) );
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
