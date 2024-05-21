#include "PolygonBitmap.h"
#include <unordered_map>
#include <SDL/SDL_image.h>

PolygonBitmap::PolygonBitmap(int minX, int minY, int maxX, int maxY, int w, int h, const Polygon& polygon)
	:
	PixelBuffer(w, h)
{
	assert(w == maxX - minX);
	assert(h == maxY - minY);
	polygonMinX = minX;
	polygonMinY = minY;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			uint8_t value = polygon.isPointInside(Ved2(x + minX, y + minY)) ? INSIDE : OUTSIDE;
			this->setPixel(x, y, value);
		}
	}
}

PolygonBitmap PolygonBitmap::makeFrom(const Polygon& polygon)
{
	int minX = INT32_MAX, minY = INT32_MAX, maxX = INT32_MIN, maxY = INT32_MIN;

	for (const auto& [lineStart, lineEnd] : polygon.lines)
	{
		minX = std::min<int>(minX, lineStart.x);
		minY = std::min<int>(minY, lineStart.y);
		maxX = std::max<int>(maxX, lineStart.x);
		maxY = std::max<int>(maxY, lineStart.y);

		minX = std::min<int>(minX, lineEnd.x);
		minY = std::min<int>(minY, lineEnd.y);
		maxX = std::max<int>(maxX, lineEnd.x);
		maxY = std::max<int>(maxY, lineEnd.y);
	}

	int w = maxX - minX;
	int h = maxY - minY;
	return PolygonBitmap(minX, minY, maxX, maxY, w, h, polygon);
}

void PolygonBitmap::saveTo(std::string path)
{
	constexpr bool POLYGON_BITMAPS_DEBUG_PNGS = true;
	if (!POLYGON_BITMAPS_DEBUG_PNGS) return;

	std::unordered_map<uint8_t, Color> palette;
	palette[INSIDE] = Color(255, 255, 255);
	palette[OUTSIDE] = Color(0, 0, 0);
	palette[CARVED] = Color(200, 0, 0);
	palette[RECTANGLEFIED] = Color(0, 0, 150);

	std::vector<uint32_t> pixels(size.w * size.h);
	for (int y = 0; y < size.h; ++y)
	{
		for (int x = 0; x < size.w; ++x)
		{
			uint8_t value = getPixel(x, size.h - 1 - y); //y is inverted in our coordinate system compared to Doom (our bitmap: +y = down, Doom: +y = up)
			Color c = palette[value];
			pixels[y * size.w + x] = c;
		}
	}
	SDL_Surface* png = SDL_CreateRGBSurfaceWithFormatFrom(&pixels.front(), size.w, size.h, 32, size.w * 4, SDL_PIXELFORMAT_RGBA32);
	IMG_SavePNG(png, path.c_str());
	SDL_FreeSurface(png);
}

void PolygonBitmap::blitOver(PolygonBitmap& dst, bool doNotBlitOutsides, PolygonBitmapValue valueOverride)
{
	assert(this->isInBoundsOf(dst));

	int offsetX = this->getMinX() - dst.getMinX();
	int offsetY = this->getMinY() - dst.getMinY();
	for (int y = 0; y < size.h; ++y)
	{
		for (int x = 0; x < size.w; ++x)
		{
			uint8_t val = this->getPixel(x, y);
			assert(val != NONE);
			if (doNotBlitOutsides && val == OUTSIDE) continue;
			if (val != OUTSIDE && valueOverride != NONE) val = valueOverride;
			dst.setPixel(x + offsetX, y + offsetY, val);
		}
	}
}

bool PolygonBitmap::areAllPointsInside(const PolygonBitmap& other) const
{
	assert(this->isInBoundsOf(other));
	for (int y = this->getMinY(); y < this->getMaxY(); ++y)
	{
		for (int x = this->getMinX(); x < this->getMaxX(); ++x)
		{
			if (this->getValueAt(x, y) != OUTSIDE && other.getValueAt(x, y) == OUTSIDE) return false;
			//TODO: maybe other conditions? What if this is outside, but other == inside?
		}
	}
	return true;
}

uint8_t PolygonBitmap::getValueAt(int x, int y) const
{
	return atXY(x, y);
}

void PolygonBitmap::setValueAt(int x, int y, uint8_t val)
{
	atXY(x, y) = val;
}

uint8_t& PolygonBitmap::atXY(int x, int y)
{
	return const_cast<uint8_t&>(static_cast<const PolygonBitmap&>(*this).atXY(x, y));
}

const uint8_t& PolygonBitmap::atXY(int x, int y) const
{
	return at(x - polygonMinX, y - polygonMinY);
}

bool PolygonBitmap::isInBoundsOf(const PolygonBitmap& other) const
{
	return
		other.getMinX() <= this->getMinX() &&
		other.getMinY() <= this->getMinY() &&
		other.getMaxX() >= this->getMaxX() &&
		other.getMaxY() >= this->getMaxY();
}

int PolygonBitmap::getMaxX() const
{
	return polygonMinX + size.w;
}

int PolygonBitmap::getMaxY() const
{
	return polygonMinY + size.h;
}

int PolygonBitmap::getMinX() const
{
	return polygonMinX;
}

int PolygonBitmap::getMinY() const
{
	return polygonMinY;
}
