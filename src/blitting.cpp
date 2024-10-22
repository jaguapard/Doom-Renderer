#include "blitting.h"
#include "Lehmer.h"
#include "avx_helpers.h"
#include "IntPack16.h"

const IntPack16 sequence512 = IntPack16::sequence();
//static thread_local LehmerRNG rng;

void blitting::lightIntoFrameBuffer(FloatColorBuffer& frameBuf, const PixelBuffer<real>& lightBuf, size_t minY, size_t maxY)
{
	assert(frameBuf.getW() == lightBuf.getW());
	assert(frameBuf.getH() == lightBuf.getH());
	assert(minY < maxY);

	size_t startIndex = minY * frameBuf.getW();
	size_t endIndex = maxY * frameBuf.getW();

	for (size_t i = startIndex; i < endIndex; i += 16)
	{
		__mmask16 bounds = _mm512_cmplt_epi32_mask(_mm512_add_epi32(_mm512_set1_epi32(i), sequence512), _mm512_set1_epi32(endIndex));
		VectorPack16 multiplied = frameBuf.getPixels16(i) * (lightBuf.getRawPixels() + i);
		frameBuf.setPixels16(i, multiplied, bounds);
	}
}

void blitting::frameBufferIntoSurface(const FloatColorBuffer& frameBuf, SDL_Surface* surf, size_t minY, size_t maxY, const std::array<uint32_t, 4> shifts, const bool ditheringEnabled, const uint32_t ssaaMult, LehmerRNG& rngSource)
{
	assert(frameBuf.getW() == surf->w * ssaaMult);
	assert(frameBuf.getH() == surf->h * ssaaMult);
	assert(minY < maxY);
	assert(surf->pitch == surf->w * sizeof(Color));

	const int w = surf->w;
	Uint32* surfPixelsStart = reinterpret_cast<Uint32*>(surf->pixels);

	int32_t intraColorReshuffleMask = shifts[0] >> 3 | shifts[1] << 5 | shifts[2] << 13 | shifts[3] << 21;
	__m128i shuffleMask128 = _mm_add_epi32(_mm_set1_epi32(intraColorReshuffleMask), _mm_setr_epi32(0, 0x04040404, 0x08080808, 0x0c0c0c0c)); //broadcast the intraColorReshuffleMask and add 0, 4, 8 or 12 to each byte.

	/*int byteIndexR = shifts[0] >> 3;
	int byteIndexG = shifts[1] << 5;
	int byteIndexB = shifts[0] << 13;
	int byteIndexA = shifts[0] << 21;
	std::array<int8_t, 64> permuteMask_rg, permuteMask_ba;

	for (int i = 0; i < 16; ++i)
	{
		permuteMask_rg[i*4+byteIndexR] =  
	}*/

	IntPack16 step = IntPack16::sequence(ssaaMult);
	for (int dstY = minY; dstY < maxY; ++dstY)
	{
		for (int dstX = 0; dstX < w; dstX += 16)
		{
			IntPack16 dstX_vec = dstX + sequence512;
			Mask16 loopBounds = dstX_vec < w;

			VectorPack16 screenPixels = 0;
			if (ssaaMult > 1)
			{
				for (int y = 0; y < ssaaMult; ++y)
				{
					for (int x = 0; x < ssaaMult; ++x)
					{
						IntPack16 xCoords = dstX * ssaaMult + x + step;
						IntPack16 yCoords = dstY * ssaaMult + y;
						screenPixels += frameBuf.gatherPixels16(xCoords, yCoords, loopBounds); //gather horizontal pixels in steps of ssaaMult from ssaaMult consecutive rows
					}
				}
				screenPixels *= 255.0 / (ssaaMult * ssaaMult); //now screenPixels have averaged pixels ready to be converted to ints for further manipulations
			}
			else screenPixels = frameBuf.getPixels16(dstX, dstY) * 255;

			IntPack16 surfacePixels = 0;
			for (int i = 0; i < 4; ++i)
			{
				IntPack16 channelValue = screenPixels[i].trunc(); //now lower bits of each epi32 contain values of channels
				if (ditheringEnabled)
				{
					IntPack16 rngOut = rngSource.next();
					FloatPack16 increaseThreshold = (screenPixels[i] - _mm512_floor_ps(screenPixels[i])) * float(UINT32_MAX);

					Mask16 increaseMask = _mm512_cmplt_epu32_mask(rngOut, _mm512_cvttps_epu32(increaseThreshold));
					channelValue += IntPack16(1, increaseMask);
				}

				surfacePixels |= channelValue.clamp(0, 255) << shifts[i];
			}

			surfacePixels.store(surfPixelsStart + dstY * w + dstX, loopBounds);
		}
	}
}

void blitting::applyFog(FloatColorBuffer& frameBuf, const FloatColorBuffer& worldPos, Vec4 camPos, float fogIntensity, Vec4 fogColor, size_t minY, size_t maxY, FogEffectVersion fogEffectVersion)
{
	size_t startIndex = minY * frameBuf.getW();
	size_t endIndex = maxY * frameBuf.getW();

	VectorPack16 fogColorPack = fogColor;

	for (size_t i = startIndex; i < endIndex; i += 16)
	{
		__mmask16 bounds = _mm512_cmplt_epi32_mask(_mm512_add_epi32(_mm512_set1_epi32(i), sequence512), _mm512_set1_epi32(endIndex));
		VectorPack16 origColors = frameBuf.getPixels16(i);

        FloatPack16 dist = _mm512_sqrt_ps((worldPos.getPixels16(i) - camPos).lenSq3d());
		Mask16 emptyMask = dist == 0.0;

		FloatPack16 lerpT;
		if (fogEffectVersion == FogEffectVersion::LINEAR_WITH_CLAMP)
		{
			lerpT = dist / fogIntensity;
			lerpT = _mm512_mask_blend_ps(emptyMask, lerpT, FloatPack16(1));
			lerpT = lerpT.clamp(0, 1);
		}
		else if (fogEffectVersion == FogEffectVersion::EXPONENTIAL)
		{
			lerpT = FloatPack16(fogIntensity) / dist;
			lerpT = _mm512_mask_blend_ps(emptyMask, lerpT, FloatPack16(1));
			for (int j = 0; j < 16; ++j) lerpT[j] = exp(-lerpT[j]); //no need to bicycle this exp - it is getting properly optimized by MSVC
			lerpT = lerpT.clamp(0, 1);
		}

		VectorPack16 lerpedColor = origColors + (fogColorPack - origColors) * lerpT;
		frameBuf.setPixels16(i, lerpedColor, bounds);
	}
}