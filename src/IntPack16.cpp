#include "IntPack16.h"

IntPack16::IntPack16(const int32_t x, Mask16 mask, const IntPack16& fillerVal)
{
	zmm = _mm512_mask_mov_epi32(fillerVal, mask, _mm512_set1_epi32(x));
}

IntPack16::IntPack16()
{
	zmm = _mm512_set1_epi32(0);
}

IntPack16::IntPack16(const __m512i& m, Mask16 mask, const IntPack16& fillerVal)
{
	zmm = _mm512_mask_mov_epi32(fillerVal, mask, m);
}

IntPack16::IntPack16(const int32_t* p, Mask16 mask, const IntPack16& fillerVal)
{
	zmm = _mm512_mask_loadu_epi32(fillerVal, mask, p);
}

/*
IntPack16::IntPack16(const FloatPack16& f, Mask16 mask, const IntPack16& fillerVal)
{
	zmm = _mm512_mask_cvttps_epi32(fillerVal, mask, f);
}*/

IntPack16::IntPack16(const IntPack16& other, Mask16 mask, const IntPack16& fillerVal)
{
	zmm = _mm512_mask_mov_epi32(fillerVal, mask, other);
}

IntPack16::IntPack16(int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t i6, int32_t i7, int32_t i8, int32_t i9, int32_t i10, int32_t i11, int32_t i12, int32_t i13, int32_t i14, int32_t i15, int32_t i16)
{
	zmm = _mm512_setr_epi32(i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13, i14, i15, i16);
}

void IntPack16::store(void* dst, Mask16 mask)
{
	_mm512_mask_store_epi32(dst, mask, *this);
}

IntPack16 operator+(const IntPack16& lhs, const int32_t rhs)
{
	return _mm512_add_epi32(lhs.zmm, _mm512_set1_epi32(rhs));
}

IntPack16 operator-(const IntPack16& lhs, const int32_t rhs)
{
	return _mm512_sub_epi32(lhs.zmm, _mm512_set1_epi32(rhs));
}

IntPack16 operator*(const IntPack16& lhs, const int32_t rhs)
{
	return  _mm512_mullo_epi32(lhs.zmm, _mm512_set1_epi32(rhs));
}

IntPack16 operator/(const IntPack16& lhs, const int32_t rhs)
{
	__m512d thisDouble1 = _mm512_cvtepi32_pd(_mm512_extracti32x8_epi32(lhs.zmm, 0));
	__m512d thisDouble2 = _mm512_cvtepi32_pd(_mm512_extracti32x8_epi32(lhs.zmm, 1));

	__m512d div1 = _mm512_div_pd(thisDouble1, _mm512_set1_pd(rhs));
	__m512d div2 = _mm512_div_pd(thisDouble2, _mm512_set1_pd(rhs));
	__m256i ret1 = _mm512_cvttpd_epi32(div1);
	__m256i ret2 = _mm512_cvttpd_epi32(div2);
	return _mm512_inserti32x8(_mm512_castsi256_si512(ret1), ret2, 1);
}

IntPack16 operator+(const IntPack16& lhs, const IntPack16& rhs)
{
	return _mm512_add_epi32(lhs, rhs);
}

IntPack16 operator-(const IntPack16& lhs, const IntPack16& rhs)
{
	return _mm512_sub_epi32(lhs, rhs);
}

IntPack16 operator*(const IntPack16& lhs, const IntPack16& rhs)
{
	return _mm512_mullo_epi32(lhs, rhs);
}

IntPack16 operator/(const IntPack16& lhs, const IntPack16& rhs)
{
	__m512d lhsDouble1 = _mm512_cvtepi32_pd(_mm512_extracti32x8_epi32(lhs.zmm, 0));
	__m512d lhsDouble2 = _mm512_cvtepi32_pd(_mm512_extracti32x8_epi32(lhs.zmm, 1));
	__m512d rhsDouble1 = _mm512_cvtepi32_pd(_mm512_extracti32x8_epi32(rhs.zmm, 0));
	__m512d rhsDouble2 = _mm512_cvtepi32_pd(_mm512_extracti32x8_epi32(rhs.zmm, 1));

	__m512d div1 = _mm512_div_pd(lhsDouble1, rhsDouble1);
	__m512d div2 = _mm512_div_pd(lhsDouble2, rhsDouble2);
	__m256i ret1 = _mm512_cvttpd_epi32(div1);
	__m256i ret2 = _mm512_cvttpd_epi32(div2);
	return _mm512_inserti32x8(_mm512_castsi256_si512(ret1), ret2, 1);
}

IntPack16 IntPack16::operator-() const
{
	return 0 - *this;
}

IntPack16 IntPack16::operator~() const
{
	return _mm512_xor_epi32(zmm, _mm512_set1_epi32(-1));
}

IntPack16::operator __m512i() const
{
	return zmm;
}

IntPack16::operator FloatPack16() const
{
	return _mm512_cvtepi32_ps(zmm);
}

IntPack16::operator __m512() const
{
	return _mm512_cvtepi32_ps(zmm);
}

IntPack16 IntPack16::clamp(int32_t min, int32_t max) const
{
	return this->clamp(IntPack16(min), IntPack16(max));
}

IntPack16 IntPack16::clamp(const IntPack16& min, const IntPack16& max) const
{
	__m512i clampLo = _mm512_max_epi32(zmm, min);
	return _mm512_min_epi32(clampLo, max);
}

IntPack16 IntPack16::sequence(int32_t mult)
{
	return IntPack16(0, mult * 1, mult * 2, mult * 3, mult * 4, mult * 5, mult * 6, mult * 7, mult * 8, mult * 9, mult * 10, mult * 11, mult * 12, mult * 13, mult * 14, mult * 15);
}

const int32_t& IntPack16::operator[](size_t i) const
{
	return el[i];
}

int32_t& IntPack16::operator[](size_t i)
{
	return el[i];
}


IntPack16& IntPack16::operator+=(const int32_t other)
{
	return *this = *this + other;
}

IntPack16& IntPack16::operator-=(const int32_t other)
{
	return *this = *this - other;
}

IntPack16& IntPack16::operator*=(const int32_t other)
{
	return *this = *this * other;
}

IntPack16& IntPack16::operator/=(const int32_t other)
{
	return *this = *this / other;
}

IntPack16 IntPack16::operator<<(const int32_t other) const
{
	return _mm512_sllv_epi32(zmm, _mm512_set1_epi32(other));
}

IntPack16 IntPack16::operator<<(const IntPack16& other) const
{
	return _mm512_sllv_epi32(zmm, other);
}

IntPack16 IntPack16::operator>>(const int32_t other) const
{
	return _mm512_srlv_epi32(zmm, _mm512_set1_epi32(other));
}

IntPack16 IntPack16::operator>>(const IntPack16& other) const
{
	return _mm512_srlv_epi32(zmm, other);
}

IntPack16& IntPack16::operator<<=(const int32_t other)
{
	return *this = *this << other;
}

IntPack16& IntPack16::operator<<=(const IntPack16& other)
{
	return *this = *this << other;
}

IntPack16& IntPack16::operator+=(const IntPack16& other)
{
	return *this = *this + other;
}

IntPack16& IntPack16::operator-=(const IntPack16& other)
{
	return *this = *this - other;
}

IntPack16& IntPack16::operator*=(const IntPack16& other)
{
	return *this = *this * other;
}

IntPack16& IntPack16::operator/=(const IntPack16& other)
{
	return *this = *this / other;
}

Mask16 IntPack16::operator>(const IntPack16& other) const
{
	return _mm512_cmpgt_epi32_mask(zmm, other);
}

Mask16 IntPack16::operator>=(const IntPack16& other) const
{
	return _mm512_cmpge_epi32_mask(zmm, other);
}

Mask16 IntPack16::operator<(const IntPack16& other) const
{
	return _mm512_cmplt_epi32_mask(zmm, other);
}

Mask16 IntPack16::operator<=(const IntPack16& other) const
{
	return _mm512_cmple_epi32_mask(zmm, other);
}

Mask16 IntPack16::operator==(const IntPack16& other) const
{
	return _mm512_cmpeq_epi32_mask(zmm, other);
}

Mask16 IntPack16::operator!=(const IntPack16& other) const
{
	return _mm512_cmpneq_epi32_mask(zmm, other);
}

IntPack16 IntPack16::operator&(const IntPack16& other) const
{
	return _mm512_and_epi32(zmm, other);
}

IntPack16 IntPack16::operator|(const IntPack16& other) const
{
	return _mm512_or_epi32(zmm, other);
}

IntPack16 IntPack16::operator^(const IntPack16& other) const
{
	return _mm512_xor_epi32(zmm, other);
}

IntPack16& IntPack16::operator&=(const IntPack16& other)
{
	return *this = *this & other;
}

IntPack16& IntPack16::operator|=(const IntPack16& other)
{
	return *this = *this | other;
}

IntPack16& IntPack16::operator^=(const IntPack16& other)
{
	return *this = *this ^ other;
}
