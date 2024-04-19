#include "PerformanceMonitor.h"

PerformanceMonitor::PerformanceMonitor()
{
	std::string path = "C:/Windows/Fonts/Arial.ttf";
	font = Smart_Font(TTF_OpenFont(path.c_str(), 14));
	if (!font) throw std::runtime_error("Failed to open font: " + path + ": " + TTF_GetError());
}

void PerformanceMonitor::reset()
{
	frameTimesMs.clear();
	timer.restart();
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
	std::stringstream text;
	text << "Frame " << frameNumber << "\n" <<
		1000 / frameTimesMs.back() << " FPS\n" <<
		frameTimesMs.back() << " ms\n";
	
	auto s = Smart_Surface(TTF_RenderUTF8_Blended(font.get(), text.str().c_str(), {255,255,255,SDL_ALPHA_OPAQUE}));
	if (!s) throw std::runtime_error(std::string("Failed to draw FPS info: ") + SDL_GetError());
	SDL_Rect rect = { pixelsFromUpperLeftCorner.x, pixelsFromUpperLeftCorner.y, s->w, s->h };
	SDL_BlitSurface(s.get(), nullptr, dst, &rect);
}
