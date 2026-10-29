#import <UIKit/UIKit.h>
#include "sys_ios.h"

// UIKit feedback is a no-op on devices without a Taptic Engine (all iPads).
static UIImpactFeedbackGenerator *hapticGenerator;

static void Sys_IOS_HapticRun( bool click ) {
	if ( !hapticGenerator )
		hapticGenerator = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleRigid];
	if ( click )
		[hapticGenerator impactOccurred];
	[hapticGenerator prepare];
}

static void Sys_IOS_HapticOnMain( bool click ) {
	if ( [NSThread isMainThread] ) Sys_IOS_HapticRun( click );
	else dispatch_async( dispatch_get_main_queue(), ^{ Sys_IOS_HapticRun( click ); } );
}

void Sys_IOS_HapticInit( void ) { Sys_IOS_HapticOnMain( false ); }
void Sys_IOS_HapticClick( void ) { Sys_IOS_HapticOnMain( true ); }
