#include "stdafx.h"
#include "Rect.h"
#include "catch.hpp"

namespace bmf
{
	TEST_CASE("Rect works properly", "[BitmapFontCache]")
	{
		SECTION("Create rect")
		{
			Rect newRect(0, 0, 1024, 512);
			REQUIRE(newRect.width() == 1024);
			REQUIRE(newRect.height() == 512);
			REQUIRE(newRect.left() == 0);
			REQUIRE(newRect.top() == 0);
			
			Rect newRect2;
			REQUIRE(newRect2.width() == 0);
			REQUIRE(newRect2.height() == 0);
			REQUIRE(newRect2.left() == 0);
			REQUIRE(newRect2.top() == 0);

			Rect newRect3(-256, -256, 1024, 512);
			REQUIRE(newRect3.width() == 1024);
			REQUIRE(newRect3.height() == 512);
			REQUIRE(newRect3.left() == -256);
			REQUIRE(newRect3.top() == -256);
		}

		SECTION("surface")
		{
			Rect newRect(0, 0, 1024, 1024);
			REQUIRE(newRect.surface() == (1024 * 1024));

			Rect newRect2;
			REQUIRE(newRect2.surface() == 0);
		}

		SECTION("Right / bottom")
		{
			Rect newRect(0, 0, 1024, 1024);
			REQUIRE(newRect.right() == 1024);
			REQUIRE(newRect.bottom() == 1024);

			Rect newRect2;
			REQUIRE(newRect2.right() == 0);
			REQUIRE(newRect2.bottom() == 0);

			Rect newRect3(-256, -256, 1024, 1024);
			REQUIRE(newRect3.right() == 768);
			REQUIRE(newRect3.bottom() == 768);
		}

		SECTION("isSmallerOrEqualThan")
		{
			Rect newRect1(-1, -1, 2, 2);
			REQUIRE(newRect1.isSmallerOrEqualThan(newRect1));

			Rect newRect2(-4, -4, 8, 8);
			REQUIRE(newRect1.isSmallerOrEqualThan(newRect2));
			REQUIRE(!newRect2.isSmallerOrEqualThan(newRect1));

			Rect newRect3(-4, -4, 4, 1);
			REQUIRE(!newRect1.isSmallerOrEqualThan(newRect3));
		}

		SECTION("offset")
		{
			Rect newRect1(-1, -1, 2, 2);
			newRect1.offset(5, 10);
			REQUIRE(newRect1.left() == 4);
			REQUIRE(newRect1.top() == 9);
		}

		SECTION("intersectsWith")
		{
			SECTION("Test bijectivity")
			{
				Rect newRect1(-1, -1, 2, 2);
				Rect newRect2(-256, -256, 256, 256);

				REQUIRE(newRect1.intersectsWith(newRect2));
				REQUIRE(newRect2.intersectsWith(newRect1));

				Rect newRect3(10, 10, 2, 2);
				REQUIRE(!newRect1.intersectsWith(newRect3));
				REQUIRE(!newRect3.intersectsWith(newRect1));
			}

			SECTION("Empty rect")
			{
				Rect emptyRect;
				Rect newRect1(-1, -1, 2, 2);
				REQUIRE(newRect1.intersectsWith(emptyRect));
			}

			SECTION("Almost intersecting rectangles")
			{
				Rect newRectCenter(-1, -1, 2, 2);
				Rect newRectTR(1, -3, 2, 2);
				Rect newRectR(1, -1, 2, 2);
				Rect newRectBR(1, 1, 2, 2);
				Rect newRectB(-1, 1, 2, 2);
				Rect newRectBL(-3, 1, 2, 2);
				Rect newRectL(-3, -1, 2, 2);
				Rect newRectTL(-3, -3, 2, 2);
				Rect newRectT(-1, -3, 2, 2);
				REQUIRE(!newRectTR.intersectsWith(newRectCenter));
				REQUIRE(!newRectR.intersectsWith(newRectCenter));
				REQUIRE(!newRectBR.intersectsWith(newRectCenter));
				REQUIRE(!newRectB.intersectsWith(newRectCenter));
				REQUIRE(!newRectBL.intersectsWith(newRectCenter));
				REQUIRE(!newRectL.intersectsWith(newRectCenter));
				REQUIRE(!newRectTL.intersectsWith(newRectCenter));
				REQUIRE(!newRectT.intersectsWith(newRectCenter));

				newRectCenter.offset(10, 10);
				newRectTR.offset(10, 10);
				newRectR.offset(10, 10);
				newRectBR.offset(10, 10);
				newRectB.offset(10, 10);
				newRectBL.offset(10, 10);
				newRectL.offset(10, 10);
				newRectTL.offset(10, 10);
				newRectT.offset(10, 10);
				REQUIRE(!newRectTR.intersectsWith(newRectCenter));
				REQUIRE(!newRectR.intersectsWith(newRectCenter));
				REQUIRE(!newRectBR.intersectsWith(newRectCenter));
				REQUIRE(!newRectB.intersectsWith(newRectCenter));
				REQUIRE(!newRectBL.intersectsWith(newRectCenter));
				REQUIRE(!newRectL.intersectsWith(newRectCenter));
				REQUIRE(!newRectTL.intersectsWith(newRectCenter));
				REQUIRE(!newRectT.intersectsWith(newRectCenter));

			}
		}
	}
}