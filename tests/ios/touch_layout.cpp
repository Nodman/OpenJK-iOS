#include "../../code/client/keycodes.h"
namespace mp_touch_keys {
#include "../../codemp/client/keycodes.h"
}

#include "sys/ios/ios_touch_layout.cpp"
#include "sys/ios/ios_touch_state.h"
#include "qcommon/aspect_layout.h"

#include <boost/test/unit_test.hpp>

#include <math.h>

BOOST_AUTO_TEST_SUITE( ios )
BOOST_AUTO_TEST_SUITE( touch_layout )

BOOST_AUTO_TEST_CASE( button_definitions )
{
	const touchButtonDef_t expected[TB_COUNT] = {
		{ "ATK", TOUCH_ANCHOR_BR, .413f, .590f, .14f, TG_ATTACK, 16, NULL },
		{ "ALT", TOUCH_ANCHOR_BR, .510f, .438f, .14f, TG_ALTATTACK, 17, NULL },
		{ "JUMP", TOUCH_ANCHOR_BR, .246f, .659f, .14f, TG_JUMP, 18, NULL },
		{ "DUCK", TOUCH_ANCHOR_BR, .40f, .08f, .12f, TG_CROUCH, 19, NULL },
		{ "USE", TOUCH_ANCHOR_BL, .12f, .08f, .12f, TG_USE, 20, NULL },
		{ "FORCE", TOUCH_ANCHOR_TR, .49f, .26f, .12f, TG_FORCE, 21, NULL },
		{ "F>>", TOUCH_ANCHOR_TR, .65f, .26f, .12f, TG_FORCENEXT, 22, NULL },
		{ "STYLE", TOUCH_ANCHOR_BR, .12f, .08f, .12f, TG_STYLE, 23, NULL },
		{ "<W", TOUCH_ANCHOR_TR, .65f, .10f, .12f, TG_WEAPPREV, 24, NULL },
		{ "W>", TOUCH_ANCHOR_TR, .33f, .10f, .12f, TG_WEAPNEXT, 25, NULL },
		{ "MENU", TOUCH_ANCHOR_TL, .12f, .10f, .12f, TG_MENU, TOUCH_KEY_ESCAPE, NULL },
		{ NULL, TOUCH_ANCHOR_TL, .95f, .08f, .11f, TG_CONSOLE, TOUCH_KEY_NONE, NULL },
		{ "SABER", TOUCH_ANCHOR_TR, .49f, .10f, .12f, TG_SABER_SELECT, 26, NULL },
		{ "CHAT", TOUCH_ANCHOR_TL, .28f, .10f, .12f, TG_CHAT, TOUCH_KEY_CHAT, NULL },
		{ "PAD", TOUCH_ANCHOR_TL, .12f, .26f, .12f, TG_PAD, TOUCH_KEY_TAB, NULL }
	};
	for ( int i = 0; i < TB_COUNT; ++i ) {
		BOOST_CHECK_EQUAL( touchButtonDefs[i].anchor, expected[i].anchor );
		BOOST_CHECK_EQUAL( touchButtonDefs[i].cx, expected[i].cx );
		BOOST_CHECK_EQUAL( touchButtonDefs[i].cy, expected[i].cy );
		BOOST_CHECK_EQUAL( touchButtonDefs[i].size, expected[i].size );
		BOOST_CHECK_EQUAL( touchButtonDefs[i].glyph, expected[i].glyph );
		BOOST_CHECK_EQUAL( touchButtonDefs[i].key, expected[i].key );
	}
	BOOST_CHECK( touchButtonDefs[TB_CONSOLE].label == NULL );
	const int glyphs[] = { TG_MENU, TG_CHAT, TG_PAD, TG_WEAPPREV, TG_SABER_SELECT, TG_WEAPNEXT,
		TG_FORCENEXT, TG_FORCE, TG_CONSOLE, TG_ALTATTACK, TG_ATTACK, TG_JUMP, TG_CROUCH, TG_STYLE, TG_USE, TG_PLATE };
	for ( int i = 0; i < 16; ++i ) BOOST_CHECK_EQUAL( glyphs[i], i );
}

BOOST_AUTO_TEST_CASE( anchored_target_fixtures_and_arc_gaps )
{
	const float fixtures[][7] = {
		{ 2868, 1320, 132, 0, 132, 102, 3 },
		{ 1434, 660, 66, 0, 66, 51, 1.5f },
		{ 2048, 1536, 0, 0, 0, 0, 2 },
		{ 1024, 768, 0, 0, 0, 0, 1 },
		{ 2868, 1320, 144, 30, 44, 90, 3 },
		{ 2868, 1320, 44, 30, 144, 90, 3 }
	};
	for ( const auto &f : fixtures ) {
		touchRect_t rects[TB_COUNT];
		TouchLayout_Build( f[0], f[1], f[2], f[3], f[4], f[5], f[6], rects );
		const float unit = TouchLayout_Unit( f[0] - f[2] - f[4], f[1] - f[3] - f[5], f[6] );
		for ( int i = 0; i < TB_COUNT; ++i ) {
			const auto &r = rects[i];
			BOOST_CHECK_GE( r.x, f[2] );
			BOOST_CHECK_GE( r.y, f[3] );
			BOOST_CHECK_LE( r.x + r.w, f[0] - f[4] );
			BOOST_CHECK_LE( r.y + r.h, f[1] - f[5] );
			if ( i != TB_CONSOLE ) BOOST_CHECK_GE( r.w / f[6], 44 );
			BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, r.x + r.w / 2, r.y + r.h / 2, ~0u ), i );
			BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, r.x + r.w / 2, r.y + r.h / 2, ~( 1u << i ) ), -1 );
			for ( int j = 0; j < i; ++j ) {
				const auto &other = rects[j];
				BOOST_CHECK( r.x + r.w <= other.x || other.x + other.w <= r.x ||
					r.y + r.h <= other.y || other.y + other.h <= r.y );
			}
		}
		const int arc[] = { TB_ALTATTACK, TB_ATTACK, TB_JUMP };
		const float angles[] = { 165, 130, 95 };
		const float homeX = f[0] - f[4] - .22f * unit;
		const float homeY = f[1] - f[5] - .36f * unit;
		for ( int i = 0; i < 3; ++i ) {
			const auto &r = rects[arc[i]];
			const float dx = r.x + r.w / 2 - homeX, dy = homeY - r.y - r.h / 2;
			BOOST_CHECK_SMALL( hypotf( dx, dy ) / unit - .30f, .001f );
			BOOST_CHECK_SMALL( atan2f( dy, dx ) * 180 / 3.14159265f - angles[i], .15f );
			if ( i > 0 ) {
				const auto &prev = rects[arc[i - 1]];
				const float gap = hypotf( r.x + r.w / 2 - prev.x - prev.w / 2, r.y + r.h / 2 - prev.y - prev.h / 2 ) - ( r.w + prev.w ) / 2;
				BOOST_CHECK_SMALL( gap / unit - .040f, .002f );
			}
		}
	}
}

BOOST_AUTO_TEST_CASE( small_windows_are_finite_safe_and_deterministic )
{
	for ( const auto &f : { touchRect_t{ 600, 400, 20, 30 }, touchRect_t{ 400, 1000, 20, 30 }, touchRect_t{ 60, 40, 2, 3 } } ) {
		touchRect_t rects[TB_COUNT];
		TouchLayout_Build( f.x, f.y, f.w, f.h, 4, 5, 3, rects );
		for ( int i = 0; i < TB_COUNT; ++i ) {
			if ( i == TB_CONSOLE ) continue;
			BOOST_CHECK( IOS_TouchFinite( rects[i].x ) && IOS_TouchFinite( rects[i].y ) && IOS_TouchFinite( rects[i].w ) );
			BOOST_CHECK_GE( rects[i].x, f.w );
			BOOST_CHECK_GE( rects[i].y, f.h );
			BOOST_CHECK_LE( rects[i].x + rects[i].w, f.x - 4 );
			BOOST_CHECK_LE( rects[i].y + rects[i].h, f.y - 5 );
		}
	}
	touchRect_t rects[TB_COUNT];
	TouchLayout_Build( 2048, 1536, 0, 0, 0, 0, 0, rects );
	for ( int i = 0; i < TB_COUNT; ++i ) BOOST_CHECK_EQUAL( rects[i].w, 0 );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 0, 0, ~0u ), -1 );
}

BOOST_AUTO_TEST_CASE( console_geometry_matches_layout )
{
	const float fixtures[][7] = {
		{ 1434, 660, 66, 0, 66, 51, 1.5f },
		{ 2868, 1320, 132, 0, 132, 102, 3 },
		{ 1434, 660, 66, 0, 0, 51, 1.5f },
		{ 1434, 660, 0, 0, 66, 51, 1.5f },
		{ 600, 400, 20, 30, 40, 50, 1 },
		{ 400, 1000, 20, 30, 40, 50, 1 }
	};
	for ( const auto &f : fixtures ) {
		touchRect_t rects[TB_COUNT], con;
		TouchLayout_Build( f[0], f[1], f[2], f[3], f[4], f[5], f[6], rects );
		IOS_TouchConsoleRectPixels( f[0], f[1], f[2], f[3], f[4], f[5], &con.x, &con.y, &con.w, &con.h );
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

BOOST_AUTO_TEST_CASE( explicit_key_map_and_paired_pad_chat_trace )
{
	const int expected[TB_COUNT] = { A_AUX16, A_AUX17, A_AUX18, A_AUX19, A_AUX20, A_AUX21, A_AUX22,
		A_AUX23, A_AUX24, A_AUX25, A_ESCAPE, 0, A_AUX26, 'y', A_TAB };
	const int expectedMP[TB_COUNT] = { mp_touch_keys::A_AUX16, mp_touch_keys::A_AUX17, mp_touch_keys::A_AUX18,
		mp_touch_keys::A_AUX19, mp_touch_keys::A_AUX20, mp_touch_keys::A_AUX21, mp_touch_keys::A_AUX22,
		mp_touch_keys::A_AUX23, mp_touch_keys::A_AUX24, mp_touch_keys::A_AUX25, mp_touch_keys::A_ESCAPE,
		0, mp_touch_keys::A_AUX26, 'y', mp_touch_keys::A_TAB };
	for ( int i = 0; i < TB_COUNT; ++i ) {
		BOOST_CHECK_EQUAL( TouchLayout_Key( i, A_AUX16, A_ESCAPE, A_TAB ), expected[i] );
		BOOST_CHECK_EQUAL( TouchLayout_Key( i, mp_touch_keys::A_AUX16, mp_touch_keys::A_ESCAPE, mp_touch_keys::A_TAB ), expectedMP[i] );
	}
	BOOST_CHECK_EQUAL( TouchLayout_Key( -1, A_AUX16, A_ESCAPE, A_TAB ), 0 );
	BOOST_CHECK_EQUAL( TouchLayout_Key( TB_COUNT, A_AUX16, A_ESCAPE, A_TAB ), 0 );
	for ( int button : { TB_PAD, TB_CHAT } ) {
		touchGestureState_t state;
		TouchGesture_Reset( &state );
		touchGestureInput_t input = {};
		input.target = TOUCH_TARGET_BUTTON;
		input.button = button;
		input.visibleButtons = ~0u;
		for ( int event : { TOUCH_DOWN, TOUCH_UP } ) {
			input.event = event;
			const auto out = TouchGesture_Step( &state, input );
			BOOST_REQUIRE_EQUAL( out.keyCount, 1 );
			BOOST_CHECK_EQUAL( TouchLayout_Key( out.keys[0].button, A_AUX16, A_ESCAPE, A_TAB ), button == TB_PAD ? A_TAB : 'y' );
			BOOST_CHECK_EQUAL( out.keys[0].down, event == TOUCH_DOWN );
		}
	}
}

BOOST_AUTO_TEST_CASE( default_binds_restore_empty_aux_keys_after_configs )
{
	const char *commands[] = { "+attack", "+altattack", "+moveup", "+movedown", "+use", "+useforce",
		"forcenext", "saberAttackCycle", "weapprev", "weapnext", "weapon 1" };
	const char *bindings[TB_COUNT] = {};
	const auto restore = [&]() {
		int count = 0;
		for ( int button = 0; button < TB_COUNT; ++button ) {
			const char *command = TouchLayout_DefaultBind( button, bindings[button] );
			if ( command ) {
				bindings[button] = command;
				++count;
			}
		}
		return count;
	};
	BOOST_CHECK_EQUAL( restore(), 11 );
	for ( int button = 0; button < TB_COUNT; ++button ) {
		const int key = touchButtonDefs[button].key;
		if ( key >= 16 && key <= 26 ) {
			BOOST_REQUIRE( bindings[button] );
			BOOST_CHECK_EQUAL( bindings[button], commands[key - 16] );
			BOOST_CHECK( TouchLayout_DefaultBind( button, "custom action" ) == NULL );
			BOOST_CHECK( TouchLayout_DefaultBind( button, " " ) == NULL );
		} else {
			BOOST_CHECK( bindings[button] == NULL );
			BOOST_CHECK( TouchLayout_DefaultBind( button, "" ) == NULL );
		}
	}
	BOOST_CHECK_EQUAL( restore(), 0 );
	bindings[TB_SABER_SELECT] = "";
	bindings[TB_ATTACK] = "+altattack";
	bindings[TB_PAD] = "datapad";
	bindings[TB_CHAT] = "messagemode";
	BOOST_CHECK_EQUAL( restore(), 1 );
	BOOST_CHECK_EQUAL( bindings[TB_SABER_SELECT], "weapon 1" );
	BOOST_CHECK_EQUAL( bindings[TB_ATTACK], "+altattack" );
	BOOST_CHECK_EQUAL( bindings[TB_PAD], "datapad" );
	BOOST_CHECK_EQUAL( bindings[TB_CHAT], "messagemode" );
	bindings[TB_SABER_SELECT] = "weapon 3";
	bindings[TB_PAD] = bindings[TB_CHAT] = "";
	BOOST_CHECK_EQUAL( restore(), 0 );
	BOOST_CHECK_EQUAL( bindings[TB_SABER_SELECT], "weapon 3" );
	BOOST_CHECK_EQUAL( bindings[TB_PAD], "" );
	BOOST_CHECK_EQUAL( bindings[TB_CHAT], "" );
	for ( int button = 0; button < TB_COUNT; ++button ) bindings[button] = "";
	BOOST_CHECK_EQUAL( restore(), 11 );
	BOOST_CHECK( TouchLayout_DefaultBind( -1, NULL ) == NULL );
	BOOST_CHECK( TouchLayout_DefaultBind( TB_COUNT, NULL ) == NULL );
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
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 25, 40, ~0u ), TB_ATTACK );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 25, 40, 1u << TB_CONSOLE ), TB_CONSOLE );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 9, 40, ~0u ), -1 );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 41, 40, ~0u ), -1 );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 25, 19, ~0u ), -1 );
	BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, 25, 61, ~0u ), -1 );
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
	TouchLayout_Build( 640, 600, 0, 0, 0, 0, 1, rects );
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
		TouchLayout_Build( layout.W, layout.H, layout.safeL, layout.safeT, layout.safeR, layout.safeB, 2, rects );
		for ( int i = 0; i < TB_COUNT; ++i ) {
			BOOST_TEST_CONTEXT( "button " << i << ", width " << layout.W << ", branch " << layout.branch ) {
				CheckTouchRectRoundTrip( layout, rects[i] );
				BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, rects[i].x + rects[i].w / 2,
					rects[i].y + rects[i].h / 2, ~0u ), i );
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
	TouchLayout_Build( before.W, before.H, before.safeL, before.safeT, before.safeR, before.safeB, 3, oldRects );
	TouchLayout_Build( after.W, after.H, after.safeL, after.safeT, after.safeR, after.safeB, 3, newRects );
	for ( int i = 0; i < TB_COUNT; ++i ) {
		BOOST_CHECK_SMALL( newRects[i].x - oldRects[i].x + 100.0f, 1e-3f );
		BOOST_CHECK_EQUAL( newRects[i].y, oldRects[i].y );
		BOOST_CHECK_EQUAL( newRects[i].w, oldRects[i].w );
		BOOST_CHECK_EQUAL( newRects[i].h, oldRects[i].h );
		CheckTouchRectRoundTrip( before, oldRects[i] );
		CheckTouchRectRoundTrip( after, newRects[i] );
		BOOST_CHECK_EQUAL( TouchLayout_Hit( newRects, newRects[i].x + newRects[i].w / 2,
			newRects[i].y + newRects[i].h / 2, ~0u ), i );
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
