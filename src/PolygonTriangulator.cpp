#include <unordered_map>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "PolygonTriangulator.h"
#include "Color.h"

double scalarCross2d(Ved2 a, Ved2 b)
{
	return a.x * b.y - a.y * b.x;
}

bool rayCrossesLine(const Ved2& p, const std::pair<Ved2, Ved2>& lines)
{
	const Ved2& sv = lines.first;
	const Ved2& ev = lines.second;

	//floating point precision is a bitch here, force use doubles everywhere
	Ved2 v1 = p - sv;
	Ved2 v2 = ev - sv;
	Ved2 v3 = { -sqrt(2), sqrt(3) }; //using irrational values to never get pucked by linedefs being perfectly collinear to our ray by chance
	v3 /= v3.len(); //idk, seems like a good measure
	double dot = v2.dot(v3);
	//if (abs(dot) < 1e-9) return false;

	double t1 = scalarCross2d(v2, v1) / dot;
	double t2 = v1.dot(v3) / dot;

	//we are fine with false positives (point is considered inside when it's not), but false negatives are absolutely murderous
	if (t1 >= 1e-9 && (t2 >= 1e-9 && t2 <= 1.0 - 1e-9))
		return true;
	return false;

	//these are safeguards against rampant global find-and-replace mishaps breaking everything
	static_assert(sizeof(sv) == 16);
	static_assert(sizeof(ev) == 16);
	static_assert(sizeof(v1) == 16);
	static_assert(sizeof(v2) == 16);
	static_assert(sizeof(v3) == 16);
	static_assert(sizeof(dot) == 8);
	static_assert(sizeof(t1) == 8);
	static_assert(sizeof(t2) == 8);
	static_assert(sizeof(p) == 16);
}

bool isPointInsidePolygon(const Ved2& p, const std::vector<std::pair<Ved2, Ved2>>& lines)
{
	assert(lines.size() >= 3);
	int intersections = 0;
	for (const auto& it : lines) intersections += rayCrossesLine(p, it);
	return intersections % 2 == 1;
}

PolygonBitmap::PolygonBitmap(int minX, int minY, int maxX, int maxY, int w, int h, const std::vector<Line>& polygon)
	:
	PixelBuffer(w,h)
{	
	assert(w == maxX - minX);
	assert(h == maxY - minY);
	polygonMinX = minX;
	polygonMinY = minY;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			uint8_t value = isPointInsidePolygon(Ved2(x + minX, y + minY), polygon) ? INSIDE : OUTSIDE;
			this->setPixelUnsafe(x, y, value);
		}
	}
}

PolygonBitmap PolygonBitmap::makeFrom(const std::vector<Line>& polygon)
{
	int minX = INT32_MAX, minY = INT32_MAX, maxX = INT32_MIN, maxY = INT32_MIN;

	for (const auto& line : polygon)
	{
		minX = std::min<int>(minX, line.first.x);
		minY = std::min<int>(minY, line.first.y);
		maxX = std::max<int>(maxX, line.first.x);
		maxY = std::max<int>(maxY, line.first.y);

		minX = std::min<int>(minX, line.second.x);
		minY = std::min<int>(minY, line.second.y);
		maxX = std::max<int>(maxX, line.second.x);
		maxY = std::max<int>(maxY, line.second.y);
	}

	int w = maxX - minX;
	int h = maxY - minY;
	return PolygonBitmap(minX, minY, maxX, maxY, w, h, polygon);
}

void PolygonBitmap::saveTo(std::string path)
{
	std::unordered_map<uint8_t, Color> palette;
	palette[INSIDE] = Color(255, 255, 255);
	palette[OUTSIDE] = Color(0, 0, 0);
	palette[CARVED] = Color(200, 0, 0);

	std::vector<uint32_t> pixels(w * h);
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			uint8_t value = getPixelUnsafe(x, y);
			Color c = palette[value];
			pixels[y * w + x] = c;
		}
	}
	SDL_Surface* png = SDL_CreateRGBSurfaceWithFormatFrom(&pixels.front(), w, h, 32, w * 4, SDL_PIXELFORMAT_RGBA32);
	IMG_SavePNG(png, path.c_str());
	SDL_FreeSurface(png);
}

void PolygonBitmap::blitOver(PolygonBitmap& dst, bool doNotBlitOutsides, PolygonBitmapValue valueOverride)
{
	assert(dst.getMinX() <= this->getMinX());
	assert(dst.getMinY() <= this->getMinY());
	assert(dst.getMaxX() >= this->getMaxX());
	assert(dst.getMaxY() >= this->getMaxY());

	int offsetX = this->getMinX() - dst.getMinX();
	int offsetY = this->getMinY() - dst.getMinY();
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			uint8_t val = this->getPixelUnsafe(x, y);
			assert(val != NONE);
			if (doNotBlitOutsides && val == OUTSIDE) continue;
			if (val != OUTSIDE && valueOverride != NONE) val = valueOverride;
			dst.setPixelUnsafe(x + offsetX, y + offsetY, val);
		}
	}
}

int PolygonBitmap::getMaxX() const
{
	return polygonMinX + w;
}

int PolygonBitmap::getMaxY() const
{
	return polygonMinY + h;
}

int PolygonBitmap::getMinX() const
{
	return polygonMinX;
}

int PolygonBitmap::getMinY() const
{
	return polygonMinY;
}
