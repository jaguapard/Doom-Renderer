#pragma once
#include <immintrin.h>

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
	explicit operator bool() const;
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
