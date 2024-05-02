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
	static std::vector<Ved2> tryCarve(const std::array<Ved2, 3>& trianglePoints, PolygonBitmap& bitmap);
};