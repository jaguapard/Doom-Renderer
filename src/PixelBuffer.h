#pragma once
#include <vector>
#include <stdexcept>
#include <cassert>
#include <bit>
#include <execution>
#include <immintrin.h>

#include <SDL/SDL.h>

#include "Color.h"
#include "Vec.h"
#include "helpers.h"
#include "VectorPack.h"

struct PixelBufferSize
{
	uint32_t w, h;
	float fw, fh;


	PixelBufferSize() = default;
	PixelBufferSize(int w, int h)
	{
		this->w = w;
		this->h = h;
		this->fw = w;
		this->fh = h;
	}
};

struct Rect
{
	int xLeft, yLeft, w, h;
};

template <typename T>
class PixelBufferBase
{
public:
	PixelBufferBase();
	PixelBufferBase(int w, int h);

	int getW() const;
	int getH() const;
	const PixelBufferSize& getSize() const;

	T getPixel(int x, int y) const; //does not perform bounds checks
	T getPixel(const __m128i& pos) const; //returns a pixel from x=pos[0], y=pos[1], other values are ignored
	T getPixel(const Vec4& pos) const;
	T getPixel64(const __m128i& pos) const;
	T getPixel64(const __m128d& pos) const;

	void setPixel(int x, int y, const T& px); //does not perform bounds checks

	bool isOutOfBounds(int x, int y) const;
	bool isInBounds(int x, int y) const;

	void clear(T value = T());
	void clearRows(int minY, int maxY, T value = T());

	T* getRawPixels();
	const T* getRawPixels() const;

	T* begin();
	T* end();

	T& operator[](uint64_t i);
	const T& operator[](uint64_t i) const;

	void saveToFile(const std::string& path) const;
	virtual Color toColor(T value) const; //cannot make this = 0: compiler complains about abstract class. However, if not used, it doesn't matter that this is undefined. Only children of this class may have this

	void operator=(const PixelBufferBase<T>& other);

	__m512i calcIndices(__m512i x, __m512i y) const
	{
		return _mm512_add_epi32(x, _mm512_mullo_epi32(y, _mm512_set1_epi32(this->getW())));
	}
protected:
	T& at(int x, int y);
	const T& at(int x, int y) const;
	void assignSizes(int w, int h);

	std::vector<T> store;

	//a bunch of precomputed and properly formatted values for SIMD
	PixelBufferSize size;
};

template<typename T>
inline PixelBufferBase<T>::PixelBufferBase() {};

template<typename T>
inline PixelBufferBase<T>::PixelBufferBase(int w, int h)
{
	store.resize(w * h);
	store.shrink_to_fit();
	this->assignSizes(w, h);
}

template<typename T>
inline int PixelBufferBase<T>::getW() const
{
	return size.w;
}

template<typename T>
inline int PixelBufferBase<T>::getH() const
{
	return size.h;
}

template<typename T>
inline T PixelBufferBase<T>::getPixel(int x, int y) const
{
	return at(x, y);
}
/*
template<typename T>
inline T PixelBufferBase<T>::getPixel(const __m128i& pos) const
{
	__m128i offsets = _mm_mullo_epi32(pos, size.pitchInt32);
	return (*this)[_mm_extract_epi32(offsets, 0) + _mm_extract_epi32(offsets, 1)];
}

template<typename T>
inline T PixelBufferBase<T>::getPixel(const Vec4& pos) const
{
	return getPixel(_mm_cvttps_epi32(pos));
}

template<typename T>
inline T PixelBufferBase<T>::getPixel64(const __m128i& pos) const
{
	__m128i interm = _mm_mul_epi32(pos, this->size.pitchInt64);
	return (*this)[_mm_extract_epi64(interm, 0) + _mm_extract_epi64(interm, 1)];
}

template<typename T>
inline T PixelBufferBase<T>::getPixel64(const __m128d& pos) const
{
	return getPixel(_mm_cvttpd_epi32(pos));
}
*/
template<typename T>
inline void PixelBufferBase<T>::setPixel(int x, int y, const T& px)
{
	at(x, y) = px;
}

template<typename T>
inline bool PixelBufferBase<T>::isOutOfBounds(int x, int y) const
{
	return !isInBounds(x, y);
}

template<typename T>
inline bool PixelBufferBase<T>::isInBounds(int x, int y) const
{
	return x >= 0 && y >= 0 && x < size.w && y < size.h;
}

template<typename T>
inline void PixelBufferBase<T>::clear(T value)
{
#if 0
	//#ifdef  //__AVX2__
	int bytesRemaining = this->getW() * this->getH() * sizeof(T);
	assert(bytesRemaining % 32 == 0);
	char* curr = reinterpret_cast<char*>(&(*this)[0]);

	__m256i fill;
	switch (sizeof(T))
	{
	case 1:
		fill = _mm256_set1_epi8(*reinterpret_cast<char*>(&value));
		break;
	case 2:
		fill = _mm256_set1_epi16(*reinterpret_cast<uint16_t*>(&value));
		break;
	case 4:
		fill = _mm256_set1_epi32(*reinterpret_cast<uint32_t*>(&value));
		break;
	case 8:
		fill = _mm256_set1_epi32(*reinterpret_cast<uint64_t*>(&value));
		break;
	default:
		assert(false);
		break;
	}

	while (bytesRemaining >= 32)
	{
		_mm256_store_si256(reinterpret_cast<__m256i*>(curr), fill);

		bytesRemaining -= 32;
		curr += 32;
	}

#else
	//for (auto& it : store) it = value;
	//std::fill(std::execution::par_unseq, this->begin(), this->end(), value);
	this->clearRows(0, size.h, value);
#endif
}

template<typename T>
inline void PixelBufferBase<T>::clearRows(int minY, int maxY, T value)
{
	T* start = this->getRawPixels() + size_t(minY) * size.w;
	T* end = this->getRawPixels() + size_t(maxY) * size.w;
	while (start < end) *start++ = value;
}

template<typename T>
inline T* PixelBufferBase<T>::getRawPixels()
{
	return const_cast<T*>(static_cast<const PixelBufferBase<T>*>(this)->getRawPixels());
}

template<typename T>
inline const T* PixelBufferBase<T>::getRawPixels() const
{
	return &store.front();
}

template<typename T>
inline T* PixelBufferBase<T>::begin()
{
	return &store.front();
}

template<typename T>
inline T* PixelBufferBase<T>::end()
{
	return &store.back() + 1;
}

template<typename T>
inline T& PixelBufferBase<T>::operator[](uint64_t i)
{
	return const_cast<T&>(static_cast<const PixelBufferBase<T>&>(*this)[i]);
}

template<typename T>
inline const T& PixelBufferBase<T>::operator[](uint64_t i) const
{
	return store[i];
}

template<typename T>
inline void PixelBufferBase<T>::saveToFile(const std::string& path) const
{
	std::vector<Uint32> pix(size.w * size.h);
	for (int y = 0; y < size.h; ++y)
	{
		for (int x = 0; x < size.w; ++x)
		{
			pix[y * size.w + x] = this->toColor(this->getPixel(x, y));
		}
	}

	Uint32* px = &pix.front();
	SDL_Surface* s = SDL_CreateRGBSurfaceWithFormatFrom(px, size.w, size.h, 32, size.w * 4, SDL_PIXELFORMAT_RGBA32);
	IMG_SavePNG(s, path.c_str());
	SDL_FreeSurface(s);
}

template<>
inline Color PixelBufferBase<Color>::toColor(Color value) const
{
	return value;
}

template<>
inline Color PixelBufferBase<real>::toColor(real value) const
{
	uint8_t fv = value * 255;
	return Color(fv, fv, fv);
}

template<>
inline Color PixelBufferBase<uint8_t>::toColor(uint8_t value) const
{
	return Color(value, value, value);
}

template<typename T>
inline void PixelBufferBase<T>::operator=(const PixelBufferBase<T>& other)
{
	//if (w != 0 && size.h != 0 && (w != other.w || size.h != other.h)) throw std::runtime_error("Attempted to assign pixel buffer of mismatched size");

	this->store = other.store;
	store.shrink_to_fit();

	this->assignSizes(other.size.w, other.size.h);
}

template<typename T>
inline T& PixelBufferBase<T>::at(int x, int y)
{
	return const_cast<T&>(static_cast<const PixelBufferBase<T>&>(*this).at(x, y));
}

template<typename T>
inline const T& PixelBufferBase<T>::at(int x, int y) const
{
	assert(x >= 0);
	assert(y >= 0);
	assert(x < size.w);
	assert(y < size.h);
	return store[y * size.w + x];
}

template<typename T>
inline void PixelBufferBase<T>::assignSizes(int w, int h)
{
	size = PixelBufferSize(w, h);
}

template<typename T>
inline const PixelBufferSize& PixelBufferBase<T>::getSize() const
{
	return size;
}

template <typename T>
class PixelBuffer : public PixelBufferBase<T>
{
	using PixelBufferBase<T>::PixelBufferBase;
};

template <>
class PixelBuffer<float> : public PixelBufferBase<float>
{
public:
	using PixelBufferBase<float>::PixelBufferBase;

	__mmask16 checkBounds(__m512 x, __m512 y) const
	{
		FloatPack16 fx = FloatPack16(x);
		FloatPack16 fy = FloatPack16(y);
		return fx >= 0.f & fx < this->getSize().fw & fy >= 0.f & fy < this->getSize().fh;
	}
	__m512 gatherPixels16(__m512i indices, __mmask16 mask) const
	{
		return _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask, indices, store.data(), 4);
	}
	__m512 gatherPixels16(__m512i x, __m512i y, __mmask16 mask) const
	{
		return gatherPixels16(this->calcIndices(x, y), mask);
	}

	__m512 getPixels16(size_t indexStart, __mmask16 mask = 0xFFFF, __m512 fillerVal = _mm512_set1_ps(0)) const
	{
		return _mm512_mask_loadu_ps(fillerVal, mask, store.data()+indexStart);
	}
	
	__m512 getPixels16(size_t xStart, size_t y, __mmask16 mask = 0xFFFF, __m512 fillerVal = _mm512_set1_ps(0)) const
	{
		return getPixels16(y * getW() + xStart, mask, fillerVal);
	}
	void setPixels16(size_t indexStart, __m512 pixels, __mmask16 mask)
	{
		assert(indexStart < size_t(getW()) * getH());
		_mm512_mask_store_ps(store.data()+indexStart, mask, pixels);
	}

	void setPixels16(size_t xStart, size_t y, __m512 pixels, __mmask16 mask)
	{
		assert(xStart >= 0);
		assert(y >= 0);
		assert(xStart < getW());
		assert(y < getH());

		setPixels16(y * getW() + xStart, pixels, mask);
	}

	void scatterPixels16(__m512i x, __m512i y, __m512 pixels, __mmask16 mask = 0xFFFF)
	{
		_mm512_mask_i32scatter_ps(store.data(), mask, calcIndices(x, y), pixels, 4);
	}
};

template <>
class PixelBuffer<Color> : public PixelBufferBase<Color>
{
public:
	using PixelBufferBase<Color>::PixelBufferBase;

	__m512i gatherPixels16(__m512i indices, __mmask16 mask = 0xFFFF, __m512i fillerVal = _mm512_set1_epi32(0)) const
	{
		return _mm512_mask_i32gather_epi32(fillerVal, mask, indices, this->store.data(), sizeof(Color));
	}
	
	__m512i gatherPixels16(__m512i x, __m512i y, __mmask16 mask = 0xFFFF, __m512i fillerVal = _mm512_set1_epi32(0)) const
	{
		return gatherPixels16(this->calcIndices(x, y), mask, fillerVal);
	}
};