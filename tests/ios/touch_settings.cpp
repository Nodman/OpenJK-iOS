#include "sys/ios/ios_touch_settings.h"
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

BOOST_AUTO_TEST_CASE( group_centres_and_done_remains_safe_when_expanded )
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
			if(f.x>500) {
				BOOST_CHECK_SMALL(layout.bar.x+layout.bar.w/2-f.x/2,.001f);
				const float bottom=tab?layout.panel.y+layout.panel.h:layout.bar.y+layout.bar.h;
				BOOST_CHECK_SMALL((layout.bar.y+bottom)/2-f.y/2,.001f);
			}
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
