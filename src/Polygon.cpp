#include "Polygon.h"

double scalarCross2d(Ved2 a, Ved2 b)
{
	return a.x * b.y - a.y * b.x;
}

bool rayCrossesLine(const Ved2& p, const Line& line)
{
	const Ved2& sv = line.start;
	const Ved2& ev = line.end;

	//floating point precision is a bitch here, force use doubles everywhere
	Ved2 v1 = p - sv;
	Ved2 v2 = ev - sv;
	Ved2 v3 = { -sqrt(2), sqrt(3) }; //using irrational values to never get pucked by linedefs being perfectly collinear to our ray by chance
	v3 /= v3.len(); //idk, seems like a good measure
	double dot = v2.dot(v3);
	//if (abs(dot) < 1e-9) return false;

	double t1 = scalarCross2d(v2, v1) / dot;
	double t2 = v1.dot(v3) / dot;

	//we are fine with false positives (point is considered inside when it's not), but false negatives are absolutely murderous
	if (t1 >= 1e-9 && (t2 >= 1e-9 && t2 <= 1.0 - 1e-9))
		return true;
	return false;

	//these are safeguards against rampant global find-and-replace mishaps breaking everything
	static_assert(sizeof(sv) == 16);
	static_assert(sizeof(ev) == 16);
	static_assert(sizeof(v1) == 16);
	static_assert(sizeof(v2) == 16);
	static_assert(sizeof(v3) == 16);
	static_assert(sizeof(dot) == 8);
	static_assert(sizeof(t1) == 8);
	static_assert(sizeof(t2) == 8);
	static_assert(sizeof(p) == 16);
}

bool Polygon::isPointInside(const Ved2& point) const
{
	assert(lines.size() >= 3);
	int intersections = 0;
	for (const auto& it : lines) intersections += rayCrossesLine(point, it);
	return intersections % 2 == 1;
}

Ved2 Polygon::getCenterPoint() const
{
	Ved2 sum = { 0,0 };
	for (const auto& it : lines) sum += it.start + it.end;
	return sum / (lines.size() * 2);
}
