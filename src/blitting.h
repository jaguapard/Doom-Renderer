#pragma once

#include <array>
#include <SDL/SDL.h>

#include "PixelBuffer.h"
#include "FloatColorBuffer.h"
#include "ZBuffer.h"
#include "misc/Enums.h"

namespace blitting
{
	void lightIntoFrameBuffer(FloatColorBuffer& frameBuf, const PixelBuffer<real>& lightBuf, size_t minY, size_t maxY);
	void frameBufferIntoSurface(const FloatColorBuffer& frameBuf, SDL_Surface* surf, size_t minY, size_t maxY, std::array<uint32_t, 4> shifts, bool ditheringEnabled, uint32_t ssaaMult);
	void applyFog(FloatColorBuffer& frameBuf, const FloatColorBuffer& worldPos, Vec4 camPos, float fogIntensity, Vec4 fogColor, size_t minY, size_t maxY, FogEffectVersion fogEffectVersion);
}