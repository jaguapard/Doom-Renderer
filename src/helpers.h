#pragma once
#include <cassert>
#include <string>
#include <vector>

#include <SDL/SDL.h>

#include "real.h"

#include <immintrin.h>
#include "Vec.h"

template <typename T>
inline T lerp(const T& start, const T& end, real amount)
{
	return start + (end - start) * amount;
}

inline real inverse_lerp(real from, real to, real value)
{
	return (value - from) / (to - from);
}

inline void setPixel(SDL_Surface* s, int x, int y, uint32_t color)
{
	if (x >= 0 && y >= 0 && x < s->w && y < s->h)
	{
		uint32_t* px = (uint32_t*)s->pixels;
		px[y * s->w + x] = color;
	}
}

//WAD file names are not 0 terminated if they are 8 symbols long.
inline std::string wadStrToStd(const char* wadStr, int maxLen = 8)
{
	std::vector<char> buf(maxLen + 1, 0);
	memcpy(&buf.front(), wadStr, maxLen);
	return std::string(&buf.front()); 
}

template <typename T>
bool isInRange(T val, T min, T max)
{
	return val >= min && val <= max;
}

inline std::string toThousandsSeparatedString(int64_t value, std::string sep = ",")
{
	if (value == 0) return "0";
	int64_t positive = abs(value);
	std::string ret;
	while (positive > 0)
	{		
		int mod = positive % 1000;
		std::string modStr = std::to_string(mod);

		if (positive > 999) //prepend the resulting string part with zeros if it's not the leftmost part
		{
			if (mod < 10) modStr = "00" + modStr;
			else if (mod < 100) modStr = "0" + modStr;
		}

		ret = modStr + sep + ret;

		positive /= 1000;
	}

	ret.pop_back(); //due to how algorithm works, there's always an unnecessary trailing separator after last digit. Just remove it
	if (value < 0) ret = "-" + ret;
	return ret;
}

#if !(_MSC_VER && !__INTEL_COMPILER)
inline __m128i _mm_setr_epi64x(int64_t a, int64_t b)
{
    return _mm_setr_epi32(a, a >> 32, b, b >> 32);
}
#endif