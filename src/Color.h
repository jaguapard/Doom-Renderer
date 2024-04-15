#pragma once
#include <stdint.h>

struct Color : SDL_Color
{
	Color() = default;	
	Color(uint32_t u); //pixel format is assumed to be SDL_PIXELFORMAT_RGBA32
	Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

	operator uint32_t() const;
	Uint32 toSDL_Uint32(const uint32_t* shifts = _shift) const; //only SDL_PIXELFORMAT_RGBA32 suported!!!

	static constexpr uint32_t _masks[4] = {0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000};
	static constexpr uint32_t _shift[4] = { 0, 8, 16, 24 };
};