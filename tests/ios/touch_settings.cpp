#include "sys/ios/ios_touch_settings.h"
#include "sys/ios/ios_touch_layout_edit.h"
#include "touch_phone_fixture.h"
#include "touch_tablet_fixture.h"
#include <boost/test/unit_test.hpp>
#include <cstring>

BOOST_AUTO_TEST_SUITE( ios )
BOOST_AUTO_TEST_SUITE( touch_settings )

static touchSettingsLayout_t Build( const touchSettingsState_t &state,float W=2048,float H=1536,float p=2 )
{
	touchSettingsLayout_t layout;
	TouchSettings_Build(&state,.55f,W,H,20,10,40,30,p,&layout);
	return layout;
}

static touchRect_t Control( const touchSettingsLayout_t &layout,int id )
{
	for(int i=0;i<layout.count;++i) if(layout.items[i].control==id) return layout.items[i].rect;
	BOOST_ERROR("Missing panel control"); return {0,0,0,0};
}

static touchSettingsAction_t Tap( touchSettingsState_t &state,int control )
{
	const auto layout=Build(state);
	const auto r=Control(layout,control);
	const float x=r.x+r.w/2,y=r.y+r.h/2;
	BOOST_CHECK(TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,x,y).consumed);
	return TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,x,y);
}

BOOST_AUTO_TEST_CASE( group_starts_top_centre_and_done_remains_safe_when_expanded )
{
	for(const auto &f:{touchRect_t{2868,1320,3,0},touchRect_t{1434,660,1.5f,0},touchRect_t{2048,1536,2,0},touchRect_t{200,120,1,0}}) {
		touchSettingsState_t state={};
		for(int tab=0;tab<=3;++tab) {
			state.tab=tab; state.dropdown=1;
			const auto layout=Build(state,f.x,f.y,f.w);
			BOOST_CHECK_GE(layout.bar.x,20); BOOST_CHECK_GE(layout.bar.y,10);
			BOOST_CHECK_LE(layout.bar.x+layout.bar.w,f.x-40);
			BOOST_CHECK_LE(layout.bar.y+layout.bar.h,f.y-30);
			BOOST_CHECK_LE(layout.panel.y+layout.panel.h,f.y-30);
			const auto done=Control(layout,TOUCH_SETTINGS_DONE);
			BOOST_CHECK_GT(done.w,0); BOOST_CHECK_GT(done.h,0);
			BOOST_CHECK_EQUAL(layout.bar.y,10);
			if(f.x>500) BOOST_CHECK_SMALL(layout.bar.x+layout.bar.w/2-f.x/2,.001f);
		}
	}
}

BOOST_AUTO_TEST_CASE( row_slider_custom_values_endpoints_and_modal_capture )
{
	touchSettingsState_t state={};Tap(state,TOUCH_SETTINGS_APPEARANCE);
	const auto layout=Build(state);const auto r=Control(layout,TOUCH_SETTINGS_OPACITY);
	for(int i=0;i<layout.count;++i) if(layout.items[i].control==TOUCH_SETTINGS_OPACITY) {
		BOOST_CHECK_EQUAL(layout.items[i].label,"Opacity 55%");BOOST_CHECK_CLOSE(layout.items[i].fill,.55f,.001f);
	}
	BOOST_CHECK_EQUAL(state.dropdown,0);
	auto out=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,r.x+r.w*.37f,r.y+r.h/2);
	BOOST_CHECK(out.consumed && out.cancelDrag && out.setOpacity);BOOST_CHECK_CLOSE(out.opacity,.37f,.001f);
	BOOST_CHECK(TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,2,r.x,r.y).consumed);
	out=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,1,r.x-100,r.y-500);
	BOOST_CHECK(out.setOpacity);BOOST_CHECK_EQUAL(out.opacity,0);BOOST_CHECK_EQUAL(state.scroll,0);
	out=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,r.x+r.w+100,r.y+500);
	BOOST_CHECK(out.setOpacity);BOOST_CHECK_EQUAL(out.opacity,1);
	out=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,2,r.x+r.w,r.y);BOOST_CHECK(out.consumed && !out.setOpacity);
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,2,r.x,r.y);
	Tap(state,TOUCH_SETTINGS_APPEARANCE);BOOST_CHECK_EQUAL(state.tab,0);
	Tap(state,TOUCH_SETTINGS_AIM);BOOST_CHECK(Tap(state,TOUCH_SETTINGS_DONE).done);
}

BOOST_AUTO_TEST_CASE( reset_layout_requires_primary_true_tap_and_preserves_panel )
{
	touchSettingsState_t state={};state.tab=TOUCH_SETTINGS_APPEARANCE;
	const auto layout=Build(state);const auto r=Control(layout,TOUCH_SETTINGS_RESET_LAYOUT),opacity=Control(layout,TOUCH_SETTINGS_OPACITY);
	BOOST_CHECK_SMALL(r.y-opacity.y-opacity.h-4*layout.p,.001f);
	const float x=r.x+r.w/2,y=r.y+r.h/2;
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,x,y);
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,2,x,y);
	auto action=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,x,y);
	BOOST_CHECK(action.resetLayout && action.consumed && !action.done && !action.setOpacity);
	BOOST_CHECK_EQUAL(state.tab,TOUCH_SETTINGS_APPEARANCE);
	BOOST_CHECK(!TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,2,x,y).resetLayout);
	for(int moved=0;moved<2;++moved) {
		TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,3,x,y);
		if(moved) TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,3,x+20*layout.p,y);
		BOOST_CHECK(!TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,3,x+20*layout.p,y).resetLayout);
	}
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,4,x,y);
	BOOST_CHECK(!TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,4,0,0).resetLayout);
}

BOOST_AUTO_TEST_CASE( panel_gaps_capture_secondary_fingers_and_no_drag_leak )
{
	touchSettingsState_t state={}; state.tab=TOUCH_SETTINGS_APPEARANCE;
	const auto layout=Build(state);
	const float x=layout.bar.x+5,y=layout.bar.y+layout.bar.h+1;
	auto action=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,11,x,y);
	BOOST_CHECK(action.consumed && action.cancelDrag);
	BOOST_CHECK(TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,12,0,0).consumed);
	BOOST_CHECK(TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,11,0,0).consumed);
	BOOST_CHECK(TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,11,0,0).consumed);
	BOOST_CHECK(TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,12,0,0).consumed);
	BOOST_CHECK(!TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,13,0,0).consumed);
	BOOST_CHECK_EQUAL(state.tab,TOUCH_SETTINGS_APPEARANCE);
}

BOOST_AUTO_TEST_CASE( small_panel_scrolls_without_selecting_and_keeps_done )
{
	touchSettingsState_t state={}; state.tab=TOUCH_SETTINGS_FORCE; state.picker=1;state.learned=0x1ffe;
	const auto layout=Build(state,400,180,1);
	BOOST_CHECK_GT(layout.maxScroll,0);
	const float x=layout.panel.x+10,y=layout.panel.y+10;
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,x,y);
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,1,x,y-1000);
	BOOST_CHECK_GT(state.scroll,0);
	const auto action=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,x,y-1000);
	BOOST_CHECK(!action.setOpacity && !action.done);
	const auto scrolled=Build(state,400,180,1);
	bool lastVisible=false;
	for(int i=0;i<scrolled.count;++i) {
		BOOST_CHECK_GE(scrolled.items[i].rect.y,10);
		BOOST_CHECK_LE(scrolled.items[i].rect.y+scrolled.items[i].rect.h,150);
		if(scrolled.items[i].control==TOUCH_SETTINGS_POWER+12) lastVisible=true;
	}
	BOOST_CHECK(lastVisible);
	BOOST_CHECK_GT(Control(scrolled,TOUCH_SETTINGS_DONE).h,0);
}

BOOST_AUTO_TEST_CASE( short_joystick_panel_scrolls_four_rows_and_curve_choices )
{
	touchSettingsState_t state={};state.tab=TOUCH_SETTINGS_AIM;state.dropdown=3;
	const auto layout=Build(state,400,180,1);BOOST_CHECK_GT(layout.maxScroll,0);
	const float x=layout.panel.x+1,y=layout.panel.y+layout.panel.h-1;
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,x,y);
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,1,x,y-1000);
	const auto action=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,x,y-1000);
	BOOST_CHECK(!action.aimSetting && !action.done);
	const auto scrolled=Build(state,400,180,1);
	BOOST_CHECK_GT(Control(scrolled,TOUCH_SETTINGS_CURVE_CHOICE+5).h,0);
	BOOST_CHECK_GT(Control(scrolled,TOUCH_SETTINGS_DONE).h,0);
	for(int i=0;i<scrolled.count;++i) {BOOST_CHECK_GE(scrolled.items[i].rect.y,10);BOOST_CHECK_LE(scrolled.items[i].rect.y+scrolled.items[i].rect.h,150);}
}

BOOST_AUTO_TEST_CASE( sideways_drag_cancels_tap_and_secondary_never_promotes )
{
	touchSettingsState_t state={};
	const auto layout=Build(state);
	const auto r=Control(layout,TOUCH_SETTINGS_APPEARANCE);
	const float x=r.x+r.w/2,y=r.y+r.h/2;
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,x,y);
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,2,x,y);
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,1,x+20,y);
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,x,y);
	BOOST_CHECK_EQUAL(state.tab,0);
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,2,x,y);
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,2,x,y);
	BOOST_CHECK_EQUAL(state.tab,0);
	BOOST_CHECK_EQUAL(state.scroll,0);
}

BOOST_AUTO_TEST_CASE( bar_drag_moves_group_and_saves_on_lift )
{
	touchSettingsState_t state={};state.tab=TOUCH_SETTINGS_APPEARANCE;
	auto layout=Build(state);
	const auto tab=Control(layout,TOUCH_SETTINGS_AIM);
	const float x=tab.x+tab.w/2,y=tab.y+tab.h/2,barX=layout.bar.x,barY=layout.bar.y,offset=layout.panel.y-layout.bar.y;
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,x,y);
	auto action=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,1,x+5*layout.p,y);
	BOOST_CHECK(!action.movePanel && !state.panelPlaced);
	for(int step=1;step<=4;++step) {
		action=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,1,x-30*step,y+40*step);
		BOOST_CHECK(action.movePanel && !action.savePanel);
		layout=Build(state);
	}
	BOOST_CHECK_SMALL(layout.bar.x-(barX-120),.01f);BOOST_CHECK_SMALL(layout.bar.y-(barY+160),.01f);
	BOOST_CHECK_SMALL(layout.panel.y-layout.bar.y-offset,.001f);BOOST_CHECK_EQUAL(layout.panel.x,layout.bar.x);
	BOOST_CHECK_EQUAL(state.scroll,0);
	action=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,x-120,y+160);
	BOOST_CHECK(action.savePanel && !action.done);
	BOOST_CHECK_EQUAL(state.tab,TOUCH_SETTINGS_APPEARANCE);
	char text[32];TouchSettings_FormatPos(&state,text,sizeof(text));
	touchSettingsState_t loaded={};loaded.tab=TOUCH_SETTINGS_APPEARANCE;
	BOOST_REQUIRE(TouchSettings_ParsePos(text,&loaded.panelX,&loaded.panelY));loaded.panelPlaced=1;
	const auto restored=Build(loaded);
	BOOST_CHECK_SMALL(restored.bar.x-layout.bar.x,.5f);BOOST_CHECK_SMALL(restored.bar.y-layout.bar.y,.5f);
	const auto moved=Control(layout,TOUCH_SETTINGS_AIM);
	BOOST_CHECK(!Tap(state,TOUCH_SETTINGS_AIM).savePanel);
	BOOST_CHECK_EQUAL(state.tab,TOUCH_SETTINGS_AIM);
	BOOST_CHECK_EQUAL(Control(Build(state),TOUCH_SETTINGS_AIM).x,moved.x);
}

BOOST_AUTO_TEST_CASE( bar_drag_clamps_inside_safe_area )
{
	for(int tab:{0,(int)TOUCH_SETTINGS_FORCE}) {
		touchSettingsState_t state={};state.tab=tab;state.picker=1;state.learned=0x1ffe;
		auto layout=Build(state);
		const float x=layout.bar.x+10,y=layout.bar.y+10;
		const float ends[][2]={{-5000,-5000},{5000,5000},{-5000,5000},{5000,-5000}};
		for(const auto &end:ends) {
			layout=Build(state);
			TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,layout.bar.x+10,layout.bar.y+10);
			TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,1,x+end[0],y+end[1]);
			TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,x+end[0],y+end[1]);
			BOOST_CHECK(state.panelPlaced);
			BOOST_CHECK(state.panelX>=0 && state.panelX<=1 && state.panelY>=0 && state.panelY<=1);
			layout=Build(state);
			BOOST_CHECK_GE(layout.bar.x,20-.01f);BOOST_CHECK_GE(layout.bar.y,10-.01f);
			BOOST_CHECK_LE(layout.bar.x+layout.bar.w,2048-40+.01f);
			BOOST_CHECK_LE(layout.panel.y+layout.panel.h,1536-30+.01f);
			if(end[0]<0) BOOST_CHECK_SMALL(layout.bar.x-20,.01f); else BOOST_CHECK_SMALL(layout.bar.x+layout.bar.w-(2048-40),.01f);
			if(end[1]<0) BOOST_CHECK_SMALL(layout.bar.y-10,.01f);
			else BOOST_CHECK_SMALL((tab?layout.panel.y+layout.panel.h:layout.bar.y+layout.bar.h)-(1536-30),.01f);
		}
	}
	touchSettingsState_t tall={};tall.tab=TOUCH_SETTINGS_FORCE;tall.picker=1;tall.learned=0x1ffe;tall.panelPlaced=1;tall.panelX=.5f;tall.panelY=1;
	const auto layout=Build(tall,400,180,1);
	BOOST_CHECK_EQUAL(layout.bar.y,10);BOOST_CHECK_GT(layout.maxScroll,0);
	BOOST_CHECK_LE(layout.panel.y+layout.panel.h,150);
}

BOOST_AUTO_TEST_CASE( panel_body_drag_scrolls_and_does_not_move )
{
	touchSettingsState_t state={};state.tab=TOUCH_SETTINGS_FORCE;state.picker=1;state.learned=0x1ffe;
	const auto layout=Build(state,400,180,1);
	const float x=layout.panel.x+10,y=layout.panel.y+10;
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,x,y);
	BOOST_CHECK(!TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,1,x+50,y-40).movePanel);
	BOOST_CHECK(!TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,x+50,y-40).savePanel);
	BOOST_CHECK_GT(state.scroll,0);BOOST_CHECK(!state.panelPlaced);
}

BOOST_AUTO_TEST_CASE( saved_position_parse_rejects_bad_values )
{
	float x=-1,y=-1;
	BOOST_CHECK(TouchSettings_ParsePos("0.2500,0.7500",&x,&y));BOOST_CHECK_EQUAL(x,.25f);BOOST_CHECK_EQUAL(y,.75f);
	BOOST_CHECK(TouchSettings_ParsePos("0,1",&x,&y));
	for(const char *bad:{"","0.5","0.5,","0.5;0.5","0.5,0.5x","1.5,0.5","0.5,-0.1","nan,0.5","0.5,inf","abc"}) {
		x=y=-1;BOOST_CHECK(!TouchSettings_ParsePos(bad,&x,&y));BOOST_CHECK_EQUAL(x,-1);
	}
	BOOST_CHECK(!TouchSettings_ParsePos(NULL,&x,&y));
	touchSettingsState_t state={};state.panelX=.123456f;state.panelY=1;
	char text[32];TouchSettings_FormatPos(&state,text,sizeof(text));
	BOOST_CHECK_EQUAL(text,"0.1235,1.0000");
}

BOOST_AUTO_TEST_CASE( default_group_clears_lock_and_camera )
{
	touchLayoutOverrides_t phoneSaved,tabletSaved;
	TouchEdit_Parse(touchPhoneLayoutFixture,&phoneSaved);TouchEdit_Parse(touchTabletLayoutFixture,&tabletSaved);
	struct { float W,H,L,T,R,B; touchLayoutProfile_t profile; const touchLayoutOverrides_t *saved; } screens[]={
		{956,440,44,0,44,21,TOUCH_LAYOUT_PHONE,NULL},{956,440,44,0,44,21,TOUCH_LAYOUT_PHONE,&phoneSaved},
		{1194,834,0,24,0,20,TOUCH_LAYOUT_TABLET,NULL},{1194,834,0,24,0,20,TOUCH_LAYOUT_TABLET,&tabletSaved},
		{1194,834,0,0,0,0,TOUCH_LAYOUT_TABLET,NULL}};
	for(const auto &s:screens) for(float p:{1.f,2.f,3.f}) {
		touchRect_t rects[TB_COUNT];
		TouchLayout_Build(s.W*p,s.H*p,s.L*p,s.T*p,s.R*p,s.B*p,p,rects,s.saved,s.profile);
		for(int tab=0;tab<=4;++tab) for(int open=0;open<2;++open) {
			touchSettingsState_t state={};state.tab=tab;state.learned=0x1ffe;
			if(open) { state.picker=1;state.dropdown=3; }
			touchSettingsLayout_t layout;
			TouchSettings_Build(&state,1,s.W*p,s.H*p,s.L*p,s.T*p,s.R*p,s.B*p,p,&layout);
			touchRect_t group=layout.bar;
			if(layout.panel.h>0) group.h=layout.panel.y+layout.panel.h-group.y;
			BOOST_CHECK_SMALL(layout.bar.y-s.T*p,.001f);
			for(int button:{TB_LOCK,TB_CAMERA}) {
				const touchRect_t &r=rects[button];
				const bool overlap=r.x<group.x+group.w && group.x<r.x+r.w && r.y<group.y+group.h && group.y<r.y+r.h;
				BOOST_CHECK_MESSAGE(!overlap,"panel covers button "<<button<<" W="<<s.W<<" p="<<p<<" tab="<<tab);
			}
		}
	}
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(ios)
BOOST_AUTO_TEST_SUITE(force_picker)
BOOST_AUTO_TEST_CASE(slot_tap_then_empty_assignment) {
	touchSettingsState_t state={};state.tab=TOUCH_SETTINGS_FORCE;state.learned=1u<<1;state.slots[0]=1;
	touchSettingsLayout_t layout;
	TouchSettings_Build(&state,.5f,844,390,0,0,0,0,1,&layout);
	for(int control: {TOUCH_SETTINGS_SLOT,TOUCH_SETTINGS_POWER}) {
		TouchSettings_Build(&state,.5f,844,390,0,0,0,0,1,&layout);
		touchRect_t rect={};
		for(int i=0;i<layout.count;++i) if(layout.items[i].control==control) rect=layout.items[i].rect;
		BOOST_REQUIRE(rect.w>0 && rect.h>0);
		TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,7,rect.x+rect.w/2,rect.y+rect.h/2);
		const auto action=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,7,rect.x+rect.w/2,rect.y+rect.h/2);
		if(control==TOUCH_SETTINGS_SLOT) BOOST_CHECK_EQUAL(state.picker,1);
		else { BOOST_CHECK(action.assign);BOOST_CHECK_EQUAL(action.slot,0);BOOST_CHECK_EQUAL(action.power,0);BOOST_CHECK_EQUAL(state.picker,0); }
	}
}
BOOST_AUTO_TEST_CASE(learned_only_modal_choices) {
	touchSettingsState_t state={};state.tab=TOUCH_SETTINGS_FORCE;state.picker=1;
	state.learned=(1u<<1)|(1u<<12);state.slots[0]=1;
	touchSettingsLayout_t layout;TouchSettings_Build(&state,.5f,844,390,0,0,0,0,1,&layout);
	int choices=0;
	for(int i=0;i<layout.count;++i) {
		const auto &item=layout.items[i];
		if(item.control>=TOUCH_SETTINGS_POWER && item.control<=TOUCH_SETTINGS_POWER+12) {
			BOOST_CHECK(item.control==TOUCH_SETTINGS_POWER || item.control==TOUCH_SETTINGS_POWER+1 || item.control==TOUCH_SETTINGS_POWER+12);++choices;
		}
	}
	BOOST_CHECK_EQUAL(choices,3);
	state.learned=0;memset(state.slots,0,sizeof(state.slots));
	TouchSettings_Build(&state,.5f,844,390,0,0,0,0,1,&layout);
	for(int i=0;i<layout.count;++i) { BOOST_CHECK_EQUAL(layout.items[i].force,0);BOOST_CHECK(!strstr(layout.items[i].label,"Push")); }
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
