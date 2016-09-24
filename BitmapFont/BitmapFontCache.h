#pragma once

#include <map>
#include <list>
#include <cassert>
#include <algorithm>
#include <vector>
#include "rect.h"

typedef struct FT_LibraryRec_  *FT_Library;
typedef struct FT_FaceRec_  *FT_Face;
typedef struct  FT_Bitmap_ FT_Bitmap;

const int POOL_COUNT = 1;

namespace BitmapFont
{
	class BitmapFontCache
	{
	public:
		BitmapFontCache(FT_Library _library);
		~BitmapFontCache();

		void showImage() const; // for debug

		int loadFont(const char* _filename);

		enum ReturnCode
		{
			NotEnoughSpace,
			AlreadyAdded,
			NotFound,
			OK
		};

		ReturnCode addGlyph(int _fontIndex, int _char, int _pixelSize);
		ReturnCode removeGlyph(int _fontIndex, int _char, int _pixelSize);

		unsigned int  getFreeSlotsCount() const
		{
			unsigned int count = 0;
			for (auto& pool : m_pools)
				count += pool.getFreeSlotsCount();
			return count;
		}
		unsigned int  getGlyphsCount() const
		{
			unsigned int count = 0;
			for (auto& pool : m_pools)
				count += pool.getGlyphsCount();
			return count;
		}
		unsigned int  getFontCount() const { return m_faces.size(); }

	private:
		unsigned int  getPoolIndex(int _pixelSize) const;

		class Pool
		{
		public:
			class Key
			{
			public:
				Key(int _fontIndex, int _unicodeChar, int _pixelSize) : fontIndex(_fontIndex), unicodeChar(_unicodeChar), pixelSize(_pixelSize){}
				bool Key::operator <(const Key& b) const
				{
					return fontIndex == b.fontIndex ? (unicodeChar == b.unicodeChar ? (pixelSize < b.pixelSize) : unicodeChar < b.unicodeChar) : fontIndex < b.fontIndex;
				}

			private:
				int unicodeChar;
				int pixelSize;
				int fontIndex;
			};

			class Slot
			{
			public:
				explicit Slot(Slot*_owner, const Rect&_rect) : m_rect(_rect), m_owner(_owner) {}
				~Slot()
				{
					if (m_slot1)
						delete m_slot1;
					if (m_slot2)
						delete m_slot2;
				}

				void setAsOccupied(std::list<Slot *>& _freeSlots)
				{
					assert(m_state == State::Free);
					m_state = State::Occupied;
					_freeSlots.remove(this);
				}

				void setAsFree(std::list<Slot *>& _freeSlots)
				{
					assert(m_state != State::Free);
					if (m_state == State::Divided)
					{
						assert(m_slot1 != nullptr && m_slot1->m_state == State::Free && m_slot2 != nullptr && m_slot2->m_state == State::Free);
						_freeSlots.remove(m_slot1);
						delete m_slot1;
						m_slot1 = nullptr;
						_freeSlots.remove(m_slot2);
						delete m_slot2;
						m_slot2 = nullptr;
					}

					m_state = State::Free;
					_freeSlots.push_back(this);

					if (m_owner)
					{
						if (m_owner->m_slot1->m_state == State::Free
							&& m_owner->m_slot2->m_state == State::Free)
						{
							m_owner->setAsFree(_freeSlots);
						}
					}
				}

				Slot *addRect(const Rect &_rect, std::list<Slot *>& _freeSlots)
				{
					assert(m_state == State::Free && _rect.width() <= m_rect.width() && _rect.height() <= m_rect.height());

					Rect newRectA1(0, 0, m_rect.width() - _rect.width(), m_rect.height());
					Rect newRectA2(0, 0, _rect.width(), m_rect.height() - _rect.height());

					Rect newRectB1(0, 0, m_rect.width(), m_rect.height() - _rect.height());
					Rect newRectB2(0, 0, m_rect.width() - _rect.width(), _rect.height());

					// Chose the division which creates the biggest surface
					if (std::max<int>(newRectA1.surface(), newRectA2.surface()) > std::max<int>(newRectB1.surface(), newRectB1.surface()))
					{
						divideByWidth(_rect.width());
						m_slot1->divideByHeight(_rect.height());

					}
					else
					{
						divideByHeight(_rect.height());
						m_slot1->divideByWidth(_rect.width());
					}
					_freeSlots.push_back(m_slot1->m_slot2);
					_freeSlots.push_back(m_slot2);
					_freeSlots.remove(this);

					Slot *newSlot = m_slot1->m_slot1;
					newSlot->setAsOccupied(_freeSlots);
					return newSlot;
				}

				const Rect& getRect() const { return m_rect; }

				enum State
				{
					Free,
					Divided,
					Occupied
				};
				State getState() const { return m_state; }

			protected:
				void divideByHeight(int _h)
				{
					assert(m_state == State::Free);
					m_state = State::Divided;
					Rect newRectB1(m_rect.left(), m_rect.top(), m_rect.width(), _h);
					Rect newRectB2(m_rect.left(), m_rect.top() + _h, m_rect.width(), m_rect.height() - _h);

					assert(m_slot1 == nullptr);
					m_slot1 = new Slot(this, newRectB1);
					assert(m_slot2 == nullptr);
					m_slot2 = new Slot(this, newRectB2);
					assert((newRectB1.surface() + newRectB2.surface()) == m_rect.surface());
				}

				void divideByWidth(int _w)
				{
					assert(m_state == State::Free);
					m_state = State::Divided;
					Rect newRectA1(m_rect.left(), m_rect.top(), _w, m_rect.height());
					Rect newRectA2(m_rect.left() + _w, m_rect.top(), m_rect.width() - _w, m_rect.height());

					assert(m_slot1 == nullptr);
					m_slot1 = new Slot(this, newRectA1);
					assert(m_slot2 == nullptr);
					m_slot2 = new Slot(this, newRectA2);
					assert((newRectA1.surface() + newRectA2.surface()) == m_rect.surface());
				}

				Slot* m_slot1 = nullptr;
				Slot* m_slot2 = nullptr;
				Slot* m_owner = nullptr;
				Rect  m_rect;
				State m_state = State::Free;
			};

		public:
			void init(const Rect &_initRect, int _paddingX, int _paddingY)
			{
				m_paddingX = _paddingX;
				m_paddingY = _paddingY;
				Rect initRect(_initRect.left() + m_paddingX, _initRect.top() + m_paddingY, _initRect.width() - m_paddingX, _initRect.height() - m_paddingY);
				assert(m_rootSlot == nullptr);
				m_rootSlot = new Slot(nullptr, initRect);
				m_freeSlots.push_back(m_rootSlot);
			}

			~Pool()
			{
				if (m_rootSlot)
					delete m_rootSlot;
			}

			const std::list<Slot*> &getFreeSlots() const { return m_freeSlots; }
			int  getFreeSlotsCount() const { return m_freeSlots.size(); }

			const std::map<Key, Slot *>& getGlyphs() const { return m_glyphs; }
			int  getGlyphsCount() const { return m_glyphs.size(); }

			Slot *findBestSlotForRect(const Rect &_glyph);

			int  getPaddingX() const { return m_paddingX; }
			int  getPaddingY() const { return m_paddingY; }

			bool findGlyph(int _fontIndex, int _char, int _pixelSize);
			ReturnCode addGlyph(const BitmapFontCache* _owner, FT_Bitmap& _bitmap, int _fontIndex, int _char, int _pixelSize);
			ReturnCode removeGlyph(int _fontIndex, int _char, int _pixelSize);

		protected:
			Slot*					m_rootSlot = nullptr;
			std::list<Slot*>		m_freeSlots;
			std::map<Key, Slot *>	m_glyphs;
			int						m_paddingX = 2;
			int						m_paddingY = 2;
		};

		Pool					m_pools[POOL_COUNT];
		std::vector<FT_Face>	m_faces;
		unsigned char*			m_image = nullptr;
		FT_Library			    m_library = nullptr;
	};
}

