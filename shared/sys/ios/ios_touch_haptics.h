#pragma once
#include "ios_touch_layout.h"

// Only button ids reach this; stick and look input never do.
IOS_TOUCH_INLINE int TouchHaptics_ShouldClick( int button, int down, int hapticsCvar )
{
	return down && hapticsCvar != 0 && button >= 0 && button < TB_COUNT;
}

// Settings panel: first finger down on a control clicks; background, extra fingers, moves and lifts do not.
IOS_TOUCH_INLINE int TouchHaptics_PanelShouldClick( int down, int consumed, int primary, int control, int hapticsCvar )
{
	return down && consumed && primary && control > 0 && hapticsCvar != 0;
}

IOS_TOUCH_INLINE int TouchHaptics_ToggleShouldClick( int turnedOn, int hapticsCvar )
{
	return turnedOn && hapticsCvar != 0;
}
