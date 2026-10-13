#include "sys_ios.h"
// Hooks: P4 (T4.3) replaces these bodies.
void IN_TouchInit( struct SDL_Window * ) {}
void IN_TouchShutdown( void ) {}
void IN_TouchHandleEvent( const union SDL_Event * ) {}
void IN_TouchFrame( void ) {}
void IN_TouchReleaseAll( void ) {}
int IN_TouchGetDrawList( touchDrawItem_t *, int ) { return 0; }
