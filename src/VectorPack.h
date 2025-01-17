#pragma once
#include <immintrin.h>
#include <array>

#include "bob/SSE_Vec4.h"
#include "FloatPack8.h"
#include "FloatPack16.h"

//class representing 8 packed 4-dimensional vectors.
//Only AVX2 is supported with this one!
//The pack fits into 4 ymm registers.
//Each ymm register contains values from 8 vectors,
//i.e. ymm0 may have x0,x1,...,x7, ymm1 - y0,y1,...,y7, etc

template <typename PackType>
struct
#ifdef __AVX512F__
	alignas(64)
#else
	alignas(32)
#endif
	VectorPack
{
	union {
		struct { PackType x, y, z, w; };
		struct { PackType r, g, b, a; };
		PackType packs[4];
	};

	VectorPack() = default;
	VectorPack(const float x); //broadcast x to all elements of all vectors
	VectorPack(const bob::_SSE_Vec4_float& v); //broadcast a single vector to all vectors in the pack
	VectorPack(const PackType& pack); //broadcast a single pack to all values
	VectorPack(const PackType& x, const PackType& y, const PackType& z, const PackType& w);
	//VectorPack<PackType>(const __m256& pack);
	VectorPack(const std::initializer_list<bob::_SSE_Vec4_float>& list);

	VectorPack(const VectorPack<PackType>& other);
	//VectorPack<PackType>& operator=(const VectorPack<PackType>& other);

	template <typename Container>
	static VectorPack<PackType> fromHorizontalVectors(const Container& cont);

	VectorPack<PackType> operator+(const float other) const; //Add a single value to all elements of the vector pack
	VectorPack<PackType> operator-(const float other) const; //Subtract a single value from all elements of the vector pack
	VectorPack<PackType> operator*(const float other) const; //Multiply all elements of the vector pack by a single value 
	VectorPack<PackType> operator/(const float other) const; //Divide all elements of the vector pack by a single value

	VectorPack<PackType> operator+=(const float other); //Add a single value to all elements of the vector in-place
	VectorPack<PackType> operator-=(const float other); //Subtract a single value from all elements of the vector in-place
	VectorPack<PackType> operator*=(const float other); //Multiply all elements of the vector by a single value in-place
	VectorPack<PackType> operator/=(const float other); //Divide all elements of the vector by a single value in-place

	VectorPack<PackType> operator+(const VectorPack<PackType>& other) const;
	VectorPack<PackType> operator-(const VectorPack<PackType>& other) const;
	VectorPack<PackType> operator*(const VectorPack<PackType>& other) const;
	VectorPack<PackType> operator/(const VectorPack<PackType>& other) const;
	VectorPack<PackType>& operator+=(const VectorPack<PackType>& other);
	VectorPack<PackType>& operator-=(const VectorPack<PackType>& other);
	VectorPack<PackType>& operator*=(const VectorPack<PackType>& other);
	VectorPack<PackType>& operator/=(const VectorPack<PackType>& other);

	std::array<Mask16, 4> operator>(const VectorPack<PackType>& other) const;
	std::array<Mask16, 4> operator>=(const VectorPack<PackType>& other) const;
	std::array<Mask16, 4> operator<(const VectorPack<PackType>& other) const;
	std::array<Mask16, 4> operator<=(const VectorPack<PackType>& other) const;
	std::array<Mask16, 4> operator==(const VectorPack<PackType>& other) const;
	std::array<Mask16, 4> operator!=(const VectorPack<PackType>& other) const;
	

	VectorPack<PackType> operator+(const PackType& other) const;
	VectorPack<PackType> operator-(const PackType& other) const;
	VectorPack<PackType> operator*(const PackType& other) const;
	VectorPack<PackType> operator/(const PackType& other) const;
	VectorPack<PackType>& operator+=(const PackType& other);
	VectorPack<PackType>& operator-=(const PackType& other);
	VectorPack<PackType>& operator*=(const PackType& other);
	VectorPack<PackType>& operator/=(const PackType& other);

	std::array<__mmask16, 4> operator>(const PackType& other) const;
	std::array<__mmask16, 4> operator>=(const PackType& other) const;
	std::array<__mmask16, 4> operator<(const PackType& other) const;
	std::array<__mmask16, 4> operator<=(const PackType& other) const;
	std::array<__mmask16, 4> operator==(const PackType& other) const;
	std::array<__mmask16, 4> operator!=(const PackType& other) const;


	VectorPack<PackType> operator&(const VectorPack<PackType>& other) const;
	VectorPack<PackType> operator|(const VectorPack<PackType>& other) const;
	VectorPack<PackType> operator^(const VectorPack<PackType>& other) const;
	VectorPack<PackType>& operator&=(const VectorPack<PackType>& other);
	VectorPack<PackType>& operator|=(const VectorPack<PackType>& other);
	VectorPack<PackType>& operator^=(const VectorPack<PackType>& other);

	PackType* begin();
	PackType* end();
	const PackType* begin() const;
	const PackType* end() const;

	PackType& operator[](size_t i);
	const PackType& operator[](size_t i) const;

	float product() const;
	PackType len3d() const;
	PackType lenSq() const;
	PackType lenSq3d() const;

	VectorPack<PackType> operator-() const;
	VectorPack<PackType> operator~() const;
	VectorPack<PackType> unit() const;

	PackType dot(const VectorPack<PackType>& other) const;
	PackType dot3d(const VectorPack<PackType>& other) const; //ignore w coordinate
	PackType cross2d(const VectorPack<PackType>& other) const;
	VectorPack<PackType> cross3d(const VectorPack<PackType>& other) const;
	VectorPack<PackType> lerp(const VectorPack<PackType>& dst, const PackType& amount) const;

	bob::_SSE_Vec4_float extractHorizontalVector(size_t index) const;
};

template <typename PackType>
inline VectorPack<PackType>::VectorPack(const float x)
{
	for (auto& it : *this) it = x;
}

template <typename PackType>
inline VectorPack<PackType>::VectorPack(const bob::_SSE_Vec4_float& v)
{
	x = v.x;
	y = v.y;
	z = v.z;
	w = v.w;
}

template <typename PackType>
inline VectorPack<PackType>::VectorPack(const PackType& pack)
{
	for (auto& it : *this) it = pack;
}

template<typename PackType>
inline VectorPack<PackType>::VectorPack(const PackType& x, const PackType& y, const PackType& z, const PackType& w)
	:x(x), y(y), z(z), w(w)
{}

template <typename PackType>
inline VectorPack<PackType>::VectorPack(const std::initializer_list<bob::_SSE_Vec4_float>& list)
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

template<typename PackType>
inline VectorPack<PackType>::VectorPack(const VectorPack<PackType>& other) : x(other.x), y(other.y), z(other.z), w(other.w)
{}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator+(const float other) const
{
	VectorPack<PackType> ret;
	ret.x = x + other;
	ret.y = y + other;
	ret.z = z + other;
	ret.w = w + other;
	return ret;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator-(const float other) const
{
	VectorPack<PackType> ret;
	ret.x = x - other;
	ret.y = y - other;
	ret.z = z - other;
	ret.w = w - other;
	return ret;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator*(const float other) const
{
	VectorPack<PackType> ret;
	ret.x = x * other;
	ret.y = y * other;
	ret.z = z * other;
	ret.w = w * other;
	return ret;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator/(const float other) const
{
	VectorPack<PackType> ret;
	ret.x = x / other;
	ret.y = y / other;
	ret.z = z / other;
	ret.w = w / other;
	return ret;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator+=(const float other)
{
	return *this = *this + other;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator-=(const float other)
{
	return *this = *this - other;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator*=(const float other)
{
	return *this = *this * other;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator/=(const float other)
{
	return *this = *this / other;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator+(const VectorPack<PackType>& other) const
{
	VectorPack<PackType> ret;
	ret.x = x + other.x;
	ret.y = y + other.y;
	ret.z = z + other.z;
	ret.w = w + other.w;
	return ret;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator-(const VectorPack<PackType>& other) const
{
	VectorPack<PackType> ret;
	ret.x = x - other.x;
	ret.y = y - other.y;
	ret.z = z - other.z;
	ret.w = w - other.w;
	return ret;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator*(const VectorPack<PackType>& other) const
{
	VectorPack<PackType> ret;
	ret.x = x * other.x;
	ret.y = y * other.y;
	ret.z = z * other.z;
	ret.w = w * other.w;
	return ret;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator/(const VectorPack<PackType>& other) const
{
	VectorPack<PackType> ret;
	ret.x = x / other.x;
	ret.y = y / other.y;
	ret.z = z / other.z;
	ret.w = w / other.w;
	return ret;
}

template <typename PackType>
inline std::array<Mask16, 4> VectorPack<PackType>::operator>(const VectorPack<PackType>& other) const
{
    std::array<Mask16, 4> ret;
    for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] > other[i];
    return ret;
}

template <typename PackType>
inline std::array<Mask16, 4> VectorPack<PackType>::operator>=(const VectorPack<PackType>& other) const
{
    std::array<Mask16, 4> ret;
    for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] >= other[i];
    return ret;
}

template <typename PackType>
inline std::array<Mask16, 4> VectorPack<PackType>::operator<(const VectorPack<PackType>& other) const
{
    std::array<Mask16, 4> ret;
    for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] < other[i];
    return ret;
}

template <typename PackType>
inline std::array<Mask16, 4> VectorPack<PackType>::operator<=(const VectorPack<PackType>& other) const
{
    std::array<Mask16, 4> ret;
    for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] <= other[i];
    return ret;
}

template <typename PackType>
inline std::array<Mask16, 4> VectorPack<PackType>::operator==(const VectorPack<PackType>& other) const
{
    std::array<Mask16, 4> ret;
    for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] == other[i];
    return ret;
}

template <typename PackType>
inline std::array<Mask16, 4> VectorPack<PackType>::operator!=(const VectorPack<PackType>& other) const
{
    std::array<Mask16, 4> ret;
    for (size_t i = 0; i < std::size(packs); ++i) ret[i] = (*this)[i] != other[i];
    return ret;
}

template<typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator+(const PackType& other) const
{
	VectorPack<PackType> ret;
	ret.x = x + other;
	ret.y = y + other;
	ret.z = z + other;
	ret.w = w + other;
	return ret;
}

template<typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator-(const PackType& other) const
{
	VectorPack<PackType> ret;
	ret.x = x - other;
	ret.y = y - other;
	ret.z = z - other;
	ret.w = w - other;
	return ret;
}

template<typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator*(const PackType& other) const
{
	VectorPack<PackType> ret;
	ret.x = x * other;
	ret.y = y * other;
	ret.z = z * other;
	ret.w = w * other;
	return ret;
}


template<typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator/(const PackType& other) const
{
	VectorPack<PackType> ret;
	ret.x = x / other;
	ret.y = y / other;
	ret.z = z / other;
	ret.w = w / other;
	return ret;
}



template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator&(const VectorPack<PackType>& other) const
{
	VectorPack<PackType> ret;
	ret.x = x & other.x;
	ret.y = y & other.y;
	ret.z = z & other.z;
	ret.w = w & other.w;
	return ret;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator|(const VectorPack<PackType>& other) const
{
	VectorPack<PackType> ret;
	ret.x = x | other.x;
	ret.y = y | other.y;
	ret.z = z | other.z;
	ret.w = w | other.w;
	return ret;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator^(const VectorPack<PackType>& other) const
{
	VectorPack<PackType> ret;
	ret.x = x ^ other.x;
	ret.y = y ^ other.y;
	ret.z = z ^ other.z;
	ret.w = w ^ other.w;
	return ret;
}

template <typename PackType>
inline VectorPack<PackType>& VectorPack<PackType>::operator&=(const VectorPack<PackType>& other)
{
	return *this = *this & other;
}

template <typename PackType>
inline VectorPack<PackType>& VectorPack<PackType>::operator|=(const VectorPack<PackType>& other)
{
	return *this = *this | other;
}

template <typename PackType>
inline VectorPack<PackType>& VectorPack<PackType>::operator^=(const VectorPack<PackType>& other)
{
	return *this = *this ^ other;
}

template <typename PackType>
inline PackType* VectorPack<PackType>::begin()
{
	return const_cast<PackType*>(static_cast<const VectorPack*>(this)->begin());
}

template <typename PackType>
inline PackType* VectorPack<PackType>::end()
{
	return const_cast<PackType*>(static_cast<const VectorPack*>(this)->end());
}

template <typename PackType>
inline const PackType* VectorPack<PackType>::begin() const
{
	return &this->packs[0];
}

template <typename PackType>
inline const PackType* VectorPack<PackType>::end() const
{
	return &this->packs[0] + std::size(packs);
}

template <typename PackType>
inline PackType& VectorPack<PackType>::operator[](size_t i)
{
	const VectorPack<PackType>& p = *this;
	return const_cast<PackType&>(p[i]);
}

template <typename PackType>
inline const PackType& VectorPack<PackType>::operator[](size_t i) const
{
	return packs[i];
}

template <typename PackType>
inline VectorPack<PackType>& VectorPack<PackType>::operator+=(const VectorPack<PackType>& other)
{
	return *this = *this + other;
}

template <typename PackType>
inline VectorPack<PackType>& VectorPack<PackType>::operator-=(const VectorPack<PackType>& other)
{
	return *this = *this - other;
}

template <typename PackType>
inline VectorPack<PackType>& VectorPack<PackType>::operator*=(const VectorPack<PackType>& other)
{
	return *this = *this * other;
}

template <typename PackType>
inline VectorPack<PackType>& VectorPack<PackType>::operator/=(const VectorPack<PackType>& other)
{
	return *this = *this / other;
}

template <typename PackType>
inline VectorPack<PackType>& VectorPack<PackType>::operator+=(const PackType& other)
{
	return *this = *this + other;
}

template <typename PackType>
inline VectorPack<PackType>& VectorPack<PackType>::operator-=(const PackType& other)
{
	return *this = *this - other;
}

template <typename PackType>
inline VectorPack<PackType>& VectorPack<PackType>::operator*=(const PackType& other)
{
	return *this = *this * other;
}

template <typename PackType>
inline VectorPack<PackType>& VectorPack<PackType>::operator/=(const PackType& other)
{
	return *this = *this / other;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator-() const
{
	VectorPack<PackType> ret;
	ret.x = -x;
	ret.y = -y;
	ret.z = -z;
	ret.w = -w;
	return ret;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::operator~() const
{
	VectorPack<PackType> ret;
	ret.x = ~x;
	ret.y = ~y;
	ret.z = ~z;
	ret.w = ~w;
	return ret;
}

template <typename PackType>
inline PackType VectorPack<PackType>::dot(const VectorPack<PackType>& other) const
{
	return x * other.x + y * other.y + z * other.z + w * other.w;
}

template <typename PackType>
inline PackType VectorPack<PackType>::dot3d(const VectorPack<PackType>& other) const
{
	return x * other.x + y * other.y + z * other.z;
}

template <typename PackType>
inline PackType VectorPack<PackType>::cross2d(const VectorPack<PackType>& other) const
{
	return x * other.y - y * other.x;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::cross3d(const VectorPack<PackType>& other) const
{
	VectorPack<PackType> ret;
	ret.x = y * other.z - z * other.y;
	ret.y = z * other.x - x * other.z;
	ret.z = x * other.y - y * other.x;
	ret.w = 0.0f;
	return ret;
}

template <typename PackType>
inline VectorPack<PackType> VectorPack<PackType>::lerp(const VectorPack<PackType>& dst, const PackType& amount) const
{
	return *this + (dst - *this) * amount;
}

template <typename PackType>
inline bob::_SSE_Vec4_float VectorPack<PackType>::extractHorizontalVector(size_t index) const
{
	return bob::_SSE_Vec4_float(x.f[index], y.f[index], z.f[index], w.f[index]);
}

template <typename PackType>
template<typename Container>
inline VectorPack<PackType> VectorPack<PackType>::fromHorizontalVectors(const Container& cont)
{
	//assert(std::size(cont) <= std::size(packs));
	VectorPack<PackType> ret;
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

template<typename PackType>
inline PackType VectorPack<PackType>::len3d() const
{
	return this->lenSq3d().sqrt();
}

template <typename PackType>
inline PackType VectorPack<PackType>::lenSq() const
{
	return this->dot(*this);
}

template <typename PackType>
inline PackType VectorPack<PackType>::lenSq3d() const
{
	return this->dot3d(*this);
}

typedef VectorPack<FloatPack8> VectorPack8;
typedef VectorPack<FloatPack16> VectorPack16;