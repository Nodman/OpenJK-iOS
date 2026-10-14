#include "sys/ios/ios_touch_layout.cpp"
#include "qcommon/aspect_layout.h"

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

BOOST_AUTO_TEST_CASE( console_geometry_matches_layout )
{
	const float fixtures[][6] = {
		{ 1434, 660, 66, 0, 66, 51 },
		{ 2868, 1320, 132, 0, 132, 102 },
		{ 1434, 660, 66, 0, 0, 51 },
		{ 1434, 660, 0, 0, 66, 51 },
		{ 600, 400, 20, 30, 40, 50 },
		{ 400, 1000, 20, 30, 40, 50 }
	};
	for ( const auto &f : fixtures ) {
		touchRect_t rects[TB_COUNT], con;
		TouchLayout_Build( f[0], f[1], f[2], f[3], f[4], f[5], rects );
		IOS_TouchConsoleRectPixels( f[0], f[1], f[2], f[3], f[4], f[5],
			&con.x, &con.y, &con.w, &con.h );
		BOOST_CHECK_EQUAL( con.x, rects[TB_CONSOLE].x );
		BOOST_CHECK_EQUAL( con.y, rects[TB_CONSOLE].y );
		BOOST_CHECK_EQUAL( con.w, rects[TB_CONSOLE].w );
		BOOST_CHECK_EQUAL( con.h, rects[TB_CONSOLE].h );
	}
	float x, y, w, h;
	IOS_TouchConsoleRectPixels( 1434, 660, 66, 0, 66, 51, &x, &y, &w, &h );
	BOOST_CHECK_SMALL( x - 1269.405f, 1e-3f );
	BOOST_CHECK_SMALL( y - 15.225f, 1e-3f );
	BOOST_CHECK_SMALL( w - 66.99f, 1e-3f );
	BOOST_CHECK_EQUAL( w, h );
}

BOOST_AUTO_TEST_CASE( fps_clears_console )
{
	for ( int factor = 1; factor <= 2; ++factor ) {
		for ( int corrected = 0; corrected <= 1; ++corrected ) {
			for ( int side = 0; side < 3; ++side ) {
				const int L = side == 2 ? 0 : 66 * factor;
				const int R = side == 1 ? 0 : 66 * factor;
				const aspectLayout_t layout = AspectLayout_Build( 1434 * factor, 660 * factor,
					corrected, 1.5f, L, 0, R, 51 * factor );
				touchRect_t con;
				IOS_TouchConsoleRectPixels( layout.W, layout.H, L, 0, R, layout.safeB,
					&con.x, &con.y, &con.w, &con.h );
				const float scale = AspectLayout_SharpTextScale( &layout, 18, 22 );
				BOOST_CHECK_SMALL( scale - ( factor == 1 ? 0.888889f : 0.444444f ), 1e-6f );
				const double right = AspectLayout_ToVirtual( &layout,
					fminf( layout.W - R, con.x - 6 ), 0 ).x;
				// Retail ocr_a "60fps" has five glyphs and 51 total advances; emulate
				// integer width and draw coordinates using only the pure geometry helpers.
				const int width = (int)ceilf( 51 * scale );
				const aspectPoint_t origin = AspectLayout_ToPixels( &layout,
					(int)( right - width ), (int)( layout.fullRect.y + 2 ) );
				const aspectPoint_t size = AspectLayout_SizeToPixels( &layout, width, 18 * scale );
				BOOST_CHECK_LE( origin.x + size.x, con.x - 6 + 1e-3 );
				BOOST_CHECK_LE( origin.x + size.x, layout.W - R );
				BOOST_CHECK( origin.x + size.x <= con.x || origin.x >= con.x + con.w ||
					origin.y + size.y <= con.y || origin.y >= con.y + con.h );
			}
		}
	}
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
	const aspectLayout_t layout = AspectLayout_Build( 2388, 1668, 1, 1.5f, 0, 0, 0, 0 );
	BOOST_CHECK_EQUAL( layout.branch, ASPECT_LEGACY );
	int x, y;
	TouchLayout_ToVirtual( &layout, 0, 0, &x, &y );
	BOOST_CHECK_EQUAL( x, 0 );
	BOOST_CHECK_EQUAL( y, 0 );
	TouchLayout_ToVirtual( &layout, 1, 1, &x, &y );
	BOOST_CHECK_EQUAL( x, 640 );
	BOOST_CHECK_EQUAL( y, 480 );
	TouchLayout_ToVirtual( &layout, -0.1f, 1.2f, &x, &y );
	BOOST_CHECK_EQUAL( x, 0 );
	BOOST_CHECK_EQUAL( y, 480 );
	TouchLayout_ToVirtual( &layout, 0.5f, 0.5f, &x, &y );
	BOOST_CHECK_EQUAL( x, 320 );
	BOOST_CHECK_EQUAL( y, 240 );
	TouchLayout_ToVirtual( &layout, 0.5f / 640, 0.5f / 480, &x, &y );
	BOOST_CHECK_EQUAL( x, 1 );
	BOOST_CHECK_EQUAL( y, 1 );
	TouchLayout_ToVirtual( &layout, 10.75f / 640, 20.25f / 480, &x, &y );
	BOOST_CHECK_EQUAL( x, 11 );
	BOOST_CHECK_EQUAL( y, 20 );
}

BOOST_AUTO_TEST_CASE( disabled_wide_cursor )
{
	const aspectLayout_t layout = AspectLayout_Build( 2868, 1320, 0, 1.5f, 144, 30, 44, 90 );
	BOOST_CHECK_EQUAL( layout.branch, ASPECT_LEGACY );
	int x, y;
	TouchLayout_ToVirtual( &layout, 0.5f, 0.5f, &x, &y );
	BOOST_CHECK_EQUAL( x, 320 );
	BOOST_CHECK_EQUAL( y, 240 );
	TouchLayout_ToVirtual( &layout, 1, 1, &x, &y );
	BOOST_CHECK_EQUAL( x, 640 );
	BOOST_CHECK_EQUAL( y, 480 );
	TouchLayout_ToVirtual( &layout, 0, 0, &x, &y );
	BOOST_CHECK_EQUAL( x, 0 );
	BOOST_CHECK_EQUAL( y, 0 );
	TouchLayout_ToVirtual( &layout, 0.5f / 640, 0.5f / 480, &x, &y );
	BOOST_CHECK_EQUAL( x, 1 );
	BOOST_CHECK_EQUAL( y, 1 );
}

BOOST_AUTO_TEST_CASE( corrected_cursor_and_gutters )
{
	const aspectLayout_t layout = AspectLayout_Build( 2868, 1320, 1, 1.5f, 144, 30, 44, 90 );
	BOOST_CHECK_EQUAL( layout.branch, ASPECT_CORRECTED );
	BOOST_CHECK_EQUAL( layout.ox, 554 );
	int x, y;
	TouchLayout_ToVirtual( &layout, 554.0f / 2868, 0, &x, &y );
	BOOST_CHECK_EQUAL( x, 0 );
	BOOST_CHECK_EQUAL( y, 0 );
	TouchLayout_ToVirtual( &layout, 2314.0f / 2868, 1, &x, &y );
	BOOST_CHECK_EQUAL( x, 640 );
	BOOST_CHECK_EQUAL( y, 480 );
	TouchLayout_ToVirtual( &layout, 0.5f, 0.5f, &x, &y );
	BOOST_CHECK_EQUAL( x, 320 );
	BOOST_CHECK_EQUAL( y, 240 );
	TouchLayout_ToVirtual( &layout, -0.1f, 1.2f, &x, &y );
	BOOST_CHECK_EQUAL( x, 0 );
	BOOST_CHECK_EQUAL( y, 480 );
	TouchLayout_ToVirtual( &layout, 144.0f / 2868, 0.5f, &x, &y );
	BOOST_CHECK_EQUAL( x, 0 );
	BOOST_CHECK_EQUAL( y, 240 );
	TouchLayout_ToVirtual( &layout, 2500.0f / 2868, 0.5f, &x, &y );
	BOOST_CHECK_EQUAL( x, 640 );
	BOOST_CHECK_EQUAL( y, 240 );
	const touchRect_t left = TouchLayout_RectToVirtual( &layout, { 0, 0, 144, 144 } );
	const touchRect_t right = TouchLayout_RectToVirtual( &layout, { 2500, 0, 144, 144 } );
	BOOST_CHECK_LT( left.x, 0 );
	BOOST_CHECK_GT( right.x, 640 );
	BOOST_CHECK_SMALL( left.x + 554.0f / 2.75f, 1e-4f );
	BOOST_CHECK_SMALL( left.w - 144.0f / 2.75f, 1e-4f );
	BOOST_CHECK_EQUAL( left.w, left.h );
}

BOOST_AUTO_TEST_CASE( snapped_cursor )
{
	const aspectLayout_t layout = AspectLayout_Build( 2151, 990, 1, 1.5f, 0, 0, 0, 0 );
	BOOST_CHECK_EQUAL( layout.ox, 416 );
	int x, y;
	TouchLayout_ToVirtual( &layout, ( 416.0f + 10.75f * 2.0625f ) / 2151,
		20.25f * 2.0625f / 990, &x, &y );
	BOOST_CHECK_EQUAL( x, 11 );
	BOOST_CHECK_EQUAL( y, 20 );
}

static void CheckTouchRectRoundTrip( const aspectLayout_t &layout, const touchRect_t &rect )
{
	const touchRect_t overlay = TouchLayout_RectToVirtual( &layout, rect );
	const aspectPoint_t origin = AspectLayout_ToPixels( &layout, overlay.x, overlay.y );
	const aspectPoint_t size = AspectLayout_SizeToPixels( &layout, overlay.w, overlay.h );
	BOOST_CHECK_SMALL( origin.x - rect.x, 1e-3 );
	BOOST_CHECK_SMALL( origin.y - rect.y, 1e-3 );
	BOOST_CHECK_SMALL( size.x - rect.w, 1e-3 );
	BOOST_CHECK_SMALL( size.y - rect.h, 1e-3 );
}

BOOST_AUTO_TEST_CASE( vertical_gutters )
{
	const aspectLayout_t layout = AspectLayout_Build( 640, 600, 1, 0, 0, 0, 0, 0 );
	BOOST_CHECK_EQUAL( layout.branch, ASPECT_CORRECTED );
	BOOST_CHECK_EQUAL( layout.ox, 0 );
	BOOST_CHECK_EQUAL( layout.oy, 60 );
	int x, y;
	TouchLayout_ToVirtual( &layout, 0.5f, 30.0f / 600, &x, &y );
	BOOST_CHECK_EQUAL( x, 320 );
	BOOST_CHECK_EQUAL( y, 0 );
	TouchLayout_ToVirtual( &layout, 0.5f, 570.0f / 600, &x, &y );
	BOOST_CHECK_EQUAL( x, 320 );
	BOOST_CHECK_EQUAL( y, 480 );
	TouchLayout_ToVirtual( &layout, 0.5f, 0.5f, &x, &y );
	BOOST_CHECK_EQUAL( x, 320 );
	BOOST_CHECK_EQUAL( y, 240 );
	const touchRect_t top = { 100, 20, 40, 30 };
	const touchRect_t bottom = { 100, 560, 40, 30 };
	BOOST_CHECK_EQUAL( TouchLayout_RectToVirtual( &layout, top ).y, -40 );
	BOOST_CHECK_EQUAL( TouchLayout_RectToVirtual( &layout, bottom ).y, 500 );
	CheckTouchRectRoundTrip( layout, top );
	CheckTouchRectRoundTrip( layout, bottom );
	touchRect_t rects[TB_COUNT];
	TouchLayout_Build( 640, 600, 0, 0, 0, 0, rects );
	for ( int i = 0; i < TB_COUNT; ++i )
		CheckTouchRectRoundTrip( layout, rects[i] );
}

BOOST_AUTO_TEST_CASE( every_button_round_trips )
{
	const aspectLayout_t layouts[] = {
		AspectLayout_Build( 2388, 1668, 1, 1.5f, 44, 30, 12, 90 ),
		AspectLayout_Build( 2868, 1320, 1, 1.5f, 144, 30, 44, 90 ),
		AspectLayout_Build( 2868, 1320, 0, 1.5f, 144, 30, 44, 90 ),
		AspectLayout_Build( 2151, 990, 1, 1.5f, 99, 10, 33, 60 )
	};
	for ( const aspectLayout_t &layout : layouts ) {
		touchRect_t rects[TB_COUNT];
		TouchLayout_Build( layout.W, layout.H, layout.safeL, layout.safeT, layout.safeR, layout.safeB, rects );
		for ( int i = 0; i < TB_COUNT; ++i ) {
			BOOST_TEST_CONTEXT( "button " << i << ", width " << layout.W << ", branch " << layout.branch ) {
				CheckTouchRectRoundTrip( layout, rects[i] );
				BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, rects[i].x + rects[i].w / 2,
					rects[i].y + rects[i].h / 2, 0 ), i );
				BOOST_CHECK_GE( rects[i].x, layout.safeL );
				BOOST_CHECK_GE( rects[i].y, layout.safeT );
				BOOST_CHECK_LE( rects[i].x + rects[i].w, layout.W - layout.safeR );
				BOOST_CHECK_LE( rects[i].y + rects[i].h, layout.H - layout.safeB );
			}
		}
		CheckTouchRectRoundTrip( layout, { 50, 100, 280, 280 } );
		CheckTouchRectRoundTrip( layout, { 148, 198, 84, 84 } );
	}
}

BOOST_AUTO_TEST_CASE( equal_size_rotation )
{
	const aspectLayout_t before = AspectLayout_Build( 2868, 1320, 1, 1.5f, 144, 30, 44, 90 );
	const aspectLayout_t after = AspectLayout_Build( 2868, 1320, 1, 1.5f, 44, 30, 144, 90 );
	touchRect_t oldRects[TB_COUNT], newRects[TB_COUNT];
	TouchLayout_Build( before.W, before.H, before.safeL, before.safeT, before.safeR, before.safeB, oldRects );
	TouchLayout_Build( after.W, after.H, after.safeL, after.safeT, after.safeR, after.safeB, newRects );
	for ( int i = 0; i < TB_COUNT; ++i ) {
		BOOST_CHECK_SMALL( newRects[i].x - oldRects[i].x + 100.0f, 1e-3f );
		BOOST_CHECK_EQUAL( newRects[i].y, oldRects[i].y );
		BOOST_CHECK_EQUAL( newRects[i].w, oldRects[i].w );
		BOOST_CHECK_EQUAL( newRects[i].h, oldRects[i].h );
		CheckTouchRectRoundTrip( before, oldRects[i] );
		CheckTouchRectRoundTrip( after, newRects[i] );
		BOOST_CHECK_EQUAL( TouchLayout_Hit( newRects, newRects[i].x + newRects[i].w / 2,
			newRects[i].y + newRects[i].h / 2, 0 ), i );
	}
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_CASE( stick_zero_radius_and_nan_virtual )
{
	float sx = 1, sy = 1;
	TouchLayout_Stick( 0, 0, 5, 5, 0, &sx, &sy );
	BOOST_CHECK_EQUAL( sx, 0 );
	BOOST_CHECK_EQUAL( sy, 0 );
	const aspectLayout_t layout = AspectLayout_Build( 2868, 1320, 1, 1.5f, 0, 0, 0, 0 );
	int vx, vy;
	TouchLayout_ToVirtual( &layout, 0.0f / 0.0f, 1e9f, &vx, &vy );
	BOOST_CHECK_EQUAL( vx, 0 );
	BOOST_CHECK_EQUAL( vy, 480 );
}

BOOST_AUTO_TEST_SUITE_END()
