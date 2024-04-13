#pragma once
#include <cassert>
#include <string>
#include <vector>

template <typename T>
inline T lerp(const T& start, const T& end, double amount)
{
	return start + (end - start) * amount;
}

inline double inverse_lerp(double from, double to, double value)
{
	return (value - from) / (to - from);
}

inline void setPixel(SDL_Surface* s, int x, int y, uint32_t color)
{
	if (x >= 0 && y >= 0 && x < s->w && y < s->h)
	{
		uint32_t* px = (uint32_t*)s->pixels;
		px[y * s->w + x] = color;
	}
}

inline std::string wadStrToStd(const char* wadStr, int maxLen = 8)
{
	std::vector<char> buf(maxLen + 1, 0);
	memcpy(&buf.front(), wadStr, maxLen);
	return std::string(&buf.front()); 
}