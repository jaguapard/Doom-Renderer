#pragma once
#include <stdint.h>

struct Color
{
	Color() = default;	
	Color(uint32_t u);
	Color(double r, double g, double b, double a);

	Uint32 toSDL_Uint32(SDL_Surface* s);

	double r, g, b, a;
};