#include "blitting.h"

void blitting::lightIntoFrameBuffer(FloatColorBuffer& frameBuf, const PixelBuffer<real>& lightBuf, size_t minY, size_t maxY)
{
	assert(frameBuf.getW() == lightBuf.getW());
	assert(frameBuf.getH() == lightBuf.getH());
	assert(minY < maxY);

	size_t pixelCount = (maxY - minY) * frameBuf.getW();

	int startIndex = minY * frameBuf.getW();
	int endIndex = maxY * frameBuf.getW();
	__m512i sequence = _mm512_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

	for (int i = startIndex; i < endIndex; i += 16)
	{
		__mmask16 bounds = _mm512_cmplt_epi32_mask(_mm512_add_epi32(_mm512_set1_epi32(i), sequence), _mm512_set1_epi32(endIndex));
		FloatPack16 lightMult = lightBuf.getRawPixels() + i;

		FloatPack16 newR = FloatPack16(frameBuf.getp_R() + i) * lightMult;
		FloatPack16 newG = FloatPack16(frameBuf.getp_G() + i) * lightMult;
		FloatPack16 newB = FloatPack16(frameBuf.getp_B() + i) * lightMult;
		FloatPack16 newA = FloatPack16(frameBuf.getp_A() + i) * lightMult;

		_mm512_mask_store_ps(frameBuf.getp_R() + i, bounds, newR);
		_mm512_mask_store_ps(frameBuf.getp_G() + i, bounds, newG);
		_mm512_mask_store_ps(frameBuf.getp_B() + i, bounds, newB);
		_mm512_mask_store_ps(frameBuf.getp_A() + i, bounds, newA);
	}
}

__m512i avx512_clamp_i32(__m512i val, int32_t low, int32_t high)
{
	__m512i clampLo = _mm512_max_epi32(val, _mm512_set1_epi32(low));
	return _mm512_min_epi32(clampLo, _mm512_set1_epi32(high));
}

void blitting::frameBufferIntoSurface(FloatColorBuffer& frameBuf, SDL_Surface* surf, size_t minY, size_t maxY, const std::array<uint32_t, 4>& shifts)
{
	assert(frameBuf.getW() == surf->w);
	assert(frameBuf.getH() == surf->h);
	assert(minY < maxY);
	assert(surf->pitch == surf->w * sizeof(Color));

	int startIndex = minY * frameBuf.getW();
	int endIndex = maxY * frameBuf.getW();
	__m512i sequence = _mm512_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

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

	for (int i = startIndex; i < endIndex; i += 16)
	{
		__mmask16 bounds = _mm512_cmplt_epi32_mask(_mm512_add_epi32(_mm512_set1_epi32(i), sequence), _mm512_set1_epi32(endIndex));
		
		__m512i cvtR = _mm512_cvttps_epi32(FloatPack16(frameBuf.getp_R() + i));
		__m512i cvtG = _mm512_cvttps_epi32(FloatPack16(frameBuf.getp_G() + i));
		__m512i cvtB = _mm512_cvttps_epi32(FloatPack16(frameBuf.getp_B() + i));
		__m512i cvtA = _mm512_cvttps_epi32(FloatPack16(frameBuf.getp_A() + i)); //now lower bits of each epu32 contain values of 16 colors' channels

		__m512i clampR = avx512_clamp_i32(cvtR, 0, 255);
		__m512i clampG = avx512_clamp_i32(cvtG, 0, 255);
		__m512i clampB = avx512_clamp_i32(cvtB, 0, 255);
		__m512i clampA = avx512_clamp_i32(cvtA, 0, 255);

		__m512i r = _mm512_sllv_epi32(clampR, _mm512_set1_epi32(shifts[0]));
		__m512i g = _mm512_sllv_epi32(clampG, _mm512_set1_epi32(shifts[1]));
		__m512i b = _mm512_sllv_epi32(clampB, _mm512_set1_epi32(shifts[2]));
		__m512i a = _mm512_sllv_epi32(clampA, _mm512_set1_epi32(shifts[3]));

		__m512i rg = _mm512_or_epi32(r, g);
		__m512i ba = _mm512_or_epi32(b, a);

		_mm512_mask_store_epi32(surfPixelsStart + i, bounds, _mm512_or_epi32(rg, ba));
	}
}
