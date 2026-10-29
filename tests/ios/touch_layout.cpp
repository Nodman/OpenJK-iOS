#include "../../code/client/keycodes.h"
namespace mp_touch_keys {
#include "../../codemp/client/keycodes.h"
}

#include "sys/ios/ios_touch_layout.cpp"
#include "sys/ios/ios_touch_state.h"
#include "qcommon/aspect_layout.h"

#include <boost/test/unit_test.hpp>

#include <math.h>
#include "touch_tablet_fixture.h"
#include <cstring>

static void HistoricalLayout(float W,float H,float L,float T,float R,float B,float p,touchRect_t out[TB_COUNT],const touchLayoutOverrides_t *o=NULL) {
	TouchLayout_Build(W,H,L,T,R,B,p,out,o,TOUCH_LAYOUT_LEGACY);
}

BOOST_AUTO_TEST_SUITE( ios )
BOOST_AUTO_TEST_SUITE( touch_layout_legacy )

BOOST_AUTO_TEST_CASE( button_definitions )
{
	const touchButtonDef_t expected[TB_COUNT] = {
		{ "ATK", TOUCH_ANCHOR_BR, .413f, .590f, .14f, TG_ATTACK, 16, NULL },
		{ "ALT", TOUCH_ANCHOR_BR, .510f, .438f, .14f, TG_ALTATTACK, 17, NULL },
		{ "JUMP", TOUCH_ANCHOR_BR, .246f, .659f, .14f, TG_JUMP, 18, NULL },
		{ "DUCK", TOUCH_ANCHOR_BR, .40f, .08f, .12f, TG_CROUCH, 19, NULL },
		{ "USE", TOUCH_ANCHOR_BL, .12f, .08f, .12f, TG_USE, 20, NULL },
		{ "FORCE", TOUCH_ANCHOR_TR, .49f, .26f, .12f, TG_FORCE, 21, NULL },
		{ "F>>", TOUCH_ANCHOR_TR, .41f, .26f, .12f, TG_WEAPNEXT, 22, NULL },
		{ "STYLE", TOUCH_ANCHOR_BR, .12f, .08f, .12f, TG_SABER_SELECT, 23, NULL },
		{ "<W", TOUCH_ANCHOR_TR, .65f, .10f, .12f, TG_WEAPPREV, 24, NULL },
		{ "W>", TOUCH_ANCHOR_TR, .33f, .10f, .12f, TG_WEAPNEXT, 25, NULL },
		{ "MENU", TOUCH_ANCHOR_TL, .12f, .10f, .12f, TG_MENU, TOUCH_KEY_ESCAPE, NULL },
		{ NULL, TOUCH_ANCHOR_TL, .95f, .08f, .11f, TG_CONSOLE, TOUCH_KEY_NONE, NULL },
		{ "SABER", TOUCH_ANCHOR_TR, .49f, .10f, .12f, TG_SABER_SELECT, 26, NULL },
		{ "CHAT", TOUCH_ANCHOR_TL, .28f, .10f, .12f, TG_CHAT, TOUCH_KEY_CHAT, NULL },
		{ "PAD", TOUCH_ANCHOR_TL, .12f, .26f, .12f, TG_PAD, TOUCH_KEY_TAB, NULL },
		{ NULL, TOUCH_ANCHOR_TR, .11f, .34f, .12f, TG_LOCK, TOUCH_KEY_NONE, NULL },
		{ "F<<", TOUCH_ANCHOR_TR, .57f, .26f, .12f, TG_WEAPPREV, 27, NULL },
		{ "1", TOUCH_ANCHOR_TR, 1.04f, .26f, .12f, -1, TOUCH_KEY_NONE, NULL },
		{ "2", TOUCH_ANCHOR_TR, .96f, .26f, .12f, -1, TOUCH_KEY_NONE, NULL },
		{ "3", TOUCH_ANCHOR_TR, .88f, .26f, .12f, -1, TOUCH_KEY_NONE, NULL },
		{ "4", TOUCH_ANCHOR_TR, .80f, .26f, .12f, -1, TOUCH_KEY_NONE, NULL },
		{ "BOTH", TOUCH_ANCHOR_BR, .68f, .25f, .14f, TG_BOTH_ATTACK, 28, NULL }
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
		TG_RING, TG_FORCE, TG_CONSOLE, TG_ALTATTACK, TG_ATTACK, TG_JUMP, TG_CROUCH, TG_STYLE_BLUE, TG_USE, TG_PLATE };
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
		HistoricalLayout( f[0], f[1], f[2], f[3], f[4], f[5], f[6], rects );
		const float unit = TouchLayout_Unit( f[0] - f[2] - f[4], f[1] - f[3] - f[5], f[6] );
		for ( int i = 0; i < TB_COUNT; ++i ) {
			const auto &r = rects[i];
			BOOST_CHECK_GE( r.x, f[2] );
			BOOST_CHECK_GE( r.y, f[3] );
			BOOST_CHECK_LE( r.x + r.w, f[0] - f[4] );
			BOOST_CHECK_LE( r.y + r.h, f[1] - f[5] );
			if ( i != TB_CONSOLE ) BOOST_CHECK_GE( r.w / f[6], 44 * TouchLayout_ButtonScale( i ) );
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
		HistoricalLayout( f.x, f.y, f.w, f.h, 4, 5, 3, rects );
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
	HistoricalLayout( 2048, 1536, 0, 0, 0, 0, 0, rects );
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
		HistoricalLayout( f[0], f[1], f[2], f[3], f[4], f[5], f[6], rects );
		IOS_TouchConsoleRectPixels( f[0], f[1], f[2], f[3], f[4], f[5], &con.x, &con.y, &con.w, &con.h );
		con.x += con.w / 4;
		con.y += con.h / 4;
		con.w /= 2;
		con.h /= 2;
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

BOOST_AUTO_TEST_CASE( combat_force_medium_sabers_and_default_row_clearance )
{
	const float fixtures[][7]={{2868,1320,132,0,132,102,3},{1434,660,66,0,66,51,1.5f},{844,390,0,0,0,0,1},{667,320,0,0,0,0,1},{2048,1536,0,0,0,0,2},{2388,1668,0,0,0,0,2},{1024,768,0,0,0,0,2}};
	const int row[7]={TB_FORCE_SLOT1,TB_FORCE_SLOT2,TB_FORCE_SLOT3,TB_FORCE_SLOT4,TB_FORCEPREV,TB_FORCE,TB_FORCENEXT};
	for(const auto &f:fixtures) {
		touchRect_t rects[TB_COUNT];HistoricalLayout(f[0],f[1],f[2],f[3],f[4],f[5],f[6],rects,NULL);
		BOOST_CHECK_EQUAL(rects[TB_FORCE].w,rects[TB_ATTACK].w);BOOST_CHECK_EQUAL(rects[TB_FORCE].h,rects[TB_ALTATTACK].h);
		for(int button:{TB_STYLE,TB_SABER_SELECT,TB_FORCE_SLOT1,TB_FORCE_SLOT2,TB_FORCE_SLOT3,TB_FORCE_SLOT4}) {
			const auto &d=touchButtonDefs[button];touchRect_t old;
			TouchLayout_AnchorRect(f[0],f[1],f[2],f[3],f[4],f[5],f[6],d.anchor,d.cx,d.cy,d.size,&old.x,&old.y,&old.w,&old.h);
			BOOST_CHECK_EQUAL(rects[button].w,old.w*.75f);
		}
		for(int i=0;i<7;++i) {
			const auto &a=rects[row[i]];
			BOOST_CHECK_GE(a.x,f[2]-.001f);BOOST_CHECK_GE(a.y,f[3]-.001f);BOOST_CHECK_LE(a.x+a.w,f[0]-f[4]+.001f);BOOST_CHECK_LE(a.y+a.h,f[1]-f[5]+.001f);
			for(int j=0;j<TB_COUNT;++j) {
				if(j==row[i]) continue;
				const auto &b=rects[j];
				BOOST_CHECK_MESSAGE(a.x+a.w<=b.x+.001f || b.x+b.w<=a.x+.001f || a.y+a.h<=b.y+.001f || b.y+b.h<=a.y+.001f,"overlap "<<row[i]<<" / "<<j<<" in "<<f[0]<<"x"<<f[1]);
			}
			if(i) BOOST_CHECK_GT(a.x,rects[row[i-1]].x);
		}
		touchLayoutOverrides_t saved={};saved.version=2;saved.mask=(1u<<TB_FORCE)|(1u<<TB_STYLE)|(1u<<TB_SABER_SELECT)|(15u<<TB_FORCE_SLOT1);
		for(int button:{TB_FORCE,TB_STYLE,TB_SABER_SELECT,TB_FORCE_SLOT1,TB_FORCE_SLOT2,TB_FORCE_SLOT3,TB_FORCE_SLOT4}) {saved.cx[button]=.25f;saved.cy[button]=.6f;}
		HistoricalLayout(f[0],f[1],f[2],f[3],f[4],f[5],f[6],rects,&saved);
		for(int button:{TB_FORCE,TB_STYLE,TB_SABER_SELECT,TB_FORCE_SLOT1,TB_FORCE_SLOT2,TB_FORCE_SLOT3,TB_FORCE_SLOT4}) {BOOST_CHECK_SMALL(rects[button].x+rects[button].w/2-f[0]*.25f,.001f);BOOST_CHECK_SMALL(rects[button].y+rects[button].h/2-f[1]*.6f,.001f);}
	}
}

BOOST_AUTO_TEST_CASE( compact_controls_halve_old_clamped_rectangles_only )
{
	const unsigned int small = ( 1u << TB_MENU ) | ( 1u << TB_CHAT ) | ( 1u << TB_PAD ) |
		( 1u << TB_CONSOLE ) | ( 1u << TB_LOCK ) | ( 1u << TB_WEAPPREV ) | ( 1u << TB_WEAPNEXT ) |
		( 1u << TB_FORCENEXT ) | ( 1u << TB_FORCEPREV );
	const float fixtures[][7] = {
		{ 2868, 1320, 132, 0, 132, 102, 3 },
		{ 1434, 660, 66, 0, 66, 51, 1.5f },
		{ 2048, 1536, 0, 0, 0, 0, 2 },
		{ 3000, 2000, 0, 0, 0, 0, 1 },
		{ 600, 400, 20, 10, 40, 30, 3 }
	};
	for ( const auto &f : fixtures ) {
		for ( int edge = 0; edge <= 1; ++edge ) {
			touchLayoutOverrides_t overrides = {};
			if ( edge ) {
				overrides.mask = ((1u<<16)-1u) & ~(1u<<TB_CONSOLE);
				for ( int button = 0; button < TB_COUNT; ++button )
					overrides.cx[button] = overrides.cy[button] = 0;
			}
			touchButtonDef_t oldDefs[TB_COUNT];
			TouchLegacy_Apply( &overrides, oldDefs, f[0], f[1], f[2], f[3], f[4], f[5], f[6] );
			touchRect_t rects[TB_COUNT];
			HistoricalLayout( f[0], f[1], f[2], f[3], f[4], f[5], f[6], rects, &overrides );
			for ( int button = 0; button < TB_COUNT; ++button ) {
				touchRect_t old;
				const auto &d = oldDefs[button];
				if ( button == TB_CONSOLE )
					IOS_TouchConsoleRectPixels( f[0], f[1], f[2], f[3], f[4], f[5], &old.x, &old.y, &old.w, &old.h );
				else
					TouchLayout_AnchorRect( f[0], f[1], f[2], f[3], f[4], f[5], f[6], d.anchor,
						d.cx, d.cy, d.size, &old.x, &old.y, &old.w, &old.h );
				const float scale = small & ( 1u << button ) ? .5f : button==TB_STYLE || button==TB_SABER_SELECT || (button>=TB_FORCE_SLOT1 && button<=TB_FORCE_SLOT4)?.75f:1;
				float expected=old.w*scale;
				if(button==TB_FORCE) {
					touchRect_t combat;TouchLayout_AnchorRect(f[0],f[1],f[2],f[3],f[4],f[5],f[6],d.anchor,d.cx,d.cy,.14f,&combat.x,&combat.y,&combat.w,&combat.h);
					expected=combat.w;
				}
				BOOST_CHECK_EQUAL( rects[button].w, expected );
				BOOST_CHECK_EQUAL( rects[button].h, expected );
				bool obstructed=false;
				if ( button == TB_LOCK ) {
					touchRect_t console;
					IOS_TouchConsoleRectPixels(f[0],f[1],f[2],f[3],f[4],f[5],&console.x,&console.y,&console.w,&console.h);
					console.x+=console.w/4; console.y+=console.h/4; console.w/=2; console.h/=2;
					const touchRect_t candidate={old.x+(old.w-rects[button].w)/2,old.y+(old.h-rects[button].h)/2,rects[button].w,rects[button].h};
					const auto bar=TouchSettings_BarRect(f[0],f[1],f[2],f[3],f[4],f[5],f[6]);
					for ( const auto &blocked : { console,bar } ) {
						obstructed |= candidate.x+candidate.w>blocked.x && blocked.x+blocked.w>candidate.x &&
							candidate.y+candidate.h>blocked.y && blocked.y+blocked.h>candidate.y;
						BOOST_CHECK(rects[button].x+rects[button].w<=blocked.x || blocked.x+blocked.w<=rects[button].x ||
							rects[button].y+rects[button].h<=blocked.y || blocked.y+blocked.h<=rects[button].y);
					}
				}
				const bool forceRow=button==TB_FORCE || button==TB_FORCENEXT || button==TB_FORCEPREV || button>=TB_FORCE_SLOT1;
				if ( !obstructed && (!forceRow || (edge && (overrides.mask&(1u<<button)))) ) {
					BOOST_CHECK_SMALL( rects[button].x + rects[button].w / 2 - fmaxf(rects[button].w/2,fminf(old.x+old.w/2,f[0]-rects[button].w/2)), .001f );
					BOOST_CHECK_SMALL( rects[button].y + rects[button].h / 2 - fmaxf(rects[button].h/2,fminf(old.y+old.h/2,f[1]-rects[button].h/2)), .001f );
				}
				for ( int corner = 0; corner < 4; ++corner )
					BOOST_CHECK_EQUAL( TouchLayout_Hit( rects, rects[button].x + ( corner & 1 ? rects[button].w : 0 ),
						rects[button].y + ( corner & 2 ? rects[button].h : 0 ), 1u << button ), button );
			}
		}
	}
	// A floor-sized saved edge control keeps its old centre, not its raw anchor.
	touchLayoutOverrides_t edge = {};
	edge.mask = 1u << TB_MENU;
	touchRect_t rects[TB_COUNT];
	HistoricalLayout( 600, 400, 20, 10, 40, 30, 3, rects, &edge );
	BOOST_CHECK_EQUAL( rects[TB_MENU].x, 53 );
	BOOST_CHECK_EQUAL( rects[TB_MENU].y, 43 );
	BOOST_CHECK_EQUAL( rects[TB_MENU].w, 66 );
	BOOST_CHECK_SMALL( rects[TB_CONSOLE].w - 29.7f, .0001f );
}

BOOST_AUTO_TEST_CASE( explicit_key_map_and_paired_pad_chat_trace )
{
	const int expected[TB_COUNT] = { A_AUX16, A_AUX17, A_AUX18, A_AUX19, A_AUX20, A_AUX21, A_AUX22,
		A_AUX23, A_AUX24, A_AUX25, A_ESCAPE, 0, A_AUX26, 'y', A_TAB, 0, A_AUX27, 0, 0, 0, 0, A_AUX28 };
	const int expectedMP[TB_COUNT] = { mp_touch_keys::A_AUX16, mp_touch_keys::A_AUX17, mp_touch_keys::A_AUX18,
		mp_touch_keys::A_AUX19, mp_touch_keys::A_AUX20, mp_touch_keys::A_AUX21, mp_touch_keys::A_AUX22,
		mp_touch_keys::A_AUX23, mp_touch_keys::A_AUX24, mp_touch_keys::A_AUX25, mp_touch_keys::A_ESCAPE,
		0, mp_touch_keys::A_AUX26, 'y', mp_touch_keys::A_TAB, 0, mp_touch_keys::A_AUX27, 0, 0, 0, 0, mp_touch_keys::A_AUX28 };
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
		"forcenext", "saberAttackCycle", "weapprev", "weapnext", "weapon 1", "forceprev", "+attack;+altattack" };
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
	BOOST_CHECK_EQUAL( restore(), 13 );
	for ( int button = 0; button < TB_COUNT; ++button ) {
		const int key = touchButtonDefs[button].key;
		if ( key >= 16 && key <= 28 ) {
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
	BOOST_CHECK_EQUAL( restore(), 13 );
	BOOST_CHECK( TouchLayout_DefaultBind( -1, NULL ) == NULL );
	BOOST_CHECK( TouchLayout_DefaultBind( TB_COUNT, NULL ) == NULL );
}

BOOST_AUTO_TEST_CASE( fps_clears_lock_and_console_across_fixtures_and_stack_offsets )
{
	const float fixtures[][7]={
		{2868,1320,132,0,132,102,3},{1434,660,66,0,66,51,1.5f},
		{2048,1536,0,0,0,0,2},{1024,768,0,0,0,0,1},
		{2868,1320,144,30,44,90,3},{1434,660,72,15,22,45,1.5f},
		{2868,1320,44,30,144,90,3},{1434,660,22,15,72,45,1.5f}
	};
	for (const auto &f : fixtures) for (int corrected : {0,1}) {
		const auto layout=AspectLayout_Build((int)f[0],(int)f[1],corrected,1.5f,(int)f[2],(int)f[3],(int)f[4],(int)f[5]);
		touchRect_t buttons[TB_COUNT]; HistoricalLayout(f[0],f[1],f[2],f[3],f[4],f[5],f[6],buttons);
		const auto &con=buttons[TB_CONSOLE]; const auto &lock=buttons[TB_LOCK];
		const float scale=AspectLayout_SharpTextScale(&layout,18,22);
		const double right=AspectLayout_ToVirtual(&layout,fminf(f[0]-f[4],con.x-6),0).x;
		const double top=layout.fullRect.y+AspectLayout_SizeToVirtual(&layout,0,f[3]).y;
		const int width=(int)ceilf(51*scale);
		// Timers follow FPS; also stress two debug-row advances.
		for(int engine=0;engine<2;++engine) for(int rows=0;rows<=2;++rows) for(int team=0;team<=(engine?8:0);++team) {
			const int offset=rows*(16+(engine?4:10))+team*8;
			const auto origin=AspectLayout_ToPixels(&layout,(int)(right-width),(int)(top+offset+2));
			const auto size=AspectLayout_SizeToPixels(&layout,width,18*scale);
			BOOST_CHECK_LE(origin.x+size.x,con.x-6+1e-3);
			BOOST_CHECK_LE(origin.x+size.x,f[0]-f[4]);
			BOOST_CHECK(origin.x+size.x<=con.x || origin.x>=con.x+con.w || origin.y+size.y<=con.y || origin.y>=con.y+con.h);
			BOOST_CHECK(origin.x+size.x<=lock.x || origin.x>=lock.x+lock.w || origin.y+size.y<=lock.y || origin.y>=lock.y+lock.h);
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
	HistoricalLayout( 640, 600, 0, 0, 0, 0, 1, rects );
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
		HistoricalLayout( layout.W, layout.H, layout.safeL, layout.safeT, layout.safeR, layout.safeB, 2, rects );
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
	HistoricalLayout( before.W, before.H, before.safeL, before.safeT, before.safeR, before.safeB, 3, oldRects );
	HistoricalLayout( after.W, after.H, after.safeL, after.safeT, after.safeR, after.safeB, 3, newRects );
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


BOOST_AUTO_TEST_SUITE(ios)
BOOST_AUTO_TEST_SUITE(touch_tablet)

BOOST_AUTO_TEST_CASE(reference_fixed_centers_and_current_console_size) {
	touchLayoutOverrides_t source;TouchEdit_Parse(touchTabletLayoutFixture,&source);
	const int fixed[]={TB_ATTACK,TB_ALTATTACK,TB_JUMP,TB_CROUCH,TB_STYLE,TB_USE,TB_LOCK,TB_CONSOLE};
	for(float p:{1.f,1.5f,2.f}) for(float bottom:{0.f,20.f}) {
		touchRect_t r[TB_COUNT];TouchLayout_Build(1194*p,834*p,0,0,0,bottom*p,p,r,NULL,TOUCH_LAYOUT_TABLET);
		for(int i:fixed) {
			BOOST_CHECK_SMALL((r[i].x+r[i].w/2)/p-source.cx[i]*1194,.001f);
			BOOST_CHECK_SMALL((r[i].y+r[i].h/2)/p-source.cy[i]*834,.001f);
		}
		for(int i:{TB_MENU,TB_PAD,TB_CHAT,TB_LOCK}) {
			BOOST_CHECK_EQUAL(r[TB_CONSOLE].w,r[i].w);BOOST_CHECK_EQUAL(r[TB_CONSOLE].h,r[i].h);
		}
		const int groups[][6]={{TB_MENU,TB_CHAT,TB_PAD,-1,-1,-1},{TB_WEAPPREV,TB_SABER_SELECT,TB_WEAPNEXT,TB_FORCEPREV,TB_FORCE,TB_FORCENEXT},{TB_FORCE_SLOT1,TB_FORCE_SLOT2,TB_FORCE_SLOT3,TB_FORCE_SLOT4,-1,-1}};
		for(const auto &group:groups) {
			float dx=0,dy=0;
			for(int i:group) if(i>=0) {dx+=(r[i].x+r[i].w/2)/p-source.cx[i]*1194;dy+=(r[i].y+r[i].h/2)/p-source.cy[i]*834;}
			BOOST_CHECK_SMALL(dx,.002f);BOOST_CHECK_SMALL(dy,.002f);
		}
		const float sourceStep=((source.cx[TB_CHAT]-source.cx[TB_MENU])*1194+(source.cy[TB_PAD]-source.cy[TB_MENU])*834)/2;
		BOOST_CHECK_SMALL((r[TB_CHAT].x-r[TB_MENU].x)/p-sourceStep,.001f);
	}
}

static float CenterX(const touchRect_t &r) {return r.x+r.w/2;}
static float CenterY(const touchRect_t &r) {return r.y+r.h/2;}
static bool Disjoint(const touchRect_t &a,const touchRect_t &b) {
	return a.x+a.w<=b.x+.001f || b.x+b.w<=a.x+.001f || a.y+a.h<=b.y+.001f || b.y+b.h<=a.y+.001f;
}

BOOST_AUTO_TEST_CASE(compiled_tablet_matrix_matches_groups_and_documented_window_limits) {
	const int sizes[][2]={{1194,834},{1210,834},{1024,768},{1366,1024},{1180,820},{1133,744},{834,1194},{768,1024},{744,1133},{694,768},{507,768},{320,768}};
	const float insets[][4]={{0,0,0,20},{0,24,0,20},{0,0,0,0}};
	int supported=0,stress=0;
	for(const auto &size:sizes) for(float render:{1.f,.75f,.5f}) for(const auto &safe:insets) {
		const float p=2*render,W=size[0]*p,H=size[1]*p;
		touchRect_t r[TB_COUNT],again[TB_COUNT];TouchLayout_Build(W,H,safe[0]*p,safe[1]*p,safe[2]*p,safe[3]*p,p,r,NULL,TOUCH_LAYOUT_TABLET);
		TouchLayout_Build(W,H,safe[0]*p,safe[1]*p,safe[2]*p,safe[3]*p,p,again,NULL,TOUCH_LAYOUT_TABLET);
		BOOST_CHECK_EQUAL(memcmp(r,again,sizeof(r)),0);
		const bool narrow=size[0]<=507;if(narrow) ++stress;else ++supported;
		for(int i=0;i<TB_COUNT;++i) {
			BOOST_CHECK(IOS_TouchFinite(r[i].x) && IOS_TouchFinite(r[i].y));
			BOOST_CHECK_GE(r[i].x,-.001f);BOOST_CHECK_GE(r[i].y,-.001f);
			BOOST_CHECK_LE(r[i].x+r[i].w,W+.001f);BOOST_CHECK_LE(r[i].y+r[i].h,H+.001f);
			BOOST_CHECK_EQUAL(TouchLayout_Hit(r,CenterX(r[i]),CenterY(r[i]),1u<<i),i);
			touchRect_t editor;TouchEdit_PositionRect(NULL,i,W,H,safe[0]*p,safe[1]*p,safe[2]*p,safe[3]*p,p,&editor,TOUCH_LAYOUT_TABLET);
			BOOST_CHECK_EQUAL(memcmp(&editor,&r[i],sizeof(editor)),0);
			for(int j=0;j<i;++j) {
				const bool known=size[0]==507?(i==TB_USE && j==TB_ALTATTACK):size[0]==320 &&
					((i==TB_FORCE_SLOT2 && j==TB_WEAPPREV) || (i==TB_FORCE_SLOT3 && j==TB_FORCEPREV) || (i==TB_FORCE_SLOT4 && j==TB_FORCE));
				BOOST_CHECK_MESSAGE(Disjoint(r[i],r[j])!=known,"pair "<<i<<"/"<<j<<" in "<<size[0]<<"x"<<size[1]<<" render "<<render);
			}
		}
		BOOST_CHECK_SMALL(CenterX(r[TB_MENU])-CenterX(r[TB_PAD]),.001f);
		BOOST_CHECK_SMALL(CenterY(r[TB_MENU])-CenterY(r[TB_CHAT]),.001f);
		BOOST_CHECK_SMALL((r[TB_CHAT].x-r[TB_MENU].x-r[TB_MENU].w)-(r[TB_PAD].y-r[TB_MENU].y-r[TB_MENU].h),.001f);
		for(int i=0;i<3;++i) {const int top[]={TB_WEAPPREV,TB_SABER_SELECT,TB_WEAPNEXT},bottom[]={TB_FORCEPREV,TB_FORCE,TB_FORCENEXT};BOOST_CHECK_SMALL(CenterX(r[top[i]])-CenterX(r[bottom[i]]),.001f);}
		BOOST_CHECK_SMALL(CenterY(r[TB_WEAPPREV])-CenterY(r[TB_WEAPNEXT]),.001f);
		BOOST_CHECK_SMALL(CenterY(r[TB_FORCEPREV])-CenterY(r[TB_FORCENEXT]),.001f);
		BOOST_CHECK_SMALL(CenterY(r[TB_FORCE_SLOT1])-CenterY(r[TB_FORCE_SLOT2]),.001f);
		BOOST_CHECK_SMALL(CenterY(r[TB_FORCE_SLOT3])-CenterY(r[TB_FORCE_SLOT4]),.001f);
		BOOST_CHECK_SMALL((CenterX(r[TB_FORCE_SLOT2])-CenterX(r[TB_FORCE_SLOT1]))-(CenterX(r[TB_FORCE_SLOT4])-CenterX(r[TB_FORCE_SLOT3])),.001f);
		BOOST_CHECK_SMALL((CenterX(r[TB_FORCE_SLOT3])-CenterX(r[TB_FORCE_SLOT1]))-(CenterX(r[TB_FORCE_SLOT4])-CenterX(r[TB_FORCE_SLOT2])),.001f);
		BOOST_CHECK_SMALL((CenterY(r[TB_FORCE_SLOT3])-CenterY(r[TB_FORCE_SLOT1]))-(CenterY(r[TB_FORCE_SLOT4])-CenterY(r[TB_FORCE_SLOT2])),.001f);
	}
	BOOST_CHECK_EQUAL(supported,90);BOOST_CHECK_EQUAL(stress,18);
}

BOOST_AUTO_TEST_CASE(saved_tablet_centers_and_partial_console_survive_current_profiles) {
	touchLayoutOverrides_t saved;TouchEdit_Parse(touchTabletLayoutFixture,&saved);const auto original=saved;
	for(auto profile:{TOUCH_LAYOUT_TABLET,TOUCH_LAYOUT_PHONE}) for(const auto &size:{touchRect_t{1194,834,0,0},touchRect_t{834,1194,0,0}}) for(float p:{1.f,2.f}) {
		touchRect_t r[TB_COUNT];TouchLayout_Build(size.x*p,size.y*p,0,24*p,0,20*p,p,r,&saved,profile);
		for(int i=0;i<TB_COUNT;++i) {
			BOOST_CHECK_SMALL(CenterX(r[i])-saved.cx[i]*size.x*p,.002f);
			BOOST_CHECK_SMALL(CenterY(r[i])-saved.cy[i]*size.y*p,.002f);
		}
	}
	BOOST_CHECK_EQUAL(memcmp(&saved,&original,sizeof(saved)),0);
	touchLayoutOverrides_t partial={};partial.version=2;partial.mask=1u<<TB_CONSOLE;partial.cx[TB_CONSOLE]=.5f;partial.cy[TB_CONSOLE]=.5f;
	const auto untouched=partial;
	for(auto profile:{TOUCH_LAYOUT_TABLET,TOUCH_LAYOUT_PHONE}) {
		touchRect_t r[TB_COUNT];TouchLayout_Build(2388,1668,0,0,0,40,2,r,&partial,profile);
		BOOST_CHECK_SMALL(CenterX(r[TB_CONSOLE])-1194,.001f);BOOST_CHECK_SMALL(CenterY(r[TB_CONSOLE])-834,.001f);
		BOOST_CHECK_EQUAL(r[TB_CONSOLE].w,r[TB_MENU].w);
	}
	BOOST_CHECK_EQUAL(memcmp(&partial,&untouched,sizeof(partial)),0);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
