#include "blitting.h"

void blitting::lightIntoFrameBuffer(PixelBuffer<Color>& frameBuf, const PixelBuffer<real>& lightBuf, size_t minY, size_t maxY)
{
	assert(frameBuf.getW() == lightBuf.getW());
	assert(frameBuf.getH() == lightBuf.getH());
	assert(minY < maxY);

	Color* currFrameBufPtr = frameBuf.getRawPixels() + minY * frameBuf.getW();
	const real* currLightBufPtr = lightBuf.getRawPixels() + minY * lightBuf.getW();
	size_t pixelCount = (maxY - minY) * frameBuf.getW();

	constexpr char z = 1 << 7; //if upper bit of the 8 bit index for shuffle is set, then the element will be filled with 0's automagically.

#if __AVX512F__
	__m512i rExtractMask = _mm512_broadcast_i32x4(_mm_setr_epi8(0, z, z, z, 4, z, z, z, 8, z, z, z, 12, z, z, z));
	__m512i gExtractMask = _mm512_add_epi8(rExtractMask, _mm512_set1_epi32(1));
	__m512i bExtractMask = _mm512_add_epi8(gExtractMask, _mm512_set1_epi32(1));

	for (; pixelCount >= 16; currFrameBufPtr += 16, currLightBufPtr += 16, pixelCount -= 16)
	{
		__m512 light = _mm512_load_ps(currLightBufPtr); //load light intensities for pixels from light buffer
		__m512i origColors = _mm512_load_epi32(currFrameBufPtr); //packed colors in this order: rgba,rgba,rgba,...		

		//shuffle the bytes such that lowest bytes of each epi32 component in __m256i's are the values of corresponding colors and convert them to floats
		__m512 floatR = _mm512_cvtepi32_ps(_mm512_shuffle_epi8(origColors, rExtractMask));
		__m512 floatG = _mm512_cvtepi32_ps(_mm512_shuffle_epi8(origColors, gExtractMask));
		__m512 floatB = _mm512_cvtepi32_ps(_mm512_shuffle_epi8(origColors, bExtractMask));
		//alpha can stay broken lol

		__m512i mulR = _mm512_cvttps_epi32(_mm512_mul_ps(floatR, light));
		__m512i mulG = _mm512_cvttps_epi32(_mm512_mul_ps(floatG, light));
		__m512i mulB = _mm512_cvttps_epi32(_mm512_mul_ps(floatB, light));

		__m512i res = _mm512_or_si512(mulR, _mm512_or_si512(_mm512_slli_epi32(mulG, 8), _mm512_slli_epi32(mulB, 16)));
		*reinterpret_cast<__m512i*>(currFrameBufPtr) = res;
	}
#elif __AVX2__
	__m256i rExtractMask = _mm256_broadcastsi128_si256(_mm_setr_epi8(0, z, z, z, 4, z, z, z, 8, z, z, z, 12, z, z, z));
	__m256i gExtractMask = _mm256_add_epi8(rExtractMask, _mm256_set1_epi32(1));
	__m256i bExtractMask = _mm256_add_epi8(gExtractMask, _mm256_set1_epi32(1));

	for (; pixelCount >= 8; currFrameBufPtr += 8, currLightBufPtr += 8, pixelCount -= 8)
	{
		__m256 light = _mm256_load_ps(currLightBufPtr); //load light intensities for pixels from light buffer
		__m256i origColors = _mm256_load_si256(reinterpret_cast<const __m256i*>(currFrameBufPtr)); //packed colors in this order: rgba,rgba,rgba,...		

		//shuffle the bytes such that lowest bytes of each epi32 component in __m256i's are the values of corresponding colors and convert them to floats
		__m256 floatR = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(origColors, rExtractMask));
		__m256 floatG = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(origColors, gExtractMask));
		__m256 floatB = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(origColors, bExtractMask));
		//alpha can stay broken lol

		__m256i mulR = _mm256_cvttps_epi32(_mm256_mul_ps(floatR, light));
		__m256i mulG = _mm256_cvttps_epi32(_mm256_mul_ps(floatG, light));
		__m256i mulB = _mm256_cvttps_epi32(_mm256_mul_ps(floatB, light));

		__m256i res = _mm256_or_si256(mulR, _mm256_or_si256(_mm256_slli_epi32(mulG, 8), _mm256_slli_epi32(mulB, 16)));
		*reinterpret_cast<__m256i*>(currFrameBufPtr) = res;
	}
#elif SSE_VER >= 20
	__m128i rExtractMask = _mm_setr_epi8(0, z, z, z, 4, z, z, z, 8, z, z, z, 12, z, z, z);
	__m128i gExtractMask = _mm_add_epi8(rExtractMask, _mm_set1_epi32(1));
	__m128i bExtractMask = _mm_add_epi8(gExtractMask, _mm_set1_epi32(1));

	for (; pixelCount >= 4; currFrameBufPtr += 4, currLightBufPtr += 4, pixelCount -= 4)
	{
		__m128 light = _mm_load_ps(currLightBufPtr); //load light intensities for pixels from light buffer
		__m128i origColors = _mm_load_si128(reinterpret_cast<const __m128i*>(currFrameBufPtr)); //packed colors in this order: rgba,rgba,rgba,...		

		//shuffle the bytes such that lowest bytes of each epi32 component in __m256i's are the values of corresponding colors and convert them to floats
		__m128 floatR = _mm_cvtepi32_ps(_mm_shuffle_epi8(origColors, rExtractMask));
		__m128 floatG = _mm_cvtepi32_ps(_mm_shuffle_epi8(origColors, gExtractMask));
		__m128 floatB = _mm_cvtepi32_ps(_mm_shuffle_epi8(origColors, bExtractMask));
		//alpha can stay broken lol

		__m128i mulR = _mm_cvttps_epi32(_mm_mul_ps(floatR, light));
		__m128i mulG = _mm_cvttps_epi32(_mm_mul_ps(floatG, light));
		__m128i mulB = _mm_cvttps_epi32(_mm_mul_ps(floatB, light));

		__m128i res = _mm_or_si128(mulR, _mm_or_si128(_mm_slli_epi32(mulG, 8), _mm_slli_epi32(mulB, 16)));
		*reinterpret_cast<__m128i*>(currFrameBufPtr) = res;
	}
#endif
	for (; pixelCount; currFrameBufPtr += 1, currLightBufPtr += 1, pixelCount -= 1)
	{
		*currFrameBufPtr = currFrameBufPtr->multipliedByLight(*currLightBufPtr);
	}
}

void blitting::frameBufferIntoSurface(const PixelBuffer<Color>& frameBuf, SDL_Surface* surf, size_t minY, size_t maxY, const std::array<uint32_t, 4>& shifts)
{
	assert(frameBuf.getW() == surf->w);
	assert(frameBuf.getH() == surf->h);
	assert(minY < maxY);
	assert(surf->pitch == surf->w * sizeof(Color));

	const Color* currFrameBufPtr = frameBuf.getRawPixels() + minY * frameBuf.getW();
	Uint32* currSurfacePixelsPtr = (Uint32*)(surf->pixels) + minY * frameBuf.getW();
	size_t pixelCount = (maxY - minY) * frameBuf.getW();

#if __AVX2__
	//index of bytes to "pull" from one color in rgba order. Byte 0 of resulting SDL_Surface color will be pulled from color buffer's byte (shift[0]/8), 1 from (shift[1]/8), etc
	int32_t intraColorReshuffleMask = shifts[0] >> 3 | shifts[1] << 5 | shifts[2] << 13 | shifts[3] << 21;
	__m128i shuffleMask128 = _mm_add_epi32(_mm_set1_epi32(intraColorReshuffleMask), _mm_setr_epi32(0, 0x04040404, 0x08080808, 0x0c0c0c0c)); //broadcast the intraColorReshuffleMask and add 0, 4, 8 or 12 to each byte.
	__m256i shuffleMask256 = _mm256_broadcastsi128_si256(shuffleMask128); //AVX2's shuffle is basically 2 SSE shuffles in one, so just mirror the mask128

#if __AVX512BW__
	__m512i shuffleMask512 = _mm512_broadcast_i32x4(shuffleMask128);
	for (; pixelCount >= 16; currSurfacePixelsPtr += 16, currFrameBufPtr += 16, pixelCount -= 16)
	{
		__m512i origColors = *reinterpret_cast<const __m512i*>(currFrameBufPtr);
		__m512i res = _mm512_shuffle_epi8(origColors, shuffleMask512);
		*reinterpret_cast<__m512i*>(currSurfacePixelsPtr) = res;
	}
#else
	for (; pixelCount >= 8; currSurfacePixelsPtr += 8, currFrameBufPtr += 8, pixelCount -= 8)
	{
		__m256i origColors = *reinterpret_cast<const __m256i*>(currFrameBufPtr);
		__m256i res = _mm256_shuffle_epi8(origColors, shuffleMask256);
		*reinterpret_cast<__m256i*>(currSurfacePixelsPtr) = res;
	}
#endif
#endif
	for (; pixelCount; currSurfacePixelsPtr += 1, currFrameBufPtr += 1, pixelCount -= 1)
	{
		*currSurfacePixelsPtr = currFrameBufPtr->toSDL_Uint32(shifts);
	}
}
