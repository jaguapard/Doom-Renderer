#pragma once
#include <vector>
#include <array>
#include "PixelBuffer.h"
#include "Vec.h"

typedef std::pair<Ved2, Ved2> Line;

enum PolygonBitmapValue : uint8_t
{
	OUTSIDE = 0,
	INSIDE = 1,
	CARVED = 2,

	NONE, //value used to signalize the abscence of value
};

class PolygonBitmap : PixelBuffer<uint8_t>
{
public:
	static PolygonBitmap makeFrom(const std::vector<Line>& polygon);
	void saveTo(std::string path);
	void blitOver(PolygonBitmap& dst, bool doNotBlitOutsides, PolygonBitmapValue valueOverride); //if doNotBlitOutsides is true, then OUTSIDE values from this bitmap will not be blitted to dst

	int getMaxX() const;
	int getMaxY() const;
	int getMinX() const;
	int getMinY() const;	
private:
	PolygonBitmap(int minX, int minY, int maxX, int maxY, int w, int h, const std::vector<Line>& polygon);
	int polygonMinX, polygonMinY;
};

class PolygonTriangulator
{
public:
	static std::vector<Ved2> triangulate(std::vector<Line> polygonLines);
private:	
	static std::vector<Ved2> tryCarve(const std::array<Line, 3>& triangle, PolygonBitmap& bitmap);
};