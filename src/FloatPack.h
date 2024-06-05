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
	FloatPack8 ret;
	return _mm256_add_ps(ymm, other.ymm);

}

inline FloatPack8 FloatPack8::operator-(const FloatPack8& other) const
{
	FloatPack8 ret;
	return _mm256_sub_ps(ymm, other.ymm);

}

inline FloatPack8 FloatPack8::operator*(const FloatPack8& other) const
{
	FloatPack8 ret;
	return _mm256_mul_ps(ymm, other.ymm);

}

inline FloatPack8 FloatPack8::operator/(const FloatPack8& other) const
{
	FloatPack8 ret;
	return _mm256_div_ps(ymm, other.ymm);

}

inline FloatPack8 FloatPack8::operator>(const FloatPack8& other) const
{
	FloatPack8 ret;
	return _mm256_cmp_ps(ymm, other.ymm, _CMP_GT_OQ);

}

inline FloatPack8 FloatPack8::operator>=(const FloatPack8& other) const
{
	FloatPack8 ret;
	return _mm256_cmp_ps(ymm, other.ymm, _CMP_GE_OQ);

}

inline FloatPack8 FloatPack8::operator<(const FloatPack8& other) const
{
	FloatPack8 ret;
	return _mm256_cmp_ps(ymm, other.ymm, _CMP_LT_OQ);

}

inline FloatPack8 FloatPack8::operator<=(const FloatPack8& other) const
{
	FloatPack8 ret;
	return _mm256_cmp_ps(ymm, other.ymm, _CMP_LE_OQ);

}

inline FloatPack8 FloatPack8::operator==(const FloatPack8& other) const
{
	FloatPack8 ret;
	return _mm256_cmp_ps(ymm, other.ymm, _CMP_EQ_OQ);

}

inline FloatPack8 FloatPack8::operator!=(const FloatPack8& other) const
{
	FloatPack8 ret;
	return _mm256_cmp_ps(ymm, other.ymm, _CMP_NEQ_OQ);

}

inline FloatPack8 FloatPack8::operator&(const FloatPack8& other) const
{
	FloatPack8 ret;
	return _mm256_and_ps(ymm, other.ymm);

}

inline FloatPack8 FloatPack8::operator|(const FloatPack8& other) const
{
	FloatPack8 ret;
	return _mm256_or_ps(ymm, other.ymm);

}

inline FloatPack8 FloatPack8::operator^(const FloatPack8& other) const
{
	FloatPack8 ret;
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
	FloatPack8 ret;
	__m256 zeros = _mm256_setzero_ps();
	return _mm256_sub_ps(zeros, ymm);
}

inline FloatPack8 FloatPack8::operator~() const
{
	FloatPack8 ret;
	__m256 allOnes = _mm256_cmp_ps(ymm, ymm, _CMP_EQ_OQ);
	return _mm256_xor_ps(ymm, allOnes);
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