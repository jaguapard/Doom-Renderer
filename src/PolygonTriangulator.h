#pragma once
#include <vector>
#include <array>
#include <string>

#include "PixelBuffer.h"
#include "Vec.h"
#include "Polygon.h"

class PolygonBitmap;

class PolygonTriangulator
{
public:
	static std::vector<Ved2> triangulate(Polygon polygon);
private:
};