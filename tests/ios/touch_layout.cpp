#include "sys/ios/ios_touch_layout.cpp"

#include <boost/test/unit_test.hpp>

#include <math.h>

BOOST_AUTO_TEST_SUITE( ios )
BOOST_AUTO_TEST_SUITE( touch_layout )

BOOST_AUTO_TEST_CASE( button_definitions )
{
	const touchButtonDef_t expected[TB_COUNT] = {
		{ "ATK",   0.90f, 0.72f, 0.20f, 1 },
		{ "ALT",   0.78f, 0.82f, 0.15f, 1 },
		{ "JUMP",  0.90f, 0.45f, 0.15f, 1 },
		{ "DUCK",  0.78f, 0.58f, 0.13f, 1 },
		{ "USE",   0.66f, 0.82f, 0.13f, 1 },
		{ "FORCE", 0.78f, 0.33f, 0.13f, 1 },
		{ "F>>",   0.66f, 0.33f, 0.11f, 1 },
		{ "STYLE", 0.66f, 0.58f, 0.11f, 1 },
		{ "<W",    0.40f, 0.10f, 0.11f, 1 },
		{ "W>",    0.52f, 0.10f, 0.11f, 1 },
		{ "MENU",  0.05f, 0.08f, 0.11f, 1 },
		{ "CON",   0.95f, 0.08f, 0.11f, 0 }
	};
	for ( int i = 0; i < TB_COUNT; ++i ) {
		BOOST_CHECK_EQUAL( touchButtonDefs[i].label, expected[i].label );
		BOOST_CHECK_EQUAL( touchButtonDefs[i].cx, expected[i].cx );
		BOOST_CHECK_EQUAL( touchButtonDefs[i].cy, expected[i].cy );
		BOOST_CHECK_EQUAL( touchButtonDefs[i].size, expected[i].size );
		BOOST_CHECK_EQUAL( touchButtonDefs[i].gameplayOnly, expected[i].gameplayOnly );
	}
}

BOOST_AUTO_TEST_CASE( attack_without_insets )
{
	touchRect_t rects[TB_COUNT];
	TouchLayout_Build( 2000, 1000, 0, 0, 0, 0, rects );
	BOOST_CHECK_SMALL( rects[TB_ATTACK].x - 1700.0f, 1e-3f );
	BOOST_CHECK_SMALL( rects[TB_ATTACK].y - 620.0f, 1e-3f );
	BOOST_CHECK_SMALL( rects[TB_ATTACK].w - 200.0f, 1e-3f );
	BOOST_CHECK_SMALL( rects[TB_ATTACK].h - 200.0f, 1e-3f );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 1800, 720, 0 ), TB_ATTACK );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 1800, 720, 1 ), -1 );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 0, 1000, 0 ), -1 );
}

BOOST_AUTO_TEST_CASE( left_safe_inset )
{
	touchRect_t rects[TB_COUNT];
	TouchLayout_Build( 2000, 1000, 44, 0, 0, 0, rects );
	BOOST_CHECK_SMALL( rects[TB_ATTACK].x - 1704.4f, 1e-3f );
	BOOST_CHECK_SMALL( rects[TB_MENU].x - 86.8f, 1e-3f );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 44 + 0.95f * 1956, 80, 1 ), TB_CONSOLE );
}

BOOST_AUTO_TEST_CASE( small_window )
{
	touchRect_t rects[TB_COUNT];
	TouchLayout_Build( 600, 400, 0, 0, 0, 0, rects );
	BOOST_CHECK_SMALL( rects[TB_ATTACK].w - 108.0f, 1e-3f );
	BOOST_CHECK_SMALL( rects[TB_ATTACK].h - 108.0f, 1e-3f );
	BOOST_CHECK_SMALL( rects[TB_ATTACK].x - 486.0f, 1e-3f );
	BOOST_CHECK_SMALL( rects[TB_ATTACK].y - 234.0f, 1e-3f );
}

BOOST_AUTO_TEST_CASE( narrow_window )
{
	touchRect_t rects[TB_COUNT];
	TouchLayout_Build( 400, 1000, 0, 0, 0, 0, rects );
	BOOST_CHECK_SMALL( rects[TB_ATTACK].w - 80.0f, 1e-3f );
	BOOST_CHECK_SMALL( rects[TB_ATTACK].h - 80.0f, 1e-3f );
}

BOOST_AUTO_TEST_CASE( safe_area_clamps )
{
	touchRect_t rects[TB_COUNT];
	TouchLayout_Build( 400, 1000, 20, 30, 40, 50, rects );
	BOOST_CHECK_SMALL( rects[TB_MENU].x - 20.0f, 1e-3f );
	BOOST_CHECK_SMALL( rects[TB_CONSOLE].x + rects[TB_CONSOLE].w - 360.0f, 1e-3f );
	TouchLayout_Build( 600, 400, 20, 30, 40, 50, rects );
	BOOST_CHECK_SMALL( rects[TB_MENU].y - 30.0f, 1e-3f );
	BOOST_CHECK_SMALL( rects[TB_CONSOLE].y - 30.0f, 1e-3f );
	for ( int i = 0; i < TB_COUNT; ++i ) {
		BOOST_CHECK_GE( rects[i].x, 20.0f );
		BOOST_CHECK_GE( rects[i].y, 30.0f );
		BOOST_CHECK_LE( rects[i].x + rects[i].w, 560.0f );
		BOOST_CHECK_LE( rects[i].y + rects[i].h, 350.0f );
	}
	TouchLayout_Build( 600, 120, 0, 10, 0, 10, rects );
	BOOST_CHECK_SMALL( rects[TB_ALTATTACK].y + rects[TB_ALTATTACK].h - 110.0f, 1e-3f );
}

BOOST_AUTO_TEST_CASE( first_hit_and_ui_filter )
{
	touchRect_t rects[TB_COUNT];
	for ( int i = 0; i < TB_COUNT; ++i ) {
		rects[i] = { 10, 20, 30, 40 };
	}
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 25, 40, 0 ), TB_ATTACK );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 25, 40, 1 ), TB_CONSOLE );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 9, 40, 0 ), -1 );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 41, 40, 0 ), -1 );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 25, 19, 0 ), -1 );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 25, 61, 0 ), -1 );
}

BOOST_AUTO_TEST_CASE( stick_outside_radius )
{
	float x, y;
	TouchLayout_Stick( 10, 20, 130, 180, 100, &x, &y );
	BOOST_CHECK_SMALL( sqrtf( x * x + y * y ) - 1.0f, 1e-5f );
	BOOST_CHECK_SMALL( x - 0.6f, 1e-5f );
	BOOST_CHECK_SMALL( y - 0.8f, 1e-5f );
}

BOOST_AUTO_TEST_CASE( stick_inside_radius )
{
	float x, y;
	TouchLayout_Stick( 10, 20, 60, 20, 100, &x, &y );
	BOOST_CHECK_SMALL( x - 0.5f, 1e-5f );
	BOOST_CHECK_SMALL( y, 1e-5f );
	TouchLayout_Stick( 10, 20, 10, -30, 100, &x, &y );
	BOOST_CHECK_SMALL( x, 1e-5f );
	BOOST_CHECK_SMALL( y + 0.5f, 1e-5f );
}

BOOST_AUTO_TEST_CASE( virtual_cursor )
{
	int x, y;
	TouchLayout_ToVirtual( 0, 0, &x, &y );
	BOOST_CHECK_EQUAL( x, 0 );
	BOOST_CHECK_EQUAL( y, 0 );
	TouchLayout_ToVirtual( 1, 1, &x, &y );
	BOOST_CHECK_EQUAL( x, 640 );
	BOOST_CHECK_EQUAL( y, 480 );
	TouchLayout_ToVirtual( -0.1f, 1.2f, &x, &y );
	BOOST_CHECK_EQUAL( x, 0 );
	BOOST_CHECK_EQUAL( y, 480 );
	TouchLayout_ToVirtual( 0.5f, 0.5f, &x, &y );
	BOOST_CHECK_EQUAL( x, 320 );
	BOOST_CHECK_EQUAL( y, 240 );
	TouchLayout_ToVirtual( 10.75f / 640, 20.25f / 480, &x, &y );
	BOOST_CHECK_EQUAL( x, 11 );
	BOOST_CHECK_EQUAL( y, 20 );
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_CASE( stick_zero_radius_and_nan_virtual )
{
	float sx = 1, sy = 1;
	TouchLayout_Stick( 0, 0, 5, 5, 0, &sx, &sy );
	BOOST_CHECK_EQUAL( sx, 0 );
	BOOST_CHECK_EQUAL( sy, 0 );
	int vx, vy;
	TouchLayout_ToVirtual( 0.0f / 0.0f, 1e9f, &vx, &vy );
	BOOST_CHECK( vx >= 0 && vx <= 640 );
	BOOST_CHECK_EQUAL( vy, 480 );
}

BOOST_AUTO_TEST_SUITE_END()
