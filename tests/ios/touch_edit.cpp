#include "sys/ios/ios_touch_layout_edit.h"
#include "sys/ios/ios_touch_edit_state.h"
#include "ios/touch/touch_atlas.h"
#include <boost/test/unit_test.hpp>
#include <cstring>

BOOST_AUTO_TEST_SUITE( ios )
BOOST_AUTO_TEST_SUITE( touch_edit )

static bool Disjoint( const touchRect_t &a,const touchRect_t &b )
{
	return a.x+a.w<=b.x || b.x+b.w<=a.x || a.y+a.h<=b.y || b.y+b.h<=a.y;
}

BOOST_AUTO_TEST_CASE( versioned_parse_bounds_unknown_ids_and_round_trip )
{
	touchLayoutOverrides_t o;
	TouchEdit_Parse("v2;0:.4,.6;11:1,0;20:0,1;21:.2,.3;1:nan,0;2:1.1,0;3:0,0junk;4:-1,0;",&o);
	BOOST_CHECK_EQUAL(o.version,2);
	BOOST_CHECK_EQUAL(o.mask,(1u<<TB_ATTACK)|(1u<<TB_CONSOLE)|(1u<<TB_FORCE_SLOT4));
	char text[IOS_TOUCH_LAYOUT_TEXT];
	BOOST_REQUIRE(TouchEdit_Serialize(&o,text,sizeof(text)));
	BOOST_CHECK_EQUAL(text,"v2;0:0.400000,0.600000;11:1.000000,0.000000;20:0.000000,1.000000;");
	touchLayoutOverrides_t copy; TouchEdit_Parse(text,&copy);
	BOOST_CHECK_EQUAL(copy.mask,o.mask);
	char shortText[3]="xx";
	BOOST_CHECK(!TouchEdit_Serialize(&o,shortText,sizeof(shortText)));
	BOOST_CHECK_EQUAL(shortText[0],0);
	TouchEdit_Parse("v3;0:.3,.4;",&copy); BOOST_CHECK_EQUAL(copy.mask,0);
	TouchEdit_Parse("0:.4,.6;11:0,0;16:0,0;20:0,0;",&copy);
	BOOST_CHECK_EQUAL(copy.version,1); BOOST_CHECK_EQUAL(copy.mask,1u);
	TouchEdit_Parse(NULL,&copy); BOOST_CHECK_EQUAL(copy.mask,0);
}

BOOST_AUTO_TEST_CASE( migration_uses_original_floor_clamp_and_repairs_old_lock )
{
	touchLayoutOverrides_t o; TouchEdit_Parse("10:0,0;0:0,0;",&o);
	BOOST_CHECK(!TouchEdit_Migrate(&o,0,0,0,0,0,0,0));
	BOOST_REQUIRE(TouchEdit_Migrate(&o,600,400,20,10,40,30,3));
	BOOST_CHECK_EQUAL(o.version,2);
	BOOST_CHECK_CLOSE(o.cx[TB_MENU]*600,86,.0001);
	BOOST_CHECK_CLOSE(o.cy[TB_MENU]*400,76,.0001);
	touchRect_t r[TB_COUNT]; TouchLayout_Build(600,400,20,10,40,30,3,r,&o);
	BOOST_CHECK_EQUAL(r[TB_MENU].x,53); BOOST_CHECK_EQUAL(r[TB_MENU].y,43);
	BOOST_CHECK(!TouchEdit_Migrate(&o,600,400,20,10,40,30,3));
	TouchEdit_Parse("15:0.100,0.080;",&o);
	touchButtonDef_t legacy[TB_COUNT]; TouchLegacy_Apply(&o,legacy,2048,1536,0,0,0,0,2);
	touchRect_t old;
	TouchLayout_AnchorRect(2048,1536,0,0,0,0,2,legacy[TB_LOCK].anchor,legacy[TB_LOCK].cx,legacy[TB_LOCK].cy,legacy[TB_LOCK].size,&old.x,&old.y,&old.w,&old.h);
	BOOST_REQUIRE(TouchEdit_Migrate(&o,2048,1536,0,0,0,0,2));
	BOOST_CHECK_SMALL(o.cx[TB_LOCK]*2048-old.x-old.w/2,.0003f);
	BOOST_CHECK_SMALL(o.cy[TB_LOCK]*1536-old.y-old.h/2,.0003f);
}

BOOST_AUTO_TEST_CASE( full_drawable_drag_console_sizes_and_rounding )
{
	for(const auto &f : {touchRect_t{2868,1320,3,0},touchRect_t{1434,660,1.5f,0},touchRect_t{2048,1536,2,0}}) {
		for(int button=0;button<TB_COUNT;++button) {
			touchLayoutOverrides_t o={};
			BOOST_REQUIRE(TouchEdit_Drag(&o,button,-1000,5000,f.x,f.y,40,10,50,30,f.w));
			char text[IOS_TOUCH_LAYOUT_TEXT]; BOOST_REQUIRE(TouchEdit_Serialize(&o,text,sizeof(text)));
			TouchEdit_Parse(text,&o);
			touchRect_t rects[TB_COUNT],defaults[TB_COUNT];
			TouchLayout_Build(f.x,f.y,40,10,50,30,f.w,rects,&o);
			TouchLayout_Build(f.x,f.y,40,10,50,30,f.w,defaults);
			BOOST_CHECK_SMALL(rects[button].x,.002f);
			BOOST_CHECK_SMALL(rects[button].y+rects[button].h-f.y,.002f);
			BOOST_CHECK_EQUAL(rects[button].w,defaults[button].w);
			BOOST_CHECK_EQUAL(rects[button].h,defaults[button].h);
		}
	}
	touchLayoutOverrides_t o={};
	BOOST_CHECK(!TouchEdit_Drag(&o,0,100,100,0,0,0,0,0,0,0));
	BOOST_CHECK(!TouchEdit_Drag(&o,TB_COUNT,100,100,2048,1536,0,0,0,0,2));
}

BOOST_AUTO_TEST_CASE( normalized_positions_survive_resize_without_safe_area_pull )
{
	touchLayoutOverrides_t o; TouchEdit_Parse("v2;10:0.01,0.01;11:0.99,0.01;0:0.7,0.6;",&o);
	for(const auto &f : {touchRect_t{2868,1320,3,0},touchRect_t{1434,660,1.5f,0},touchRect_t{1320,2868,3,0}}) {
		touchRect_t rects[TB_COUNT]; TouchLayout_Build(f.x,f.y,132,30,44,90,f.w,rects,&o);
		BOOST_CHECK_SMALL(rects[TB_MENU].x,.001f);
		BOOST_CHECK_SMALL(rects[TB_CONSOLE].x+rects[TB_CONSOLE].w-f.x,.001f);
		BOOST_CHECK_SMALL(rects[TB_ATTACK].x+rects[TB_ATTACK].w/2-.7f*f.x,.001f);
	}
	BOOST_CHECK_CLOSE(o.cx[TB_MENU],.01f,.001f);
}

BOOST_AUTO_TEST_CASE( lock_repairs_against_moved_console_and_centred_bar )
{
	const float fixtures[][7]={{2868,1320,132,0,132,102,3},{1434,660,66,0,66,51,1.5f},{2048,1536,0,0,0,0,2}};
	for(const auto &f:fixtures) {
		const auto bar=TouchSettings_BarRect(f[0],f[1],f[2],f[3],f[4],f[5],f[6]);
		for(int point=0;point<2;++point) {
			touchLayoutOverrides_t o={};
			BOOST_REQUIRE(TouchEdit_Drag(&o,TB_CONSOLE,bar.x+bar.w/2,bar.y+bar.h/2,f[0],f[1],f[2],f[3],f[4],f[5],f[6]));
			BOOST_REQUIRE(TouchEdit_Drag(&o,TB_LOCK,point?f[0]:bar.x+bar.w/2,point?0:bar.y+bar.h/2,f[0],f[1],f[2],f[3],f[4],f[5],f[6]));
			char text[IOS_TOUCH_LAYOUT_TEXT]; BOOST_REQUIRE(TouchEdit_Serialize(&o,text,sizeof(text))); TouchEdit_Parse(text,&o);
			touchRect_t rects[TB_COUNT]; TouchLayout_Build(f[0],f[1],f[2],f[3],f[4],f[5],f[6],rects,&o);
			BOOST_CHECK(Disjoint(rects[TB_LOCK],rects[TB_CONSOLE])); BOOST_CHECK(Disjoint(rects[TB_LOCK],bar));
			const float x=rects[TB_LOCK].x+rects[TB_LOCK].w/2,y=rects[TB_LOCK].y+rects[TB_LOCK].h/2;
			BOOST_CHECK_EQUAL(TouchLayout_Hit(rects,x,y,1u<<TB_LOCK),TB_LOCK);
		}
	}
}

BOOST_AUTO_TEST_CASE( editor_console_drag_and_lock_tap_capture )
{
	touchEditState_t state={}; touchEditInput_t in={}; in.event=TOUCH_EDIT_ENTER;
	BOOST_CHECK(TouchEdit_State(&state,in).release);
	in.event=TOUCH_EDIT_DOWN; in.button=TB_CONSOLE; in.finger=9; in.x=in.cx=100; in.y=in.cy=200; in.slop=6;
	TouchEdit_State(&state,in); BOOST_CHECK(state.dragging);
	in.event=TOUCH_EDIT_MOVE; in.finger=10; BOOST_CHECK(!TouchEdit_State(&state,in).move);
	in.finger=9; in.x=150; BOOST_CHECK(TouchEdit_State(&state,in).move);
	in.event=TOUCH_EDIT_UP; BOOST_CHECK(!TouchEdit_State(&state,in).save);
	in.event=TOUCH_EDIT_DOWN; in.button=TB_LOCK; TouchEdit_State(&state,in);
	in.event=TOUCH_EDIT_UP; BOOST_CHECK(TouchEdit_State(&state,in).save); BOOST_CHECK(!state.active);
	in.event=TOUCH_EDIT_ENTER; TouchEdit_State(&state,in); in.event=TOUCH_EDIT_EXIT;
	const auto out=TouchEdit_State(&state,in); BOOST_CHECK(out.save && out.zero && out.release);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
