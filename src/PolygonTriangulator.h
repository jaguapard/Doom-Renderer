#pragma once
#include <vector>
#include <array>
#include "PixelBuffer.h"
#include "Vec.h"
#include <string>

struct Line
{
	Ved2 start, end;

};
enum PolygonBitmapValue : uint8_t
{
	OUTSIDE = 0,
	INSIDE = 1,
	CARVED = 2,
	RECTANGLEFIED = 3,

	NONE, //value used to signalize the abscence of value
};

class PolygonBitmap : public PixelBuffer<uint8_t>
{
public:
	static PolygonBitmap makeFrom(const std::vector<Line>& polygon);
	void saveTo(std::string path);
	void blitOver(PolygonBitmap& dst, bool doNotBlitOutsides, PolygonBitmapValue valueOverride); //if doNotBlitOutsides is true, then OUTSIDE values from this bitmap will not be blitted to dst

	bool areAllPointsInside(const PolygonBitmap& other) const;

	uint8_t getValueAt(int x, int y) const; //"world" XY, not pixel xy inside the buffer
	void setValueAt(int x, int y, uint8_t val);

	uint8_t& atXY(int x, int y);
	const uint8_t& atXY(int x, int y) const;

	bool isInBoundsOf(const PolygonBitmap& other) const;

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
	static std::vector<Ved2> tryCarve(const std::array<Ved2, 3>& trianglePoints, PolygonBitmap& bitmap);
};