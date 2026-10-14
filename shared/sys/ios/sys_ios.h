#pragma once
#include "qcommon/q_shared.h"	// per-app copy (codemp/ or code/) via include path

struct SDL_Window;
union SDL_Event;

// sys_ios_main.mm (P0b)
void Sys_IOS_EarlyInit( void );			// SDL hints; first line of main()
void Sys_IOS_PreInit( void );			// before Com_Init: lifecycle, documents, assets

// sys_ios_paths.mm (path P0b; rest P2)
const char *Sys_IOS_DocumentsPath( void );	// "<container>/Documents"
void Sys_IOS_PrepareDocuments( void );
void Sys_IOS_WaitForAssets( void );

// sys_ios_view.mm (P0b)
void Sys_IOS_SetDrawableScale( struct SDL_Window *window, float scale );
void Sys_IOS_GetViewPointSize( struct SDL_Window *window, float *width, float *height );
float Sys_IOS_GetNativeScale( struct SDL_Window *window );	// pixels per point of the screen
#define IOS_MIN_DRAWABLE_HEIGHT 540.0f	// Slide Over / small windows: render and lay out for at least this
// View-relative insets in drawable pixels; zeros until the view is attached and laid out.
void Sys_IOS_GetSafeAreaPixels( struct SDL_Window *window, float *left, float *top, float *right, float *bottom );
qboolean Sys_IOS_HardwareKeyboardConnected( void );
qboolean Sys_IOS_PointerConnected( void );

// sdl_window.cpp (P0b)
struct SDL_Window *GLimp_IOS_Window( void );

// sys_ios_device.mm (stubs P0b; filled P0c T0c.0; P3 adds a line)
float Sys_IOS_DefaultRenderScale( qboolean highRefresh );
const char *Sys_IOS_DeviceDefaults( void );

// sys_ios_pacing.mm (P0c; extended P3)
int Sys_IOS_MaxFramesPerSecond( void );
void Sys_IOS_StartFrameLoop( void );
void Sys_IOS_SetFramePaused( qboolean paused );

// sys_ios_lifecycle.mm (stub P0b; filled P6)
void Sys_IOS_InitLifecycle( void );

// ios_touch.cpp (stubs P0b; filled P4)
void IN_TouchInit( struct SDL_Window *window );
void IN_TouchShutdown( void );
void IN_TouchHandleEvent( const union SDL_Event *e );
void IN_TouchFrame( void );
void IN_TouchReleaseAll( void );
void IN_GamepadReleaseAll( void );
typedef struct { float x, y, w, h; const char *label; int pressed; } touchDrawItem_t;	// 640x480 virtual
int IN_TouchGetDrawList( touchDrawItem_t *items, int maxItems );

// ios_gamepad.cpp (P4)
void IN_GamepadZeroAxes( void );
void IN_GamepadInit( void );
void IN_GamepadShutdown( void );
void IN_GamepadHandleEvent( const union SDL_Event *e );
void IN_GamepadFrame( void );
qboolean IN_GamepadActive( void );
