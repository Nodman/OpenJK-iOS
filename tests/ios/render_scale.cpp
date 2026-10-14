#include "qcommon/ios_render_scale.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( ios )
BOOST_AUTO_TEST_SUITE( render_scale )

BOOST_AUTO_TEST_CASE( auto_tiers_and_fixed_presets )
{
	const float bases[] = { 0.6f, 0.75f, 1.0f };
	const float expected[][2] = { { 0.6f, 0.5f }, { 0.75f, 0.6f }, { 1.0f, 0.8f } };
	for ( int tier = 0; tier < 3; ++tier ) {
		for ( int high = 0; high < 2; ++high ) {
			const iosRenderScale_t result = IOS_RenderScale_Resolve( 0, bases[tier], high, 2868, 1320 );
			BOOST_CHECK_SMALL( result.scale - expected[tier][high], 1e-6f );
			BOOST_CHECK_EQUAL( result.floorRaised, 0 );
			for ( int id = 1; id < 4; ++id ) {
				const float fixed = IOS_RenderScale_ForPreset( id );
				BOOST_CHECK_EQUAL( IOS_RenderScale_Resolve( fixed, bases[tier], high, 2868, 1320 ).scale, fixed );
			}
		}
	}
	BOOST_CHECK_EQUAL( IOS_RenderScale_Resolve( 0.1f, 1, 0, 2868, 1320 ).scale, 0.5f );
	BOOST_CHECK_EQUAL( IOS_RenderScale_Resolve( 2, 1, 0, 2868, 1320 ).scale, 1 );
	BOOST_CHECK_EQUAL( IOS_RenderScale_Resolve( -1, 0.75f, 0, 2868, 1320 ).scale, 0.75f );
}

BOOST_AUTO_TEST_CASE( minimum_drawable_height )
{
	const iosRenderScale_t raised = IOS_RenderScale_Resolve( 0, 0.5f, 0, 2000, 1000 );
	BOOST_CHECK_SMALL( raised.scale - 0.5405f, 1e-6f );
	BOOST_CHECK_EQUAL( raised.floorRaised, 1 );
	BOOST_CHECK_EQUAL( raised.width, 1081 );
	BOOST_CHECK_EQUAL( raised.height, 540 );
	const iosRenderScale_t small = IOS_RenderScale_Resolve( 0, 0.5f, 0, 1000, 500 );
	BOOST_CHECK_EQUAL( small.scale, 1 );
	BOOST_CHECK_EQUAL( small.floorRaised, 1 );
	BOOST_CHECK_EQUAL( small.width, 1000 );
	BOOST_CHECK_EQUAL( small.height, 500 );
	const iosRenderScale_t boundary = IOS_RenderScale_Resolve( 0.5f, 1, 0, 2000, 1080 );
	BOOST_CHECK_EQUAL( boundary.scale, 0.5f );
	BOOST_CHECK_EQUAL( boundary.floorRaised, 0 );
}

BOOST_AUTO_TEST_CASE( predicted_drawables )
{
	const int sizes[][2] = { { 2868, 1320 }, { 2151, 990 }, { 1434, 660 } };
	for ( int id = 1; id < 4; ++id ) {
		const iosRenderScale_t result = IOS_RenderScale_Resolve( IOS_RenderScale_ForPreset( id ), 0.6f, 1, 2868, 1320 );
		BOOST_CHECK_EQUAL( result.width, sizes[id - 1][0] );
		BOOST_CHECK_EQUAL( result.height, sizes[id - 1][1] );
	}
	const iosRenderScale_t fractional = IOS_RenderScale_Resolve( 0.75f, 1, 0, 2869, 1321 );
	BOOST_CHECK_EQUAL( fractional.width, 2151 );
	BOOST_CHECK_EQUAL( fractional.height, 990 );
}

BOOST_AUTO_TEST_CASE( preset_mapping_and_tolerance )
{
	for ( int id = 0; id < 4; ++id ) {
		BOOST_CHECK_EQUAL( IOS_RenderScale_PresetFromScale( IOS_RenderScale_ForPreset( id ) ), id );
	}
	BOOST_CHECK_EQUAL( IOS_RenderScale_ForPreset( -1 ), -1 );
	BOOST_CHECK_EQUAL( IOS_RenderScale_ForPreset( 4 ), -1 );
	BOOST_CHECK_EQUAL( IOS_RenderScale_PresetFromScale( -0.5f ), 0 );
	BOOST_CHECK_EQUAL( IOS_RenderScale_PresetFromScale( 0.9f ), 4 );
	BOOST_CHECK_EQUAL( IOS_RenderScale_PresetFromScale( 0.7499f ), 2 );
	BOOST_CHECK_EQUAL( IOS_RenderScale_PresetFromScale( 0.7491f ), 2 );
	BOOST_CHECK_EQUAL( IOS_RenderScale_PresetFromScale( 0.7509f ), 2 );
	BOOST_CHECK_EQUAL( IOS_RenderScale_PresetFromScale( 0.7489f ), 4 );
	BOOST_CHECK_EQUAL( IOS_RenderScale_PresetFromScale( 0.7511f ), 4 );
	BOOST_CHECK_EQUAL( IOS_RenderScale_PresetFromScale( 0.0001f ), 4 );
}

BOOST_AUTO_TEST_CASE( invalid_native_dimensions )
{
	const int sizes[][2] = { { 0, 1320 }, { 2868, 0 }, { -1, 500 }, { 1000, -1 }, { 0, 0 } };
	for ( const auto &size : sizes ) {
		const iosRenderScale_t result = IOS_RenderScale_Resolve( 0, 0.75f, 1, size[0], size[1] );
		BOOST_CHECK_SMALL( result.scale - 0.6f, 1e-6f );
		BOOST_CHECK_EQUAL( result.floorRaised, 0 );
		BOOST_CHECK_EQUAL( result.width, 0 );
		BOOST_CHECK_EQUAL( result.height, 0 );
	}
}

BOOST_AUTO_TEST_CASE( fixed_to_auto_independence )
{
	const iosRenderScale_t before = IOS_RenderScale_Resolve( 0, 0.75f, 1, 2868, 1320 );
	for ( int id = 1; id < 4; ++id ) {
		IOS_RenderScale_Resolve( IOS_RenderScale_ForPreset( id ), 0.75f, 1, 2868, 1320 );
		// Auto depends only on its inputs, regardless of the last fixed request.
		BOOST_CHECK_EQUAL( IOS_RenderScale_Resolve( 0, 0.75f, 1, 2868, 1320 ).scale, before.scale );
	}
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
