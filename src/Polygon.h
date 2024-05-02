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
	
	Polygon() = default;

	template <class Container>
	Polygon(const Container& lines);

	bool isPointInside(const Ved2& point) const;
	Ved2 getCenterPoint() const;

	std::pair<Polygon, Polygon> splitByLine(const Line& line) const; //first element is a polygon part behind the line, second - in front of. Line is considered infinite and defined by two points. Any returned polygon may have 0 lines
};

template<class Container>
inline Polygon::Polygon(const Container& lines) : lines(std::begin(lines), std::end(lines)) {}
