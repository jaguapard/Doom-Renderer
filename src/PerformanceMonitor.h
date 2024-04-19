#pragma once
#include <bob/Timer.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <deque>

#include "smart.h"
class PerformanceMonitor
{
public:
	PerformanceMonitor();
	void reset();
	void registerFrameDone(bool remember = true);
	void drawOn(SDL_Surface* dst, SDL_Point pixelsFromUpperLeftCorner);
private:
	bob::Timer timer;
	uint64_t frameNumber = 0;
	Smart_Font font;
	std::deque<double> frameTimesMs;

	struct PercentileInfo
	{
		PercentileInfo(uint64_t frameNumber, std::deque<double> frameTimesMs); //copy is intentional, since constructor will garble the data
		std::string toString();

		double fps_inst = 0, fps_avg = 0, fps_1pct_low = 0, fps_point1pct_low = 0;
		uint64_t frameNumber;
	};
};