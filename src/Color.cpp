#include <SDL/SDL.h>
#include "Color.h"
#include <cassert>
//#include "Vec.h"

//A bunch of sanity checks. The code in this project assumes all of this
static_assert(sizeof(Color) == sizeof(Uint32), "Color must have the same size as SDL's Uint32");
static_assert(offsetof(Color, r) == offsetof(SDL_Color, r));
static_assert(offsetof(Color, g) == offsetof(SDL_Color, g));
static_assert(offsetof(Color, b) == offsetof(SDL_Color, b));
static_assert(offsetof(Color, a) == offsetof(SDL_Color, a));

static_assert(offsetof(Color, r) == 0);
static_assert(offsetof(Color, g) == 1);
static_assert(offsetof(Color, b) == 2);
static_assert(offsetof(Color, a) == 3);

Color::Color(uint32_t u)
{
	*this = *reinterpret_cast<const Color*>(&u);
}

Color::Color(uint8_t r, uint8_t g, uint8_t b) : SDL_Color{r,g,b,SDL_ALPHA_OPAQUE} {}
Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : SDL_Color{ r,g,b,a } {}

Color Color::multipliedByLight(real lightMult) const
{
	return Color(r * lightMult, g * lightMult, b * lightMult, a);
}

Color::operator uint32_t() const
{
	return *reinterpret_cast<const uint32_t*>(this);
}

Uint32 Color::toSDL_Uint32(const std::array<uint32_t, 4>& shifts) const
{
	uint32_t ret = 0;
	uint32_t vals[4] = { r, g, b, a };
	for (int i = 0; i < 4; ++i) ret |= vals[i] << shifts[i];

	return ret;
}