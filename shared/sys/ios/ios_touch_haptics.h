#pragma once
#include "ios_touch_layout.h"

// Only button ids reach this; stick and look input never do.
IOS_TOUCH_INLINE int TouchHaptics_ShouldClick( int button, int down, int hapticsCvar )
{
	return down && hapticsCvar != 0 && button >= 0 && button < TB_COUNT;
}
