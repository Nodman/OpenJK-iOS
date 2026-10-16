#include "sys/ios/ios_touch_state.h"
#include "sys/ios/ios_touch_settings.h"
#include <boost/test/unit_test.hpp>
#include <limits>

BOOST_AUTO_TEST_SUITE(ios)
BOOST_AUTO_TEST_SUITE(touch_aim)

BOOST_AUTO_TEST_CASE(curve_contract)
{
	const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
	for(int curve=0;curve<6;++curve) {
		float previous=0;
		BOOST_CHECK_EQUAL(TouchAim_Response(0,curve),0);
		BOOST_CHECK_SMALL(TouchAim_Response(1,curve)-(curve==3?.6f:1),1e-6f);
		for(int i=0;i<=100;++i) {
			const float q=i*.01f,value=TouchAim_Response(q,curve);
			BOOST_CHECK_GE(value,previous);BOOST_CHECK_LE(value,1);previous=value;
		}
		BOOST_CHECK_EQUAL(TouchAim_Response(nan,curve),0);BOOST_CHECK_EQUAL(TouchAim_Response(inf,curve),0);
		float x,y;TouchStick_Response(8,0,100,curve,&x,&y);BOOST_CHECK_EQUAL(x,0);BOOST_CHECK_EQUAL(y,0);
		TouchStick_Response(60,80,100,curve,&x,&y);
		BOOST_CHECK_SMALL(x-.6f*(curve==3?.6f:1),1e-6f);BOOST_CHECK_SMALL(y-.8f*(curve==3?.6f:1),1e-6f);
	}
	BOOST_CHECK_SMALL(TouchAim_Response(.5f,0)-.5f,1e-6f);
	BOOST_CHECK_SMALL(TouchAim_Response(.5f,1)-sqrtf(.5f),1e-6f);
	BOOST_CHECK_SMALL(TouchAim_Response(.5f,2)-powf(.5f,1.6f),1e-6f);
	BOOST_CHECK_SMALL(TouchAim_Response(.5f,3)-.3f,1e-6f);
	BOOST_CHECK_EQUAL(TouchAim_Response(.00001f,4),1);
	BOOST_CHECK_SMALL(TouchAim_Response(.5f,5)-.5f,1e-6f);
	for(int curve : {-2,-1,6,200}) BOOST_CHECK_EQUAL(TouchAim_Response(.5f,curve),0);
	BOOST_CHECK_SMALL(TouchAim_Sensitivity(-1)-.1f,1e-6f);
	BOOST_CHECK_EQUAL(TouchAim_Sensitivity(11),10);
	BOOST_CHECK_EQUAL(TouchAim_Sensitivity(nan),1);BOOST_CHECK_EQUAL(TouchAim_Sensitivity(inf),1);
	BOOST_CHECK_SMALL(TouchAim_Sensitivity(1.17f)-1.17f,1e-6f);
}

static touchGestureInput_t Event(int type,int id,float x,float y,int target=TOUCH_TARGET_AIM)
{
	touchGestureInput_t in={};in.event=type;in.finger=id;in.x=x;in.y=y;in.target=target;
	in.aimMode=1;in.aimCurve=2;in.radius=100;in.slop=10;in.visibleButtons=~0u;in.moveEnabled=in.lookEnabled=1;
	return in;
}

BOOST_AUTO_TEST_CASE(owner_delta_refresh_release_and_promotion)
{
	touchGestureState_t state;TouchGesture_Reset(&state);
	TouchGesture_Step(&state,Event(TOUCH_DOWN,1,100,100));TouchGesture_Step(&state,Event(TOUCH_DOWN,2,200,200));
	auto out=TouchGesture_Step(&state,Event(TOUCH_MOTION,2,220,220));BOOST_CHECK_EQUAL(out.deltaX,0);
	out=TouchGesture_Step(&state,Event(TOUCH_MOTION,1,112.5f,97.25f));
	BOOST_CHECK_EQUAL(out.deltaX,12.5f);BOOST_CHECK_EQUAL(out.deltaY,-2.75f);BOOST_CHECK_EQUAL(out.lookX,0);BOOST_CHECK_EQUAL(out.lookY,0);
	for(int i=0;i<5;++i) {out=TouchGesture_Step(&state,Event(TOUCH_REFRESH,1,0,0));BOOST_CHECK_EQUAL(out.deltaX,0);}
	out=TouchGesture_Step(&state,Event(TOUCH_UP,1,112.5f,97.25f));BOOST_CHECK(!out.cancelLook);
	out=TouchGesture_Step(&state,Event(TOUCH_MOTION,2,240,240));BOOST_CHECK_EQUAL(out.deltaX,0);BOOST_CHECK_EQUAL(state.lookOwner,-1);
	TouchGesture_Step(&state,Event(TOUCH_DOWN,3,100,100,TOUCH_TARGET_FIRE));
	out=TouchGesture_Step(&state,Event(TOUCH_MOTION,3,110,100));BOOST_CHECK_EQUAL(state.lookOwner,-1);
	out=TouchGesture_Step(&state,Event(TOUCH_MOTION,3,111,100));BOOST_CHECK_GE(state.lookOwner,0);BOOST_CHECK_EQUAL(out.deltaX,0);
	out=TouchGesture_Step(&state,Event(TOUCH_MOTION,3,112.25f,100));BOOST_CHECK_EQUAL(out.deltaX,1.25f);
	out=TouchGesture_Step(&state,Event(TOUCH_CANCEL,3,0,0));BOOST_CHECK(out.cancelLook);BOOST_CHECK_EQUAL(out.keyCount,1);BOOST_CHECK(!out.keys[0].down);
}

static touchSettingsAction_t Tap(touchSettingsState_t &state,int control)
{
	touchSettingsLayout_t layout;TouchSettings_Build(&state,1,1024,768,0,0,0,0,1,&layout);
	for(int i=0;i<layout.count;++i) if(layout.items[i].control==control) {
		const auto r=layout.items[i].rect;const float x=r.x+r.w/2,y=r.y+r.h/2;
		TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,x,y);
		return TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,x,y);
	}
	BOOST_ERROR("Missing Aim choice");return {};
}
BOOST_AUTO_TEST_CASE(mode_specific_panel_choices_and_custom_scale)
{
	touchSettingsState_t state={};state.tab=TOUCH_SETTINGS_AIM;state.lookScale=1.17f;state.dragScale=2.33f;state.lookScaleY=.73f;state.dragScaleY=1.93f;state.aimCurve=2;
	touchSettingsLayout_t layout;TouchSettings_Build(&state,1,1024,768,0,0,0,0,1,&layout);
	for(int mode:{0,1}) {
		state.aimMode=mode;TouchSettings_Build(&state,1,1024,768,0,0,0,0,1,&layout);
		for(int i=0;i<layout.count;++i) if(layout.items[i].control==TOUCH_SETTINGS_SENSITIVITY || layout.items[i].control==TOUCH_SETTINGS_SENSITIVITY_Y) {
			const auto &item=layout.items[i];const auto r=item.rect;const int axis=item.control==TOUCH_SETTINGS_SENSITIVITY_Y;
			const float value=mode?(axis?1.93f:2.33f):(axis?.73f:1.17f);
			BOOST_CHECK_SMALL(item.fill-(value-.25f)/3.75f,1e-6f);
			BOOST_CHECK(strstr(item.label,axis?"Vertical:":"Horizontal:")!=NULL);
			for(float fraction:{0.f,.42f,1.f}) {
				const auto out=TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,r.x+r.w*fraction,r.y+r.h/2);
				BOOST_CHECK_EQUAL(out.aimSetting,2);BOOST_CHECK_EQUAL(out.aimMode,mode);BOOST_CHECK_EQUAL(out.aimAxis,axis);BOOST_CHECK_CLOSE(out.aimValue,.25f+fraction*3.75f,.001f);
				BOOST_CHECK_EQUAL(state.lookScale,1.17f);BOOST_CHECK_EQUAL(state.dragScale,2.33f);BOOST_CHECK_EQUAL(state.lookScaleY,.73f);BOOST_CHECK_EQUAL(state.dragScaleY,1.93f);
				TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,r.x,r.y);
			}
			TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,r.x,r.y);
			state.aimMode=!mode;
			BOOST_CHECK(!TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,1,r.x+r.w,r.y).aimSetting);
			state.aimMode=mode;
			BOOST_CHECK(!TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,r.x+r.w,r.y).aimSetting);
			TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_DOWN,1,r.x,r.y);
			state.tab=TOUCH_SETTINGS_APPEARANCE;
			BOOST_CHECK(!TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_MOVE,1,r.x+r.w,r.y).aimSetting);
			state.tab=TOUCH_SETTINGS_AIM;
			BOOST_CHECK(!TouchSettings_Step(&state,&layout,TOUCH_SETTINGS_UP,1,r.x+r.w,r.y).aimSetting);
		}
	}
	state.aimMode=0;
	for(int i=0;i<6;++i) {state.dropdown=3;const auto out=Tap(state,TOUCH_SETTINGS_CURVE_CHOICE+i);BOOST_CHECK_EQUAL(out.aimSetting,3);BOOST_CHECK_EQUAL(out.aimValue,i);}
	Tap(state,TOUCH_SETTINGS_MODE);const auto out=Tap(state,TOUCH_SETTINGS_MODE_CHOICE+1);BOOST_CHECK_EQUAL(out.aimSetting,1);BOOST_CHECK_EQUAL(out.aimValue,1);
	state.aimMode=1;state.dropdown=3;TouchSettings_Build(&state,1,1024,768,0,0,0,0,1,&layout);
	for(int i=0;i<layout.count;++i) {BOOST_CHECK_NE(layout.items[i].control,TOUCH_SETTINGS_CURVE);BOOST_CHECK(layout.items[i].control<TOUCH_SETTINGS_CURVE_CHOICE || layout.items[i].control>=TOUCH_SETTINGS_CURVE_CHOICE+6);}
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
