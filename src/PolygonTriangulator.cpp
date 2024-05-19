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

#include <GLUTesselator/src/tessellate.h>
//#pragma comment(lib, "GLUTesselator.lib")
#pragma comment(lib, "D:/Dropbox/_Programming/C++/Projects/Doom Rendering/x64/Debug/GLUTesselator.lib")

extern "C" void gluTesselate(double** verts, int* nverts, int** tris, int* ntris, const double** contoursbegin, const double** contoursend);

std::vector<Ved2> PolygonTriangulator::triangulate(Polygon polygon)
{
	std::vector<Ved2> ret;
	if (polygon.lines.size() == 0) return ret;
	if (polygon.lines.size() < 3) throw std::runtime_error("Sector triangulation attempted with less than 3 lines!");

	auto contours = polygon.createContours(); //make contours and convert data to feed it into GLUTesselator
	std::vector<double> rawVerts;
	std::vector<size_t> contourOffsets = { 0 };

	for (const auto& cont : contours)
	{
		const auto& lines = cont.getLines();
		for (size_t i = 0; i < lines.size(); ++i)
		{
			rawVerts.push_back(lines[i].start.x);
			rawVerts.push_back(lines[i].start.y);
		}
		contourOffsets.push_back(rawVerts.size());
	}

	std::vector<double*> contourAddrs;
	for (auto& it : contourOffsets) contourAddrs.push_back(rawVerts.data() + it);

	const double** contoursBegin = (const double**)&contourAddrs.front();
	const double** contoursEnd = (const double**)(& contourAddrs.back()) + 1;
	double* outVerts;
	int* outTris;

	int out_nVerts, out_nTris;
	gluTesselate(&outVerts, &out_nVerts, &outTris, &out_nTris, contoursBegin, contoursEnd);
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
