#pragma once
#include "bob/Timer.h"
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <deque>
#include <map>

#include "smart.h"
#include "Statsman.h"
#include "Vec.h"

class PerformanceMonitor
{
public:
	struct OptionalInfo;

	PerformanceMonitor();
	void reset();
	void registerFrameBegin(); //must be called before any drawing and processing is attempted
	void registerFrameDone(bool remember = true);
	void drawOn(SDL_Surface* dst, SDL_Point pixelsFromUpperLeftCorner, const std::map<std::string, std::string>& additionalInfo);

	struct PercentileInfo
	{
		PercentileInfo(uint64_t frameNumber, std::deque<double> frameTimesMs); //copy is intentional, since constructor will garble the data
		std::string toString();

		double fps_inst = 0, fps_avg = 0, fps_1pct_low = 0, fps_point1pct_low = 0;
		uint64_t frameNumber;
	};

	PercentileInfo getPercentileInfo() const;
private:
	Smart_Font font;
	bob::Timer timer;

	Statsman oldStats;
	uint64_t frameNumber = 0;
	std::deque<double> frameTimesMs;
};