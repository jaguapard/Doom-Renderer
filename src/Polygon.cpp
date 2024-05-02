#include "Polygon.h"

double scalarCross2d(Ved2 a, Ved2 b)
{
	return a.x * b.y - a.y * b.x;
}

std::optional<Ved2> getRayLineIntersectionPoint(const Ved2& rayOrigin, const Line& line, Ved2 rayDir = { sqrt(2), sqrt(3) })
{
	const Ved2& sv = line.start;
	const Ved2& ev = line.end;

	//floating point precision is a bitch here, force use doubles everywhere
	Ved2 v1 = rayOrigin - sv;
	Ved2 v2 = ev - sv;
	Ved2 v3 = { -rayDir.y, rayDir.x };

	double dot = v2.dot(v3);
	//if (abs(dot) < 1e-9) return false;

	double t1 = scalarCross2d(v2, v1) / dot;
	double t2 = v1.dot(v3) / dot;

	//we are fine with false positives (point is considered inside when it's not), but false negatives are absolutely murderous
	if (t1 >= 1e-9 && (t2 >= 1e-9 && t2 <= 1.0 - 1e-9))
		return sv + (ev - sv) * t2;
	return std::nullopt;

	//these are safeguards against rampant global find-and-replace mishaps breaking everything
	static_assert(sizeof(sv) == 16);
	static_assert(sizeof(ev) == 16);
	static_assert(sizeof(v1) == 16);
	static_assert(sizeof(v2) == 16);
	static_assert(sizeof(v3) == 16);
	static_assert(sizeof(dot) == 8);
	static_assert(sizeof(t1) == 8);
	static_assert(sizeof(t2) == 8);
	static_assert(sizeof(rayDir) == 16);
}

bool Polygon::isPointInside(const Ved2& point) const
{
	assert(lines.size() >= 3);
	int intersections = 0;
	for (const auto& it : lines) intersections += getRayLineIntersectionPoint(point, it).has_value();
	return intersections % 2 == 1;
}

Ved2 Polygon::getCenterPoint() const
{
	Ved2 sum = { 0,0 };
	for (const auto& it : lines) sum += it.start + it.end;
	return sum / (lines.size() * 2);
}
