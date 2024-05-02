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
	
	template <class Container>
	Polygon(const Container& lines);

	bool isPointInside(const Ved2& point) const;
	Ved2 getCenterPoint() const;
};

template<class Container>
inline Polygon::Polygon(const Container& lines) : lines(std::begin(lines), std::end(lines)) {}
