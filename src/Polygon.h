#pragma once
#include <vector>
#include "Vec.h"

struct Line
{
	Ved2 start, end;

};

struct Polygon
{
	std::vector<Line> lines;
	Polygon(const std::vector<Line>& lines);

	bool isPointInside(const Ved2& point) const;
};