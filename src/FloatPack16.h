#pragma once
#include <immintrin.h>
#include "bob/SSE_Vec4.h"

struct Mask16
{
	__mmask16 mask;
	Mask16() = default;
	Mask16(const __mmask16& m);

	Mask16 operator&(const Mask16& other) const;
	Mask16 operator|(const Mask16& other) const;
	Mask16 operator^(const Mask16& other) const;
	Mask16 operator~() const;
	Mask16& operator&=(const Mask16& other);
	Mask16& operator|=(const Mask16& other);
	Mask16& operator^=(const Mask16& other);

	operator __mmask16() const;
	operator bool() const;
};

inline Mask16::Mask16(const __mmask16& m)
{
	mask = m;
}

inline Mask16 Mask16::operator&(const Mask16& other) const
{
	return _kand_mask16(mask, other.mask);
}

inline Mask16 Mask16::operator|(const Mask16& other) const
{
	return _kor_mask16(mask, other.mask);
}

inline Mask16 Mask16::operator^(const Mask16& other) const
{
	return _kxor_mask16(mask, other.mask);
}

inline Mask16 Mask16::operator~() const
{
	return _knot_mask16(*this);
}

inline Mask16& Mask16::operator&=(const Mask16& other)
{
	*this = *this & other;
    return *this;
}

inline Mask16& Mask16::operator|=(const Mask16& other)
{
	*this = *this | other;
    return *this;
}

inline Mask16& Mask16::operator^=(const Mask16& other)
{
	*this = *this ^ other;
    return *this;
}

inline Mask16::operator __mmask16() const
{
	return mask;
}

inline Mask16::operator bool() const
{
	return !_ktestz_mask16_u8(mask, mask);
}


struct alignas(64) FloatPack16
{
	union {
		float f[16];
		__m512 zmm;
	};

	FloatPack16() = default;
	FloatPack16(const float x);
	FloatPack16(const __m512& m);
	FloatPack16(const float* p);
	FloatPack16(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, float f10, float f11, float f12, float f13, float f14, float f15, float f16);

	FloatPack16(const FloatPack16& other);

	FloatPack16 operator+(const float other) const;
	FloatPack16 operator-(const float other) const;
	FloatPack16 operator*(const float other) const;
	FloatPack16 operator/(const float other) const;

	FloatPack16 operator+=(const float other);
	FloatPack16 operator-=(const float other);
	FloatPack16 operator*=(const float other);
	FloatPack16 operator/=(const float other);

	FloatPack16 operator+(const FloatPack16& other) const;
	FloatPack16 operator-(const FloatPack16& other) const;
	FloatPack16 operator*(const FloatPack16& other) const;
	FloatPack16 operator/(const FloatPack16& other) const;
	FloatPack16& operator+=(const FloatPack16& other);
	FloatPack16& operator-=(const FloatPack16& other);
	FloatPack16& operator*=(const FloatPack16& other);
	FloatPack16& operator/=(const FloatPack16& other);

	Mask16 operator>(const FloatPack16& other) const;
	Mask16 operator>=(const FloatPack16& other) const;
	Mask16 operator<(const FloatPack16& other) const;
	Mask16 operator<=(const FloatPack16& other) const;
	Mask16 operator==(const FloatPack16& other) const;
	Mask16 operator!=(const FloatPack16& other) const;


	FloatPack16 operator&(const FloatPack16& other) const;
	FloatPack16 operator|(const FloatPack16& other) const;
	FloatPack16 operator^(const FloatPack16& other) const;
	FloatPack16& operator&=(const FloatPack16& other);
	FloatPack16& operator|=(const FloatPack16& other);
	FloatPack16& operator^=(const FloatPack16& other);

	FloatPack16 operator-() const;
	FloatPack16 operator~() const;
	operator __m512() const;

	FloatPack16 clamp(float min, float max) const;
	FloatPack16 sqrt() const;
	FloatPack16 rsqrt14() const;
	FloatPack16 rsqrt28() const;

	static FloatPack16 sequence(float mult = 1.0);

	const float& operator[](size_t i) const;
	float& operator[](size_t i);
};

inline FloatPack16::FloatPack16(const float x)
{
	zmm = _mm512_set1_ps(x);
}

inline FloatPack16::FloatPack16(const __m512& m)
{
	zmm = m;
}

inline FloatPack16::FloatPack16(const float* p)
{
	zmm = _mm512_loadu_ps(p);
}

inline FloatPack16::FloatPack16(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, float f10, float f11, float f12, float f13, float f14, float f15, float f16)
{
	*this = _mm512_setr_ps(f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16);
}

//#pragma optimize("", off)
inline FloatPack16::FloatPack16(const FloatPack16& other)
{
	//zmm = _mm512_loadu_ps(&other.zmm);
	zmm = other.zmm;
}
//#pragma optimize("", on)

inline FloatPack16 FloatPack16::operator+(const float other) const
{
	return _mm512_add_ps(zmm, _mm512_set1_ps(other));
}

inline FloatPack16 FloatPack16::operator-(const float other) const
{
	return _mm512_sub_ps(zmm, _mm512_set1_ps(other));
}

inline FloatPack16 FloatPack16::operator*(const float other) const
{
	return _mm512_mul_ps(zmm, _mm512_set1_ps(other));
}

inline FloatPack16 FloatPack16::operator/(const float other) const
{
	return _mm512_div_ps(zmm, _mm512_set1_ps(other));
}

inline FloatPack16 FloatPack16::operator+=(const float other)
{
	return *this = *this + other;
}

inline FloatPack16 FloatPack16::operator-=(const float other)
{
	return *this = *this - other;
}

inline FloatPack16 FloatPack16::operator*=(const float other)
{
	return *this = *this * other;
}

inline FloatPack16 FloatPack16::operator/=(const float other)
{
	return *this = *this / other;
}

inline FloatPack16 FloatPack16::operator+(const FloatPack16& other) const
{
	return _mm512_add_ps(zmm, other.zmm);
}

inline FloatPack16 FloatPack16::operator-(const FloatPack16& other) const
{
	return _mm512_sub_ps(zmm, other.zmm);
}

inline FloatPack16 FloatPack16::operator*(const FloatPack16& other) const
{
	return _mm512_mul_ps(zmm, other.zmm);
}

inline FloatPack16 FloatPack16::operator/(const FloatPack16& other) const
{
	return _mm512_div_ps(zmm, other.zmm);
}

inline Mask16 FloatPack16::operator>(const FloatPack16& other) const
{
	return _mm512_cmp_ps_mask(zmm, other.zmm, _CMP_GT_OQ);
}

inline Mask16 FloatPack16::operator>=(const FloatPack16& other) const
{
	return _mm512_cmp_ps_mask(zmm, other.zmm, _CMP_GE_OQ);
}

inline Mask16 FloatPack16::operator<(const FloatPack16& other) const
{
	return _mm512_cmp_ps_mask(zmm, other.zmm, _CMP_LT_OQ);
}

inline Mask16 FloatPack16::operator<=(const FloatPack16& other) const
{
	return _mm512_cmp_ps_mask(zmm, other.zmm, _CMP_LE_OQ);
}

inline Mask16 FloatPack16::operator==(const FloatPack16& other) const
{
	return _mm512_cmp_ps_mask(zmm, other.zmm, _CMP_EQ_OQ);
}

inline Mask16 FloatPack16::operator!=(const FloatPack16& other) const
{
	return _mm512_cmp_ps_mask(zmm, other.zmm, _CMP_NEQ_OQ);
}

inline FloatPack16 FloatPack16::operator&(const FloatPack16& other) const
{
	return _mm512_and_ps(zmm, other.zmm);
}

inline FloatPack16 FloatPack16::operator|(const FloatPack16& other) const
{
	return _mm512_or_ps(zmm, other.zmm);
}

inline FloatPack16 FloatPack16::operator^(const FloatPack16& other) const
{
	return _mm512_xor_ps(zmm, other.zmm);
}

inline FloatPack16& FloatPack16::operator&=(const FloatPack16& other)
{
	return *this = *this & other;
}

inline FloatPack16& FloatPack16::operator|=(const FloatPack16& other)
{
	return *this = *this | other;
}

inline FloatPack16& FloatPack16::operator^=(const FloatPack16& other)
{
	return *this = *this ^ other;
}

inline FloatPack16& FloatPack16::operator+=(const FloatPack16& other)
{
	return *this = *this + other;
}

inline FloatPack16& FloatPack16::operator-=(const FloatPack16& other)
{
	return *this = *this - other;
}

inline FloatPack16& FloatPack16::operator*=(const FloatPack16& other)
{
	return *this = *this * other;
}

inline FloatPack16& FloatPack16::operator/=(const FloatPack16& other)
{
	return *this = *this / other;
}

inline FloatPack16 FloatPack16::operator-() const
{
	return _mm512_sub_ps(_mm512_setzero_ps(), zmm);
}

inline FloatPack16 FloatPack16::operator~() const
{
	return _mm512_xor_ps(zmm, _mm512_castsi512_ps(_mm512_set1_epi32(-1)));
}

inline FloatPack16::operator __m512() const
{
	return zmm;
}

inline FloatPack16 FloatPack16::clamp(float min, float max) const
{
	FloatPack16 ret;
	__m512 c = _mm512_min_ps(zmm, _mm512_set1_ps(max));
	return _mm512_max_ps(c, _mm512_set1_ps(min));
}

inline FloatPack16 FloatPack16::sqrt() const
{
	return _mm512_sqrt_ps(*this);
}

inline FloatPack16 FloatPack16::rsqrt14() const
{
	return _mm512_rsqrt14_ps(*this);
}

inline FloatPack16 FloatPack16::rsqrt28() const
{
	return _mm512_rsqrt28_ps(*this);
}

inline FloatPack16 FloatPack16::sequence(float mult)
{
	return FloatPack16(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15) * mult;
}

inline const float& FloatPack16::operator[](size_t i) const
{
	return f[i];
}

inline float& FloatPack16::operator[](size_t i)
{
	return f[i];
}
