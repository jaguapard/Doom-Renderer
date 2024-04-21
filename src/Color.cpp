#include <SDL/SDL.h>
#include "Color.h"
#include <cassert>

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

Color::Color(uint8_t r, uint8_t g, uint8_t b)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = SDL_ALPHA_OPAQUE;
}

Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

Color Color::multipliedByLight(real lightMult) const
{
	return Color(r * lightMult, g * lightMult, b * lightMult, a);
}

void Color::multipliyByLightInPlace(const real* lightMults, Color* colors, int pixelCount)
{
	assert(pixelCount % 8 == 0);
	while (pixelCount >= 8)
	{
		__m256 light = _mm256_load_ps(lightMults); //load light intensities for pixels from light buffer
		__m256 expandedLight = _mm256_mul_ps(light, _mm256_set1_ps(256)); //and turn them into integers to perform fixed-point arithmetic with them
		__m256i intLight = _mm256_cvttps_epi32(expandedLight); //0.0f ... 1.0f -> 0 ... 255 (uint8)

		__m256i origColors = _mm256_load_si256(reinterpret_cast<const __m256i*>(colors));
		__m256i lastByteMask = _mm256_set1_epi32(0xFF);

		//shift bytes for color values into correct places and cut away everything else. After this the lower bytes of each epi32 hold original color byte
		__m256i r = _mm256_and_epi32(origColors, lastByteMask);
		__m256i g = _mm256_and_epi32(_mm256_srli_epi32(origColors, 8), lastByteMask);
		__m256i b = _mm256_and_epi32(_mm256_srli_epi32(origColors, 16), lastByteMask);
		//alpha can stay broken lol

		//calculate new values for colors: new = ((old*intLight) >> 8) & 0xFF and place into correct places for further store. 
		// This is very close to truly multiplying with a float, but avoids lots of unnecessary int->float and back conversions
		__m256i mulR = _mm256_mullo_epi32(r, intLight);
		__m256i mulG = _mm256_mullo_epi32(g, intLight);
		__m256i mulB = _mm256_mullo_epi32(b, intLight);
		__m256i bBytes = _mm256_and_si256(_mm256_slli_epi32(mulB, 8), _mm256_set1_epi32(0xFF0000));
		__m256i gBytes = _mm256_and_si256(mulG, _mm256_set1_epi32(0xFF00));
		__m256i rBytes = _mm256_srli_epi32(mulR, 8);

		__m256i res = _mm256_or_si256(rBytes, _mm256_or_si256(gBytes, bBytes));
		_mm256_store_si256(reinterpret_cast<__m256i*>(colors), res);

		lightMults += 8;
		colors += 8;
		pixelCount -= 8;
	}

	while (pixelCount--)
	{
		*colors = colors->multipliedByLight(*lightMults++);
		++colors;
	}
}

void Color::toSDL_Uint32(const Color* colors, Uint32* u, int pixelCount, const std::array<uint32_t, 4>& shifts)
{
	assert(pixelCount % 8 == 0);
	__m256i rShift = _mm256_set1_epi32(shifts[0]);
	__m256i gShift = _mm256_set1_epi32(shifts[1]);
	__m256i bShift = _mm256_set1_epi32(shifts[2]);
	__m256i aShift = _mm256_set1_epi32(shifts[3]);

	while (pixelCount >= 8)
	{
		__m256i origColors = _mm256_load_si256(reinterpret_cast<const __m256i*>(colors));
		__m256i lastByteMask = _mm256_set1_epi32(0xFF);

		__m256i r = _mm256_and_epi32(origColors, lastByteMask);
		__m256i g = _mm256_and_epi32(_mm256_srli_epi32(origColors, 8), lastByteMask);
		__m256i b = _mm256_and_epi32(_mm256_srli_epi32(origColors, 16), lastByteMask);
		__m256i a = _mm256_and_epi32(_mm256_srli_epi32(origColors, 24), lastByteMask);

		__m256i nr = _mm256_sllv_epi32(r, rShift);
		__m256i ng = _mm256_sllv_epi32(g, gShift);
		__m256i nb = _mm256_sllv_epi32(b, bShift);
		__m256i na = _mm256_sllv_epi32(a, aShift);

		__m256i res = _mm256_or_epi32(_mm256_or_epi32(_mm256_or_epi32(na, nb), ng), nr);
		_mm256_store_si256(reinterpret_cast<__m256i*>(u), res);

		u += 8;
		colors += 8;
		pixelCount -= 8;
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