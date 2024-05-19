#pragma once
#include <initializer_list>
#include <cassert>
#include <algorithm>

namespace bob
{
	template <typename T>
	struct _Vec2
	{
		T x, y;

		_Vec2() = default;
		_Vec2(const T& x, const T& y);
		_Vec2(const std::initializer_list<T>& list);

		_Vec2<T> operator+(const T& other) const; //Add a single value to all elements of the vector
		_Vec2<T> operator-(const T& other) const; //Subtract a single value from all elements of the vector
		_Vec2<T> operator*(const T& other) const; //Multiply all elements of the vector by a single value
		_Vec2<T> operator/(const T& other) const; //Divide all elements of the vector by a single value

		_Vec2<T>& operator+=(const T& other); //Add a single value to all elements of the vector in-place
		_Vec2<T>& operator-=(const T& other); //Subtract a single value from all elements of the vector in-place
		_Vec2<T>& operator*=(const T& other); //Multiply all elements of the vector by a single value in-place
		_Vec2<T>& operator/=(const T& other); //Divide all elements of the vector by a single in-place

		_Vec2<T> operator+(const _Vec2<T>& other) const;
		_Vec2<T> operator-(const _Vec2<T>& other) const;
		_Vec2<T> operator*(const _Vec2<T>& other) const;
		_Vec2<T> operator/(const _Vec2<T>& other) const;

		_Vec2<T>& operator+=(const _Vec2<T>& other);
		_Vec2<T>& operator-=(const _Vec2<T>& other);
		_Vec2<T>& operator*=(const _Vec2<T>& other);
		_Vec2<T>& operator/=(const _Vec2<T>& other);

		bool operator<(const _Vec2<T>& other) const;
		bool operator==(const _Vec2<T>& other) const;

		T product() const;
		T len() const;
		T lenSq() const;

		_Vec2<T> operator-() const;
		_Vec2<T> unit() const;

		T dot(const _Vec2<T>& other) const;

		template<typename U> operator _Vec2<U>() const;
		//explicit operator _Vec3<T>() const;
	};

	template<typename T> inline _Vec2<T>::_Vec2(const T& x, const T& y) : x(x), y(y) {};

	template<typename T> inline _Vec2<T>::_Vec2(const std::initializer_list<T>& list)
	{
		assert(std::distance(list.begin(), list.end()) == 2);
		x = *(list.begin());
		y = *(list.begin() + 1);
	}

	template<typename T>
	inline _Vec2<T> _Vec2<T>::operator+(const T& other) const
	{
		return _Vec2<T>(x + other, y + other);
	}

	template<typename T>
	inline _Vec2<T> _Vec2<T>::operator-(const T& other) const
	{
		return _Vec2<T>(x - other, y - other);
	}

	template<typename T>
	inline _Vec2<T> _Vec2<T>::operator*(const T& other) const
	{
		return _Vec2<T>(x * other, y * other);
	}

	template<typename T>
	inline _Vec2<T> _Vec2<T>::operator/(const T& other) const
	{
		return _Vec2<T>(x / other, y / other);
	}

	template<typename T>
	inline _Vec2<T>& _Vec2<T>::operator+=(const T& other)
	{
		x += other;
		y += other;
		return *this;
	}

	template<typename T>
	inline _Vec2<T>& _Vec2<T>::operator-=(const T& other)
	{
		x -= other;
		y -= other;
		return *this;
	}

	template<typename T>
	inline _Vec2<T>& _Vec2<T>::operator*=(const T& other)
	{
		x *= other;
		y *= other;
		return *this;
	}

	template<typename T>
	inline _Vec2<T>& _Vec2<T>::operator/=(const T& other)
	{
		x /= other;
		y /= other;
		return *this;
	}

	template<typename T>
	inline _Vec2<T> _Vec2<T>::operator+(const _Vec2<T>& other) const
	{
		return _Vec2<T>(x + other.x, y + other.y);
	}

	template<typename T>
	inline _Vec2<T> _Vec2<T>::operator-(const _Vec2<T>& other) const
	{
		return _Vec2<T>(x - other.x, y - other.y);
	}

	template<typename T>
	inline _Vec2<T> _Vec2<T>::operator*(const _Vec2<T>& other) const
	{
		return _Vec2<T>(x * other.x, y * other.y);
	}

	template<typename T>
	inline _Vec2<T> _Vec2<T>::operator/(const _Vec2<T>& other) const
	{
		return _Vec2<T>(x / other.x, y / other.y);
	}

	template<typename T>
	inline _Vec2<T>& _Vec2<T>::operator+=(const _Vec2<T>& other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	template<typename T>
	inline _Vec2<T>& _Vec2<T>::operator-=(const _Vec2<T>& other)
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}

	template<typename T>
	inline _Vec2<T>& _Vec2<T>::operator*=(const _Vec2<T>& other)
	{
		x *= other.x;
		y *= other.y;
		return *this;
	}

	template<typename T>
	inline _Vec2<T>& _Vec2<T>::operator/=(const _Vec2<T>& other)
	{
		x /= other.x;
		y /= other.y;
		return *this;
	}

	template <typename T>
	inline bool _Vec2<T>::operator<(const _Vec2<T>& other) const
	{
		if (x < other.x) return true;
		if (x > other.x) return false;
		return y < other.y;
	}

	template <typename T>
	inline bool _Vec2<T>::operator==(const _Vec2<T>& other) const
	{
		return x == other.x && y == other.y;
	}

	template<typename T>
	inline T _Vec2<T>::product() const
	{
		return x * y;
	}

	template<typename T>
	inline T _Vec2<T>::len() const
	{
		return sqrt(this->lenSq());
	}

	template<typename T>
	inline T _Vec2<T>::lenSq() const
	{
		return x * x + y * y;
	}

	template<typename T>
	inline _Vec2<T> _Vec2<T>::operator-() const
	{
		return _Vec2<T>(-x, -y);
	}

	template<typename T>
	inline _Vec2<T> _Vec2<T>::unit() const
	{
		T len = this->len();
		return _Vec2<T>(x / len, y / len);
	}

	template<typename T>
	inline T _Vec2<T>::dot(const _Vec2<T>& other) const
	{
		return x * other.x + y * other.y;
	}

	template<typename T>
	template<typename U>
	inline _Vec2<T>::operator _Vec2<U>() const
	{
		return _Vec2<U>(x, y);
	}
}