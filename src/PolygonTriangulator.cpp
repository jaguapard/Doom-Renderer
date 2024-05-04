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

	//originalPolygon.createContours();
	auto p = polygon.splitByLine({ {0,0},{1,1} });
	/*
	

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
	*/
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
