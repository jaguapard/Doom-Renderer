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

//template <typename T>
struct PixelBufferSize
{
	int w, h;
	__m128i dimensionsInt;
	__m128i pitchVec;
	Vec4 dimensionsFloat;

	PixelBufferSize() = default;
	PixelBufferSize(int w, int h)
	{
		this->w = w;
		this->h = h;

		this->dimensionsInt = _mm_setr_epi32(w, h, 0, 0);
		this->dimensionsFloat = Vec4(w, h, 0, 0);

		this->pitchVec = _mm_setr_epi32(h, 1, 0, 0);
	}
};

template <typename T>
class PixelBuffer
{
public:
	PixelBuffer();
	PixelBuffer(int w, int h);

	int getW() const;
	int getH() const;
	const PixelBufferSize& getSize() const;
	
	T getPixel(int x, int y) const; //does not perform bounds checks
	T getPixel(const __m128i& pos) const; //returns a pixel from x=pos[0], y=pos[1], other values are ignored
	T getPixel(const Vec4& pos) const; 

	void setPixelUnsafe(int x, int y, const T& px); //does not perform bounds checks

	bool isOutOfBounds(int x, int y) const;
	bool isInBounds(int x, int y) const;

	void clear(T value = T());
	T* getRawPixels();

	T* begin();
	T* end();

	T& operator[](int i);
	const T& operator[](int i) const;

	void saveToFile(const std::string& path) const;
	virtual Color toColor(T value) const;

	void operator=(const PixelBuffer<T>& other);
protected:
	T& at(int x, int y);
	const T& at(int x, int y) const;
	void assignSizes(int w, int h);

	std::vector<T> store;

	//a bunch of precomputed and properly formatted values for SIMD
	PixelBufferSize size;
};

template<typename T>
inline PixelBuffer<T>::PixelBuffer() {};

template<typename T>
inline PixelBuffer<T>::PixelBuffer(int w, int h)
{
	store.resize(w * h);
	store.shrink_to_fit();
	this->assignSizes(w, h);
}

template<typename T>
inline int PixelBuffer<T>::getW() const
{
	return size.w;
}

template<typename T>
inline int PixelBuffer<T>::getH() const
{
	return size.h;
}

template<typename T>
inline T PixelBuffer<T>::getPixel(int x, int y) const
{
	return at(x, y);
}

template<typename T>
inline T PixelBuffer<T>::getPixel(const __m128i& pos) const
{
	return getPixel(_mm_extract_epi32(pos, 0), _mm_extract_epi32(pos, 1));
}

template<typename T>
inline T PixelBuffer<T>::getPixel(const Vec4& pos) const
{
	return getPixel(_mm_cvtps_epi32(pos.xmm));
}

template<typename T>
inline void PixelBuffer<T>::setPixelUnsafe(int x, int y, const T& px)
{
	at(x, y) = px;
}

template<typename T>
inline bool PixelBuffer<T>::isOutOfBounds(int x, int y) const
{
	return !isInBounds(x, y);
}

template<typename T>
inline bool PixelBuffer<T>::isInBounds(int x, int y) const
{
	return x >= 0 && y >= 0 && x < size.w && y < size.h;
}

template<typename T>
inline void PixelBuffer<T>::clear(T value)
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
	std::fill(std::execution::par_unseq, this->begin(), this->end(), value);
#endif
}

template<typename T>
inline T* PixelBuffer<T>::getRawPixels()
{
	return &store.front();
}

template<typename T>
inline T* PixelBuffer<T>::begin()
{
	return &store.front();
}

template<typename T>
inline T* PixelBuffer<T>::end()
{
	return &store.back() + 1;
}

template<typename T>
inline T& PixelBuffer<T>::operator[](int i)
{
	return const_cast<T&>(static_cast<const PixelBuffer<T>&>(*this)[i]);
}

template<typename T>
inline const T& PixelBuffer<T>::operator[](int i) const
{
	return store[i];
}

template<typename T>
inline void PixelBuffer<T>::saveToFile(const std::string& path) const
{
	std::vector<Uint32> pix(size.w*size.h);
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
inline Color PixelBuffer<Color>::toColor(Color value) const
{
	return value;
}

template<>
inline Color PixelBuffer<real>::toColor(real value) const
{
	uint8_t fv = value * 255;
	return Color(fv, fv, fv);
}

template<>
inline Color PixelBuffer<uint8_t>::toColor(uint8_t value) const
{
	return Color(value, value, value);
}

template<typename T>
inline void PixelBuffer<T>::operator=(const PixelBuffer<T>& other)
{
	//if (w != 0 && h != 0 && (w != other.w || h != other.h)) throw std::runtime_error("Attempted to assign pixel buffer of mismatched size");
	
	this->store = other.store;
	store.shrink_to_fit();

	this->assignSizes(other.size.w, other.size.h);
}

template<typename T>
inline T& PixelBuffer<T>::at(int x, int y)
{
	return const_cast<T&>(static_cast<const PixelBuffer<T>&>(*this).at(x, y));
}

template<typename T>
inline const T& PixelBuffer<T>::at(int x, int y) const
{
	assert(x >= 0);
	assert(y >= 0);
	assert(x < w);
	assert(y < h);
	return store[y * size.w + x];
}

template<typename T>
inline void PixelBuffer<T>::assignSizes(int w, int h)
{
	size = PixelBufferSize(w, h);
}

template<typename T>
inline const PixelBufferSize& PixelBuffer<T>::getSize() const
{
	return size;
}