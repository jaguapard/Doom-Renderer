#include <unordered_map>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "PolygonTriangulator.h"
#include "Color.h"
#include <functional>
#include <set>
#include <unordered_set>

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

	for (const auto& [lineStart, lineEnd] : polygon)
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
	std::vector<Ved2> ret;
	if (polygonLines.size() == 0) return ret;

	std::vector<Ved2> originalVerts;
	std::set<Ved2> vertsFilter;
	for (auto& [v1, v2] : polygonLines) //make a list of unique vertices
	{
		originalVerts.push_back(v1);
		originalVerts.push_back(v2);
	}

	std::vector<Ved2> uniqueVerts;
	for (int i = 0; i < originalVerts.size(); ++i)
	{
		if (vertsFilter.find(originalVerts[i]) == vertsFilter.end())
		{
			vertsFilter.insert(originalVerts[i]);
			uniqueVerts.push_back(originalVerts[i]);
		}
	}

	if (uniqueVerts.size() < 3) throw std::runtime_error("Sector triangulator attempted to triangulate with less than 3 vertices");
	uniqueVerts = originalVerts;

	//fan triangulation works only for convex polygons. TODO: split the original polygon into convex ones. We don't care about holes, since they will be covered by other sector's walls. Also, tree sorting may fuck stuff up. 
	Ved2 fanVertex = uniqueVerts[0];
	Ved2 second = uniqueVerts[1];
	Ved2 third = uniqueVerts[2];

	for (int i = 2; i < uniqueVerts.size(); ++i)
	{
		ret.push_back(uniqueVerts[0]); //fan vertex
		ret.push_back(uniqueVerts[i - 1]);
		ret.push_back(uniqueVerts[i]);
	}

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
