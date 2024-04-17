#pragma once
#include <vector>
#include <stdexcept>
#include <cassert>

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

	T& operator[](int i);
	const T& operator[](int i) const;
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
	for (auto& it : store) it = value;
}

template<typename T>
inline T* PixelBuffer<T>::getRawPixels()
{
	return &store.front();
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
