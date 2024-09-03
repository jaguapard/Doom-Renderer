#pragma once
#include <stdint.h>
#include "Mask16.h"
#include "FloatPack16.h"

struct alignas(64) IntPack16
{
	union {
		int32_t el[16];
		__m512i zmm;
	};

	IntPack16();
	IntPack16(const __m512i& m, Mask16 mask = 0xFFFF, const IntPack16& fillerVal = {});
	IntPack16(const int32_t x, Mask16 mask = 0xFFFF, const IntPack16& fillerVal = {});
	
	IntPack16(const int32_t* p, Mask16 mask = 0xFFFF, const IntPack16& fillerVal = {});
	//IntPack16(const FloatPack16& f, Mask16 mask = 0xFFFF, const IntPack16& fillerVal = {});
	IntPack16(const IntPack16& other, Mask16 mask = 0xFFFF, const IntPack16& fillerVal = {});
	IntPack16(int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t i6, int32_t i7, int32_t i8, int32_t i9, int32_t i10, int32_t i11, int32_t i12, int32_t i13, int32_t i14, int32_t i15, int32_t i16);

	void store(void* dst, Mask16 mask = 0xFFFF);
	

	friend IntPack16 operator+(const IntPack16& lhs, const int32_t rhs);
	friend IntPack16 operator-(const IntPack16& lhs, const int32_t rhs);
	friend IntPack16 operator*(const IntPack16& lhs, const int32_t rhs);
	friend IntPack16 operator/(const IntPack16& lhs, const int32_t rhs); //AVX512 does not have integer division, so it is emulated by double-precision division.

	IntPack16& operator+=(const int32_t other);
	IntPack16& operator-=(const int32_t other);
	IntPack16& operator*=(const int32_t other);
	IntPack16& operator/=(const int32_t other);

	friend IntPack16 operator+(const IntPack16& lhs, const IntPack16& rhs);
	friend IntPack16 operator-(const IntPack16& lhs, const IntPack16& rhs);
	friend IntPack16 operator*(const IntPack16& lhs, const IntPack16& rhs);
	friend IntPack16 operator/(const IntPack16& lhs, const IntPack16& rhs); //AVX512 does not have integer division, so it is emulated by double-precision division.
	IntPack16& operator+=(const IntPack16& other);
	IntPack16& operator-=(const IntPack16& other);
	IntPack16& operator*=(const IntPack16& other);
	IntPack16& operator/=(const IntPack16& other);

	Mask16 operator>(const IntPack16& other) const;
	Mask16 operator>=(const IntPack16& other) const;
	Mask16 operator<(const IntPack16& other) const;
	Mask16 operator<=(const IntPack16& other) const;
	Mask16 operator==(const IntPack16& other) const;
	Mask16 operator!=(const IntPack16& other) const;


	IntPack16 operator&(const IntPack16& other) const;
	IntPack16 operator|(const IntPack16& other) const;
	IntPack16 operator^(const IntPack16& other) const;
	IntPack16& operator&=(const IntPack16& other);
	IntPack16& operator|=(const IntPack16& other);
	IntPack16& operator^=(const IntPack16& other);

	IntPack16 operator<<(const int32_t other) const;
	IntPack16 operator<<(const IntPack16& other) const;
	IntPack16 operator>>(const int32_t other) const;
	IntPack16 operator>>(const IntPack16& other) const;

	IntPack16& operator<<=(const int32_t other);
	IntPack16& operator<<=(const IntPack16& other);
	

	IntPack16 operator-() const;
	IntPack16 operator~() const;

	operator __m512i() const;
	operator FloatPack16() const;
	operator __m512() const;

	IntPack16 clamp(int32_t min, int32_t max) const;
	IntPack16 clamp(const IntPack16& min, const IntPack16& max) const;
	//IntPack16 sqrt() const;
	//IntPack16 rsqrt14() const;
	//IntPack16 rsqrt28() const;

	static IntPack16 sequence(int32_t mult = 1);

	const int32_t& operator[](size_t i) const;
	int32_t& operator[](size_t i);
};