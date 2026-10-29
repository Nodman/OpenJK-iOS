#pragma once
#include "ios_touch_layout.h"
#include "ios_touch_settings.h"

// Only button ids reach this; stick and look input never do.
IOS_TOUCH_INLINE int TouchHaptics_ShouldClick( int button, int down, int hapticsCvar )
{
	return down && hapticsCvar != 0 && button >= 0 && button < TB_COUNT;
}

// Settings panel: first finger down on a control clicks; background, extra fingers, moves and lifts do not.
// The Haptics toggle clicks only through TouchHaptics_ToggleShouldClick, after it turns on.
IOS_TOUCH_INLINE int TouchHaptics_PanelShouldClick( int down, int consumed, int primary, int control, int hapticsCvar )
{
	return down && consumed && primary && control > 0 && control != TOUCH_SETTINGS_HAPTICS_TOGGLE && hapticsCvar != 0;
}

IOS_TOUCH_INLINE int TouchHaptics_ToggleShouldClick( int turnedOn, int hapticsCvar )
{
	return turnedOn && hapticsCvar != 0;
}
