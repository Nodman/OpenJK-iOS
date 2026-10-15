#include "sys/ios/ios_touch_force_media.h"
#include "ios/touch/touch_atlas.h"
#include <boost/test/unit_test.hpp>
#include <cmath>

BOOST_AUTO_TEST_SUITE(ios)
BOOST_AUTO_TEST_SUITE(touch_force)
BOOST_AUTO_TEST_CASE(draw_identity_and_original_colour) {
	const aspectLayout_t layout=AspectLayout_Build(2796,1290,1,1.5,0,0,0,0);
	touchRect_t rects[TB_COUNT]={};
	for(int i=0;i<TB_COUNT;++i) rects[i]={100.0f+i*10,100,80,80};
	touchGestureState_t gestures={};
	touchDrawInput_t input={};
	input.layout=&layout;input.rects=rects;input.gestures=&gestures;
	input.visibleButtons=(1u<<TB_FORCE)|(1u<<TB_FORCEPREV)|(1u<<TB_FORCENEXT);
	input.selectedForce=4;
	touchDrawItem_t items[IOS_TOUCH_MAX_DRAW_ITEMS];
	const int count=TouchDraw_Build(&input,items,IOS_TOUCH_MAX_DRAW_ITEMS);
	BOOST_REQUIRE_EQUAL(count,3);
	for(int i=0;i<count;++i) {
		BOOST_CHECK_EQUAL(items[i].force,items[i].button==TB_FORCE?4:0);
		BOOST_CHECK_EQUAL(TouchDraw_Backdrop(&items[i]),-1);
		float rgba[4];TouchDraw_Color(&items[i],1,.6f,rgba);
		BOOST_CHECK_EQUAL(rgba[0],1);BOOST_CHECK_EQUAL(rgba[1],1);BOOST_CHECK_EQUAL(rgba[2],1);
		BOOST_CHECK_CLOSE(rgba[3],.51f,.001f);
	}
}
BOOST_AUTO_TEST_CASE(retail_quad_stays_inside_physical_circle) {
	const int fixtures[][2]={{2796,1290},{2388,1668},{1024,768},{844,390}};
	for(const auto &fixture:fixtures) for(int corrected=0;corrected<2;++corrected) {
		const aspectLayout_t layout=AspectLayout_Build(fixture[0],fixture[1],corrected,1.5,0,0,0,0);
		const auto origin=AspectLayout_ToVirtual(&layout,100,200);
		const auto size=AspectLayout_SizeToVirtual(&layout,80,80);
		const auto q=TouchForceMedia_Quad(origin.x,origin.y,size.x,size.y);
		const auto centre=AspectLayout_ToPixels(&layout,q.x+q.w/2,q.y+q.h/2);
		BOOST_CHECK_SMALL(centre.x-140,.0001);BOOST_CHECK_SMALL(centre.y-240,.0001);
		const auto physical=AspectLayout_SizeToPixels(&layout,q.w,q.h);
		BOOST_CHECK(std::hypot(physical.x/2,physical.y/2)<=40.00001);
		BOOST_CHECK_CLOSE(physical.x,physical.y,.001);
	}
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
