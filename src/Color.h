#pragma once
#include <stdint.h>

struct Color : SDL_Color
{
	Color() = default;	
	Color(uint32_t u); //pixel format is assumed to be SDL_PIXELFORMAT_RGBA32
	Color(uint8_t r, uint8_t g, uint8_t b); //alpha value is set to SDL_ALPHA_OPAQUE
	Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

	Color multipliedByLight(double lightMult) const; //lighting calculation must preserve alpha value

	operator uint32_t() const;
	Uint32 toSDL_Uint32(const uint32_t* shifts = _shift) const; //only SDL_PIXELFORMAT_RGBA32 suported!!!

	static constexpr uint32_t _masks[4] = { 0xFFu << 0, 0xFFu << 8, 0xFFu << 16, 0xFFu << 24 };
	static constexpr uint32_t _shift[4] = { 0, 8, 16, 24 };
};