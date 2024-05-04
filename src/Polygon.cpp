#include "Polygon.h"
#include "PolygonBitmap.h"
#include <cassert>

double scalarCross2d(Ved2 a, Ved2 b)
{
	return a.x * b.y - a.y * b.x;
}

std::optional<std::pair<Ved2, double>> getRayLineIntersectionPoint(const Ved2& rayOrigin, const Line& line, Ved2 rayDir = { sqrt(2), sqrt(3) })
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
		//return std::make_pair(sv + (ev - sv) * t2, t2);
		//return std::make_pair(rayOrigin + v3 * t1, t1);
		return std::make_pair(rayOrigin + rayDir * t1, t1);
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

std::optional<std::pair<Ved2, double>> getLineToInfiniteLineIntersectionPoint(const Line& line, const Line& infiniteLine)
{
	Ved2 infLineDir = infiniteLine.end - infiniteLine.start;
	auto intersect = getRayLineIntersectionPoint(infiniteLine.start, line, infLineDir);
	if (intersect) return intersect;
	
	intersect = getRayLineIntersectionPoint(infiniteLine.start, line, -infLineDir);
	if (intersect) return std::make_pair(intersect.value().first, -intersect.value().second);
	return std::nullopt;  //2 opposite rays = infinite line
}

std::vector<Contour> Polygon::createContours() const
{
	return Contour::getContoursOfPolygon(*this);
}

bool Polygon::isPointInside(const Ved2& point) const
{
	int intersections = 0;
	for (const auto& it : lines) intersections += getRayLineIntersectionPoint(point, it).has_value();
	return intersections % 2 == 1;
}

//Line is considered infinite and defined by two points
bool isPointBehindLine(const Ved2& point, const Line& line)
{
	Ved2 lineDir = line.end - line.start;
	Ved2 normal = { -lineDir.y, lineDir.x };

	Ved2 lineToPoint = point - line.start;
	double dot = normal.dot(lineToPoint);
	return dot < 0;
}

Ved2 Polygon::getCenterPoint() const
{
	Ved2 sum = { 0,0 };
	for (const auto& it : lines) sum += it.start + it.end;
	return sum / (lines.size() * 2);
}

std::pair<Polygon, Polygon> Polygon::splitByLine(const Line& splitLine) const
{
	static int nSector = -1;
	++nSector;
	Polygon front, back;
	Ved2 splitLineDir = splitLine.end - splitLine.start;
	double minIntersectT = std::numeric_limits<double>::infinity();
	double maxIntersectT = -minIntersectT;
	for (const auto& it : this->lines)
	{
		auto intersect = getLineToInfiniteLineIntersectionPoint(it, splitLine);

		if (!intersect) //if current line does not intersect the splitting line, we just need to check the side
		{
			if (isPointBehindLine(it.start, splitLine)) back.lines.push_back(it);
			else front.lines.push_back(it);
			continue;
		}

		//otherwise it means that this line is partially behind and partially in front of splitting line. We must split it and put the pieces into correct polygons
		Ved2 splitPoint = intersect.value().first;
		Line piece1 = { it.start, splitPoint };
		Line piece2 = { splitPoint, it.end };

		(isPointBehindLine(it.start, splitLine) ? back : front).lines.push_back(piece1);
		(isPointBehindLine(it.end, splitLine) ? back : front).lines.push_back(piece2);
		minIntersectT = std::min(minIntersectT, intersect.value().second);
		maxIntersectT = std::max(maxIntersectT, intersect.value().second);
	}

	//TODO: make sure polygons are closed? Add splitting line to them?
	Line plug;
	plug.start = splitLine.start + splitLineDir * minIntersectT;
	plug.end = splitLine.start + splitLineDir * maxIntersectT;
	if (front.lines.size() > 0 && back.lines.size() > 0) //stitch the cut. This does not work, since line indiscriminantly covers up stuff, even if they are outside the sector
	{
		front.lines.push_back(plug); 
		back.lines.push_back(plug);
	}

	PolygonBitmap::makeFrom(front).saveTo("sectors_debug/" + std::to_string(nSector - 1) + "_split1.png");
	PolygonBitmap::makeFrom(back).saveTo("sectors_debug/" + std::to_string(nSector - 1) + "_split2.png");
	return std::make_pair(front, back);
}

std::vector<Contour> Contour::getContoursOfPolygon(Polygon polygon)
{
	//find contours these lines make
	std::vector<Contour> contours;

	PolygonBitmap bitmap = PolygonBitmap::makeFrom(polygon);
	static int nSector = 0;
	bitmap.saveTo("sectors_debug/" + std::to_string(nSector++) + ".png");

	while (!polygon.lines.empty())
	{
		Line startingLine = polygon.lines[0];
		contours.push_back(std::deque<Line>(1, startingLine));
		polygon.lines[0] = std::move(polygon.lines.back());
		polygon.lines.pop_back();

		std::deque<Line>& currContour = contours.back().lines;

		int preGrowContourSize;
		do {
			preGrowContourSize = currContour.size();
			for (int i = 0; i < polygon.lines.size(); ++i)
			{
				const int oldContourSize = currContour.size();
				const Line& currLine = polygon.lines[i];
				Line flippedLine = { currLine.end, currLine.start };

				const Line& head = currContour.front();
				const Line& tail = currContour.back();

				if (currLine.start == tail.end) currContour.push_back(currLine); //if current line starts at the tail of current contour, then add it to the back
				else if (currLine.end == tail.end) currContour.push_back(flippedLine); //contours don't care about line's direction, but if we just connect it blindly, then the algorithm will die, so we flip it in case of mismatch
				else if (currLine.end == head.start) currContour.push_front(currLine);
				else if (currLine.start == head.start) currContour.push_front(flippedLine);

				assert(currContour.size() >= oldContourSize);
				assert(currContour.size() - oldContourSize <= 1); //no more than 1 add should be done per iteration
				if (currContour.size() != oldContourSize)
				{
					polygon.lines[i--] = std::move(polygon.lines.back());
					polygon.lines.pop_back();
				}
			}
		} while (currContour.size() > preGrowContourSize && (currContour.back().end != currContour.front().start)); //try to grow contour while there are still more lines that can continue it and until it gets closed
	}

	for (auto& it : contours)
	{
		auto cont_map = PolygonBitmap::makeFrom(it);
		auto copy = bitmap;
		cont_map.blitOver(copy, true, CARVED);
		static int nCont = 0;
		copy.saveTo("sectors_debug/" + std::to_string(nSector - 1) + "_" + std::to_string(nCont++) + ".png");

		assert(!it.lines.empty());
		assert(it.lines.back().end == it.lines.front().start);
	}
	assert(contours.size() > 0);

	return contours;
}

bool Contour::isClosed() const
{
	return lines.size() >= 3 && lines.back().end == lines.front().start;
}

Contour Contour::closeByLine(const Line& line) const
{
	if (this->isClosed()) return *this;

	Contour copy = *this;
	return copy;
}
