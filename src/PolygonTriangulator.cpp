#include <unordered_map>
#include <functional>
#include <set>
#include <unordered_set>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "PolygonTriangulator.h"
#include "Color.h"
#include "Polygon.h"
#include "PolygonBitmap.h"

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

std::vector<Ved2> PolygonTriangulator::triangulate(Polygon polygon)
{
	std::vector<Ved2> ret;
	if (polygon.lines.size() == 0) return ret;
	if (polygon.lines.size() < 3) throw std::runtime_error("Sector triangulation attempted with less than 3 lines!");

	Polygon originalPolygon = polygon;
	auto originalLines = polygon.lines;
	PolygonBitmap bitmap = PolygonBitmap::makeFrom(polygon);
	static int nSector = 0;
	bitmap.saveTo("sectors_debug/" + std::to_string(nSector++) + ".png");

	//find contours these lines make
	std::vector<std::deque<Line>> contours;
	while (!polygon.lines.empty())
	{
		Line startingLine = polygon.lines[0];
		contours.push_back({startingLine});
		polygon.lines[0] = std::move(polygon.lines.back());
		polygon.lines.pop_back();

		std::deque<Line>& currContour = contours.back();

		int preGrowContourSize;
		do {
			preGrowContourSize = currContour.size();
			for (int i = 0; i < polygon.lines.size(); ++i)
			{
				const int oldContourSize = currContour.size();
				const Line& currLine = polygon.lines[i];
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
					polygon.lines[i--] = std::move(polygon.lines.back());
					polygon.lines.pop_back();
				}
			}
		} while (currContour.size() > preGrowContourSize && (currContour.back().end != currContour.front().start)); //try to grow contour while there are still more lines that can continue it and until it gets closed
	}

	for (auto& it : contours)
	{
		auto cont_map = PolygonBitmap::makeFrom(it);
		auto copy = bitmap;
		cont_map.blitOver(copy, true, CARVED);
		static int nCont = 0;
		copy.saveTo("sectors_debug/" + std::to_string(nSector-1) + "_" + std::to_string(nCont++) + ".png");

		assert(it.size() > 0);
		assert(it.back().end == it.front().start);
	}
	assert(contours.size() > 0);

	//TODO: add inner and outer contour finding
	//inner contour is a contour inside the polygon, but it describes a polygon that is inside our sector, but is not a part of this sector (i.e. a hole)
	//outer countours are contours that describe the polygon containing our entire sector, possibly also embedding other sector within it.
	//To avoid Z-fighting of overlapping polygon or complicated measures to curb it, we must triangulate only the outer contour(s) without holes
	//In theory, there should be only one outer contour, but it is not guaranteed.



	
	//fan triangulation works only for simple convex polygons. TODO: split the original polygon into convex ones with no holes

	Ved2 center = originalPolygon.getCenterPoint();

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
