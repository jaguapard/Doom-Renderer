#pragma once
#include <immintrin.h>
#include "bob/SSE_Vec4.h"

/*
Struct representing 8 packed  independent floats.
Acts as a wrapper on top of AVX(2) intrinsics.
Freely convertible from/to __m256 and supports bitwise operations
*/
struct alignas(32) FloatPack8
{
	union {
		float f[8];
		__m256 ymm;
	};

	FloatPack8() = default;
	FloatPack8(const float x);
	FloatPack8(const __m256& m);
	FloatPack8(const float* p);
	FloatPack8(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8);

	FloatPack8 operator+(const float other) const;
	FloatPack8 operator-(const float other) const;
	FloatPack8 operator*(const float other) const;
	FloatPack8 operator/(const float other) const;

	FloatPack8 operator+=(const float other);
	FloatPack8 operator-=(const float other);
	FloatPack8 operator*=(const float other);
	FloatPack8 operator/=(const float other);

	FloatPack8 operator+(const FloatPack8& other) const;
	FloatPack8 operator-(const FloatPack8& other) const;
	FloatPack8 operator*(const FloatPack8& other) const;
	FloatPack8 operator/(const FloatPack8& other) const;
	FloatPack8& operator+=(const FloatPack8& other);
	FloatPack8& operator-=(const FloatPack8& other);
	FloatPack8& operator*=(const FloatPack8& other);
	FloatPack8& operator/=(const FloatPack8& other);

	FloatPack8 operator>(const FloatPack8& other) const;
	FloatPack8 operator>=(const FloatPack8& other) const;
	FloatPack8 operator<(const FloatPack8& other) const;
	FloatPack8 operator<=(const FloatPack8& other) const;
	FloatPack8 operator==(const FloatPack8& other) const;
	FloatPack8 operator!=(const FloatPack8& other) const;


	FloatPack8 operator&(const FloatPack8& other) const;
	FloatPack8 operator|(const FloatPack8& other) const;
	FloatPack8 operator^(const FloatPack8& other) const;
	FloatPack8& operator&=(const FloatPack8& other);
	FloatPack8& operator|=(const FloatPack8& other);
	FloatPack8& operator^=(const FloatPack8& other);

	FloatPack8 operator-() const;
	FloatPack8 operator~() const;
	operator __m256() const;

	explicit operator bool() const
	{
		return !_mm256_testz_ps(*this, *this);
	}

	FloatPack8 clamp(float min, float max) const;
	int moveMask() const;

	static FloatPack8 sequence(float mult = 1.0);
};

inline FloatPack8::FloatPack8(const float x)
{
	ymm = _mm256_set1_ps(x);
}

inline FloatPack8::FloatPack8(const __m256& m)
{
	ymm = m;
}

inline FloatPack8::FloatPack8(const float* p)
{
	ymm = *reinterpret_cast<const __m256*>(p);
}

inline FloatPack8::FloatPack8(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8)
{
	*this = _mm256_setr_ps(f1, f2, f3, f4, f5, f6, f7, f8);
}

inline FloatPack8 FloatPack8::operator+(const float other) const
{
	return _mm256_add_ps(ymm, _mm256_set1_ps(other));
}

inline FloatPack8 FloatPack8::operator-(const float other) const
{
	return _mm256_sub_ps(ymm, _mm256_set1_ps(other));
}

inline FloatPack8 FloatPack8::operator*(const float other) const
{
	return _mm256_mul_ps(ymm, _mm256_set1_ps(other));
}

inline FloatPack8 FloatPack8::operator/(const float other) const
{
	return _mm256_div_ps(ymm, _mm256_set1_ps(other));
}

inline FloatPack8 FloatPack8::operator+=(const float other)
{
	return *this = *this + other;
}

inline FloatPack8 FloatPack8::operator-=(const float other)
{
	return *this = *this - other;
}

inline FloatPack8 FloatPack8::operator*=(const float other)
{
	return *this = *this * other;
}

inline FloatPack8 FloatPack8::operator/=(const float other)
{
	return *this = *this / other;
}

inline FloatPack8 FloatPack8::operator+(const FloatPack8& other) const
{
	return _mm256_add_ps(ymm, other.ymm);
}

inline FloatPack8 FloatPack8::operator-(const FloatPack8& other) const
{
	return _mm256_sub_ps(ymm, other.ymm);
}

inline FloatPack8 FloatPack8::operator*(const FloatPack8& other) const
{
	return _mm256_mul_ps(ymm, other.ymm);
}

inline FloatPack8 FloatPack8::operator/(const FloatPack8& other) const
{
	return _mm256_div_ps(ymm, other.ymm);
}

inline FloatPack8 FloatPack8::operator>(const FloatPack8& other) const
{
	return _mm256_cmp_ps(ymm, other.ymm, _CMP_GT_OQ);
}

inline FloatPack8 FloatPack8::operator>=(const FloatPack8& other) const
{
	return _mm256_cmp_ps(ymm, other.ymm, _CMP_GE_OQ);
}

inline FloatPack8 FloatPack8::operator<(const FloatPack8& other) const
{
	return _mm256_cmp_ps(ymm, other.ymm, _CMP_LT_OQ);
}

inline FloatPack8 FloatPack8::operator<=(const FloatPack8& other) const
{
	return _mm256_cmp_ps(ymm, other.ymm, _CMP_LE_OQ);
}

inline FloatPack8 FloatPack8::operator==(const FloatPack8& other) const
{
	return _mm256_cmp_ps(ymm, other.ymm, _CMP_EQ_OQ);
}

inline FloatPack8 FloatPack8::operator!=(const FloatPack8& other) const
{
	return _mm256_cmp_ps(ymm, other.ymm, _CMP_NEQ_OQ);
}

inline FloatPack8 FloatPack8::operator&(const FloatPack8& other) const
{
	return _mm256_and_ps(ymm, other.ymm);
}

inline FloatPack8 FloatPack8::operator|(const FloatPack8& other) const
{
	return _mm256_or_ps(ymm, other.ymm);
}

inline FloatPack8 FloatPack8::operator^(const FloatPack8& other) const
{
	return _mm256_xor_ps(ymm, other.ymm);
}

inline FloatPack8& FloatPack8::operator&=(const FloatPack8& other)
{
	return *this = *this & other;
}

inline FloatPack8& FloatPack8::operator|=(const FloatPack8& other)
{
	return *this = *this | other;
}

inline FloatPack8& FloatPack8::operator^=(const FloatPack8& other)
{
	return *this = *this ^ other;
}

inline FloatPack8& FloatPack8::operator+=(const FloatPack8& other)
{
	return *this = *this + other;
}

inline FloatPack8& FloatPack8::operator-=(const FloatPack8& other)
{
	return *this = *this - other;
}

inline FloatPack8& FloatPack8::operator*=(const FloatPack8& other)
{
	return *this = *this * other;
}

inline FloatPack8& FloatPack8::operator/=(const FloatPack8& other)
{
	return *this = *this / other;
}

inline FloatPack8 FloatPack8::operator-() const
{
	return _mm256_sub_ps(_mm256_setzero_ps(), ymm);
}

inline FloatPack8 FloatPack8::operator~() const
{
	return _mm256_xor_ps(ymm, _mm256_cmp_ps(ymm, ymm, _CMP_EQ_OQ));
}

inline FloatPack8::operator __m256() const
{
	return ymm;
}

inline FloatPack8 FloatPack8::clamp(float min, float max) const
{
	FloatPack8 ret;
	__m256 c = _mm256_min_ps(ymm, _mm256_set1_ps(max));
	return _mm256_max_ps(c, _mm256_set1_ps(min));
}

inline int FloatPack8::moveMask() const
{
	return _mm256_movemask_ps(*this);
}

inline FloatPack8 FloatPack8::sequence(float mult)
{
	return FloatPack8(0, 1, 2, 3, 4, 5, 6, 7) * mult;
}
