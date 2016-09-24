#include "stdafx.h"
#include "BitmapFontCache.h"

#include "catch.hpp"
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace bmf
{
	TEST_CASE("Bitmap Font Cache works properly", "[BitmapFontCache]")
	{

		FT_Library    library;
		FT_Error error = FT_Init_FreeType(&library);              /* initialize library */
		REQUIRE(error == 0);

		SECTION("Add / remove glyph")
		{
			BitmapFontCache bitmapCache(library);
			bitmapCache.loadFont("C:/windows/fonts/arial.ttf");
			bitmapCache.loadFont("C:/windows/fonts/verdana.ttf");

			REQUIRE(bitmapCache.getGlyphsCount() == 0);
			bitmapCache.addGlyph(0, 'a', 18);
			REQUIRE(bitmapCache.getGlyphsCount() == 1);
			bitmapCache.removeGlyph(1, 'a', 18);
			REQUIRE(bitmapCache.getGlyphsCount() == 1);
			bitmapCache.removeGlyph(0, 'a', 18);
			REQUIRE(bitmapCache.getGlyphsCount() == 0);
			bitmapCache.addGlyph(1, 'a', 18);
			REQUIRE(bitmapCache.getGlyphsCount() == 1);
			bitmapCache.removeGlyph(0, 'a', 32);
			REQUIRE(bitmapCache.getGlyphsCount() == 1);
			bitmapCache.removeGlyph(0, 'b', 32);
			REQUIRE(bitmapCache.getGlyphsCount() == 1);
			unsigned int count = 1;
			for (int n = 0; n < 255; n++)
			{
				if (bitmapCache.addGlyph(1, n, 18) == BitmapFontCache::OK)
					count++;
			}
			REQUIRE(bitmapCache.getGlyphsCount() == count);
			for (int n = 0; n < 255; n++)
			{
				if (bitmapCache.removeGlyph(1, 255 - n - 1, 18) == BitmapFontCache::OK)
					count--;
			}
			REQUIRE(bitmapCache.getGlyphsCount() == count);
			REQUIRE(bitmapCache.getGlyphsCount() == 0);

			srand(896523456);
			std::vector<std::tuple<unsigned, char>> glyphsAdded;
			for (int i = 0; i < 256; i++)
			{
				unsigned char c = 32 + rand() % (255 - 32); // 32 - 255
				int size = 12 + rand() % 42; // 12 - 64
				if (bitmapCache.addGlyph(0, c, size))
					glyphsAdded.push_back(std::make_tuple(c, size));
			}
			for (auto& glyph : glyphsAdded)
			{
				bitmapCache.removeGlyph(0, std::get<0>(glyph), std::get<1>(glyph));
			}
			REQUIRE(bitmapCache.getGlyphsCount() == 0);
		}

		SECTION("Free slots are merged after glyph removal")
		{
			BitmapFontCache bitmapCache(library);
			bitmapCache.loadFont("C:/windows/fonts/arial.ttf");
			bitmapCache.loadFont("C:/windows/fonts/verdana.ttf");
			bitmapCache.loadFont("C:/windows/fonts/times.ttf");
			bitmapCache.loadFont("C:/windows/fonts/comic.ttf");

			REQUIRE(bitmapCache.getFreeSlotsCount() == POOL_COUNT);
			bitmapCache.addGlyph(0, 'a', 18);
			REQUIRE(bitmapCache.getFreeSlotsCount() == (POOL_COUNT + 1));
			bitmapCache.addGlyph(0, 'b', 18);
			REQUIRE(bitmapCache.getFreeSlotsCount() == (POOL_COUNT + 2));
			bitmapCache.removeGlyph(0, 'b', 18);
			REQUIRE(bitmapCache.getFreeSlotsCount() == (POOL_COUNT + 1));
			bitmapCache.removeGlyph(0, 'a', 18);
			REQUIRE(bitmapCache.getFreeSlotsCount() == (POOL_COUNT));

			srand(123354654);

			std::list<std::tuple<unsigned int, unsigned int, unsigned int>> glyphsAdded;
			for (int i = 0; i < 5000; i++)
			{
				unsigned int unicodeChar = 32 + rand() % (255 - 32); // 32 - 255
				unsigned int size = 12 + rand() % 50; // 12 - 138
				unsigned int fontIndex = rand() % bitmapCache.getFontCount();
				if (bitmapCache.addGlyph(fontIndex, unicodeChar, size) == BitmapFontCache::OK)
				{
					glyphsAdded.push_back(std::make_tuple(fontIndex, unicodeChar, size));
					REQUIRE(bitmapCache.getFreeSlotsCount() == (POOL_COUNT + glyphsAdded.size()));
				}
			}
			//bitmapCache.showImage();
			// Erase randomly 
			while (glyphsAdded.size() > 0)
			{
				auto it = glyphsAdded.begin();
				if (glyphsAdded.size() > 1)
					std::advance(it, rand() % (glyphsAdded.size() - 1));
				if (bitmapCache.removeGlyph(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it)) == BitmapFontCache::OK)
					glyphsAdded.erase(it);
				//bitmapCache.showImage();
			}
			//bitmapCache.showImage();
			REQUIRE(bitmapCache.getFreeSlotsCount() == POOL_COUNT);
		}

		FT_Done_FreeType(library);
	}
}

