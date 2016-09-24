#include "stdafx.h"
#include "BitmapFontCache.h"

#include <algorithm>
#include <cassert>

#include <ft2build.h>
#include <freetype/freetype.h>

#include <windows.h>    // Win32Api Header File 

const int WIDTH = 1024;
const int HEIGHT = 1024;

namespace bmf
{
	BitmapFontCache::BitmapFontCache(FT_Library _library) : m_library(_library)
	{
		m_image = new unsigned char[WIDTH * HEIGHT * sizeof(char)];
		std::memset(m_image, 0, WIDTH * HEIGHT * sizeof(char));

		m_pools[0].init(Rect(0, 0, WIDTH, HEIGHT), 1, 1);
	}

	int BitmapFontCache::loadFont(const char* _filename)
	{
		FT_Face newFace = nullptr;
		FT_Error error = FT_New_Face(m_library, _filename, 0, &newFace);
		if (error == 0)
		{
			m_faces.push_back(newFace);
			return m_faces.size() - 1;
		}
		return -1;
	}


	BitmapFontCache::~BitmapFontCache()
	{
		for (FT_Face f : m_faces)
			FT_Done_Face(f);
		delete[](m_image);
		m_image = nullptr;
	}

	BitmapFontCache::Pool::Slot *BitmapFontCache::Pool::findBestSlotForRect(const Rect &_rect)
	{
		if (_rect.height() == 0 || _rect.width() == 0)
			return false;

		Slot *bestSlot = nullptr;
		for (Slot *slot : m_freeSlots)
		{
			if ((slot->getState() == Slot::State::Free && _rect.isSmallerOrEqualThan(slot->getRect()))
		    && (bestSlot == nullptr || slot->getRect().surface() < bestSlot->getRect().surface()))
			{
				bestSlot = slot;
			}
		}

		if (bestSlot != nullptr)
			return bestSlot->addRect(_rect, m_freeSlots);

		return nullptr;
	}

	BitmapFontCache::ReturnCode BitmapFontCache::Pool::removeGlyph(int _fontIndex, int _char, int _pixelSize)
	{
		Key key(_fontIndex, _char, _pixelSize);

		auto it = m_glyphs.find(key);
		if (it != m_glyphs.end())
		{
			Slot *slot = it->second;
			slot->setAsFree(m_freeSlots);
			m_glyphs.erase(it);
			return OK;
		}

		return NotFound;
	}

	bool BitmapFontCache::Pool::findGlyph(int _fontIndex, int _char, int _pixelSize)
	{
		Key key(_fontIndex, _char, _pixelSize);

		if (m_glyphs.find(key) != m_glyphs.end())
			return true;

		return false;
	}


	BitmapFontCache::ReturnCode BitmapFontCache::Pool::addGlyph(const BitmapFontCache* _owner, FT_Bitmap &_bitmap, int _fontIndex, int _char, int _pixelSize)
	{
		Slot *slot = findBestSlotForRect(Rect(0, 0, _bitmap.width + m_paddingX, _bitmap.rows + m_paddingY));
		if (!slot)
			return NotEnoughSpace;

		Key key(_fontIndex, _char, _pixelSize);
		m_glyphs[key] = slot;

		for (unsigned int i = 0; i < _bitmap.width; i++)
		{
			for (unsigned int j = 0; j < _bitmap.rows; j++)
			{
				_owner->m_image[i + slot->getRect().left() + (j + slot->getRect().top()) * WIDTH] = _bitmap.buffer[j * _bitmap.width + i];
			}
		}

		return OK;
	}

	unsigned int  BitmapFontCache::getPoolIndex(int _pixelSize) const
	{
		return 0;
	}

	BitmapFontCache::ReturnCode BitmapFontCache::addGlyph(int _fontIndex, int _char, int _pixelSize)
	{
		unsigned int defaultPoolIndex = 0;

		// Look into pools
		if (m_pools[defaultPoolIndex].findGlyph(_fontIndex, _char, _pixelSize))
			return AlreadyAdded;
		else
		{
			for (int i = 1; i < POOL_COUNT; i++)
			{
				if (m_pools[(i + defaultPoolIndex) % POOL_COUNT].findGlyph(_fontIndex, _char, _pixelSize))
					return AlreadyAdded;
			}
		}

		// Build bitmap char
		FT_Set_Pixel_Sizes(m_faces[_fontIndex], 0, _pixelSize);
		int error = FT_Load_Char(m_faces[_fontIndex], _char, FT_LOAD_RENDER);
		if (error || m_faces[_fontIndex]->glyph->bitmap.width == 0 || m_faces[_fontIndex]->glyph->bitmap.rows == 0)
			return NotFound;

		// Add to pools
		auto ret = m_pools[defaultPoolIndex].addGlyph(this, m_faces[_fontIndex]->glyph->bitmap, _fontIndex, _char, _pixelSize);
		if (ret == OK)
			return OK;
		else if (ret == NotEnoughSpace)
		{
			for (int i = 1; i < POOL_COUNT; i++)
			{
				ret = m_pools[(i + defaultPoolIndex) % POOL_COUNT].addGlyph(this, m_faces[_fontIndex]->glyph->bitmap, _fontIndex, _char, _pixelSize);
				if (ret == OK)
					return OK;
			}
		}

		return ret;
	}

	BitmapFontCache::ReturnCode BitmapFontCache::removeGlyph(int _fontIndex, int _char, int _pixelSize)
	{
		unsigned int defaultPoolIndex = getPoolIndex(_pixelSize);
		auto ret = m_pools[defaultPoolIndex].removeGlyph(_fontIndex, _char, _pixelSize);
		if (ret == OK)
			return OK;
		else if (ret == NotFound)
		{
			for (int i = 1; i < POOL_COUNT; i++)
			{
				ret = m_pools[(i + defaultPoolIndex) % POOL_COUNT].removeGlyph(_fontIndex, _char, _pixelSize);
				if (ret == OK)
					return OK;
			}
		}

		return ret;
	}

	static HWND hwnd = NULL;

	void BitmapFontCache::showImage() const
	{
		if (hwnd == NULL)
			hwnd = ::CreateWindow(L"static", L"Debug", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, WIDTH + 100, HEIGHT + 100, 0, (HMENU)0, GetModuleHandle(0), NULL);
		HDC hdc = ::GetDC(hwnd);

		// Make a compatible DC
		HDC hdcBitmap = ::CreateCompatibleDC(hdc);
		HBITMAP hBmp = ::CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
		::SelectObject(hdcBitmap, hBmp);

		// Clear window with pink
		HBRUSH hPinkBrush = ::CreateSolidBrush(RGB(255, 0, 255));
		RECT rect = { 0, 0, WIDTH, HEIGHT };
		::FillRect(hdcBitmap, &rect, hPinkBrush);
		::DeleteObject(hPinkBrush);

		for (auto& pool : m_pools)
		{
			// Display free slots 
			const std::list<Pool::Slot *> &freeSlots = pool.getFreeSlots();
			for (auto it = freeSlots.begin(); it != freeSlots.end(); ++it)
			{
				const Rect& curGlyph = (*it)->getRect();
				RECT rect = { curGlyph.left(), curGlyph.top(), curGlyph.left() + curGlyph.width(), curGlyph.top() + curGlyph.height() };
				HBRUSH hBrush = ::CreateSolidBrush(RGB((curGlyph.left() + curGlyph.top()) % 255, curGlyph.height() % 255, curGlyph.width() % 255));
				::FillRect(hdcBitmap, &rect, hBrush);
				::DeleteObject(hBrush);
			}

			// Display glyphs
			const auto& glyphs = pool.getGlyphs();
			for (auto it = glyphs.begin(); it != glyphs.end(); ++it)
			{
				const Rect& curGlyph = it->second->getRect();
				RECT rect = { curGlyph.left(), curGlyph.top(), curGlyph.left() + curGlyph.width() - pool.getPaddingX(), curGlyph.top() + curGlyph.height() - pool.getPaddingY() };

				::FillRect(hdcBitmap, &rect, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));

				for (int i = rect.left; i < rect.right; i++)
				{
					for (int j = rect.top; j < rect.bottom; j++)
					{
						unsigned char color = m_image[i + j * WIDTH];
						if (color != 0)
							::SetPixel(hdcBitmap, i, j, RGB(color, color, color));
					}
				}
			}
		}

		::BitBlt(hdc, 0, 0, WIDTH, HEIGHT, hdcBitmap, 0, 0, SRCCOPY);

		::DeleteObject(hBmp);
		::DeleteDC(hdcBitmap);
		::ReleaseDC(hwnd, hdc);
	}
}


