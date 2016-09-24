#pragma once

#include <algorithm>

namespace BitmapFont
{
	class Rect
	{
	public:
		Rect(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h) {}
		Rect() {}

		unsigned int width() const { return w; }
		unsigned int height() const { return h; }
		int			 left() const { return x; }
		int			 right() const { return x + w; }
		int			 top() const { return y; }
		int			 bottom() const { return y + h; }
		unsigned int surface() const { return w * h; }

		bool intersectsWith(const Rect _other) const
		{
			return (abs(w + 2 * x - _other.w - 2 * _other.x) < w + _other.w)
				&& (abs(h + 2 * y - _other.h - 2 * _other.y) < h + _other.h);
		}

		bool isSmallerOrEqualThan(const Rect _other) const
		{
			return w <= _other.w &&  h <= _other.h;
		}

		void offset(int _x, int _y)
		{
			x += _x;
			y += _y;
		}

	protected:
		int w = 0, h = 0;
		int x = 0, y = 0;
	};
}

