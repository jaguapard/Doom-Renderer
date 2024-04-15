#include <SDL/SDL.h>
#include "Color.h"
#include <cassert>

static_assert(sizeof(Color) == sizeof(Uint32), "Color must have the same size as SDL's Uint32");

Color::Color(uint32_t u)
{
	*this = *reinterpret_cast<const Color*>(&u);
}

Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

Color::operator uint32_t() const
{
	return *reinterpret_cast<const uint32_t*>(this);
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