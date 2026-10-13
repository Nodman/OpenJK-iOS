#include "sys_ios.h"
#include <SDL.h>

void Sys_IOS_EarlyInit( void ) {
	SDL_SetHint( SDL_HINT_IOS_HIDE_HOME_INDICATOR, "2" );
	SDL_SetHint( SDL_HINT_TOUCH_MOUSE_EVENTS, "0" );
	SDL_SetHint( SDL_HINT_MOUSE_TOUCH_EVENTS, "0" );
	SDL_SetHint( SDL_HINT_IDLE_TIMER_DISABLED, "1" );
	SDL_SetHint( SDL_HINT_AUDIO_CATEGORY, "AVAudioSessionCategorySoloAmbient" );
}

void Sys_IOS_PreInit( void ) {
	Sys_IOS_InitLifecycle();
	Sys_IOS_PrepareDocuments();
	Sys_IOS_WaitForAssets();
}
