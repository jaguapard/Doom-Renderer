#pragma once
#include <stdint.h>

struct Color
{
	Color() = default;	
	Color(uint32_t u); //pixel format is assumed to be SDL_PIXELFORMAT_RGBA32
	Color(double r, double g, double b, double a);

	Uint32 toSDL_Uint32(const uint32_t* shifts = _shift) const; //only SDL_PIXELFORMAT_RGBA32 suported!!!

	double r, g, b, a;

	static constexpr uint32_t _masks[4] = {0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000};
	static constexpr uint32_t _shift[4] = { 0, 8, 16, 24 };
};