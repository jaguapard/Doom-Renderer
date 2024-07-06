#include "blitting.h"
#include "Lehmer.h"

const __m512i sequence512 = _mm512_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
static thread_local LehmerRNG rng;

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
		VectorPack16 multiplied = frameBuf.getPixelsStartingFrom16(i) * (lightBuf.getRawPixels() + i);
		frameBuf.storePixels16(i, multiplied, bounds);
	}
}

__m512i avx512_clamp_i32(__m512i val, int32_t low, int32_t high)
{
	__m512i clampLo = _mm512_max_epi32(val, _mm512_set1_epi32(low));
	return _mm512_min_epi32(clampLo, _mm512_set1_epi32(high));
}

void blitting::frameBufferIntoSurface(const FloatColorBuffer& frameBuf, SDL_Surface* surf, size_t minY, size_t maxY, const std::array<uint32_t, 4> shifts, const bool ditheringEnabled)
{
	assert(frameBuf.getW() == surf->w);
	assert(frameBuf.getH() == surf->h);
	assert(minY < maxY);
	assert(surf->pitch == surf->w * sizeof(Color));

	size_t startIndex = minY * frameBuf.getW();
	size_t endIndex = maxY * frameBuf.getW();

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

	for (size_t i = startIndex; i < endIndex; i += 16)
	{
		__mmask16 bounds = _mm512_cmplt_epi32_mask(_mm512_add_epi32(_mm512_set1_epi32(i), sequence512), _mm512_set1_epi32(endIndex));

		VectorPack16 floatColors = frameBuf.getPixelsStartingFrom16(i) * 255;
		__m512i cvtChannels[4];
		for (int j = 0; j < 4; ++j) cvtChannels[j] = _mm512_cvttps_epi32(floatColors[j]); //now lower bits of each epi32 contain values of channels

		if (ditheringEnabled)
		{
			for (int j = 0; j < 4; ++j)
			{
				__m512i rngOut = rng.next();
				FloatPack16 channelIncreaseChance = floatColors[j] - _mm512_floor_ps(floatColors[j]);
				FloatPack16 threshold = channelIncreaseChance * float(UINT32_MAX);
				Mask16 increaseMask = _mm512_cmplt_epu32_mask(rngOut, _mm512_cvttps_epu32(threshold));
				cvtChannels[j] = _mm512_mask_add_epi32(cvtChannels[j], increaseMask, cvtChannels[j], _mm512_set1_epi32(1));
			}
		}

		__m512i shifted[4];
		for (int j = 0; j < 4; ++j)
		{
			__m512i clamped = avx512_clamp_i32(cvtChannels[j], 0, 255);
			shifted[j] = _mm512_sllv_epi32(clamped, _mm512_set1_epi32(shifts[j]));
		}

		__m512i rg = _mm512_or_epi32(shifted[0], shifted[1]);
		__m512i ba = _mm512_or_epi32(shifted[2], shifted[3]);

		_mm512_mask_store_epi32(surfPixelsStart + i, bounds, _mm512_or_epi32(rg, ba));
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
		VectorPack16 origColors = frameBuf.getPixelsStartingFrom16(i);

        FloatPack16 dist = _mm512_sqrt_ps((worldPos.getPixelsStartingFrom16(i) - camPos).lenSq3d());
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
		frameBuf.storePixels16(i, lerpedColor, bounds);
	}
}

void blitting::integerDownscale(FloatColorBuffer &src, FloatColorBuffer &dst, uint32_t downsampleMult, size_t dstMinY, size_t dstMaxY)
{
    assert(src.getW() == dst.getW()*downsampleMult);
    assert(src.getH() == dst.getH()*downsampleMult);

    size_t dstStartIndex = dstMinY * dst.getW();
    size_t dstEndIndex = dstMaxY * dst.getW();

    __m512i step = _mm512_mullo_epi32(sequence512, _mm512_set1_epi32(downsampleMult));
	
	for (int dstY = dstMinY; dstY < dstMaxY; ++dstY)
	{
		for (int dstX = 0; dstX < dst.getW(); dstX += 16)
		{
            __m512i dstX_vec = _mm512_add_epi32(_mm512_set1_epi32(dstX), sequence512);
            Mask16 loopBounds = _mm512_cmplt_epi32_mask(dstX_vec, _mm512_set1_epi32(dst.getW()));

			VectorPack16 sum = 0;
			for (int y = 0; y < downsampleMult; ++y)
			{
				for (int x = 0; x < downsampleMult; ++x)
				{
					__m512i xCoords = _mm512_add_epi32(_mm512_set1_epi32(dstX*downsampleMult + x), step);
                    __m512i yCoords = _mm512_set1_epi32(dstY*downsampleMult+y);
					sum += src.gatherPixels16(xCoords, yCoords, loopBounds);
				}
			}

			dst.storePixels16(dstY * dst.getW() + dstX, sum / (downsampleMult*downsampleMult), loopBounds);
			//Mask16 bounds = _mm512_cmplt_epi32_mask(_mm512_set1_epi32(dstX))

		}
	}
}
