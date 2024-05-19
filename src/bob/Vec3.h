#pragma once
#include <initializer_list>
#include <cassert>
#include <iterator>

namespace bob
{
	template <typename T>
	struct _Vec3
	{
		T x, y, z;

		_Vec3() = default;
		_Vec3(const T& x, const T& y, const T& z);
		_Vec3(const std::initializer_list<T>& list);
		_Vec3(const _Vec2<T>& v2);

		_Vec3<T> operator+(const T& other) const; //Add a single value to all elements of the vector
		_Vec3<T> operator-(const T& other) const; //Subtract a single value from all elements of the vector
		_Vec3<T> operator*(const T& other) const; //Multiply all elements of the vector by a single value
		_Vec3<T> operator/(const T& other) const; //Divide all elements of the vector by a single value

		_Vec3<T>& operator+=(const T& other); //Add a single value to all elements of the vector in-place
		_Vec3<T>& operator-=(const T& other); //Subtract a single value from all elements of the vector in-place
		_Vec3<T>& operator*=(const T& other); //Multiply all elements of the vector by a single value in-place
		_Vec3<T>& operator/=(const T& other); //Divide all elements of the vector by a single in-place

		_Vec3<T> operator+(const _Vec3<T>& other) const;
		_Vec3<T> operator-(const _Vec3<T>& other) const;
		_Vec3<T> operator*(const _Vec3<T>& other) const;
		_Vec3<T> operator/(const _Vec3<T>& other) const;

		_Vec3<T>& operator+=(const _Vec3<T>& other);
		_Vec3<T>& operator-=(const _Vec3<T>& other);
		_Vec3<T>& operator*=(const _Vec3<T>& other);
		_Vec3<T>& operator/=(const _Vec3<T>& other);

		T product() const;
		T len() const;
		T lenSq() const;

		_Vec3<T> operator-() const;
		_Vec3<T> unit() const;

		T dot(const _Vec3<T>& other) const;
		_Vec3<T> cross(const _Vec3<T>& other) const;

		template<typename U> operator _Vec3<U>() const;
		template<typename U> operator _Vec2<U>() const;
	};

	template<typename T> inline _Vec3<T>::_Vec3(const T& x, const T& y, const T& z) : 
		x(x), y(y), z(z) 
	{};
	template<typename T> inline _Vec3<T>::_Vec3(const _Vec2<T>& v2) : x(v2.x), y(v2.y) {};

	template<typename T> inline _Vec3<T>::_Vec3(const std::initializer_list<T>& list)
	{
		assert(std::distance(list.begin(), list.end()) >= 3);
		x = *(list.begin());
		y = *(list.begin() + 1);
		z = *(list.begin() + 2);
	}

	template<typename T>
	inline _Vec3<T> _Vec3<T>::operator+(const T& other) const
	{
		return _Vec3<T>(x + other, y + other, z + other);
	}

	template<typename T>
	inline _Vec3<T> _Vec3<T>::operator-(const T& other) const
	{
		return _Vec3<T>(x - other, y - other, z - other);
	}

	template<typename T>
	inline _Vec3<T> _Vec3<T>::operator*(const T& other) const
	{
		return _Vec3<T>(x * other, y * other, z * other);
	}

	template<typename T>
	inline _Vec3<T> _Vec3<T>::operator/(const T& other) const
	{
		return _Vec3<T>(x / other, y / other, z / other);
	}

	template<typename T>
	inline _Vec3<T>& _Vec3<T>::operator+=(const T& other)
	{
		x += other;
		y += other;
		z += other;
		return *this;
	}

	template<typename T>
	inline _Vec3<T>& _Vec3<T>::operator-=(const T& other)
	{
		x -= other;
		y -= other;
		z -= other;
		return *this;
	}

	template<typename T>
	inline _Vec3<T>& _Vec3<T>::operator*=(const T& other)
	{
		x *= other;
		y *= other;
		z *= other;
		return *this;
	}

	template<typename T>
	inline _Vec3<T>& _Vec3<T>::operator/=(const T& other)
	{
		x /= other;
		y /= other;
		z /= other;
		return *this;
	}

	template<typename T>
	inline _Vec3<T> _Vec3<T>::operator+(const _Vec3<T>& other) const
	{
		return _Vec3<T>(x + other.x, y + other.y, z + other.z);
	}

	template<typename T>
	inline _Vec3<T> _Vec3<T>::operator-(const _Vec3<T>& other) const
	{
		return _Vec3<T>(x - other.x, y - other.y, z - other.z);
	}

	template<typename T>
	inline _Vec3<T> _Vec3<T>::operator*(const _Vec3<T>& other) const
	{
		return _Vec3<T>(x * other.x, y * other.y, z * other.z);
	}

	template<typename T>
	inline _Vec3<T> _Vec3<T>::operator/(const _Vec3<T>& other) const
	{
		return _Vec3<T>(x / other.x, y / other.y, z / other.z);
	}

	template<typename T>
	inline _Vec3<T>& _Vec3<T>::operator+=(const _Vec3<T>& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	template<typename T>
	inline _Vec3<T>& _Vec3<T>::operator-=(const _Vec3<T>& other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	template<typename T>
	inline _Vec3<T>& _Vec3<T>::operator*=(const _Vec3<T>& other)
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
		return *this;
	}

	template<typename T>
	inline _Vec3<T>& _Vec3<T>::operator/=(const _Vec3<T>& other)
	{
		x /= other.x;
		y /= other.y;
		z /= other.z;
		return *this;
	}

	template<typename T>
	inline T _Vec3<T>::product() const
	{
		return x * y * z;
	}

	template<typename T>
	inline T _Vec3<T>::len() const
	{
		return sqrt(this->lenSq());
	}

	template<typename T>
	inline T _Vec3<T>::lenSq() const
	{
		return x * x + y * y + z * z;
	}

	template<typename T>
	inline _Vec3<T> _Vec3<T>::operator-() const
	{
		return _Vec3<T>(-x, -y, -z);
	}

	template<typename T>
	inline _Vec3<T> _Vec3<T>::unit() const
	{
		T len = this->len();
		return _Vec3<T>(x / len, y / len, z / len);
	}

	template<typename T>
	inline T _Vec3<T>::dot(const _Vec3<T>& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}

	template<typename T>
	inline _Vec3<T> _Vec3<T>::cross(const _Vec3<T>& other) const
	{
		T cx = y * other.z - z * other.y;
		T cy = z * other.x - x * other.z;
		T cz = x * other.y - y * other.x;
		return _Vec3<T>(cx, cy, cz);
	}

	template<typename T>
	template<typename U>
	inline _Vec3<T>::operator _Vec3<U>() const
	{
		return _Vec3<U>(x, y, z);
	}	
	
	template<typename T>
	template<typename U>
	inline _Vec3<T>::operator _Vec2<U>() const
	{
		return _Vec2<U>(x, y);
	}
}