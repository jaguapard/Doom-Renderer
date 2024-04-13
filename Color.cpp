#include <SDL/SDL.h>
#include "Color.h"
#include <cassert>

Color::Color(uint32_t u)
{
	r = ((u & 0xFF00) >> 8) / 256.0;
	g = ((u & 0xFF0000) >> 16) / 256.0;
	b = ((u & 0xFF000000) >> 24) / 256.0;
	a = (u & 0xFF) / 256;
}

Color::Color(double r, double g, double b, double a) :r(r), g(g), b(b), a(a)
{
}

Uint32 Color::toSDL_Uint32(SDL_Surface* s)
{
	assert(r >= 0 && r <= 1);
	assert(g >= 0 && g <= 1);
	assert(b >= 0 && b <= 1);
	assert(a >= 0 && a <= 1);
	return SDL_MapRGBA(s->format, r * 255, g * 255, b * 255, a * 255);
}
