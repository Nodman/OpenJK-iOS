#include "sys/ios/ios_touch_layout_edit.h"
#include "sys/ios/ios_touch_edit_state.h"
#include "ios/touch/touch_atlas.h"
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

BOOST_AUTO_TEST_CASE( editor_transitions_capture_hidden_buttons_and_save )
{
	touchEditState_t state = {};
	touchEditInput_t in = {}; in.event = TOUCH_EDIT_ENTER;
	auto out = TouchEdit_State( &state, in );
	BOOST_CHECK( state.active && out.release && out.zero );
	in.event = TOUCH_EDIT_DOWN; in.button = TB_FORCE; in.finger = 9; in.x = in.cx = 100; in.y = in.cy = 200;
	TouchEdit_State( &state, in );
	BOOST_CHECK( state.dragging && state.button == TB_FORCE );
	in.finger = 10; in.button = TB_JUMP; TouchEdit_State( &state, in );
	BOOST_CHECK_EQUAL( state.button, TB_FORCE );
	in.event = TOUCH_EDIT_MOVE; in.finger = 10; BOOST_CHECK( !TouchEdit_State( &state, in ).move );
	in.finger = 9; in.x = 150; out = TouchEdit_State( &state, in );
	BOOST_CHECK( out.move ); BOOST_CHECK_EQUAL( out.x, 150 );
	in.event = TOUCH_EDIT_UP; BOOST_CHECK( !TouchEdit_State( &state, in ).save );
	BOOST_CHECK( state.active && !state.dragging );
	in.event = TOUCH_EDIT_EXIT; out = TouchEdit_State( &state, in );
	BOOST_CHECK( out.save && out.release && out.zero && !state.active );
	BOOST_CHECK( !TouchEdit_State( &state, in ).save );
	in.event = TOUCH_EDIT_ENTER; TouchEdit_State( &state, in );
	in.event = TOUCH_EDIT_DOWN; in.button = TB_CONSOLE; TouchEdit_State( &state, in );
	BOOST_CHECK( !state.dragging );
	in.button = TB_LOCK; TouchEdit_State( &state, in );
	in.event = TOUCH_EDIT_UP; BOOST_CHECK( TouchEdit_State( &state, in ).save );
	BOOST_CHECK( !state.active );
}

BOOST_AUTO_TEST_CASE( editor_draws_all_controls_dim_and_highlight_within_capacity )
{
	const auto layout = AspectLayout_Build(2048,1536,1,1.5f,0,0,0,0);
	touchRect_t rects[TB_COUNT]; TouchLayout_Build(2048,1536,0,0,0,0,2,rects);
	touchGestureState_t gestures; TouchGesture_Reset(&gestures);
	touchDrawInput_t input = {}; input.layout=&layout; input.rects=rects; input.gestures=&gestures;
	input.editing=1; input.dragButton=TB_FORCE; input.visibleButtons=0;
	touchDrawItem_t items[32];
	BOOST_REQUIRE_EQUAL( TouchDraw_Build(&input,items,32), TB_COUNT+2 );
	BOOST_CHECK_EQUAL( items[0].kind, TOUCH_DRAW_DIM );
	int highlights=0, buttons=0;
	for ( int i=0;i<TB_COUNT+2;++i ) {
		highlights += items[i].kind==TOUCH_DRAW_HIGHLIGHT;
		buttons += items[i].kind==TOUCH_DRAW_BUTTON;
		if (items[i].glyph==TG_LOCK) BOOST_CHECK( items[i].pressed );
	}
	BOOST_CHECK_EQUAL(highlights,1); BOOST_CHECK_EQUAL(buttons,TB_COUNT);
	BOOST_CHECK_LE(TB_COUNT+4,IOS_TOUCH_MAX_DRAW_ITEMS);
	float colour[4]; TouchDraw_Color(&items[0],0,0,colour);
	BOOST_CHECK_EQUAL(colour[0],0); BOOST_CHECK_EQUAL(colour[3],.35f);
}

BOOST_AUTO_TEST_CASE( lock_drag_and_forced_exit_do_not_leak_ownership )
{
	for(int flags=0;flags<16;++flags)
		BOOST_CHECK_EQUAL(TouchEdit_LockVisible(flags&1,flags&2,flags&4,flags&8),flags==1);
	touchEditState_t state = {}; touchEditInput_t in = {}; in.event=TOUCH_EDIT_ENTER;
	TouchEdit_State(&state,in); BOOST_CHECK(!TouchEdit_State(&state,in).release);
	in.event=TOUCH_EDIT_DOWN; in.button=-1; TouchEdit_State(&state,in); BOOST_CHECK(!state.dragging);
	in.button=TB_LOCK; in.x=100; in.cx=110; in.slop=6; TouchEdit_State(&state,in);
	in.event=TOUCH_EDIT_MOVE; in.x=120; auto out=TouchEdit_State(&state,in);
	BOOST_CHECK(out.move); BOOST_CHECK_EQUAL(out.x,130);
	in.event=TOUCH_EDIT_UP; in.finger=2; TouchEdit_State(&state,in); BOOST_CHECK(state.dragging);
	in.finger=0; BOOST_CHECK(!TouchEdit_State(&state,in).save); BOOST_CHECK(state.active);
	in.event=TOUCH_EDIT_DOWN; in.button=TB_CHAT; TouchEdit_State(&state,in);
	in.event=TOUCH_EDIT_EXIT; out=TouchEdit_State(&state,in); BOOST_CHECK(out.save && out.zero && out.release);
	BOOST_CHECK(!state.active && !state.dragging);
	in.event=TOUCH_EDIT_MOVE; BOOST_CHECK(!TouchEdit_State(&state,in).move);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
