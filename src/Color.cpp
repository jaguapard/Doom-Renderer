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

void Color::multipliyByLightInPlace(const real* lightMults, Color* colors, int pixelCount)
{
	constexpr char z = 1 << 7; //if upper bit of the 8 bit index for shuffle is set, then the element will be filled with 0's automagically.
#ifdef __AVX2__
	__m256i rExtractMask = _mm256_broadcastsi128_si256(_mm_setr_epi8(0, z, z, z, 4, z, z, z, 8, z, z,  z, 12, z,  z,  z));
	__m256i gExtractMask = _mm256_add_epi8(rExtractMask, _mm256_set1_epi32(1));
	__m256i bExtractMask = _mm256_add_epi8(gExtractMask, _mm256_set1_epi32(1));

	for (; pixelCount >= 8; lightMults += 8, colors += 8, pixelCount -= 8)
	{
		__m256 light = _mm256_load_ps(lightMults); //load light intensities for pixels from light buffer
		__m256i origColors = _mm256_load_si256(reinterpret_cast<const __m256i*>(colors)); //packed colors in this order: rgba,rgba,rgba,...		

		//shuffle the bytes such that lowest bytes of each epi32 component in __m256i's are the values of corresponding colors and convert them to floats
		__m256 floatR = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(origColors, rExtractMask));
		__m256 floatG = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(origColors, gExtractMask));
		__m256 floatB = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(origColors, bExtractMask));
		//alpha can stay broken lol

		__m256i mulR = _mm256_cvttps_epi32(_mm256_mul_ps(floatR, light));
		__m256i mulG = _mm256_cvttps_epi32(_mm256_mul_ps(floatG, light));
		__m256i mulB = _mm256_cvttps_epi32(_mm256_mul_ps(floatB, light));

		__m256i res = _mm256_or_si256(mulR, _mm256_or_si256(_mm256_slli_epi32(mulG, 8), _mm256_slli_epi32(mulB, 16)));
		*reinterpret_cast<__m256i*>(colors) = res;
	}
#elif SSE_VER >= 20
	__m128i rExtractMask = _mm_setr_epi8(0, z, z, z, 4, z, z, z, 8, z, z, z, 12, z, z, z);
	__m128i gExtractMask = _mm_add_epi8(rExtractMask, _mm_set1_epi32(1));
	__m128i bExtractMask = _mm_add_epi8(gExtractMask, _mm_set1_epi32(1));

	for (; pixelCount >= 4; lightMults += 4, colors += 4, pixelCount -= 4)
	{
		__m128 light = _mm_load_ps(lightMults); //load light intensities for pixels from light buffer
		__m128i origColors = _mm_load_si128(reinterpret_cast<const __m128i*>(colors)); //packed colors in this order: rgba,rgba,rgba,...		

		//shuffle the bytes such that lowest bytes of each epi32 component in __m256i's are the values of corresponding colors and convert them to floats
		__m128 floatR = _mm_cvtepi32_ps(_mm_shuffle_epi8(origColors, rExtractMask));
		__m128 floatG = _mm_cvtepi32_ps(_mm_shuffle_epi8(origColors, gExtractMask));
		__m128 floatB = _mm_cvtepi32_ps(_mm_shuffle_epi8(origColors, bExtractMask));
		//alpha can stay broken lol

		__m128i mulR = _mm_cvttps_epi32(_mm_mul_ps(floatR, light));
		__m128i mulG = _mm_cvttps_epi32(_mm_mul_ps(floatG, light));
		__m128i mulB = _mm_cvttps_epi32(_mm_mul_ps(floatB, light));

		__m128i res = _mm_or_si128(mulR, _mm_or_si128(_mm_slli_epi32(mulG, 8), _mm_slli_epi32(mulB, 16)));
		*reinterpret_cast<__m128i*>(colors) = res;
	}
#endif
	for (; pixelCount; colors += 1, lightMults += 1, pixelCount -= 1)
	{
		*colors = colors->multipliedByLight(*lightMults);
	}
}

void Color::toSDL_Uint32(const Color* colors, Uint32* u, int pixelCount, const std::array<uint32_t, 4>& shifts)
{
#ifdef __AVX2__
	//index of bytes to "pull" from one color in rgba order. Byte 0 of resulting SDL_Surface color will be pulled from color buffer's byte (shift[0]/8), 1 from (shift[1]/8), etc
	int32_t intraColorReshuffleMask = shifts[0] >> 3 | shifts[1] << 5 | shifts[2] << 13 | shifts[3] << 21;
	__m128i shuffleMask128 = _mm_add_epi32(_mm_set1_epi32(intraColorReshuffleMask), _mm_setr_epi32(0, 0x04040404, 0x08080808, 0x0c0c0c0c)); //broadcast the intraColorReshuffleMask and add 0, 4, 8 or 12 to each byte.
	__m256i shuffleMask256 = _mm256_broadcastsi128_si256(shuffleMask128); //AVX2's shuffle is basically 2 SSE shuffles in one, so just mirror the mask128

	for (; pixelCount >= 8; u += 8, colors += 8, pixelCount -= 8)
	{
		__m256i origColors = *reinterpret_cast<const __m256i*>(colors);
		__m256i res = _mm256_shuffle_epi8(origColors, shuffleMask256);
		*reinterpret_cast<__m256i*>(u) = res;
	} 
#endif
	for (; pixelCount; u += 1, colors += 1, pixelCount -= 1)
	{
		*u = colors->toSDL_Uint32(shifts);
	}
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