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
#pragma comment(lib, "D:/Dropbox/_Programming/C++/Projects/Doom Rendering/x64/Release/GLUTesselator.lib")

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
	const double** contoursEnd = (const double**)(&contourAddrs.back()) + 1;
	double* outVerts;
	int* outTris;

	int out_nVerts, out_nTris;
	gluTesselate(&outVerts, &out_nVerts, &outTris, &out_nTris, contoursBegin, contoursEnd);

	for (int i = 0; i < out_nTris; ++i)
	{
		int vert_inds[] = {outTris[i * 3], outTris[i * 3 + 1], outTris[i * 3 + 2]};
		for (int j = 0; j < 3; ++j)
		{
			Ved2 vec = { outVerts[vert_inds[j] * 2], outVerts[vert_inds[j] * 2 + 1] };
			ret.push_back(vec);
		}
	}
	
	if (outVerts) free(outVerts);
	if (outTris) free(outTris);
	return ret;
}