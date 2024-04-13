#include <SDL/SDL.h>
#include "Color.h"
#include <cassert>

Color::Color(uint32_t u)
{
	/*/r = ((u & 0xFF00) >> 8) / 255.0;
	g = ((u & 0xFF0000) >> 16) / 255.0;
	b = ((u & 0xFF000000) >> 24) / 255.0;
	a = (u & 0xFF) / 255.0;*/

	uint32_t ic[4];
	for (int i = 0; i < 4; ++i) ic[i] = (u & _masks[i]) >> _shift[i];
	r = ic[0] / 255.0;
	g = ic[1] / 255.0;
	b = ic[2] / 255.0;
	a = ic[3] / 255.0;

	assert(r >= 0 && r <= 1);
	assert(g >= 0 && g <= 1);
	assert(b >= 0 && b <= 1);
	assert(a >= 0 && a <= 1);
}

Color::Color(double r, double g, double b, double a) :r(r), g(g), b(b), a(a)
{
}

Uint32 Color::toSDL_Uint32(const uint32_t* shifts) const
{
	assert(r >= 0 && r <= 1);
	assert(g >= 0 && g <= 1);
	assert(b >= 0 && b <= 1);
	assert(a >= 0 && a <= 1);
	
	uint32_t ret = 0;
	uint32_t vals[4] = { r * 255, g * 255, b * 255, a * 255 };
	for (int i = 0; i < 4; ++i) ret |= vals[i] << shifts[i];

	return ret;
}