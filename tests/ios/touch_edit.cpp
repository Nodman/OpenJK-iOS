#include "sys/ios/ios_touch_layout_edit.h"
#include "sys/ios/ios_touch_edit_state.h"
#include "ios/touch/touch_atlas.h"
#include "sys/ios/ios_touch_edit_slider.h"
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
			touchButtonDef_t defs[TB_COUNT]; TouchEdit_Apply( &parsed, defs, f.x, f.y, 40, 10, 50, 30, f.w );
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

BOOST_AUTO_TEST_CASE( slider_mapping_clamps_and_round_trips )
{
	const auto r=TouchEdit_SliderRect(2868,1320,132,0,132,102,3);
	BOOST_CHECK_CLOSE(r.w,1302,.001);
	BOOST_CHECK_EQUAL(TouchEdit_SliderValue(-1000,&r),.2f);
	BOOST_CHECK_EQUAL(TouchEdit_SliderValue(5000,&r),1);
	for(int i=0;i<=8;++i) {
		const float v=.2f+.1f*i;
		BOOST_CHECK_SMALL(TouchEdit_SliderValue(TouchEdit_SliderX(v,&r),&r)-v,1e-6f);
	}
	const auto layout=AspectLayout_Build(2868,1320,1,1.5f,132,0,132,102);
	touchRect_t rects[TB_COUNT]; TouchLayout_Build(2868,1320,132,0,132,102,3,rects);
	for(int b=0;b<TB_COUNT;++b)
		BOOST_CHECK(r.x+r.w<=rects[b].x || rects[b].x+rects[b].w<=r.x || r.y+r.h<=rects[b].y || rects[b].y+rects[b].h<=r.y);
	touchGestureState_t gestures; TouchGesture_Reset(&gestures);
	touchDrawInput_t input={}; input.layout=&layout;input.rects=rects;input.gestures=&gestures;
	input.editing=1;input.dragButton=TB_FORCE;input.slider=&r;input.opacity=.6f;
	touchDrawItem_t items[33], sentinel; memset(&sentinel,0x5a,sizeof(sentinel));
	for(int capacity=0;capacity<=32;++capacity) {
		memset(items,0x5a,sizeof(items));
		const int n=TouchDraw_Build(&input,items,capacity);
		BOOST_CHECK_EQUAL(n,capacity<TB_COUNT+4?capacity:TB_COUNT+4);
		BOOST_CHECK_EQUAL(memcmp(&items[n],&sentinel,sizeof(sentinel)),0);
	}
}

static bool PlatesDisjoint( const touchRect_t &a, const touchRect_t &b )
{
	return a.x+a.w<=b.x || b.x+b.w<=a.x || a.y+a.h<=b.y || b.y+b.h<=a.y;
}

BOOST_AUTO_TEST_CASE( lock_drag_and_loaded_overrides_keep_exit_and_reentry_reachable )
{
	for (float scale : {1.0f,.5f}) {
		const float W=2048*scale,H=1536*scale,p=2*scale;
		const float unit=TouchLayout_Unit(W,H,p);
		const auto slider=TouchEdit_SliderRect(W,H,0,0,0,0,p);
		touchRect_t baseline[TB_COUNT]; TouchLayout_Build(W,H,0,0,0,0,p,baseline);
		for (const auto &blocked : {slider,baseline[TB_CONSOLE]}) {
			for (int load=0;load<2;++load) {
				const float x=blocked.x+blocked.w/2,y=blocked.y+blocked.h/2;
				touchLayoutOverrides_t o={};
				if (load) {
					o.mask=1u<<TB_LOCK; o.cx[TB_LOCK]=(W-x)/unit; o.cy[TB_LOCK]=y/unit;
					char text[1024]; BOOST_REQUIRE(TouchEdit_Serialize(&o,text,sizeof(text)));
					TouchEdit_Parse(text,&o);
				} else BOOST_REQUIRE(TouchEdit_Drag(&o,TB_LOCK,x,y,W,H,0,0,0,0,p));
				touchRect_t rects[TB_COUNT]; TouchLayout_Build(W,H,0,0,0,0,p,rects,&o);
				BOOST_CHECK(PlatesDisjoint(rects[TB_LOCK],slider));
				BOOST_CHECK(PlatesDisjoint(rects[TB_LOCK],rects[TB_CONSOLE]));
				if (load) {
					char before[1024],fixed[1024]; TouchEdit_Serialize(&o,before,sizeof(before));
					BOOST_REQUIRE(TouchEdit_RepairLock(&o,W,H,0,0,0,0,p));
					BOOST_REQUIRE(TouchEdit_Serialize(&o,fixed,sizeof(fixed)));
					BOOST_CHECK_NE(std::strcmp(before,fixed),0);
					TouchEdit_Parse(fixed,&o);
				}
				const float cx=rects[TB_LOCK].x+rects[TB_LOCK].w/2,cy=rects[TB_LOCK].y+rects[TB_LOCK].h/2;
				BOOST_CHECK(!(cx>=slider.x && cx<=slider.x+slider.w && cy>=slider.y && cy<=slider.y+slider.h));
				BOOST_CHECK_NE(TouchLayout_Hit(rects,cx,cy,1u<<TB_CONSOLE),TB_CONSOLE);
				touchEditState_t state={}; touchEditInput_t in={}; in.event=TOUCH_EDIT_ENTER;
				TouchEdit_State(&state,in);
				in.event=TOUCH_EDIT_DOWN;in.x=in.cx=cx;in.y=in.cy=cy;
				in.button=TouchLayout_Hit(rects,cx,cy,1u<<TB_LOCK);
				BOOST_REQUIRE_EQUAL(in.button,TB_LOCK);
				TouchEdit_State(&state,in);in.event=TOUCH_EDIT_UP;
				BOOST_CHECK(TouchEdit_State(&state,in).save);
				BOOST_CHECK(!state.active);
				char saved[1024]; BOOST_REQUIRE(TouchEdit_Serialize(&o,saved,sizeof(saved)));
				TouchEdit_Parse(saved,&o); TouchLayout_Build(W,H,0,0,0,0,p,rects,&o);
				BOOST_CHECK(PlatesDisjoint(rects[TB_LOCK],slider));
				BOOST_CHECK(PlatesDisjoint(rects[TB_LOCK],rects[TB_CONSOLE]));
				const float rx=rects[TB_LOCK].x+rects[TB_LOCK].w/2,ry=rects[TB_LOCK].y+rects[TB_LOCK].h/2;
				BOOST_CHECK_NE(TouchLayout_Hit(rects,rx,ry,1u<<TB_CONSOLE),TB_CONSOLE);
				BOOST_CHECK_EQUAL(TouchLayout_Hit(rects,rx,ry,1u<<TB_LOCK),TB_LOCK);
				in.event=TOUCH_EDIT_ENTER; TouchEdit_State(&state,in); BOOST_CHECK(state.active);
			}
		}
	}
}

BOOST_AUTO_TEST_CASE( lock_projection_survives_edges_rounding_and_geometry_changes )
{
	const float fixtures[][7]={{2868,1320,132,0,132,102,3},{1434,660,66,0,66,51,1.5f},
		{2048,1536,0,0,0,0,2},{1024,768,0,0,0,0,1},
		{2868,1320,144,30,44,90,3},{1434,660,72,15,22,45,1.5f}};
	for (const auto &f : fixtures) {
		for(int ix=0;ix<=8;++ix) for(int iy=0;iy<=8;++iy) {
			touchLayoutOverrides_t o={};
			BOOST_REQUIRE(TouchEdit_Drag(&o,TB_LOCK,f[0]*ix/8,f[1]*iy/8,f[0],f[1],f[2],f[3],f[4],f[5],f[6]));
			char text[1024]; BOOST_REQUIRE(TouchEdit_Serialize(&o,text,sizeof(text)));
			TouchEdit_Parse(text,&o);
			for(int rotated=0;rotated<2;++rotated) {
				const float l=rotated?f[4]:f[2],r=rotated?f[2]:f[4];
				const auto slider=TouchEdit_SliderRect(f[0],f[1],l,f[3],r,f[5],f[6]);
				touchRect_t rects[TB_COUNT]; TouchLayout_Build(f[0],f[1],l,f[3],r,f[5],f[6],rects,&o);
				BOOST_CHECK(PlatesDisjoint(rects[TB_LOCK],slider));
				BOOST_CHECK(PlatesDisjoint(rects[TB_LOCK],rects[TB_CONSOLE]));
				BOOST_CHECK_GE(rects[TB_LOCK].x,l); BOOST_CHECK_GE(rects[TB_LOCK].y,f[3]);
				BOOST_CHECK_LE(rects[TB_LOCK].x+rects[TB_LOCK].w,f[0]-r+.001f);
				BOOST_CHECK_LE(rects[TB_LOCK].y+rects[TB_LOCK].h,f[1]-f[5]+.001f);
			}
		}
	}
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
