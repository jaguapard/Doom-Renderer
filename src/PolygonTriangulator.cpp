#include <unordered_map>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "PolygonTriangulator.h"
#include "Color.h"
#include <functional>

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
	palette[RECTANGLEFIED] = Color(0, 0, 150);

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
	assert(this->isInBoundsOf(dst));

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

uint8_t* PolygonBitmap::begin()
{
	return &store.front();
}

uint8_t* PolygonBitmap::end()
{
	return &store.back();
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

std::vector<Ved2> PolygonTriangulator::triangulate(std::vector<Line> polygonLines)
{
	auto bitmap = PolygonBitmap::makeFrom(polygonLines);
	static int nSector = 0;
	bitmap.saveTo("sectors_debug/" + std::to_string(nSector) + "_a_initial.png");

	std::vector<Ved2> ret;
	while (true)
	{
		int slopedLineCount = 0;
		for (auto& it : polygonLines)
		{
			slopedLineCount += it.first.x != it.second.x && it.first.y != it.second.y;
		}
		if (slopedLineCount == 0) break;

		for (int i = 0; i < polygonLines.size(); ++i)
		{
			Line line = polygonLines[i];
			if (line.first.x == line.second.x || line.first.y == line.second.y) continue; //skip non-sloped lines

			Ved2 newVert = { line.first.x, line.second.y };
			std::array<Ved2, 3> trianglePoints = { line.first, newVert, line.second };
			auto carveResult = tryCarve(trianglePoints, bitmap);
			ret.insert(ret.end(), carveResult.begin(), carveResult.end());

			trianglePoints[1] = { line.second.x, line.first.y };
			carveResult = tryCarve(trianglePoints, bitmap);
			ret.insert(ret.end(), carveResult.begin(), carveResult.end());
		}
		break;
	}

	bitmap.saveTo("sectors_debug/" + std::to_string(nSector) + "_b_carved.png");


	int minX = bitmap.getMinX();
	int maxX = bitmap.getMaxX();
	int minY = bitmap.getMinY();
	int maxY = bitmap.getMaxY();
	int w = bitmap.getW();
	int h = bitmap.getH();

	std::vector<SDL_Rect> rects;

	std::function pointOutsidePolygon = [&](const uint8_t v) {return v == OUTSIDE; };
	while (true)
	{
		auto firstFreeIt = std::find(bitmap.begin(), bitmap.end(), INSIDE);
		if (firstFreeIt == bitmap.end()) break;

		int firstFreePos = firstFreeIt - bitmap.begin();

		SDL_Point startingPoint = { firstFreePos % w, firstFreePos / w };
		int initialWidth = std::find_if(firstFreeIt, bitmap.begin() + (startingPoint.y + 1) * w, pointOutsidePolygon) - firstFreeIt;

		assert(initialWidth > 0);
		int endX = startingPoint.x + initialWidth;
		std::fill(firstFreeIt, firstFreeIt + initialWidth, RECTANGLEFIED);

		SDL_Rect r = { startingPoint.x, startingPoint.y, initialWidth, 1 };
		for (int y = startingPoint.y + 1; y < h; ++y)
		{
			auto lineBeg = bitmap.begin() + y * w + startingPoint.x;
			auto lineEnd = bitmap.begin() + y * w + endX;
			if (std::find_if(lineBeg, lineEnd, pointOutsidePolygon) != lineEnd) break; // this line has points outside of the polygon, can't expand

			r.h++;
			//this is a little bit buggy - it overrides previous values, potentially overwriting the CARVED value. It does not cause OUTSIDE to be overwritten though, so it's almost purely cosmetical/visible on sectors_debug pngs
			std::fill(lineBeg, lineEnd, RECTANGLEFIED);
		}
		r.x += minX;
		r.y += minY;
		rects.push_back(r);
	}
	bitmap.saveTo("sectors_debug/" + std::to_string(nSector) + "_c_rectified.png");

	for (const auto& it : rects)
	{
		ret.push_back(Ved2(it.x, it.y));
		ret.push_back(Ved2(it.x + it.w, it.y));
		ret.push_back(Ved2(it.x + it.w, it.y + it.h));

		ret.push_back(Ved2(it.x + it.w, it.y + it.h));
		ret.push_back(Ved2(it.x, it.y + it.h));
		ret.push_back(Ved2(it.x, it.y));
	}

	++nSector;
	return ret;
}

std::vector<Ved2> PolygonTriangulator::tryCarve(const std::array<Ved2, 3>& trianglePoints, PolygonBitmap& bitmap)
{
	std::vector<Line> triangleLines(3);
	/*/Line p1 = std::make_pair(line.first, newVert);
			Line p2 = std::make_pair(newVert, line.second);
			Line p3 = std::make_pair(line.second, line.first);
			std::array<Line, 3> candidateTriangle = { p1, p2, p3};*/

	for (int i = 0; i < 3; ++i) triangleLines[i] = std::make_pair(trianglePoints[i], trianglePoints[i < 2 ? i + 1 : 0]);
	auto triangleBitmap = PolygonBitmap::makeFrom(triangleLines);
	if (triangleBitmap.areAllPointsInside(bitmap)) //very trivial for now, just check if entire triangle can fit
	{
		triangleBitmap.blitOver(bitmap, true, CARVED);
		return std::vector<Ved2>(trianglePoints.begin(), trianglePoints.end());
	}

	return std::vector<Ved2>();
}
