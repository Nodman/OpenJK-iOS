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

BOOST_AUTO_TEST_CASE( retap_collapse_dropdown_choices_and_custom_opacity )
{
	touchSettingsState_t state={}; Tap(state,TOUCH_SETTINGS_APPEARANCE);
	BOOST_CHECK_EQUAL(state.tab,TOUCH_SETTINGS_APPEARANCE);
	const auto layout=Build(state);
	bool found=false;
	for(int i=0;i<layout.count;++i) if(layout.items[i].control==TOUCH_SETTINGS_OPACITY) {
		found=true; BOOST_CHECK_EQUAL(layout.items[i].label,"Opacity 55%");
	}
	BOOST_CHECK(found);
	Tap(state,TOUCH_SETTINGS_OPACITY); BOOST_CHECK(state.dropdown);
	for(int percent=20;percent<=100;percent+=10) {
		if(!state.dropdown) Tap(state,TOUCH_SETTINGS_OPACITY);
		BOOST_CHECK_EQUAL(Tap(state,TOUCH_SETTINGS_PERCENT+(percent-20)/10).opacity,percent);
		BOOST_CHECK(!state.dropdown);
	}
	Tap(state,TOUCH_SETTINGS_APPEARANCE); BOOST_CHECK_EQUAL(state.tab,0);
	Tap(state,TOUCH_SETTINGS_AIM); BOOST_CHECK_EQUAL(state.tab,TOUCH_SETTINGS_AIM);
	BOOST_CHECK(Tap(state,TOUCH_SETTINGS_DONE).done);
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
	touchSettingsState_t state={}; state.tab=TOUCH_SETTINGS_APPEARANCE; state.dropdown=1;
	const auto layout=Build(state,400,180,1);
	BOOST_CHECK_GT(layout.maxScroll,0);
	const float x=layout.panel.x+10,y=layout.panel.y+10;
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,x,y);
	TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,1,x,y-100);
	BOOST_CHECK_GT(state.scroll,0);
	const auto action=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,x,y-100);
	BOOST_CHECK(!action.opacity && !action.done);
	const auto scrolled=Build(state,400,180,1);
	bool lastVisible=false;
	for(int i=0;i<scrolled.count;++i) {
		BOOST_CHECK_GE(scrolled.items[i].rect.y,10);
		BOOST_CHECK_LE(scrolled.items[i].rect.y+scrolled.items[i].rect.h,150);
		if(scrolled.items[i].control==TOUCH_SETTINGS_PERCENT+8) lastVisible=true;
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
