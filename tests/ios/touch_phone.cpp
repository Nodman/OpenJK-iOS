#include "sys/ios/ios_touch_layout_edit.h"
#include "touch_phone_fixture.h"
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <cstring>

BOOST_AUTO_TEST_SUITE(ios)
BOOST_AUTO_TEST_SUITE(touch_phone)



static bool Disjoint(const touchRect_t &a,const touchRect_t &b) {
	return a.x+a.w<=b.x+.001f || b.x+b.w<=a.x+.001f || a.y+a.h<=b.y+.001f || b.y+b.h<=a.y+.001f;
}
static float X(const touchRect_t &r) {return r.x+r.w/2;}
static float Y(const touchRect_t &r) {return r.y+r.h/2;}

BOOST_AUTO_TEST_CASE(reference_matches_approved_cleanup_and_keeps_protected_centers) {
	const float expected[TB_COUNT][2]={
		{745.416144f,214.533000f},{671.001104f,299.040280f},{851.568472f,331.220120f},{897.210780f,401.733200f},
		{104.169584f,398.733280f},{860.848045f,139.422360f},{928.006806f,139.422360f},{812.705160f,406.133200f},
		{793.689284f,58.444467f},{928.006806f,58.444467f},{112.013701f,33.283250f},{57.996424f,33.283250f},
		{860.848045f,58.444467f},{220.048256f,33.283250f},{166.030979f,33.283250f},{34.135892f,296.779120f},
		{793.689284f,139.422360f},{111.695933f,117.866540f},{192.302507f,117.866540f},
		{172.908135f,181.033380f},{253.514709f,181.033380f},{627.935216f,399.039960f},{34.135892f,350.796417f}};
	touchRect_t phone[TB_COUNT],tablet[TB_COUNT];
	TouchLayout_Build(2868,1320,132,0,132,102,3,phone,NULL,TOUCH_LAYOUT_PHONE);
	TouchLayout_Build(2868,1320,132,0,132,102,3,tablet);
	for(int i=0;i<TB_COUNT;++i) {
		BOOST_CHECK_SMALL(X(phone[i])-expected[i][0]*3,.002f);BOOST_CHECK_SMALL(Y(phone[i])-expected[i][1]*3,.002f);
		BOOST_CHECK_EQUAL(phone[i].w,tablet[i].w);BOOST_CHECK_EQUAL(phone[i].h,tablet[i].h);
		touchRect_t editor;TouchEdit_PositionRect(NULL,i,2868,1320,132,0,132,102,3,&editor,TOUCH_LAYOUT_PHONE);
		BOOST_CHECK_EQUAL(editor.x,phone[i].x);BOOST_CHECK_EQUAL(editor.y,phone[i].y);
	}
	touchLayoutOverrides_t source;TouchEdit_Parse(touchPhoneLayoutFixture,&source);
	const int fixed[9]={TB_ATTACK,TB_ALTATTACK,TB_JUMP,TB_CROUCH,TB_STYLE,TB_USE,TB_LOCK,TB_BOTHATTACK,TB_CAMERA};
	for(float p:{1.5f,2.25f,3.f}) for(float bottom:{0.f,21.f,34.f}) {
		TouchLayout_Build(956*p,440*p,44*p,0,44*p,bottom*p,p,phone,NULL,TOUCH_LAYOUT_PHONE);
		for(int i:fixed) {BOOST_CHECK_SMALL(X(phone[i])/p-source.cx[i]*956,.001f);BOOST_CHECK_SMALL(Y(phone[i])/p-source.cy[i]*440,.001f);}
	}
}

BOOST_AUTO_TEST_CASE(compiled_phone_matrix_and_documented_320_point_stress) {
	const int sizes[7][2]={{956,440},{932,430},{844,390},{812,375},{667,375},{667,320},{568,320}};
	const float insets[4][4]={{44,0,44,34},{62,0,0,21},{0,0,62,21},{0,0,0,0}};
	int cases=0,stress=0;
	for(const auto &size:sizes) for(float native:{2.f,3.f}) for(float render:{1.f,.75f,.5f}) for(const auto &safe:insets) {
		const float p=native*render,W=size[0]*p,H=size[1]*p;
		touchRect_t rects[TB_COUNT];TouchLayout_Build(W,H,safe[0]*p,safe[1]*p,safe[2]*p,safe[3]*p,p,rects,NULL,TOUCH_LAYOUT_PHONE);
		const bool limited=size[1]==320 && native==2 && render==.5f;
		++cases;if(limited) ++stress;
		for(int i=0;i<TB_COUNT;++i) {
			const auto &r=rects[i];
			BOOST_CHECK(std::isfinite(r.x) && std::isfinite(r.y));
			BOOST_CHECK_GE(r.x,-.001f);BOOST_CHECK_GE(r.y,-.001f);BOOST_CHECK_LE(r.x+r.w,W+.001f);BOOST_CHECK_LE(r.y+r.h,H+.001f);
			BOOST_CHECK_EQUAL(TouchLayout_Hit(rects,X(r),Y(r),1u<<i),i);
			for(int j=0;j<i;++j) {
				const bool known=limited && ((i==TB_CROUCH && j==TB_JUMP) || (i==TB_ALTATTACK && j==TB_ATTACK));
				BOOST_CHECK_MESSAGE(Disjoint(r,rects[j])!=known,"pair "<<i<<"/"<<j<<" in "<<size[0]<<"x"<<size[1]<<" native "<<native<<" render "<<render);
			}
		}
		const int tools[4]={TB_CONSOLE,TB_MENU,TB_PAD,TB_CHAT};float gap=-1;
		for(int i=1;i<4;++i) {
			const float next=rects[tools[i]].x-rects[tools[i-1]].x-rects[tools[i-1]].w;
			BOOST_CHECK_GE(next,0);
			if(i>1) BOOST_CHECK_SMALL(next-gap,.001f);gap=next;
			BOOST_CHECK_SMALL(Y(rects[tools[i]])-Y(rects[tools[0]]),.001f);
		}
		for(int i=0;i<3;++i) {const int top[3]={TB_WEAPPREV,TB_SABER_SELECT,TB_WEAPNEXT},bottom[3]={TB_FORCEPREV,TB_FORCE,TB_FORCENEXT};BOOST_CHECK_SMALL(X(rects[top[i]])-X(rects[bottom[i]]),.001f);}
		BOOST_CHECK_SMALL(Y(rects[TB_FORCE_SLOT1])-Y(rects[TB_FORCE_SLOT2]),.001f);
		BOOST_CHECK_SMALL(Y(rects[TB_FORCE_SLOT3])-Y(rects[TB_FORCE_SLOT4]),.001f);
		BOOST_CHECK_SMALL((X(rects[TB_FORCE_SLOT2])-X(rects[TB_FORCE_SLOT1]))-(X(rects[TB_FORCE_SLOT4])-X(rects[TB_FORCE_SLOT3])),.001f);
		BOOST_CHECK_SMALL((Y(rects[TB_FORCE_SLOT3])-Y(rects[TB_FORCE_SLOT1]))-(Y(rects[TB_FORCE_SLOT4])-Y(rects[TB_FORCE_SLOT2])),.001f);
	}
	BOOST_CHECK_EQUAL(cases,168);BOOST_CHECK_EQUAL(stress,8);
}

BOOST_AUTO_TEST_CASE(saved_v2_centers_survive_profiles_and_partial_lock_does_not_migrate) {
	touchLayoutOverrides_t saved;TouchEdit_Parse(touchPhoneLayoutFixture,&saved);const auto original=saved;
	for(auto profile:{TOUCH_LAYOUT_TABLET,TOUCH_LAYOUT_PHONE}) for(float p:{1.5f,3.f}) {
		touchRect_t r[TB_COUNT];TouchLayout_Build(956*p,440*p,44*p,0,44*p,21*p,p,r,&saved,profile);
		for(int i=0;i<TB_COUNT;++i) {BOOST_CHECK_SMALL(X(r[i])-saved.cx[i]*956*p,.002f);BOOST_CHECK_SMALL(Y(r[i])-saved.cy[i]*440*p,.002f);}
	}
	BOOST_CHECK_EQUAL(std::memcmp(&original,&saved,sizeof(saved)),0);
	touchRect_t defaults[TB_COUNT],custom[TB_COUNT];TouchLayout_Build(2868,1320,132,0,132,102,3,defaults,NULL,TOUCH_LAYOUT_PHONE);
	touchLayoutOverrides_t partial={};partial.version=2;partial.mask=1u<<TB_LOCK;
	partial.cx[TB_LOCK]=X(defaults[TB_CONSOLE])/2868;partial.cy[TB_LOCK]=Y(defaults[TB_CONSOLE])/1320;
	TouchLayout_Build(2868,1320,132,0,132,102,3,custom,&partial,TOUCH_LAYOUT_PHONE);
	BOOST_CHECK_SMALL(X(custom[TB_LOCK])-X(defaults[TB_CONSOLE]),.001f);BOOST_CHECK_SMALL(Y(custom[TB_LOCK])-Y(defaults[TB_CONSOLE]),.001f);
	BOOST_CHECK(!Disjoint(custom[TB_LOCK],custom[TB_CONSOLE]));
	BOOST_REQUIRE(TouchEdit_Drag(&partial,TB_LOCK,X(defaults[TB_CONSOLE]),Y(defaults[TB_CONSOLE]),2868,1320,132,0,132,102,3,TOUCH_LAYOUT_PHONE));
	TouchLayout_Build(2868,1320,132,0,132,102,3,custom,&partial,TOUCH_LAYOUT_PHONE);
	BOOST_CHECK(Disjoint(custom[TB_LOCK],custom[TB_CONSOLE]));
	const auto bar=TouchSettings_BarRect(2868,1320,132,0,132,102,3);BOOST_CHECK(Disjoint(custom[TB_LOCK],bar));
}

BOOST_AUTO_TEST_CASE(older_layouts_keep_both_attacks_clear_of_edited_buttons) {
	touchRect_t defaults[TB_COUNT];TouchLayout_Build(2868,1320,132,0,132,102,3,defaults,NULL,TOUCH_LAYOUT_PHONE);
	touchLayoutOverrides_t saved={};saved.version=2;saved.mask=1u<<TB_ATTACK;
	saved.cx[TB_ATTACK]=X(defaults[TB_BOTHATTACK])/2868;saved.cy[TB_ATTACK]=Y(defaults[TB_BOTHATTACK])/1320;
	touchRect_t r[TB_COUNT];TouchLayout_Build(2868,1320,132,0,132,102,3,r,&saved,TOUCH_LAYOUT_PHONE);
	BOOST_CHECK_SMALL(X(r[TB_ATTACK])-X(defaults[TB_BOTHATTACK]),.001f);
	for(int i=0;i<TB_COUNT;++i) if(i!=TB_BOTHATTACK) BOOST_CHECK_MESSAGE(Disjoint(r[TB_BOTHATTACK],r[i]),"both attacks overlaps "<<i);
	BOOST_CHECK_GE(r[TB_BOTHATTACK].x,0);BOOST_CHECK_GE(r[TB_BOTHATTACK].y,0);
	saved.mask|=1u<<TB_BOTHATTACK;saved.cx[TB_BOTHATTACK]=saved.cx[TB_ATTACK];saved.cy[TB_BOTHATTACK]=saved.cy[TB_ATTACK];
	TouchLayout_Build(2868,1320,132,0,132,102,3,r,&saved,TOUCH_LAYOUT_PHONE);
	BOOST_CHECK(!Disjoint(r[TB_BOTHATTACK],r[TB_ATTACK]));
}

BOOST_AUTO_TEST_CASE(older_layouts_keep_camera_clear_of_edited_buttons) {
	for(auto profile:{TOUCH_LAYOUT_PHONE,TOUCH_LAYOUT_TABLET}) {
		touchRect_t defaults[TB_COUNT];TouchLayout_Build(2868,1320,132,0,132,102,3,defaults,NULL,profile);
		touchLayoutOverrides_t saved={};saved.version=2;saved.mask=1u<<TB_USE;
		saved.cx[TB_USE]=X(defaults[TB_CAMERA])/2868;saved.cy[TB_USE]=Y(defaults[TB_CAMERA])/1320;
		touchRect_t r[TB_COUNT];TouchLayout_Build(2868,1320,132,0,132,102,3,r,&saved,profile);
		for(int i=0;i<TB_COUNT;++i) if(i!=TB_CAMERA) BOOST_CHECK_MESSAGE(Disjoint(r[TB_CAMERA],r[i]),"camera overlaps "<<i);
		saved.mask|=1u<<TB_CAMERA;saved.cx[TB_CAMERA]=saved.cx[TB_USE];saved.cy[TB_CAMERA]=saved.cy[TB_USE];
		TouchLayout_Build(2868,1320,132,0,132,102,3,r,&saved,profile);
		BOOST_CHECK(!Disjoint(r[TB_CAMERA],r[TB_USE]));
	}
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
