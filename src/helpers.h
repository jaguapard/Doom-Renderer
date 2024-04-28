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

/*
#ifdef __AVX__
inline Vec3 lerp(const Vec3& start, const Vec3& end, real amount)
{
	__m256i mask = _mm256_set_epi32(-1, -1, -1, 0, 0, 0, 0, 0);
	__m256 src = _mm256_loadu_ps(reinterpret_cast<const float*>(&start));
	__m256 dst = _mm256_loadu_ps(reinterpret_cast<const float*>(&end));
	__m256 am = _mm256_set1_ps(amount);
	
	__m256 diff = _mm256_sub_ps(dst, src);
	__m256 res = _mm256_fmadd_ps(diff, am, src);
	return *reinterpret_cast<Vec3*>(&res);
}
#endif
*/

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

		if (positive > 1000) //prepend the resulting string part with zeros if it's not the leftmost part
		{
			if (mod < 10) modStr = "00" + modStr;
			else if (mod < 100) modStr = "0" + modStr;
		}

		ret = modStr + sep + ret;

		positive /= 1000;
	}

	ret.pop_back(); //due to how algorithm works, there's always a unnecessary trailing separator after last digit. Just remove it
	if (value < 0) ret = "-" + ret;
	return ret;
}