#pragma once
#include <immintrin.h>
#include "bob/SSE_Vec4.h"
#include "FloatPack8.h"

//class representing 8 packed 4-dimensional vectors.
//Only AVX2 is supported with this one!
//The pack fits into 4 ymm registers.
//Each ymm register contains values from 8 vectors,
//i.e. ymm0 may have x0,x1,...,x7, ymm1 - y0,y1,...,y7, etc
struct alignas(32) VectorPack
{
	union {
		struct { FloatPack8 x, y, z, w; };
		FloatPack8 packs[4];
	};

	VectorPack() = default;
	VectorPack(const float x); //broadcast x to all elements of all vectors
	VectorPack(const bob::_SSE_Vec4_float& v); //broadcast a single vector to all vectors in the pack
	VectorPack(const FloatPack8& pack);
	VectorPack(const __m256& pack);
	VectorPack(const std::initializer_list<bob::_SSE_Vec4_float>& list);

	template <typename Container>
	static VectorPack fromHorizontalVectors(const Container& cont);

	VectorPack operator+(const float other) const; //Add a single value to all elements of the vector pack
	VectorPack operator-(const float other) const; //Subtract a single value from all elements of the vector pack
	VectorPack operator*(const float other) const; //Multiply all elements of the vector pack by a single value 
	VectorPack operator/(const float other) const; //Divide all elements of the vector pack by a single value

	VectorPack operator+=(const float other); //Add a single value to all elements of the vector in-place
	VectorPack operator-=(const float other); //Subtract a single value from all elements of the vector in-place
	VectorPack operator*=(const float other); //Multiply all elements of the vector by a single value in-place
	VectorPack operator/=(const float other); //Divide all elements of the vector by a single value in-place

	VectorPack operator+(const VectorPack& other) const;
	VectorPack operator-(const VectorPack& other) const;
	VectorPack operator*(const VectorPack& other) const;
	VectorPack operator/(const VectorPack& other) const;
	VectorPack& operator+=(const VectorPack& other);
	VectorPack& operator-=(const VectorPack& other);
	VectorPack& operator*=(const VectorPack& other);
	VectorPack& operator/=(const VectorPack& other);

	VectorPack operator>(const VectorPack& other) const;
	VectorPack operator>=(const VectorPack& other) const;
	VectorPack operator<(const VectorPack& other) const;
	VectorPack operator<=(const VectorPack& other) const;
	VectorPack operator==(const VectorPack& other) const;
	VectorPack operator!=(const VectorPack& other) const;


	VectorPack operator&(const VectorPack& other) const;
	VectorPack operator|(const VectorPack& other) const;
	VectorPack operator^(const VectorPack& other) const;
	VectorPack& operator&=(const VectorPack& other);
	VectorPack& operator|=(const VectorPack& other);
	VectorPack& operator^=(const VectorPack& other);

	FloatPack8* begin();
	FloatPack8* end();
	const FloatPack8* begin() const;
	const FloatPack8* end() const;

	FloatPack8& operator[](size_t i);
	const FloatPack8& operator[](size_t i) const;

	float product() const;
	float len() const;
	float lenSq() const;

	VectorPack operator-() const;
	VectorPack operator~() const;
	VectorPack unit() const;

	float dot(const VectorPack& other) const;
	FloatPack8 cross2d(const VectorPack& other) const;
	VectorPack cross3d(const VectorPack& other) const;

	bob::_SSE_Vec4_float extractHorizontalVector(size_t index) const;
};

inline VectorPack::VectorPack(const float x)
{
	for (auto& it : *this) it = _mm256_set1_ps(x);
}

inline VectorPack::VectorPack(const bob::_SSE_Vec4_float& v)
{
	x = FloatPack8(v.x);
	y = FloatPack8(v.y);
	z = FloatPack8(v.z);
	w = FloatPack8(v.w);
}

inline VectorPack::VectorPack(const FloatPack8& pack)
{
	for (auto& it : *this) it = pack;
}

inline VectorPack::VectorPack(const __m256& pack)
{
	for (auto& it : *this) it = pack;
}

inline VectorPack::VectorPack(const std::initializer_list<bob::_SSE_Vec4_float>& list)
{
	size_t sz = std::end(list) - std::begin(list);
	for (size_t i = 0; i < sz; ++i)
	{
		x.f[i] = (std::begin(list) + i)->x;
		y.f[i] = (std::begin(list) + i)->y;
		z.f[i] = (std::begin(list) + i)->z;
		w.f[i] = (std::begin(list) + i)->w;
	}
}

inline VectorPack VectorPack::operator+(const float other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] + other;
	return ret;
}

inline VectorPack VectorPack::operator-(const float other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] - other;
	return ret;
}

inline VectorPack VectorPack::operator*(const float other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] * other;
	return ret;
}

inline VectorPack VectorPack::operator/(const float other) const
{
	constexpr bool useReciprocal = false;
	VectorPack ret;

	if (useReciprocal)
	{
		float rcp = 1.0f / other;
		for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] * rcp;
	}
	else for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] / other;
	return ret;
}

inline VectorPack VectorPack::operator+=(const float other)
{
	return *this = *this + other;
}

inline VectorPack VectorPack::operator-=(const float other)
{
	return *this = *this - other;
}

inline VectorPack VectorPack::operator*=(const float other)
{
	return *this = *this * other;
}

inline VectorPack VectorPack::operator/=(const float other)
{
	return *this = *this / other;
}

inline VectorPack VectorPack::operator+(const VectorPack& other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] + other[i];
	return ret;
}

inline VectorPack VectorPack::operator-(const VectorPack& other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] - other[i];
	return ret;
}

inline VectorPack VectorPack::operator*(const VectorPack& other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] * other[i];
	return ret;
}

inline VectorPack VectorPack::operator/(const VectorPack& other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] / other[i];
	return ret;
}

inline VectorPack VectorPack::operator>(const VectorPack& other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] > other[i];
	return ret;
}

inline VectorPack VectorPack::operator>=(const VectorPack& other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] >= other[i];
	return ret;
}

inline VectorPack VectorPack::operator<(const VectorPack& other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] < other[i];
	return ret;
}

inline VectorPack VectorPack::operator<=(const VectorPack& other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] <= other[i];
	return ret;
}

inline VectorPack VectorPack::operator==(const VectorPack& other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] == other[i];
	return ret;
}

inline VectorPack VectorPack::operator!=(const VectorPack& other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] != other[i];
	return ret;
}

inline VectorPack VectorPack::operator&(const VectorPack& other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] & other[i];
	return ret;
}

inline VectorPack VectorPack::operator|(const VectorPack& other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] | other[i];
	return ret;
}

inline VectorPack VectorPack::operator^(const VectorPack& other) const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] ^ other[i];
	return ret;
}

inline VectorPack& VectorPack::operator&=(const VectorPack& other)
{
	return *this = *this & other;
}

inline VectorPack& VectorPack::operator|=(const VectorPack& other)
{
	return *this = *this | other;
}

inline VectorPack& VectorPack::operator^=(const VectorPack& other)
{
	return *this = *this ^ other;
}

inline FloatPack8* VectorPack::begin()
{
	return const_cast<FloatPack8*>(static_cast<const VectorPack*>(this)->begin());
}

inline FloatPack8* VectorPack::end()
{
	return const_cast<FloatPack8*>(static_cast<const VectorPack*>(this)->end());
}

inline const FloatPack8* VectorPack::begin() const
{
	return &this->packs[0];
}

inline const FloatPack8* VectorPack::end() const
{
	return &this->packs[0] + std::size(packs);
}

inline FloatPack8& VectorPack::operator[](size_t i)
{
	const VectorPack& p = *this;
	return const_cast<FloatPack8&>(p[i]);
}

inline const FloatPack8& VectorPack::operator[](size_t i) const
{
	return packs[i];
}

inline VectorPack& VectorPack::operator+=(const VectorPack& other)
{
	return *this = *this + other;
}

inline VectorPack& VectorPack::operator-=(const VectorPack& other)
{
	return *this = *this - other;
}

inline VectorPack& VectorPack::operator*=(const VectorPack& other)
{
	return *this = *this * other;
}

inline VectorPack& VectorPack::operator/=(const VectorPack& other)
{
	return *this = *this / other;
}

inline VectorPack VectorPack::operator-() const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = -(*this)[i];
	return ret;
}

inline VectorPack VectorPack::operator~() const
{
	VectorPack ret;
	for (size_t i = 0; i < std::size(packs); ++i) ret[i] = ~(*this)[i];
	return ret;
}

inline FloatPack8 VectorPack::cross2d(const VectorPack& other) const
{
	return x * other.y - y * other.x;
}

inline VectorPack VectorPack::cross3d(const VectorPack& other) const
{
	VectorPack ret;
	ret.x = y * other.z - z * other.y;
	ret.y = z * other.x - x * other.z;
	ret.z = x * other.y - y * other.x;
	ret.w = 0.0f;
	return ret;
}

inline bob::_SSE_Vec4_float VectorPack::extractHorizontalVector(size_t index) const
{
	return bob::_SSE_Vec4_float(x.f[index], y.f[index], z.f[index], w.f[index]);
}

template<typename Container>
inline VectorPack VectorPack::fromHorizontalVectors(const Container& cont)
{
	assert(std::size(cont) <= std::size(packs));
	VectorPack ret;
	int i = 0;
	for (auto it = std::begin(cont); it != std::end(cont); ++it, ++i)
	{
		ret.x[i] = (*it)[i][0];
		ret.y[i] = (*it)[i][1];
		ret.z[i] = (*it)[i][2];
		ret.w[i] = (*it)[i][4];
	}
	return ret;
}
