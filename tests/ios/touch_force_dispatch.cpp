#include "sys/ios/ios_touch_force_dispatch.h"
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_SUITE(ios)
BOOST_AUTO_TEST_SUITE(force_dispatch)
BOOST_AUTO_TEST_CASE(reservation_and_packet_barriers) {
	touchForceDispatch_t state={};
	BOOST_CHECK(!TouchForce_Reserve(&state,1,IOS_TOUCH_FORCE_SOURCE_BASE,0,100,1));
	const auto generation=TouchForce_Reserve(&state,1,IOS_TOUCH_FORCE_SOURCE_BASE,0,100,0);
	BOOST_REQUIRE(generation);BOOST_CHECK(!TouchForce_Reserve(&state,2,IOS_TOUCH_FORCE_SOURCE_BASE+1,0,100,0));
	state.lifted=state.fenceDone=state.pendingPacket=1;TouchForce_Retire(&state);BOOST_CHECK(state.generation);
	TouchForce_PacketConsumed(&state);BOOST_CHECK(!state.generation);
	BOOST_CHECK(TouchForce_Reserve(&state,2,IOS_TOUCH_FORCE_SOURCE_BASE+1,0,101,0)!=generation);
}
BOOST_AUTO_TEST_CASE(pulse_provenance) {
	touchForcePulse_t pulse={};TouchForce_PulseDown(&pulse,0);TouchForce_PulseDown(&pulse,-1);
	BOOST_CHECK(TouchForce_PulseCancel(&pulse,0));BOOST_CHECK(pulse.other);BOOST_CHECK(!pulse.touch);
	BOOST_CHECK_EQUAL(TouchForce_SourceIndex(IOS_TOUCH_FORCE_SOURCE_BASE+3,300),16);
	BOOST_CHECK_EQUAL(TouchForce_SourceIndex(299,300),-1);
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
