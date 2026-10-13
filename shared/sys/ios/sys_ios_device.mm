#include "sys_ios.h"
// Hook: P0c (T0c.0) replaces both bodies.
float Sys_IOS_DefaultRenderScale( qboolean highRefresh ) { return 1.0f; }
const char *Sys_IOS_DeviceDefaults( void ) { return ""; }
