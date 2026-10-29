#include "qcommon/aspect_layout.h"

#include <boost/test/unit_test.hpp>

#include <cmath>

BOOST_AUTO_TEST_SUITE( ios )
BOOST_AUTO_TEST_SUITE( aspect_layout )

BOOST_AUTO_TEST_CASE( wide_drawables )
{
	const int dimensions[][2] = { { 2868, 1320 }, { 1434, 660 }, { 2151, 990 } };
	const double scales[] = { 2.75, 1.375, 2.0625 };
	const int offsets[] = { 554, 277, 416 };
	const double widths[] = { 1760, 880, 1320 };
	for ( int i = 0; i < 3; ++i ) {
		const aspectLayout_t layout = AspectLayout_Build( dimensions[i][0], dimensions[i][1], 1, 1.5f, 0, 0, 0, 0 );
		BOOST_CHECK_EQUAL( layout.branch, ASPECT_CORRECTED );
		BOOST_CHECK_EQUAL( layout.W, dimensions[i][0] );
		BOOST_CHECK_EQUAL( layout.H, dimensions[i][1] );
		BOOST_CHECK_EQUAL( layout.s, scales[i] );
		BOOST_CHECK_EQUAL( layout.ox, offsets[i] );
		BOOST_CHECK_EQUAL( layout.oy, 0 );
		const aspectPoint_t size = AspectLayout_SizeToPixels( &layout, 640, 480 );
		BOOST_CHECK_EQUAL( size.x, widths[i] );
		BOOST_CHECK_EQUAL( size.y, dimensions[i][1] );
		const aspectPoint_t start = AspectLayout_ToPixels( &layout, 0, 0 );
		const aspectPoint_t end = AspectLayout_ToPixels( &layout, 640, 480 );
		BOOST_CHECK_EQUAL( start.x, offsets[i] );
		BOOST_CHECK_EQUAL( end.x, offsets[i] + widths[i] );
		BOOST_CHECK_EQUAL( dimensions[i][0] - end.x, i == 2 ? 415 : offsets[i] );
		BOOST_CHECK_SMALL( layout.left + offsets[i] / scales[i], 1e-9 );
		BOOST_CHECK_SMALL( layout.right - ( dimensions[i][0] - offsets[i] ) / scales[i], 1e-9 );
		BOOST_CHECK_EQUAL( layout.top, 0 );
		BOOST_CHECK_EQUAL( layout.bottom, 480 );
		const aspectPoint_t fullStart = AspectLayout_ToPixels( &layout, layout.fullRect.x, layout.fullRect.y );
		const aspectPoint_t fullSize = AspectLayout_SizeToPixels( &layout, layout.fullRect.w, layout.fullRect.h );
		BOOST_CHECK_SMALL( fullStart.x, 1e-9 );
		BOOST_CHECK_SMALL( fullStart.y, 1e-9 );
		BOOST_CHECK_SMALL( fullSize.x - dimensions[i][0], 1e-9 );
		BOOST_CHECK_SMALL( fullSize.y - dimensions[i][1], 1e-9 );
	}
}

BOOST_AUTO_TEST_CASE( legacy_gate )
{
	const int fixtures[][3] = { { 1024, 768, 1 }, { 2388, 1668, 1 }, { 2420, 1668, 1 }, { 2868, 1320, 0 }, { 899, 600, 1 } };
	for ( const auto &fixture : fixtures ) {
		const aspectLayout_t layout = AspectLayout_Build( fixture[0], fixture[1], fixture[2], 1.5f, 132, 10, 20, 30 );
		BOOST_CHECK_EQUAL( layout.branch, ASPECT_LEGACY );
		BOOST_CHECK_EQUAL( layout.sx, fixture[0] / 640.0 );
		BOOST_CHECK_EQUAL( layout.sy, fixture[1] / 480.0 );
		BOOST_CHECK_EQUAL( layout.ox, 0 );
		BOOST_CHECK_EQUAL( layout.oy, 0 );
		BOOST_CHECK_EQUAL( layout.left, 0 );
		BOOST_CHECK_EQUAL( layout.right, 640 );
		BOOST_CHECK_EQUAL( layout.top, 0 );
		BOOST_CHECK_EQUAL( layout.bottom, 480 );
		BOOST_CHECK_EQUAL( layout.fullRect.x, 0 );
		BOOST_CHECK_EQUAL( layout.fullRect.y, 0 );
		BOOST_CHECK_EQUAL( layout.fullRect.w, 640 );
		BOOST_CHECK_EQUAL( layout.fullRect.h, 480 );
		BOOST_CHECK_EQUAL( layout.leftTranslation, 0 );
		BOOST_CHECK_EQUAL( layout.rightTranslation, 0 );
		BOOST_CHECK_EQUAL( layout.topTranslation, 0 );
		BOOST_CHECK_EQUAL( layout.bottomTranslation, 0 );
		const aspectPoint_t end = AspectLayout_ToPixels( &layout, 640, 480 );
		const aspectPoint_t size = AspectLayout_SizeToPixels( &layout, 640, 480 );
		BOOST_CHECK_EQUAL( end.x, fixture[0] );
		BOOST_CHECK_EQUAL( end.y, fixture[1] );
		BOOST_CHECK_EQUAL( size.x, fixture[0] );
		BOOST_CHECK_EQUAL( size.y, fixture[1] );
	}
	BOOST_CHECK_EQUAL( AspectLayout_Build( 900, 600, 1, 1.5f, 0, 0, 0, 0 ).branch, ASPECT_CORRECTED );
	BOOST_CHECK_EQUAL( AspectLayout_Build( 1024, 768, 1, 0, 0, 0, 0, 0 ).branch, ASPECT_LEGACY );
	BOOST_CHECK_EQUAL( AspectLayout_Build( 640, 600, 1, 1.5f, 0, 0, 0, 0 ).branch, ASPECT_LEGACY );
}

BOOST_AUTO_TEST_CASE( gutter_rects )
{
	aspectRect_t gutters[4];
	const aspectLayout_t wide = AspectLayout_Build( 1434, 660, 1, 1.5f, 0, 0, 0, 0 );
	BOOST_REQUIRE_EQUAL( AspectLayout_GutterRects( &wide, gutters ), 2 );
	BOOST_CHECK_SMALL( gutters[0].x + 277 / 1.375, 1e-9 );
	BOOST_CHECK_SMALL( gutters[0].w - 277 / 1.375, 1e-9 );
	BOOST_CHECK_SMALL( gutters[0].x + gutters[0].w, 1e-9 );
	BOOST_CHECK_EQUAL( gutters[1].x, 640 );
	BOOST_CHECK_SMALL( gutters[1].w - 277 / 1.375, 1e-9 );
	BOOST_CHECK_SMALL( gutters[1].x + gutters[1].w - wide.fullRect.x - wide.fullRect.w, 1e-9 );
	for ( int i = 0; i < 2; ++i ) {
		BOOST_CHECK_EQUAL( gutters[i].y, wide.fullRect.y );
		BOOST_CHECK_EQUAL( gutters[i].h, wide.fullRect.h );
	}

	const aspectLayout_t narrow = AspectLayout_Build( 640, 600, 1, 0, 0, 0, 0, 0 );
	BOOST_REQUIRE_EQUAL( AspectLayout_GutterRects( &narrow, gutters ), 2 );
	BOOST_CHECK_EQUAL( gutters[0].y, -60 );
	BOOST_CHECK_EQUAL( gutters[0].h, 60 );
	BOOST_CHECK_EQUAL( gutters[0].y + gutters[0].h, 0 );
	BOOST_CHECK_EQUAL( gutters[1].y, 480 );
	BOOST_CHECK_EQUAL( gutters[1].h, 60 );
	BOOST_CHECK_EQUAL( gutters[1].y + gutters[1].h, narrow.fullRect.y + narrow.fullRect.h );
	for ( int i = 0; i < 2; ++i ) {
		BOOST_CHECK_EQUAL( gutters[i].x, narrow.fullRect.x );
		BOOST_CHECK_EQUAL( gutters[i].w, narrow.fullRect.w );
	}

	const aspectLayout_t identity = AspectLayout_Build( 1024, 768, 1, 1.5f, 0, 0, 0, 0 );
	BOOST_CHECK_EQUAL( AspectLayout_GutterRects( &identity, gutters ), 0 );
	const aspectLayout_t legacy = AspectLayout_Build( 2388, 1668, 1, 1.5f, 0, 0, 0, 0 );
	BOOST_CHECK_EQUAL( AspectLayout_GutterRects( &legacy, gutters ), 0 );
}

BOOST_AUTO_TEST_CASE( gutter_half_ties )
{
	const aspectLayout_t wide = AspectLayout_Build( 603, 402, 1, 1.5f, 0, 0, 0, 0 );
	BOOST_CHECK_EQUAL( wide.branch, ASPECT_CORRECTED );
	BOOST_CHECK_SMALL( wide.s - 402 / 480.0, 1e-12 );
	BOOST_CHECK_EQUAL( wide.ox, 34 );
	BOOST_CHECK_EQUAL( wide.oy, 0 );
	const aspectPoint_t wideEnd = AspectLayout_ToPixels( &wide, 640, 480 );
	BOOST_CHECK_SMALL( 603 - wideEnd.x - 33, 1e-6 );
	BOOST_CHECK_SMALL( wideEnd.y - 402, 1e-6 );
	const aspectLayout_t narrow = AspectLayout_Build( 640, 603, 1, 0, 0, 0, 0, 0 );
	BOOST_CHECK_EQUAL( narrow.branch, ASPECT_CORRECTED );
	BOOST_CHECK_SMALL( narrow.s - 1, 1e-12 );
	BOOST_CHECK_EQUAL( narrow.ox, 0 );
	BOOST_CHECK_EQUAL( narrow.oy, 62 );
	const aspectPoint_t narrowEnd = AspectLayout_ToPixels( &narrow, 640, 480 );
	BOOST_CHECK_SMALL( narrowEnd.x - 640, 1e-6 );
	BOOST_CHECK_SMALL( 603 - narrowEnd.y - 61, 1e-6 );
}

BOOST_AUTO_TEST_CASE( large_integer_gate )
{
	BOOST_CHECK_EQUAL( AspectLayout_Build( 2000000000, 1500000000, 1, 0, 0, 0, 0, 0 ).branch, ASPECT_LEGACY );
	BOOST_CHECK_EQUAL( AspectLayout_Build( 1800000000, 1200000000, 1, 1.5f, 0, 0, 0, 0 ).branch, ASPECT_CORRECTED );
	BOOST_CHECK_EQUAL( AspectLayout_Build( 1799999999, 1200000000, 1, 1.5f, 0, 0, 0, 0 ).branch, ASPECT_LEGACY );
	BOOST_CHECK_EQUAL( AspectLayout_Build( 1600, 1000, 1, 1.6f, 0, 0, 0, 0 ).branch, ASPECT_CORRECTED );
	BOOST_CHECK_EQUAL( AspectLayout_Build( 1700, 1000, 1, 1.7f, 0, 0, 0, 0 ).branch, ASPECT_CORRECTED );
	BOOST_CHECK_EQUAL( AspectLayout_Build( 1599, 1000, 1, 1.6f, 0, 0, 0, 0 ).branch, ASPECT_LEGACY );
}

BOOST_AUTO_TEST_CASE( invalid_dimensions )
{
	const int dimensions[][2] = { { 0, 0 }, { 0, 480 }, { 640, 0 }, { -640, 480 }, { 640, -480 }, { -640, -480 } };
	for ( const auto &dimension : dimensions ) {
		const aspectLayout_t layout = AspectLayout_Build( dimension[0], dimension[1], 1, 0, 10, 20, 30, 40 );
		BOOST_CHECK_EQUAL( layout.branch, ASPECT_LEGACY );
		const aspectPoint_t pixel = AspectLayout_ToPixels( &layout, 320, 240 );
		const aspectPoint_t size = AspectLayout_SizeToPixels( &layout, 640, 480 );
		const aspectPoint_t virtualSize = AspectLayout_SizeToVirtual( &layout, 100, 200 );
		const aspectPoint_t point = AspectLayout_ToVirtual( &layout, 100, 200 );
		const aspectPoint_t cursor = AspectLayout_ToVirtualCursor( &layout, 100, 200 );
		const double values[] = { layout.s, layout.sx, layout.sy, layout.left, layout.right, layout.top, layout.bottom,
			layout.fullRect.x, layout.fullRect.y, layout.fullRect.w, layout.fullRect.h,
			layout.leftTranslation, layout.rightTranslation, layout.topTranslation, layout.bottomTranslation,
			pixel.x, pixel.y, size.x, size.y, virtualSize.x, virtualSize.y, point.x, point.y, cursor.x, cursor.y };
		for ( double value : values ) {
			BOOST_CHECK( std::isfinite( value ) );
		}
	}
}

BOOST_AUTO_TEST_CASE( narrow_override )
{
	const aspectLayout_t layout = AspectLayout_Build( 640, 600, 1, 0, 0, 10, 0, 20 );
	BOOST_CHECK_EQUAL( layout.branch, ASPECT_CORRECTED );
	BOOST_CHECK_EQUAL( layout.s, 1 );
	BOOST_CHECK_EQUAL( layout.ox, 0 );
	BOOST_CHECK_EQUAL( layout.oy, 60 );
	BOOST_CHECK_EQUAL( layout.left, 0 );
	BOOST_CHECK_EQUAL( layout.right, 640 );
	BOOST_CHECK_EQUAL( layout.top, -60 );
	BOOST_CHECK_EQUAL( layout.bottom, 540 );
	BOOST_CHECK_EQUAL( layout.fullRect.x, 0 );
	BOOST_CHECK_EQUAL( layout.fullRect.y, -60 );
	BOOST_CHECK_EQUAL( layout.fullRect.w, 640 );
	BOOST_CHECK_EQUAL( layout.fullRect.h, 600 );
	const aspectPoint_t pixel = AspectLayout_ToPixels( &layout, 320, 240 );
	BOOST_CHECK_EQUAL( pixel.x, 320 );
	BOOST_CHECK_EQUAL( pixel.y, 300 );
	const aspectPoint_t point = AspectLayout_ToVirtual( &layout, 320, 300 );
	BOOST_CHECK_EQUAL( point.x, 320 );
	BOOST_CHECK_EQUAL( point.y, 240 );
	BOOST_CHECK_EQUAL( layout.topTranslation, -50 );
	BOOST_CHECK_EQUAL( layout.bottomTranslation, 40 );
	BOOST_CHECK_EQUAL( AspectLayout_ToPixels( &layout, 0, layout.topTranslation ).y, 10 );
	BOOST_CHECK_EQUAL( AspectLayout_ToPixels( &layout, 0, 480 + layout.bottomTranslation ).y, 580 );
	const aspectLayout_t half = AspectLayout_Build( 640, 601, 1, 0, 0, 0, 0, 0 );
	BOOST_CHECK_EQUAL( half.oy, 61 );
	BOOST_CHECK_EQUAL( 601 - AspectLayout_ToPixels( &half, 640, 480 ).y, 60 );
}

BOOST_AUTO_TEST_CASE( asymmetric_insets )
{
	const aspectLayout_t left = AspectLayout_Build( 2868, 1320, 1, 1.5f, 132, 10, 0, 20 );
	const aspectLayout_t right = AspectLayout_Build( 2868, 1320, 1, 1.5f, 0, 10, 132, 20 );
	BOOST_CHECK_SMALL( left.leftTranslation + 422 / 2.75, 1e-9 );
	BOOST_CHECK_SMALL( left.rightTranslation - 554 / 2.75, 1e-9 );
	BOOST_CHECK_SMALL( right.leftTranslation + 554 / 2.75, 1e-9 );
	BOOST_CHECK_SMALL( right.rightTranslation - 422 / 2.75, 1e-9 );
	BOOST_CHECK_SMALL( left.leftTranslation + right.rightTranslation, 1e-9 );
	BOOST_CHECK_SMALL( left.rightTranslation + right.leftTranslation, 1e-9 );
	BOOST_CHECK_EQUAL( left.topTranslation, right.topTranslation );
	BOOST_CHECK_EQUAL( left.bottomTranslation, right.bottomTranslation );
	BOOST_CHECK_SMALL( left.topTranslation - 10 / 2.75, 1e-9 );
	BOOST_CHECK_SMALL( left.bottomTranslation + 20 / 2.75, 1e-9 );
	BOOST_CHECK_EQUAL( left.s, right.s );
	BOOST_CHECK_EQUAL( left.ox, right.ox );
	BOOST_CHECK_EQUAL( left.oy, right.oy );
	BOOST_CHECK_SMALL( AspectLayout_ToPixels( &left, left.leftTranslation, 0 ).x - 132, 1e-6 );
	BOOST_CHECK_SMALL( AspectLayout_ToPixels( &left, 640 + left.rightTranslation, 0 ).x - 2868, 1e-6 );
	BOOST_CHECK_SMALL( AspectLayout_ToPixels( &right, right.leftTranslation, 0 ).x, 1e-6 );
	BOOST_CHECK_SMALL( AspectLayout_ToPixels( &right, 640 + right.rightTranslation, 0 ).x - 2736, 1e-6 );
}

BOOST_AUTO_TEST_CASE( round_trips_and_cursor_clamp )
{
	const aspectLayout_t layouts[] = {
		AspectLayout_Build( 2151, 990, 1, 1.5f, 0, 0, 0, 0 ),
		AspectLayout_Build( 2868, 1320, 0, 1.5f, 0, 0, 0, 0 ),
		AspectLayout_Build( 640, 600, 1, 0, 0, 0, 0, 0 )
	};
	const aspectPoint_t points[] = { { 0, 0 }, { 640, 480 }, { 320.125, 240.375 }, { -100.5, -30.25 }, { 750.25, 510.5 } };
	for ( const auto &layout : layouts ) {
		for ( const auto &point : points ) {
			const aspectPoint_t pixel = AspectLayout_ToPixels( &layout, point.x, point.y );
			const aspectPoint_t restored = AspectLayout_ToVirtual( &layout, pixel.x, pixel.y );
			const aspectPoint_t cursor = AspectLayout_ToVirtualCursor( &layout, pixel.x, pixel.y );
			BOOST_CHECK_SMALL( restored.x - point.x, 1e-6 );
			BOOST_CHECK_SMALL( restored.y - point.y, 1e-6 );
			BOOST_CHECK_SMALL( cursor.x - fmin( 640, fmax( 0, point.x ) ), 1e-6 );
			BOOST_CHECK_SMALL( cursor.y - fmin( 480, fmax( 0, point.y ) ), 1e-6 );
		}
		const aspectPoint_t bounds = AspectLayout_ToVirtual( &layout, 0, 0 );
		BOOST_CHECK_SMALL( bounds.x - layout.left, 1e-6 );
		BOOST_CHECK_SMALL( bounds.y - layout.top, 1e-6 );
		const aspectPoint_t end = AspectLayout_ToVirtual( &layout, layout.W, layout.H );
		BOOST_CHECK_SMALL( end.x - layout.right, 1e-6 );
		BOOST_CHECK_SMALL( end.y - layout.bottom, 1e-6 );
	}
}

BOOST_AUTO_TEST_CASE( size_round_trips )
{
	const aspectLayout_t layouts[] = {
		AspectLayout_Build( 603, 402, 1, 1.5f, 0, 0, 0, 0 ),
		AspectLayout_Build( 2868, 1320, 0, 1.5f, 0, 0, 0, 0 ),
		AspectLayout_Build( 640, 603, 1, 0, 0, 0, 0, 0 )
	};
	const aspectPoint_t sizes[] = { { 0, 0 }, { 640, 480 }, { 12.125, 23.375 } };
	for ( const auto &layout : layouts ) {
		for ( const auto &size : sizes ) {
			const aspectPoint_t pixels = AspectLayout_SizeToPixels( &layout, size.x, size.y );
			const aspectPoint_t restored = AspectLayout_SizeToVirtual( &layout, pixels.x, pixels.y );
			BOOST_CHECK_SMALL( restored.x - size.x, 1e-6 );
			BOOST_CHECK_SMALL( restored.y - size.y, 1e-6 );
		}
	}
	const aspectLayout_t zeroWidth = AspectLayout_Build( 0, 480, 1, 0, 0, 0, 0, 0 );
	const aspectPoint_t widthSize = AspectLayout_SizeToVirtual( &zeroWidth, 100, 200 );
	BOOST_CHECK_EQUAL( widthSize.x, 0 );
	BOOST_CHECK_EQUAL( widthSize.y, 200 );
	const aspectLayout_t zeroHeight = AspectLayout_Build( 640, 0, 1, 0, 0, 0, 0, 0 );
	const aspectPoint_t heightSize = AspectLayout_SizeToVirtual( &zeroHeight, 100, 200 );
	BOOST_CHECK_EQUAL( heightSize.x, 100 );
	BOOST_CHECK_EQUAL( heightSize.y, 0 );
}

BOOST_AUTO_TEST_CASE( sharp_text_scale )
{
	const int dimensions[][2] = { { 2868, 1320 }, { 1434, 660 }, { 1024, 768 } };
	const float expected[] = { 0.5f, 1.0f, 0.859375f };
	for ( int enabled = 0; enabled <= 1; ++enabled ) {
		for ( int i = 0; i < 3; ++i ) {
			const aspectLayout_t layout = AspectLayout_Build( dimensions[i][0], dimensions[i][1], enabled, 1.5f, 0, 0, 0, 0 );
			BOOST_CHECK_SMALL( AspectLayout_SharpTextScale( &layout, 16, 22 ) - expected[i], 1e-6f );
		}
	}
	const aspectLayout_t invalid = AspectLayout_Build( 640, 0, 1, 1.5f, 0, 0, 0, 0 );
	BOOST_CHECK_EQUAL( AspectLayout_SharpTextScale( &invalid, 16, 22 ), 1 );
	const aspectLayout_t tall = AspectLayout_Build( 6400, 4800, 1, 1.5f, 0, 0, 0, 0 );
	BOOST_CHECK_EQUAL( AspectLayout_SharpTextScale( &tall, 16, 22 ), 0.2f );
	BOOST_CHECK_EQUAL( AspectLayout_SharpTextScale( &tall, 0, 22 ), 1 );
	BOOST_CHECK_EQUAL( AspectLayout_SharpTextScale( NULL, 16, 22 ), 1 );
}

BOOST_AUTO_TEST_CASE( cover_rect )
{
	const aspectLayout_t wide = AspectLayout_Build( 2868, 1320, 1, 1.5f, 0, 0, 0, 0 );
	const aspectRect_t cover = AspectLayout_CoverRect( &wide );
	BOOST_CHECK_SMALL( cover.x - wide.fullRect.x, 1e-9 );
	BOOST_CHECK_SMALL( cover.w - wide.fullRect.w, 1e-9 );
	BOOST_CHECK_SMALL( cover.h - wide.fullRect.w * 0.75, 1e-9 );
	BOOST_CHECK_SMALL( cover.y + cover.h / 2 - 240, 1e-9 );
	BOOST_CHECK_LT( cover.y, 0 );

	const aspectLayout_t legacy = AspectLayout_Build( 2868, 1320, 0, 1.5f, 0, 0, 0, 0 );
	const aspectRect_t same = AspectLayout_CoverRect( &legacy );
	BOOST_CHECK_EQUAL( same.x, 0 );
	BOOST_CHECK_EQUAL( same.y, 0 );
	BOOST_CHECK_EQUAL( same.w, 640 );
	BOOST_CHECK_EQUAL( same.h, 480 );
}

BOOST_AUTO_TEST_CASE( fov_conversion )
{
	const double pi = acos( -1.0 );
	const double v4 = 2 * atan( tan( 40 * pi / 180 ) / ( 4.0 / 3.0 ) ) * 180 / pi;
	const double horizontal = AspectLayout_HorizontalFov( 80, 1434, 660 );
	const double vertical = AspectLayout_VerticalFov( horizontal, 1434, 660 );
	BOOST_CHECK_SMALL( horizontal - 107.64092087473904, 0.01 );
	BOOST_CHECK_SMALL( vertical - 64.3664437730972, 0.01 );
	BOOST_CHECK_SMALL( vertical - v4, 0.01 );
	BOOST_CHECK_EQUAL( AspectLayout_HorizontalFov( 80, 1024, 768 ), 80 );
	BOOST_CHECK_SMALL( AspectLayout_VerticalFov( 80, 1024, 768 ) - v4, 0.01 );
	BOOST_CHECK_EQUAL( AspectLayout_HorizontalFov( 80, 2868, 1320 ), horizontal );
	BOOST_CHECK_EQUAL( AspectLayout_HorizontalFov( 80, 2000000000, 1500000000 ), 80 );
	const double tablet = AspectLayout_HorizontalFov( 80, 2388, 1668 );
	BOOST_CHECK_SMALL( tablet - 84.03616577789342, 0.01 );
	BOOST_CHECK_SMALL( AspectLayout_VerticalFov( tablet, 2388, 1668 ) - v4, 0.01 );
	BOOST_CHECK_EQUAL( AspectLayout_HorizontalFov( 80, 0, 480 ), 80 );
	BOOST_CHECK_EQUAL( AspectLayout_VerticalFov( 80, 640, 0 ), 80 );
	BOOST_CHECK_EQUAL( AspectLayout_HorizontalFov( 80, -640, 480 ), 80 );
	BOOST_CHECK_EQUAL( AspectLayout_VerticalFov( 80, 640, -480 ), 80 );
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
