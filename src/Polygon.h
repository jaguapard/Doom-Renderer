#pragma once
#include <deque>
#include <vector>
#include "Vec.h"

struct Line
{
	Ved2 start, end;

};

class Contour;
struct Polygon
{
	std::deque<Line> lines;
	
	Polygon() = default;

	template <class Container>
	Polygon(const Container& lines);

	bool isPointInside(const Ved2& point) const;
	Ved2 getCenterPoint() const;

	std::vector<Contour> createContours() const;

	std::pair<Polygon, Polygon> splitByLine(const Line& line) const; //first element is a polygon part behind the line, second - in front of. Line is considered infinite and defined by two points. Any returned polygon may have 0 lines
};

class Contour
{
public:
	static std::vector<Contour> getContoursOfPolygon(Polygon polygon);
	bool isClosed() const;
	Contour closeByLine(const Line& line) const;
private:
	std::deque<Line> lines;

	template <class Container>
	Contour(const Container& lines);
};

template<class Container>
inline Polygon::Polygon(const Container& lines) : lines(std::begin(lines), std::end(lines)) {};

template<class Container>
inline Contour::Contour(const Container& lines) : lines(std::begin(lines), std::end(lines)) {};
