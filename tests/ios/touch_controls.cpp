#include "sys/ios/ios_touch_math.h"
#include "sys/ios/ios_touch_state.h"
#include "sys/ios/ios_touch_visibility.h"
#include "sys/ios/ios_touch_layout.h"

#include <boost/test/unit_test.hpp>
#include <limits>

BOOST_AUTO_TEST_SUITE( ios )
BOOST_AUTO_TEST_SUITE( touch_controls )

BOOST_AUTO_TEST_CASE( radial_curve_and_quantization )
{
	float x, y;
	TouchStick_Curve( 0, 0, 100, &x, &y );
	BOOST_CHECK_EQUAL( x, 0 );
	BOOST_CHECK_EQUAL( y, 0 );
	TouchStick_Curve( 8, 0, 100, &x, &y );
	BOOST_CHECK_EQUAL( x, 0 );
	TouchStick_Curve( 54, 0, 100, &x, &y );
	BOOST_CHECK_SMALL( x - powf( .5f, 1.6f ), 1e-6f );
	BOOST_CHECK_EQUAL( TouchStick_Quantize( x ), 42 );
	TouchStick_Curve( 60, -80, 100, &x, &y );
	BOOST_CHECK_SMALL( x - .6f, 1e-6f );
	BOOST_CHECK_SMALL( y + .8f, 1e-6f );
	TouchStick_Curve( 200, 200, 100, &x, &y );
	BOOST_CHECK_SMALL( hypotf( x, y ) - 1, 1e-6f );
	BOOST_CHECK_EQUAL( TouchStick_Quantize( x ), 90 );
	BOOST_CHECK_EQUAL( TouchStick_Quantize( -1 ), -127 );
	BOOST_CHECK_EQUAL( TouchStick_Quantize( 2 ), 127 );
	BOOST_CHECK_EQUAL( TouchStick_Quantize( .5f ), 64 );
	BOOST_CHECK_EQUAL( TouchStick_Quantize( -.5f ), -64 );
}

BOOST_AUTO_TEST_CASE( invalid_curve_and_clamped_knob )
{
	const float bad[] = { 0, -1, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() };
	float x, y;
	for ( float radius : bad ) {
		TouchStick_Curve( 50, 20, radius, &x, &y );
		BOOST_CHECK_EQUAL( x, 0 );
		BOOST_CHECK_EQUAL( y, 0 );
	}
	for ( float value : { bad[2], bad[3] } ) {
		TouchStick_Curve( value, 20, 100, &x, &y );
		BOOST_CHECK_EQUAL( x, 0 );
		BOOST_CHECK_EQUAL( y, 0 );
		TouchStick_Curve( 20, value, 100, &x, &y );
		BOOST_CHECK_EQUAL( x, 0 );
		BOOST_CHECK_EQUAL( y, 0 );
		BOOST_CHECK_EQUAL( TouchStick_Quantize( value ), 0 );
	}
	TouchStick_Clamp( 300, -400, 100, &x, &y );
	BOOST_CHECK_SMALL( x - .6f, 1e-6f );
	BOOST_CHECK_SMALL( y + .8f, 1e-6f );
	TouchStick_Curve( std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1, &x, &y );
	BOOST_CHECK_SMALL( hypotf( x, y ) - 1, 1e-6f );
}

static touchGestureInput_t Input( int event, long long finger = 1, int target = TOUCH_TARGET_FIRE, int button = 0, float x = 0, float y = 0 )
{
	touchGestureInput_t in = {};
	in.event = event;
	in.finger = finger;
	in.target = target;
	in.button = button;
	in.x = x;
	in.y = y;
	in.aimCurve=2;
	in.radius = 100;
	in.slop = 10;
	in.visibleButtons = ~0u;
	in.moveEnabled = in.lookEnabled = 1;
	return in;
}

BOOST_AUTO_TEST_CASE( immediate_same_frame_tap_and_capture )
{
	touchGestureState_t state;
	TouchGesture_Reset( &state );
	auto out = TouchGesture_Step( &state, Input( TOUCH_DOWN ) );
	BOOST_REQUIRE_EQUAL( out.keyCount, 1 );
	BOOST_CHECK_EQUAL( out.keys[0].button, 0 );
	BOOST_CHECK_EQUAL( out.keys[0].down, 1 );
	BOOST_CHECK_EQUAL( TouchGesture_Step( &state, Input( TOUCH_DOWN ) ).keyCount, 0 );
	BOOST_CHECK_EQUAL( TouchGesture_Step( &state, Input( TOUCH_DOWN, 2 ) ).keyCount, 0 );
	BOOST_CHECK_EQUAL( TouchGesture_Step( &state, Input( TOUCH_UP, 2 ) ).keyCount, 0 );
	out = TouchGesture_Step( &state, Input( TOUCH_UP ) );
	BOOST_REQUIRE_EQUAL( out.keyCount, 1 );
	BOOST_CHECK_EQUAL( out.keys[0].down, 0 );
	BOOST_CHECK_EQUAL( TouchGesture_Step( &state, Input( TOUCH_UP ) ).keyCount, 0 );

	TouchGesture_Step( &state, Input( TOUCH_DOWN, 1, TOUCH_TARGET_BUTTON, 4 ) );
	out = TouchGesture_Step( &state, Input( TOUCH_MOTION, 1, TOUCH_TARGET_FIRE, 0, 500, 500 ) );
	BOOST_CHECK_EQUAL( out.keyCount, 0 );
	BOOST_CHECK_EQUAL( out.lookX, 0 );
	out = TouchGesture_Step( &state, Input( TOUCH_UP ) );
	BOOST_REQUIRE_EQUAL( out.keyCount, 1 );
	BOOST_CHECK_EQUAL( out.keys[0].button, 4 );
}

BOOST_AUTO_TEST_CASE( fire_promotes_at_strict_slop_with_fixed_origin )
{
	touchGestureState_t state;
	TouchGesture_Reset( &state );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 1, TOUCH_TARGET_FIRE, 0, 20, 30 ) );
	auto out = TouchGesture_Step( &state, Input( TOUCH_MOTION, 1, TOUCH_TARGET_FIRE, 0, 30, 30 ) );
	BOOST_CHECK_EQUAL( state.lookOwner, -1 );
	out = TouchGesture_Step( &state, Input( TOUCH_MOTION, 1, TOUCH_TARGET_FIRE, 0, 31, 30 ) );
	BOOST_CHECK_NE( state.lookOwner, -1 );
	BOOST_CHECK_EQUAL( out.keyCount, 0 );
	out = TouchGesture_Step( &state, Input( TOUCH_MOTION, 1, TOUCH_TARGET_FIRE, 0, 500, 30 ) );
	BOOST_CHECK_EQUAL( out.lookX, 127 );
	out = TouchGesture_Step( &state, Input( TOUCH_MOTION, 1, TOUCH_TARGET_FIRE, 0, 20, 30 ) );
	BOOST_CHECK_EQUAL( out.lookX, 0 );
	out = TouchGesture_Step( &state, Input( TOUCH_CANCEL ) );
	BOOST_REQUIRE_EQUAL( out.keyCount, 1 );
	BOOST_CHECK_EQUAL( out.keys[0].down, 0 );
	BOOST_CHECK_EQUAL( state.lookOwner, -1 );
}

BOOST_AUTO_TEST_CASE( busy_first_crossing_never_retries )
{
	touchGestureState_t state;
	TouchGesture_Reset( &state );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 1, TOUCH_TARGET_AIM ) );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 2 ) );
	TouchGesture_Step( &state, Input( TOUCH_MOTION, 2, TOUCH_TARGET_FIRE, 0, 11 ) );
	TouchGesture_Step( &state, Input( TOUCH_UP, 1 ) );
	for ( float x : { 200.f, 0.f, 11.f, 300.f } ) {
		auto out = TouchGesture_Step( &state, Input( TOUCH_MOTION, 2, TOUCH_TARGET_FIRE, 0, x ) );
		BOOST_CHECK_EQUAL( state.lookOwner, -1 );
		BOOST_CHECK_EQUAL( out.lookX, 0 );
		BOOST_CHECK_EQUAL( out.keyCount, 0 );
	}
	BOOST_CHECK_EQUAL( TouchGesture_Step( &state, Input( TOUCH_UP, 2 ) ).keyCount, 1 );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 2 ) );
	BOOST_CHECK_EQUAL( TouchGesture_Step( &state, Input( TOUCH_MOTION, 2, TOUCH_TARGET_FIRE, 0, 100 ) ).lookX, 127 );
}

BOOST_AUTO_TEST_CASE( first_owner_two_fires_and_ignored_aim )
{
	touchGestureState_t state;
	TouchGesture_Reset( &state );
	TouchGesture_Step( &state, Input( TOUCH_DOWN ) );
	TouchGesture_Step( &state, Input( TOUCH_MOTION, 1, TOUCH_TARGET_FIRE, 0, 100 ) );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 2, TOUCH_TARGET_AIM ) );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 3, TOUCH_TARGET_FIRE, 1 ) );
	auto out = TouchGesture_Step( &state, Input( TOUCH_MOTION, 3, TOUCH_TARGET_FIRE, 1, -100 ) );
	BOOST_CHECK_EQUAL( out.lookX, 127 );
	out = TouchGesture_Step( &state, Input( TOUCH_UP, 3 ) );
	BOOST_REQUIRE_EQUAL( out.keyCount, 1 );
	BOOST_CHECK_EQUAL( out.keys[0].button, 1 );
	BOOST_CHECK_EQUAL( out.lookX, 127 );
	TouchGesture_Step( &state, Input( TOUCH_UP, 1 ) );
	out = TouchGesture_Step( &state, Input( TOUCH_MOTION, 2, TOUCH_TARGET_AIM, 0, 100 ) );
	BOOST_CHECK_EQUAL( out.lookX, 0 );
	BOOST_CHECK_EQUAL( TouchGesture_Step( &state, Input( TOUCH_DOWN, 2, TOUCH_TARGET_AIM ) ).lookX, 0 );
	TouchGesture_Step( &state, Input( TOUCH_UP, 2 ) );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 2, TOUCH_TARGET_AIM ) );
	BOOST_CHECK_EQUAL( TouchGesture_Step( &state, Input( TOUCH_MOTION, 2, TOUCH_TARGET_AIM, 0, 100 ) ).lookX, 127 );
}

BOOST_AUTO_TEST_CASE( capability_loss_releases_only_lost_controls )
{
	touchGestureState_t state;
	TouchGesture_Reset( &state );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 1, TOUCH_TARGET_BUTTON, 3 ) );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 2, TOUCH_TARGET_FIRE, 0 ) );
	TouchGesture_Step( &state, Input( TOUCH_MOTION, 2, TOUCH_TARGET_FIRE, 0, 100 ) );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 3, TOUCH_TARGET_MOVE ) );
	TouchGesture_Step( &state, Input( TOUCH_MOTION, 3, TOUCH_TARGET_MOVE, 0, 0, -100 ) );
	auto in = Input( TOUCH_REFRESH );
	in.visibleButtons &= ~( 1u << 3 );
	auto out = TouchGesture_Step( &state, in );
	BOOST_REQUIRE_EQUAL( out.keyCount, 1 );
	BOOST_CHECK_EQUAL( out.keys[0].button, 3 );
	BOOST_CHECK_EQUAL( out.keys[0].down, 0 );
	BOOST_CHECK_EQUAL( out.lookX, 127 );
	BOOST_CHECK_EQUAL( out.moveY, -127 );
	out = TouchGesture_Step( &state, Input( TOUCH_CANCEL_ALL ) );
	BOOST_REQUIRE_EQUAL( out.keyCount, 1 );
	BOOST_CHECK_EQUAL( out.keys[0].button, 0 );
	BOOST_CHECK_EQUAL( out.moveY, 0 );
	BOOST_CHECK_EQUAL( out.lookX, 0 );
	BOOST_CHECK_EQUAL( TouchGesture_Step( &state, Input( TOUCH_CANCEL_ALL ) ).keyCount, 0 );
}

BOOST_AUTO_TEST_CASE( move_owner_and_hidden_fire_loss )
{
	touchGestureState_t state;
	TouchGesture_Reset( &state );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 1, TOUCH_TARGET_MOVE ) );
	TouchGesture_Step( &state, Input( TOUCH_MOTION, 1, TOUCH_TARGET_MOVE, 0, 50 ) );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 2, TOUCH_TARGET_MOVE ) );
	BOOST_CHECK_EQUAL( TouchGesture_Step( &state, Input( TOUCH_MOTION, 2, TOUCH_TARGET_MOVE, 0, -100 ) ).moveX, 64 );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 3, TOUCH_TARGET_FIRE, 1 ) );
	TouchGesture_Step( &state, Input( TOUCH_MOTION, 3, TOUCH_TARGET_FIRE, 1, 100 ) );
	auto in = Input( TOUCH_REFRESH );
	in.visibleButtons &= ~( 1u << 1 );
	const auto out = TouchGesture_Step( &state, in );
	BOOST_REQUIRE_EQUAL( out.keyCount, 1 );
	BOOST_CHECK_EQUAL( out.keys[0].button, 1 );
	BOOST_CHECK_EQUAL( out.keys[0].down, 0 );
	BOOST_CHECK_EQUAL( out.lookX, 0 );
	BOOST_CHECK_EQUAL( out.moveX, 64 );
	BOOST_CHECK_EQUAL( state.lookOwner, -1 );
	TouchGesture_Step( &state, Input( TOUCH_UP, 1 ) );
	BOOST_CHECK_EQUAL( TouchGesture_Step( &state, Input( TOUCH_MOTION, 2, TOUCH_TARGET_MOVE, 0, -100 ) ).moveX, 0 );
	BOOST_CHECK_EQUAL( TouchGesture_Step( &state, Input( TOUCH_UP, 3 ) ).keyCount, 0 );
}

BOOST_AUTO_TEST_CASE( pair_precedence_and_rate_integration )
{
	for ( int hz : { 60, 120 } ) {
		touchGestureState_t state;
		TouchGesture_Reset( &state );
		TouchGesture_Step( &state, Input( TOUCH_DOWN, 1, TOUCH_TARGET_AIM ) );
		TouchGesture_Step( &state, Input( TOUCH_MOTION, 1, TOUCH_TARGET_AIM, 0, 100 ) );
		double degrees = 0;
		for ( int frame = 0; frame < hz; ++frame ) {
			const auto out = TouchGesture_Step( &state, Input( TOUCH_REFRESH ) );
			degrees += out.lookX / 127.0 * 140 * 1.5 / hz;
			const auto selected = TouchAxes_Select( 0, 9, out.lookX, 0 );
			BOOST_CHECK_EQUAL( selected.x, 0 );
			BOOST_CHECK_EQUAL( selected.y, 9 );
			BOOST_CHECK_EQUAL( selected.touch, 0 );
		}
		BOOST_CHECK_SMALL( degrees - 210, 1e-9 );
		BOOST_CHECK_EQUAL( TouchAxes_Select( 0, 0, 127, 0 ).touch, 1 );
		TouchGesture_Step( &state, Input( TOUCH_UP ) );
		BOOST_CHECK_EQUAL( TouchGesture_Step( &state, Input( TOUCH_REFRESH ) ).lookX, 0 );
	}
	BOOST_CHECK_EQUAL( TouchAxes_Select( 5, 0, 100, 100 ).y, 0 );
	BOOST_CHECK_EQUAL( TouchAxes_Select( 0, 0, 0, 0 ).touch, 0 );
}

static touchCapsFacts_t Facts()
{
	touchCapsFacts_t facts = {};
	facts.valid = facts.equipmentAllowed = facts.saberOwned = 1;
	facts.style = TOUCH_STYLE_BLUE;
	return facts;
}

BOOST_AUTO_TEST_CASE( capability_bits_style_counts_and_sp_progress )
{
	auto facts = Facts();
	facts.legalStyleCount = 1;
	facts.ownedWeaponCount = 1;
	BOOST_CHECK_EQUAL( TouchCaps_Pack( &facts, 0 ), ( 1u << 0 ) | ( 1u << 4 ) | ( 1u << 8 ) | ( 1u << 10 ) );
	const auto opening = TouchCaps_Visible( TouchCaps_Pack( &facts, 0 ), 0 );
	BOOST_CHECK( opening.menu && opening.console && opening.basic && opening.sticks && opening.saber && opening.pad );
	BOOST_CHECK( !opening.chat && !opening.style && !opening.force && !opening.forceCycle && !opening.weapons );
	facts.legalStyleCount = 2;
	facts.selectableForceCount = 2;
	facts.ownedWeaponCount = 3;
	facts.style = TOUCH_STYLE_RED;
	unsigned int mask = TouchCaps_Pack( &facts, 0 );
	BOOST_CHECK_EQUAL( mask, 1u | 2u | 4u | 8u | 16u | ( 2u << 6 ) | ( 1u << 8 ) | ( 1u << 9 ) | ( 1u << 10 ) );
	auto visible = TouchCaps_Visible( mask, 0 );
	BOOST_CHECK( visible.pad && visible.saber && visible.style && visible.force && visible.forceCycle && visible.weapons );
	BOOST_CHECK_EQUAL( visible.chat, 0 );
	facts.legalStyleCount = 1;
	facts.manualBladeToggle = 1;
	BOOST_CHECK( TouchCaps_Pack( &facts, 1 ) & TOUCH_CAP_STYLE );
	facts.manualBladeToggle = 0;
	BOOST_CHECK( TouchCaps_Pack( &facts, 1 ) & TOUCH_CAP_STYLE );
	BOOST_CHECK_EQUAL( TouchCaps_Pack( &facts, 0 ) & TOUCH_CAP_STYLE, 0 );
	facts.chatAvailable = 1;
	facts.selectableForceCount = 1;
	mask = TouchCaps_Pack( &facts, 1 );
	BOOST_CHECK_EQUAL( mask & TOUCH_CAP_PAD, 0 );
	BOOST_CHECK( mask & TOUCH_CAP_CHAT );
	BOOST_CHECK_EQUAL( mask & TOUCH_CAP_FORCE_CYCLE, 0 );
	facts.valid = 0;
	BOOST_CHECK_EQUAL( TouchCaps_Pack( &facts, 1 ), 0 );
}

BOOST_AUTO_TEST_CASE( sp_pad_follows_valid_state_without_force_or_equipment )
{
	auto facts = Facts();
	facts.saberOwned = facts.equipmentAllowed = 0;
	BOOST_CHECK_EQUAL( facts.selectableForceCount, 0 );
	const unsigned int sp = TouchCaps_Pack( &facts, 0 );
	const auto visible = TouchCaps_Visible( sp, 0 );
	BOOST_CHECK( sp & TOUCH_CAP_PAD );
	BOOST_CHECK( visible.pad );
	BOOST_CHECK( TouchCaps_ButtonMask( &visible ) & ( 1u << TB_PAD ) );
	BOOST_CHECK( TouchCaps_Visible( TOUCH_CAP_VALID, 0 ).pad );
	BOOST_CHECK( !TouchCaps_Visible( TOUCH_CAP_PAD, 0 ).pad );
	const unsigned int mp = TouchCaps_Pack( &facts, 1 );
	BOOST_CHECK_EQUAL( mp & TOUCH_CAP_PAD, 0 );
	BOOST_CHECK( TouchCaps_Visible( mp, 1 ).pad );
	touchBridgeState_t bridge = {};
	BOOST_CHECK( !TouchBridge_Step( &bridge, sp, 0, 0, 0, 0, 0, 0 ).pad );
	BOOST_CHECK( !TouchBridge_Step( &bridge, sp, 0, 1, 1, 0, 0, 0 ).pad );
	BOOST_CHECK( !TouchBridge_Step( &bridge, sp, 0, 1, 0, 1, 0, 0 ).pad );
	BOOST_CHECK( !TouchBridge_Step( &bridge, sp, 0, 1, 0, 0, 1, 0 ).pad );
	facts.valid = 0;
	BOOST_CHECK_EQUAL( TouchCaps_Pack( &facts, 0 ), 0 );
	BOOST_CHECK( !TouchCaps_Visible( TouchCaps_Pack( &facts, 0 ), 0 ).pad );
}

BOOST_AUTO_TEST_CASE( equipment_gate_preserves_basic_actions )
{
	auto facts = Facts();
	facts.equipmentAllowed = 0;
	facts.legalStyleCount = facts.selectableForceCount = facts.ownedWeaponCount = 3;
	facts.chatAvailable = 1;
	for ( int mp : { 0, 1 } ) {
		const auto v = TouchCaps_Visible( TouchCaps_Pack( &facts, mp ), mp );
		BOOST_CHECK( v.menu && v.console && v.basic && v.sticks && v.pad );
		BOOST_CHECK_EQUAL( v.chat, mp );
		BOOST_CHECK( !v.saber && !v.style && !v.force && !v.forceCycle && !v.weapons );
	}
}

BOOST_AUTO_TEST_CASE( combined_batch_latches_source_at_send_and_uses_latest_touch )
{
	touchAxesState_t axes = {};
	int engineAxes[4] = {};
	const auto drain = [&]( const touchAxesBatch_t &batch ) {
		if ( batch.send )
			for ( int i = 0; i < 4; ++i ) engineAxes[i] = batch.values[i];
	};
	touchGestureState_t gestures;
	TouchGesture_Reset( &gestures );
	TouchGesture_Step( &gestures, Input( TOUCH_DOWN, 1, TOUCH_TARGET_AIM ) );
	auto gesture = TouchGesture_Step( &gestures, Input( TOUCH_MOTION, 1, TOUCH_TARGET_AIM, 0, 100 ) );
	axes.touch[0] = 32;
	axes.touch[2] = gesture.lookX;
	BOOST_CHECK_EQUAL( axes.lookTouch, 0 );
	BOOST_CHECK_EQUAL( engineAxes[2], 0 );
	auto batch = TouchAxes_Send( &axes );
	BOOST_CHECK( batch.send );
	BOOST_CHECK_EQUAL( batch.values[2], 127 );
	BOOST_CHECK_EQUAL( axes.lookTouch, 1 );
	drain( batch );
	BOOST_CHECK_EQUAL( TouchAxes_NormalizeLook( engineAxes[2], axes.lookTouch, 2 ), 2 );
	BOOST_TEST_MESSAGE( "touch: events -> pad poll -> touch(127,0) -> send(touch) -> drain -> joystick yaw=2" );
	axes.pad[3] = -64;
	BOOST_CHECK_EQUAL( axes.lookTouch, 1 );
	batch = TouchAxes_Send( &axes );
	BOOST_CHECK_EQUAL( batch.values[0], 32 );
	BOOST_CHECK_EQUAL( batch.values[2], 0 );
	BOOST_CHECK_EQUAL( batch.values[3], -64 );
	BOOST_CHECK_EQUAL( axes.lookTouch, 0 );
	drain( batch );
	BOOST_CHECK_SMALL( TouchAxes_NormalizeLook( engineAxes[3], axes.lookTouch, 9 ) + 64.f / 127, 1e-6f );
	BOOST_TEST_MESSAGE( "pad: poll(0,-64) -> touch held -> send(pad) -> drain -> joystick pitch=-64/127; touch move=32" );
	axes.pad[3] = 0;
	batch = TouchAxes_Send( &axes );
	BOOST_CHECK_EQUAL( batch.values[2], 127 );
	BOOST_CHECK_EQUAL( axes.lookTouch, 1 );
	drain( batch );
	gesture = TouchGesture_Step( &gestures, Input( TOUCH_UP ) );
	axes.touch[0] = 0;
	axes.touch[2] = gesture.lookX;
	BOOST_CHECK_EQUAL( axes.lookTouch, 1 );
	batch = TouchAxes_Send( &axes );
	BOOST_CHECK( batch.send );
	BOOST_CHECK_EQUAL( batch.values[2], 0 );
	BOOST_CHECK_EQUAL( axes.lookTouch, 0 );
	drain( batch );
	BOOST_CHECK_EQUAL( TouchAxes_NormalizeLook( engineAxes[2], axes.lookTouch, 9 ), 0 );
	BOOST_CHECK( !TouchAxes_Send( &axes ).send );
	BOOST_TEST_MESSAGE( "zero: release -> touch(0,0) -> send(none) -> drain -> joystick yaw=0" );
}

BOOST_AUTO_TEST_CASE( same_frame_fire_tap_and_cinematic_reset_batch )
{
	touchAxesState_t axes = {};
	touchGestureState_t gestures;
	TouchGesture_Reset( &gestures );
	const auto down = TouchGesture_Step( &gestures, Input( TOUCH_DOWN ) );
	BOOST_REQUIRE_EQUAL( down.keyCount, 1 );
	BOOST_CHECK_EQUAL( down.keys[0].down, 1 );
	const auto moved = TouchGesture_Step( &gestures, Input( TOUCH_MOTION, 1, TOUCH_TARGET_FIRE, 0, 100 ) );
	axes.touch[2] = moved.lookX;
	const auto up = TouchGesture_Step( &gestures, Input( TOUCH_UP ) );
	BOOST_REQUIRE_EQUAL( up.keyCount, 1 );
	BOOST_CHECK_EQUAL( up.keys[0].down, 0 );
	axes.touch[2] = up.lookX;
	BOOST_CHECK_EQUAL( TouchAxes_Send( &axes ).values[2], 0 );
	axes.pad[0] = 50;
	axes.pad[2] = -80;
	axes.touch[1] = 100;
	axes.touch[3] = 90;
	auto batch = TouchAxes_Send( &axes );
	BOOST_CHECK_EQUAL( batch.values[0], 50 );
	BOOST_CHECK_EQUAL( batch.values[1], 0 );
	BOOST_CHECK_EQUAL( batch.values[2], -80 );
	BOOST_CHECK_EQUAL( batch.values[3], 0 );
	TouchGesture_Step( &gestures, Input( TOUCH_DOWN ) );
	const auto promoted = TouchGesture_Step( &gestures, Input( TOUCH_MOTION, 1, TOUCH_TARGET_FIRE, 0, 100 ) );
	BOOST_CHECK_EQUAL( promoted.lookX, 127 );
	const auto cancelled = TouchGesture_Step( &gestures, Input( TOUCH_CANCEL_ALL ) );
	BOOST_REQUIRE_EQUAL( cancelled.keyCount, 1 );
	BOOST_CHECK_EQUAL( cancelled.keys[0].down, 0 );
	BOOST_CHECK_EQUAL( cancelled.keys[0].button, 0 );
	BOOST_CHECK_EQUAL( cancelled.lookX, 0 );
	TouchAxes_Zero( &axes );
	BOOST_CHECK_EQUAL( axes.lookTouch, 0 );
	batch = TouchAxes_Send( &axes );
	BOOST_CHECK( batch.send );
	for ( int i = 0; i < 4; ++i ) {
		BOOST_CHECK_EQUAL( batch.values[i], 0 );
		BOOST_CHECK_EQUAL( axes.pad[i], 0 );
		BOOST_CHECK_EQUAL( axes.touch[i], 0 );
	}
	BOOST_CHECK( !TouchAxes_Send( &axes ).send );
	const auto refreshed = TouchGesture_Step( &gestures, Input( TOUCH_REFRESH ) );
	BOOST_CHECK_EQUAL( refreshed.keyCount, 0 );
	BOOST_CHECK_EQUAL( refreshed.lookX, 0 );
	axes.touch[2] = refreshed.lookX;
	axes.touch[3] = refreshed.lookY;
	batch = TouchAxes_Send( &axes );
	BOOST_CHECK_EQUAL( batch.values[2], 0 );
	BOOST_CHECK_EQUAL( batch.values[3], 0 );
	BOOST_CHECK_EQUAL( axes.lookTouch, 0 );
}

BOOST_AUTO_TEST_CASE( distinct_local_equipment_predicates )
{
	BOOST_CHECK( TouchCaps_EquipmentAllowed( 100, 0, 0, 0 ) );
	BOOST_CHECK( !TouchCaps_EquipmentAllowed( 0, 0, 0, 0 ) );
	BOOST_CHECK( !TouchCaps_EquipmentAllowed( -10, 0, 0, 0 ) );
	BOOST_CHECK( !TouchCaps_EquipmentAllowed( 100, 1, 0, 0 ) );
	BOOST_CHECK( !TouchCaps_EquipmentAllowed( 100, 0, 1, 0 ) );
	BOOST_CHECK( !TouchCaps_EquipmentAllowed( 100, 0, 0, 1 ) );
}

BOOST_AUTO_TEST_CASE( style_policy_colours_and_bridge_republication )
{
	touchCapsFacts_t mp=Facts();mp.saberOwned=1;
	for(int offense=0;offense<=3;++offense) {
		mp.legalStyleCount=offense;mp.manualBladeToggle=0;
		BOOST_CHECK(TouchCaps_Visible(TouchCaps_Pack(&mp,1),1).style);
	}
	mp.equipmentAllowed=0;BOOST_CHECK(!TouchCaps_Visible(TouchCaps_Pack(&mp,1),1).style);
	mp.equipmentAllowed=1;mp.saberOwned=0;BOOST_CHECK(!TouchCaps_Visible(TouchCaps_Pack(&mp,1),1).style);

	const int styles[] = { 1, 2, 3, 4, 5, 6, 7 };
	const int colours[] = { TOUCH_STYLE_UNKNOWN, TOUCH_STYLE_BLUE, TOUCH_STYLE_YELLOW, TOUCH_STYLE_RED,
		TOUCH_STYLE_RED, TOUCH_STYLE_BLUE, TOUCH_STYLE_YELLOW, TOUCH_STYLE_YELLOW, TOUCH_STYLE_UNKNOWN };
	for ( int i = 0; i < 9; ++i ) {
		const int colour = TouchCaps_SaberColor( i, styles, 0 );
		BOOST_CHECK_EQUAL(TouchCaps_SaberColor(i,styles,1),TOUCH_STYLE_YELLOW);
		BOOST_CHECK_EQUAL( colour, colours[i] );
		auto facts = Facts();
		facts.style = colour;
		BOOST_CHECK_EQUAL( TouchCaps_Visible( TouchCaps_Pack( &facts, 0 ), 0 ).styleColor, colour );
	}
	const auto publication = Facts();
	const unsigned int valid = TouchCaps_Pack( &publication, 0 );
	BOOST_CHECK( TouchCaps_ShouldPublish( 0, -1, 0 ) );
	BOOST_CHECK( TouchCaps_ShouldPublish( valid, 0, 0 ) );
	BOOST_CHECK( !TouchCaps_ShouldPublish( valid, valid, valid ) );
	BOOST_CHECK( TouchCaps_ShouldPublish( valid, valid, 0 ) );
	BOOST_CHECK( TouchCaps_ShouldPublish( 0, valid, valid ) );
	BOOST_CHECK( !TouchCaps_ShouldPublish( 0, 0, 0 ) );
}

BOOST_AUTO_TEST_CASE( decoded_button_mask_and_selective_capability_loss )
{
	auto facts = Facts();
	facts.legalStyleCount = facts.selectableForceCount = facts.ownedWeaponCount = 2;
	facts.chatAvailable = 1;
	const auto full = TouchCaps_Visible( TouchCaps_Pack( &facts, 1 ), 1 );
	BOOST_CHECK_EQUAL( TouchCaps_ButtonMask( &full ), IOS_TOUCH_ALL_BUTTONS & ~((1u<<TB_LOCK)|(15u<<TB_FORCE_SLOT1)) );
	touchGestureState_t state;
	TouchGesture_Reset( &state );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 1, TOUCH_TARGET_BUTTON, TB_FORCE ) );
	TouchGesture_Step( &state, Input( TOUCH_DOWN, 2, TOUCH_TARGET_AIM ) );
	TouchGesture_Step( &state, Input( TOUCH_MOTION, 2, TOUCH_TARGET_AIM, 0, 100 ) );
	facts.equipmentAllowed = 0;
	const auto dead = TouchCaps_Visible( TouchCaps_Pack( &facts, 1 ), 1 );
	const unsigned int basic = ( 1u << TB_ATTACK ) | ( 1u << TB_ALTATTACK ) | ( 1u << TB_BOTHATTACK ) | ( 1u << TB_JUMP ) | ( 1u << TB_CROUCH ) | ( 1u << TB_USE );
	BOOST_CHECK_EQUAL( TouchCaps_ButtonMask( &dead ), basic | ( 1u << TB_MENU ) | ( 1u << TB_CONSOLE ) | ( 1u << TB_CHAT ) | ( 1u << TB_PAD ) );
	auto in = Input( TOUCH_REFRESH );
	in.visibleButtons = TouchCaps_ButtonMask( &dead );
	in.moveEnabled = in.lookEnabled = dead.sticks;
	const auto out = TouchGesture_Step( &state, in );
	BOOST_REQUIRE_EQUAL( out.keyCount, 1 );
	BOOST_CHECK_EQUAL( out.keys[0].button, TB_FORCE );
	BOOST_CHECK_EQUAL( out.keys[0].down, 0 );
	BOOST_CHECK_EQUAL( out.lookX, 127 );
	TouchGesture_Step( &state, Input( TOUCH_UP, 2 ) );
	const auto hiddenMotion = TouchGesture_Step( &state, Input( TOUCH_MOTION, 1, TOUCH_TARGET_AIM, 0, -100 ) );
	BOOST_CHECK_EQUAL( hiddenMotion.keyCount, 0 );
	BOOST_CHECK_EQUAL( hiddenMotion.lookX, 0 );
	BOOST_CHECK_EQUAL( state.lookOwner, -1 );
}

BOOST_AUTO_TEST_CASE( missing_bridge_grace_recovery_priority_and_epoch )
{
	for ( int mp : { 0, 1 } ) {
		touchBridgeState_t bridge = {};
		auto v = TouchBridge_Step( &bridge, 0, mp, 1, 0, 0, 0, 50 );
		BOOST_CHECK( v.menu && v.console && !v.basic );
		v = TouchBridge_Step( &bridge, 0, mp, 1, 0, 0, 0, 1049 );
		BOOST_CHECK( !v.fallback && !v.sticks );
		v = TouchBridge_Step( &bridge, 0, mp, 1, 0, 0, 0, 1050 );
		BOOST_CHECK( v.fallback && v.logMissing && v.basic && v.sticks && v.pad && v.saber && v.style && v.weapons && v.force && v.forceCycle );
		BOOST_CHECK_EQUAL( v.chat, mp );
		BOOST_CHECK_EQUAL( v.menu + v.console + 5 * v.basic + v.saber + v.style + v.force + v.forceCycle + 2 * v.weapons + v.pad + v.chat, mp ? 15 : 14 );
		BOOST_CHECK_EQUAL( v.styleColor, TOUCH_STYLE_UNKNOWN );
		BOOST_CHECK_EQUAL( TouchBridge_Step( &bridge, 0, mp, 1, 0, 0, 0, 1051 ).logMissing, 0 );
		auto facts = Facts();
		v = TouchBridge_Step( &bridge, TouchCaps_Pack( &facts, mp ), mp, 1, 0, 0, 0, 1052 );
		BOOST_CHECK( v.basic && !v.fallback && !v.force && v.style==mp && !v.weapons );
		TouchBridge_Step( &bridge, 0, mp, 1, 0, 0, 0, 1100 );
		v = TouchBridge_Step( &bridge, 0, mp, 1, 0, 0, 0, 2100 );
		BOOST_CHECK( v.fallback && !v.logMissing );
		v = TouchBridge_Step( &bridge, 0, mp, 1, 1, 0, 0, 2101 );
		BOOST_CHECK( v.console && !v.menu && !v.basic );
		v = TouchBridge_Step( &bridge, 0, mp, 1, 0, 1, 0, 2102 );
		BOOST_CHECK( v.console && !v.menu && !v.basic );
		v = TouchBridge_Step( &bridge, 0, mp, 1, 0, 0, 1, 2103 );
		BOOST_CHECK( !v.console && !v.menu && !v.basic );
		v = TouchBridge_Step( &bridge, 0, mp, 0, 0, 0, 0, 3000 );
		BOOST_CHECK( v.menu && v.console && !v.fallback );
		TouchBridge_Step( &bridge, 0, mp, 1, 0, 0, 0, 5000 );
		BOOST_CHECK( !TouchBridge_Step( &bridge, 0, mp, 1, 0, 0, 0, 5999 ).fallback );
		TouchBridge_Reset( &bridge );
		TouchBridge_Step( &bridge, 0, mp, 1, 0, 0, 0, 6000 );
		BOOST_CHECK( TouchBridge_Step( &bridge, 0, mp, 1, 0, 0, 0, 7000 ).logMissing );
	}
}

BOOST_AUTO_TEST_CASE( anchor_units_points_clamps_and_zones )
{
	float x, y, w, h;
	BOOST_CHECK_EQUAL( TouchLayout_Unit( 2604, 1218, 3 ), 1218 );
	BOOST_CHECK_EQUAL( TouchLayout_Unit( 2048, 1536, 2 ), 880 );
	BOOST_CHECK_EQUAL( TouchLayout_Unit( 1024, 768, 1 ), 440 );
	BOOST_CHECK_EQUAL( TouchLayout_PixelsPerPoint( 2868, 1320, 956, 440 ), 3 );
	BOOST_CHECK_EQUAL( TouchLayout_PixelsPerPoint( 1434, 660, 956, 440 ), 1.5f );
	BOOST_CHECK_EQUAL( TouchLayout_PixelsPerPoint( 1434, 660, 0, 440 ), 0 );
	for ( int anchor : { TOUCH_ANCHOR_TL, TOUCH_ANCHOR_TR, TOUCH_ANCHOR_BL, TOUCH_ANCHOR_BR } ) {
		TouchLayout_AnchorRect( 2048, 1536, 0, 0, 0, 0, 2, anchor, .12f, .10f, .12f, &x, &y, &w, &h );
		BOOST_CHECK_SMALL( w - 105.6f, 1e-4f );
		BOOST_CHECK_SMALL( x + w / 2 - ( anchor == TOUCH_ANCHOR_TR || anchor == TOUCH_ANCHOR_BR ? 2048 - 105.6f : 105.6f ), 1e-4f );
		BOOST_CHECK_SMALL( y + h / 2 - ( anchor == TOUCH_ANCHOR_BL || anchor == TOUCH_ANCHOR_BR ? 1536 - 88.f : 88.f ), 1e-4f );
	}
	TouchLayout_AnchorRect( 600, 400, 20, 30, 40, 50, 3, TOUCH_ANCHOR_TL, 0, 0, .12f, &x, &y, &w, &h );
	BOOST_CHECK_EQUAL( x, 20 );
	BOOST_CHECK_EQUAL( y, 30 );
	BOOST_CHECK_EQUAL( w, 132 );
	TouchLayout_AnchorRect( 60, 40, 2, 3, 4, 5, 3, TOUCH_ANCHOR_BR, .12f, .1f, .12f, &x, &y, &w, &h );
	BOOST_CHECK_GE( x, 2 );
	BOOST_CHECK_GE( y, 3 );
	BOOST_CHECK_LE( x + w, 56 );
	BOOST_CHECK_LE( y + h, 35 );
	BOOST_CHECK_EQUAL( TouchLayout_Zone( 2000, 1000, 100, 0, 100, 0, 999, 319 ), TOUCH_ZONE_NONE );
	BOOST_CHECK_EQUAL( TouchLayout_Zone( 2000, 1000, 100, 0, 100, 0, 999, 320 ), TOUCH_ZONE_MOVE );
	BOOST_CHECK_EQUAL( TouchLayout_Zone( 2000, 1000, 100, 0, 100, 0, 1000, 0 ), TOUCH_ZONE_AIM );
	BOOST_CHECK_EQUAL( TouchLayout_Zone( 2000, 1000, 100, 0, 100, 0, 1901, 900 ), TOUCH_ZONE_AIM );
	BOOST_CHECK_EQUAL( TouchLayout_Zone( 2000, 1000, 100, 0, 100, 0, 2000, 1000 ), TOUCH_ZONE_AIM );
	BOOST_CHECK_EQUAL( TouchLayout_Zone( 2000, 1000, 100, 0, 100, 0, 2001, 500 ), TOUCH_ZONE_NONE );
	BOOST_CHECK_EQUAL( TouchLayout_Zone( 2000, 1000, 100, 0, 100, 0, 0, 1000 ), TOUCH_ZONE_MOVE );
	BOOST_CHECK_EQUAL( TouchLayout_Zone( 2000, 1000, 100, 0, 100, 0, 50, 319 ), TOUCH_ZONE_NONE );
	BOOST_CHECK_EQUAL( TouchLayout_Zone( 2000, 1000, 100, 50, 100, 50, 999, 337 ), TOUCH_ZONE_NONE );
	BOOST_CHECK_EQUAL( TouchLayout_Zone( 2000, 1000, 100, 50, 100, 50, 999, 338 ), TOUCH_ZONE_MOVE );
	BOOST_CHECK_EQUAL( TouchLayout_FireSlop( 1218, 3 ), 18.27f );
}

BOOST_AUTO_TEST_CASE( invalid_geometry_and_clock_wrap )
{
	for ( float bad : { 0.f, -1.f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() } ) {
		float x, y, w, h;
		BOOST_CHECK_EQUAL( TouchLayout_Unit( 2048, 1536, bad ), 0 );
		BOOST_CHECK_EQUAL( TouchLayout_PixelsPerPoint( 2048, 1536, bad, 768 ), 0 );
		TouchLayout_AnchorRect( 2048, 1536, 0, 0, 0, 0, bad, TOUCH_ANCHOR_TL, .12f, .1f, .12f, &x, &y, &w, &h );
		BOOST_CHECK_EQUAL( w, 0 );
	}
	touchBridgeState_t bridge = {};
	TouchBridge_Step( &bridge, 0, 1, 1, 0, 0, 0, ~0u - 499 );
	BOOST_CHECK( !TouchBridge_Step( &bridge, 0, 1, 1, 0, 0, 0, 499 ).fallback );
	BOOST_CHECK( TouchBridge_Step( &bridge, 0, 1, 1, 0, 0, 0, 500 ).fallback );
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_CASE( touch_only_axis_reset_preserves_held_pad_and_pad_query )
{
	touchAxesState_t state={}; state.pad[0]=50; state.pad[2]=60; state.touch[1]=70; state.touch[3]=80;
	BOOST_CHECK(TouchAxes_PadLookActive(&state));
	TouchAxes_ZeroTouch(&state);
	const auto batch=TouchAxes_Send(&state);
	BOOST_CHECK_EQUAL(batch.values[0],50); BOOST_CHECK_EQUAL(batch.values[1],0);
	BOOST_CHECK_EQUAL(batch.values[2],60); BOOST_CHECK_EQUAL(batch.values[3],0);
	BOOST_CHECK(batch.send); BOOST_CHECK(!state.lookTouch);
	state.pad[2]=0; BOOST_CHECK(!TouchAxes_PadLookActive(&state));
	state.touch[2]=127; BOOST_CHECK(!TouchAxes_PadLookActive(&state));
}

BOOST_AUTO_TEST_SUITE_END()
