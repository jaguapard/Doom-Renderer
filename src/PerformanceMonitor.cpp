#include "PerformanceMonitor.h"
#include <algorithm>
#include <numeric>

#include "Statsman.h"
PerformanceMonitor::PerformanceMonitor()
{
	std::string path = "C:/Windows/Fonts/Arial.ttf";
	font = Smart_Font(TTF_OpenFont(path.c_str(), 14));
	if (!font) throw std::runtime_error("Failed to open font: " + path + ": " + TTF_GetError());
}

void PerformanceMonitor::reset()
{
	frameTimesMs.clear();
	timer = bob::Timer();
}

void PerformanceMonitor::registerFrameBegin()
{
	oldStats = statsman;
}

void PerformanceMonitor::registerFrameDone(bool remember)
{
	double time = timer.getTimeSinceLastPoll() * 1000;
	if (remember)
	{
		++frameNumber;
		frameTimesMs.push_back(time);
		if (frameTimesMs.size() > 1000) frameTimesMs.pop_front();
	}
}

void PerformanceMonitor::drawOn(SDL_Surface* dst, SDL_Point pixelsFromUpperLeftCorner)
{
	std::string text = PercentileInfo(frameNumber, frameTimesMs).toString();
	std::stringstream ss;

	ss << timer.getTime() << " sec\n";
	ss << text;

	if (true && Statsman::enabled) //statsman stuff
	{
		ss << (statsman-oldStats).toString() << "\n";
	}

	auto s = Smart_Surface(TTF_RenderUTF8_Solid_Wrapped(font.get(), ss.str().c_str(), {255,255,255,SDL_ALPHA_OPAQUE}, 400));
	if (!s) throw std::runtime_error(std::string("Failed to draw FPS info: ") + SDL_GetError());
	SDL_Rect rect = { pixelsFromUpperLeftCorner.x, pixelsFromUpperLeftCorner.y, s->w, s->h };
	SDL_BlitSurface(s.get(), nullptr, dst, &rect);
}

PerformanceMonitor::PercentileInfo::PercentileInfo(uint64_t frameNumber, std::deque<double> frameTimesMs)
{
	this->frameNumber = frameNumber;
	if (frameTimesMs.empty()) return;

	fps_inst = 1000/frameTimesMs.back();
	double totalTime = std::accumulate(frameTimesMs.begin(), frameTimesMs.end(), 0);
	fps_avg = 1000 / (totalTime / frameTimesMs.size());

	std::sort(frameTimesMs.rbegin(), frameTimesMs.rend());
	int n = frameTimesMs.size();
	if (frameTimesMs.size() >= 100) fps_1pct_low = 1000.0 / frameTimesMs[n / 100 - 1];
	if (frameTimesMs.size() >= 1000) fps_point1pct_low = 1000.0 / frameTimesMs[n / 1000 - 1];
}

std::string PerformanceMonitor::PercentileInfo::toString()
{
	std::stringstream text;
	text << "Frame " << frameNumber << "\n" <<
		fps_inst << " FPS inst\n" <<
		(fps_avg ? std::to_string(fps_avg) : "n/a") << " FPS avg\n" <<
		"1% low: " << (fps_1pct_low ? std::to_string(fps_1pct_low) : "n/a") << "\n" <<
		"0.1% low: " << (fps_point1pct_low ? std::to_string(fps_point1pct_low) : "n/a") << "\n";
	return text.str();
}
