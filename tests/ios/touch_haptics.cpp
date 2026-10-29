#include "sys/ios/ios_touch_haptics.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ios)
BOOST_AUTO_TEST_SUITE(touch_haptics)

BOOST_AUTO_TEST_CASE(press_clicks_release_does_not)
{
	for(int button=0;button<TB_COUNT;++button) {
		BOOST_CHECK(TouchHaptics_ShouldClick(button,1,1));
		BOOST_CHECK(!TouchHaptics_ShouldClick(button,0,1));
	}
}

BOOST_AUTO_TEST_CASE(cvar_zero_never_clicks)
{
	for(int button=0;button<TB_COUNT;++button) {
		BOOST_CHECK(!TouchHaptics_ShouldClick(button,1,0));
		BOOST_CHECK(!TouchHaptics_ShouldClick(button,0,0));
	}
	BOOST_CHECK(TouchHaptics_ShouldClick(TB_ATTACK,1,2));
}

// Stick and look input carry no button id, so they cannot reach a click.
BOOST_AUTO_TEST_CASE(only_button_ids_click)
{
	BOOST_CHECK(!TouchHaptics_ShouldClick(-1,1,1));
	BOOST_CHECK(!TouchHaptics_ShouldClick(TB_COUNT,1,1));
	BOOST_CHECK(TouchHaptics_ShouldClick(TB_LOCK,1,1));
	BOOST_CHECK(TouchHaptics_ShouldClick(TB_CONSOLE,1,1));
}

BOOST_AUTO_TEST_CASE(panel_clicks_on_first_finger_down_on_a_control)
{
	BOOST_CHECK(TouchHaptics_PanelShouldClick(1,1,1,1,1));
	BOOST_CHECK(TouchHaptics_PanelShouldClick(1,1,1,75,1));
	BOOST_CHECK(!TouchHaptics_PanelShouldClick(1,1,1,0,1));
	BOOST_CHECK(!TouchHaptics_PanelShouldClick(0,1,1,1,1));
	BOOST_CHECK(!TouchHaptics_PanelShouldClick(1,0,1,1,1));
	BOOST_CHECK(!TouchHaptics_PanelShouldClick(1,1,0,1,1));
	BOOST_CHECK(!TouchHaptics_PanelShouldClick(1,1,1,1,0));
}

BOOST_AUTO_TEST_CASE(toggle_clicks_only_when_turned_on)
{
	BOOST_CHECK(!TouchHaptics_PanelShouldClick(1,1,1,TOUCH_SETTINGS_HAPTICS_TOGGLE,1));
	BOOST_CHECK(TouchHaptics_PanelShouldClick(1,1,1,TOUCH_SETTINGS_HAPTICS,1));
	BOOST_CHECK(TouchHaptics_ToggleShouldClick(1,1));
	BOOST_CHECK(!TouchHaptics_ToggleShouldClick(0,0));
	BOOST_CHECK(!TouchHaptics_ToggleShouldClick(1,0));
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
