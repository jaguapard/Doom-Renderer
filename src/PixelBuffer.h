#pragma once
#include <vector>
#include <stdexcept>
#include <cassert>
#include <bit>
#include <immintrin.h>

#include <SDL/SDL.h>

#include "Color.h"
template <typename T>
class PixelBuffer
{
public:
	PixelBuffer() = default;
	PixelBuffer(int w, int h);

	int getW() const;
	int getH() const;

	T getPixel(int x, int y, bool& outOfBounds) const; //Returns a pixel at (x,y) and sets `outOfBounds` to false if the point is in bounds. Else, returns default constructed T and sets `outOfBounds` to true
	T getPixelUnsafe(int x, int y) const; //does not perform bounds checks
	bool setPixel(int x, int y, const T& px); //if pixel was set, returns true. Else (pixel is out of bounds) returns false
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
protected:
	T& at(int x, int y);
	const T& at(int x, int y) const;
	std::vector<T> store;
	int w, h;
};

template<typename T>
inline PixelBuffer<T>::PixelBuffer(int w, int h) : w(w), h(h)
{
	store.resize(w * h);
}

template<typename T>
inline int PixelBuffer<T>::getW() const
{
	return w;
}

template<typename T>
inline int PixelBuffer<T>::getH() const
{
	return h;
}

template<typename T>
inline T PixelBuffer<T>::getPixel(int x, int y, bool& outOfBounds) const
{
	if (isInBounds(x, y)) return at(x, y);
	outOfBounds = true;
	return T();
}

template<typename T>
inline T PixelBuffer<T>::getPixelUnsafe(int x, int y) const
{
	return at(x, y);
}

template<typename T>
inline bool PixelBuffer<T>::setPixel(int x, int y, const T& px)
{
	if (isInBounds(x, y)) 
	{
		at(x, y) = px;
		return true;
	}
	else return false;
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
	return x >= 0 && y >= 0 && x < w && y < h;
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
	for (auto& it : store) it = value;
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
	std::vector<Uint32> pix(w*h);
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			pix[y * w + x] = this->toColor(this->getPixelUnsafe(x, y));
		}
	}

	Uint32* px = &pix.front();
	SDL_Surface* s = SDL_CreateRGBSurfaceWithFormatFrom(px, w, h, 32, w * 4, SDL_PIXELFORMAT_RGBA32);
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
	return store[y * w + x];
}
