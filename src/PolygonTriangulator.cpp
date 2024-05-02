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

bool rayCrossesLine(const Ved2& p, const Line& line)
{
	const Ved2& sv = line.start;
	const Ved2& ev = line.end;

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

bool isPointInsidePolygon(const Ved2& p, const std::vector<Line>& lines)
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

/*
//returns 1 if 
int whichVertexLinesShare(const Line& line1, const Line& line2)
{
	if (line1.first == line2.first ||
		line1.first == line2.second) return 1;
	if (line1.second == line2.first ||
		line1.second == line2.second) return 2;
	return -1;
}
*/

std::vector<Ved2> PolygonTriangulator::triangulate(std::vector<Line> polygonLines)
{
	std::vector<Ved2> ret;
	if (polygonLines.size() == 0) return ret;
	if (polygonLines.size() < 3) throw std::runtime_error("Sector triangulation attempted with less than 3 lines!");

	auto originalLines = polygonLines;
	PolygonBitmap bitmap = PolygonBitmap::makeFrom(polygonLines);
	static int nSector = 0;
	bitmap.saveTo("sectors_debug/" + std::to_string(nSector++) + ".png");

	//find contours these lines make
	std::vector<std::deque<Line>> contours;
	while (!polygonLines.empty())
	{
		Line startingLine = polygonLines[0];
		contours.push_back({startingLine});
		polygonLines[0] = std::move(polygonLines.back());
		polygonLines.pop_back();

		std::deque<Line>& currContour = contours.back();

		int preGrowContourSize;
		do {
			preGrowContourSize = currContour.size();
			for (int i = 0; i < polygonLines.size(); ++i)
			{
				const int oldContourSize = currContour.size();
				const Line& currLine = polygonLines[i];
				Line flippedLine = { currLine.end, currLine.start };

				const Line& head = currContour.front();
				const Line& tail = currContour.back();

				if (currLine.start == tail.end) currContour.push_back(currLine); //if current line starts at the tail of current contour, then add it to the back
				else if (currLine.end == tail.end) currContour.push_back(flippedLine); //contours don't care about line's direction, but if we just connect it blindly, then the algorithm will die, so we flip it in case of mismatch
				else if (currLine.end == head.start) currContour.push_front(currLine);
				else if (currLine.start == head.start) currContour.push_front(flippedLine);

				assert(currContour.size() >= oldContourSize);
				assert(currContour.size() - oldContourSize <= 1); //no more than 1 add should be done per iteration
				if (currContour.size() != oldContourSize)
				{
					polygonLines[i--] = std::move(polygonLines.back());
					polygonLines.pop_back();
				}
			}
		} while (currContour.size() > preGrowContourSize && (currContour.back().end != currContour.front().start)); //try to grow contour while there are still more lines that can continue it and until it gets closed
	}

	for (auto& it : contours)
	{
		auto cont_map = PolygonBitmap::makeFrom(std::vector<Line>(it.begin(), it.end()));
		auto copy = bitmap;
		cont_map.blitOver(copy, true, CARVED);
		static int nCont = 0;
		copy.saveTo("sectors_debug/" + std::to_string(nSector-1) + "_" + std::to_string(nCont++) + ".png");

		assert(it.size() > 0);
		assert(it.back().second == it.front().first);
	}
	assert(contours.size() > 0);

	//TODO: add inner and outer contour finding
	//inner contour is a contour inside the polygon, but it describes a polygon that is inside our sector, but is not a part of this sector (i.e. a hole)
	//outer countours are contours that describe the polygon containing our entire sector, possibly also embedding other sector within it.
	//To avoid Z-fighting of overlapping polygon or complicated measures to curb it, we must triangulate only the outer contour(s) without holes
	//In theory, there should be only one outer contour, but it is not guaranteed.



	
	//fan triangulation works only for simple convex polygons. TODO: split the original polygon into convex ones with no holes

	Ved2 center(0, 0);
	for (auto& it : originalLines) center += it.start + it.end;
	center /= originalLines.size() * 2;

	std::vector<Ved2> points;
	for (int i = 0; i < originalLines.size(); ++i)
	{
		ret.push_back(center); //fan vertex
		ret.push_back(originalLines[i].start);
		ret.push_back(originalLines[i].end);
	};

	return ret;
}

std::vector<Ved2> PolygonTriangulator::tryCarve(const std::array<Ved2, 3>& trianglePoints, PolygonBitmap& bitmap)
{
	std::vector<Line> triangleLines(3);
	/*/Line p1 = std::make_pair(line.first, newVert);
			Line p2 = std::make_pair(newVert, line.second);
			Line p3 = std::make_pair(line.second, line.first);
			std::array<Line, 3> candidateTriangle = { p1, p2, p3};*/

	for (int i = 0; i < 3; ++i) triangleLines[i] = { trianglePoints[i], trianglePoints[i < 2 ? i + 1 : 0] };
	auto triangleBitmap = PolygonBitmap::makeFrom(triangleLines);
	if (triangleBitmap.areAllPointsInside(bitmap)) //very trivial for now, just check if entire triangle can fit
	{
		triangleBitmap.blitOver(bitmap, true, CARVED);
		return std::vector<Ved2>(trianglePoints.begin(), trianglePoints.end());
	}

	return std::vector<Ved2>();
}
