#include "ios/touch/touch_atlas.h"
#include "sys/ios/ios_touch_state.h"
#include "sys/ios/ios_touch_visibility.h"
#include <boost/test/unit_test.hpp>
#include <cstring>
#include <limits>

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

static void HeldSticks( touchGestureState_t *state )
{
	TouchGesture_Reset( state );
	touchGestureInput_t in = {};
	in.visibleButtons = IOS_TOUCH_ALL_BUTTONS;
	in.moveEnabled = in.lookEnabled = 1;
	in.radius = 100;
	in.slop = 10;
	in.event = TOUCH_DOWN;
	in.finger = 1;
	in.target = TOUCH_TARGET_MOVE;
	in.x = 400;
	in.y = 800;
	TouchGesture_Step( state, in );
	in.event = TOUCH_MOTION;
	in.x = 450;
	TouchGesture_Step( state, in );
	in.event = TOUCH_DOWN;
	in.finger = 2;
	in.target = TOUCH_TARGET_FIRE;
	in.button = TB_ATTACK;
	in.x = 2000;
	TouchGesture_Step( state, in );
	in.event = TOUCH_MOTION;
	in.x = 2300;
	in.y = 400;
	TouchGesture_Step( state, in );
}

static touchDrawInput_t DrawInput( const aspectLayout_t *layout, const touchRect_t *rects, const touchGestureState_t *gestures )
{
	touchDrawInput_t input = {};
	input.layout = layout;
	input.rects = rects;
	input.gestures = gestures;
	input.visibleButtons = IOS_TOUCH_ALL_BUTTONS;
	input.pressedButtons = 1u << TB_CONSOLE;
	input.sticks = 1;
	input.styleColor = TOUCH_STYLE_BLUE;
	return input;
}

BOOST_AUTO_TEST_CASE( full_twenty_records_and_raw_knobs_round_trip )
{
	touchRect_t rects[TB_COUNT];
	TouchLayout_Build( 2868, 1320, 132, 0, 132, 102, 3, rects );
	touchGestureState_t gestures;
	HeldSticks( &gestures );
	for ( int corrected : { 0, 1 } ) {
		const auto layout = AspectLayout_Build( 2868, 1320, corrected, 1.5f, 132, 0, 132, 102 );
		auto input = DrawInput( &layout, rects, &gestures );
		touchDrawItem_t items[IOS_TOUCH_MAX_DRAW_ITEMS];
		BOOST_REQUIRE_EQUAL( TouchDraw_Build( &input, items, IOS_TOUCH_MAX_DRAW_ITEMS ), TB_COUNT+4 );
		for ( int i = 0; i < TB_COUNT; ++i ) {
			BOOST_CHECK_EQUAL( items[i].kind, TOUCH_DRAW_BUTTON );
			BOOST_CHECK_EQUAL( items[i].glyph, touchButtonDefs[i].glyph );
			const auto virtualRect = TouchLayout_RectToVirtual( &layout, rects[i] );
			BOOST_CHECK_EQUAL( items[i].x, virtualRect.x );
			BOOST_CHECK_EQUAL( items[i].y, virtualRect.y );
			BOOST_CHECK_EQUAL( items[i].w, virtualRect.w );
			BOOST_CHECK_EQUAL( items[i].h, virtualRect.h );
		}
		BOOST_CHECK_EQUAL( items[TB_ATTACK].pressed, 1 );
		BOOST_CHECK_EQUAL( items[TB_ALTATTACK].pressed, 0 );
		BOOST_CHECK_EQUAL( items[TB_CONSOLE].pressed, 1 );
		BOOST_CHECK_SMALL( items[TB_STYLE].rgb[0] - .567f, 1e-6f );
		BOOST_CHECK_EQUAL( items[TB_ATTACK].rgb[0], 1 );
		for ( int i = TB_COUNT; i < TB_COUNT+4; ++i ) {
			BOOST_CHECK_EQUAL( items[i].glyph, TG_PLATE );
			BOOST_CHECK_EQUAL( items[i].kind, (i-TB_COUNT) % 2 ? TOUCH_DRAW_STICK_KNOB : TOUCH_DRAW_STICK_BASE );
			const auto centre = AspectLayout_ToPixels( &layout, items[i].x + items[i].w / 2, items[i].y + items[i].h / 2 );
			const auto size = AspectLayout_SizeToPixels( &layout, items[i].w, items[i].h );
			const float expectedX[] = { 400, 450, 2000, 2060 }, expectedY[] = { 800, 800, 800, 720 };
			BOOST_CHECK_SMALL( centre.x - expectedX[i - TB_COUNT], 1e-3 );
			BOOST_CHECK_SMALL( centre.y - expectedY[i - TB_COUNT], 1e-3 );
			BOOST_CHECK_SMALL( size.x - ( (i-TB_COUNT) % 2 ? 60 : 200 ), 1e-3 );
			BOOST_CHECK_SMALL( size.y - ( (i-TB_COUNT) % 2 ? 60 : 200 ), 1e-3 );
		}
		BOOST_CHECK_EQUAL( gestures.fingers[gestures.lookOwner].role, TOUCH_FIRE_AIM );
	}
}

BOOST_AUTO_TEST_CASE( capacity_clips_whole_records_and_hidden_controls )
{
	const auto layout = AspectLayout_Build( 2868, 1320, 1, 1.5f, 132, 0, 132, 102 );
	touchRect_t rects[TB_COUNT];
	TouchLayout_Build( 2868, 1320, 132, 0, 132, 102, 3, rects );
	touchGestureState_t gestures;
	HeldSticks( &gestures );
	auto input = DrawInput( &layout, rects, &gestures );
	for ( int capacity = -1; capacity <= 33; ++capacity ) {
		touchDrawItem_t items[34], sentinel;
		std::memset( items, 0x5a, sizeof( items ) );
		std::memset( &sentinel, 0x5a, sizeof( sentinel ) );
		const int expected = capacity < 0 ? 0 : capacity < TB_COUNT+4 ? capacity : TB_COUNT+4;
		BOOST_CHECK_EQUAL( TouchDraw_Build( &input, items, capacity ), expected );
		BOOST_CHECK_EQUAL( std::memcmp( &items[expected], &sentinel, sizeof( sentinel ) ), 0 );
		if ( expected > 0 ) BOOST_CHECK_EQUAL( items[0].kind, TOUCH_DRAW_BUTTON );
	}
	BOOST_CHECK_EQUAL( TouchDraw_Build( &input, NULL, 32 ), 0 );
	touchDrawItem_t items[32];
	input.visibleButtons = 1u << TB_CONSOLE;
	input.sticks = 0;
	BOOST_REQUIRE_EQUAL( TouchDraw_Build( &input, items, 32 ), 1 );
	BOOST_CHECK_EQUAL( items[0].glyph, TG_CONSOLE );
	input.visibleButtons = 0;
	BOOST_CHECK_EQUAL( TouchDraw_Build( &input, items, 32 ), 0 );
	BOOST_CHECK_EQUAL( gestures.fingers[gestures.lookOwner].role, TOUCH_FIRE_AIM );
}

BOOST_AUTO_TEST_CASE( held_and_promoted_fire_remain_pressed_at_zero_opacity )
{
	const auto layout = AspectLayout_Build( 2868, 1320, 1, 1.5f, 132, 0, 132, 102 );
	touchRect_t rects[TB_COUNT];
	TouchLayout_Build( 2868, 1320, 132, 0, 132, 102, 3, rects );
	touchGestureState_t gestures;
	TouchGesture_Reset( &gestures );
	touchGestureInput_t event = {};
	event.event = TOUCH_DOWN;
	event.target = TOUCH_TARGET_FIRE;
	event.button = TB_ATTACK;
	event.radius = 100;
	event.slop = 10;
	event.visibleButtons = IOS_TOUCH_ALL_BUTTONS;
	event.moveEnabled = event.lookEnabled = 1;
	TouchGesture_Step( &gestures, event );
	auto input = DrawInput( &layout, rects, &gestures );
	touchDrawItem_t items[IOS_TOUCH_MAX_DRAW_ITEMS];
	BOOST_REQUIRE_EQUAL( TouchDraw_Build( &input, items, 32 ), TB_COUNT );
	BOOST_CHECK_EQUAL( items[TB_ATTACK].pressed, 1 );
	event.event = TOUCH_MOTION;
	event.x = 100;
	TouchGesture_Step( &gestures, event );
	BOOST_REQUIRE_EQUAL( TouchDraw_Build( &input, items, 32 ), TB_COUNT+2 );
	BOOST_CHECK_EQUAL( items[TB_ATTACK].pressed, 1 );
	float colour[4];
	TouchDraw_Color( &items[TB_ATTACK], 1, 0, colour );
	BOOST_CHECK_EQUAL( colour[3], 0 );
	event.event = TOUCH_REFRESH;
	const auto actions = TouchGesture_Step( &gestures, event );
	BOOST_CHECK_EQUAL( actions.lookX, 127 );
	BOOST_CHECK_EQUAL( actions.keyCount, 0 );
	BOOST_CHECK_EQUAL( gestures.fingers[gestures.lookOwner].role, TOUCH_FIRE_AIM );
}

BOOST_AUTO_TEST_CASE( opacity_and_style_only_affect_draw_colour )
{
	BOOST_CHECK_EQUAL( TouchDraw_Opacity( -1 ), 0 );
	BOOST_CHECK_EQUAL( TouchDraw_Opacity( 2 ), 1 );
	BOOST_CHECK_EQUAL( TouchDraw_Opacity( .5f ), .5f );
	BOOST_CHECK_EQUAL( TouchDraw_Opacity( std::numeric_limits<float>::quiet_NaN() ), 0 );
	touchDrawItem_t item = {};
	item.kind = TOUCH_DRAW_BUTTON;
	const float expected[][3] = { { .567f, .685f, 1 }, { 1, 1, 0 }, { .835f, .015f, .015f }, { 1, 1, 1 } };
	for ( int style = 0; style < 4; ++style ) {
		TouchDraw_StyleRGB( style, item.rgb );
		for ( int channel = 0; channel < 3; ++channel ) BOOST_CHECK_EQUAL( item.rgb[channel], expected[style][channel] );
		for ( int pressed : { 0, 1 } ) {
			item.pressed = pressed;
			for ( float opacity : { 0.f, .5f, 1.f } ) {
				float colour[4];
				TouchDraw_Color( &item, 0, opacity, colour );
				BOOST_CHECK_EQUAL( colour[0], 1 );
				BOOST_CHECK_EQUAL( colour[1], 1 );
				BOOST_CHECK_EQUAL( colour[2], 1 );
				BOOST_CHECK_EQUAL( colour[3], ( pressed ? .40f : .20f ) * opacity );
				TouchDraw_Color( &item, 1, opacity, colour );
				BOOST_CHECK_EQUAL( colour[3], ( pressed ? 1.f : .85f ) * opacity );
				for ( int channel = 0; channel < 3; ++channel ) BOOST_CHECK_EQUAL( colour[channel], expected[style][channel] );
			}
		}
	}
	float colour[4];
	item.kind = TOUCH_DRAW_STICK_BASE;
	TouchDraw_Color( &item, 0, 1, colour );
	BOOST_CHECK_EQUAL( colour[3], .20f );
	item.kind = TOUCH_DRAW_STICK_KNOB;
	TouchDraw_Color( &item, 0, 1, colour );
	BOOST_CHECK_EQUAL( colour[3], .40f );
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
