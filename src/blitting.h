#pragma once

#include <array>
#include <SDL/SDL.h>

#include "PixelBuffer.h"

namespace blitting
{
	void lightIntoFrameBuffer(PixelBuffer<Color>& frameBuf, const PixelBuffer<real>& lightBuf, size_t minY, size_t maxY);
	void frameBufferIntoSurface(const PixelBuffer<Color>& frameBuf, SDL_Surface* surf, size_t minY, size_t maxY, const std::array<uint32_t, 4>& shifts);
}