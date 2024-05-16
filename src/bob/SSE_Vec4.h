#pragma once
#include <initializer_list>
#include <cassert>
#include <iterator>


#include <immintrin.h>
#include <smmintrin.h>

namespace bob
{

	struct _SSE_Vec4_float
	{
		union
		{
			__m128 sseVec;
			struct { float x, y, z, w; };
			//struct { float r, g, b, a; };
			float val[4];
		};
	private:
		static constexpr bool SSE_ENABLED = true;
	public:
		_SSE_Vec4_float() = default;
		_SSE_Vec4_float(const float x);
		_SSE_Vec4_float(const float x, const float y);
		_SSE_Vec4_float(const float x, const float y, const float z);
		_SSE_Vec4_float(const float x, const float y, const float z, const float w);
		_SSE_Vec4_float(const __m128 v);
		_SSE_Vec4_float(const std::initializer_list<float> list);

		_SSE_Vec4_float operator+(const float other) const; //Add a single value to all elements of the vector
		_SSE_Vec4_float operator-(const float other) const; //Subtract a single value from all elements of the vector
		_SSE_Vec4_float operator*(const float other) const; //Multiply all elements of the vector by a single value
		_SSE_Vec4_float operator/(const float other) const; //Divide all elements of the vector by a single value

		_SSE_Vec4_float operator+=(const float other); //Add a single value to all elements of the vector in-place
		_SSE_Vec4_float operator-=(const float other); //Subtract a single value from all elements of the vector in-place
		_SSE_Vec4_float operator*=(const float other); //Multiply all elements of the vector by a single value in-place
		_SSE_Vec4_float operator/=(const float other); //Divide all elements of the vector by a single value in-place

		_SSE_Vec4_float operator+(const _SSE_Vec4_float other) const;
		_SSE_Vec4_float operator-(const _SSE_Vec4_float other) const;
		_SSE_Vec4_float operator*(const _SSE_Vec4_float other) const;
		_SSE_Vec4_float operator/(const _SSE_Vec4_float other) const;

		_SSE_Vec4_float operator+=(const _SSE_Vec4_float other);
		_SSE_Vec4_float operator-=(const _SSE_Vec4_float other);
		_SSE_Vec4_float operator*=(const _SSE_Vec4_float other);
		_SSE_Vec4_float operator/=(const _SSE_Vec4_float other);

		void assert_validX() const;
		void assert_validY() const;
		void assert_validZ() const;
		void assert_validW() const;

		float& operator[](int i);
		const float& operator[](int i) const;

		float product() const;
		float len() const;
		float lenSq() const;

		_SSE_Vec4_float operator-() const;
		_SSE_Vec4_float unit() const;

		float dot(const _SSE_Vec4_float other) const;
		_SSE_Vec4_float cross3d(const _SSE_Vec4_float other) const;
	};

	inline _SSE_Vec4_float::_SSE_Vec4_float(const float x) : x(x), y(0), z(0), w(0) {}
	inline _SSE_Vec4_float::_SSE_Vec4_float(const float x, const float y) : x(x), y(y), z(0), w(0) {}
	inline _SSE_Vec4_float::_SSE_Vec4_float(const float x, const float y, const float z) : x(x), y(y), z(z), w(0) {}
	inline _SSE_Vec4_float::_SSE_Vec4_float(const float x, const float y, const float z, const float w) : x(x), y(y), z(z), w(w) {}

	inline _SSE_Vec4_float::_SSE_Vec4_float(const __m128 v) : sseVec(v){}

	inline _SSE_Vec4_float::_SSE_Vec4_float(const std::initializer_list<float> list)
	{
		x = list.size() >= 1 ? *(list.begin()) : 0;
		y = list.size() >= 2 ? *(list.begin() + 1) : 0;
		z = list.size() >= 3 ? *(list.begin() + 2) : 0;
		w = list.size() >= 4 ? *(list.begin() + 3) : 0;
	}

	inline _SSE_Vec4_float _SSE_Vec4_float::operator+(const float other) const
	{
		if (SSE_ENABLED) return _mm_add_ps(this->sseVec, _mm_set1_ps(other));
		return _SSE_Vec4_float(x + other, y + other, z + other, w + other);
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator-(const float other) const
	{
		if (SSE_ENABLED) return _mm_sub_ps(this->sseVec, _mm_set1_ps(other));
		return _SSE_Vec4_float(x - other, y - other, z - other, w - other);
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator*(const float other) const
	{
		if (SSE_ENABLED) return _mm_mul_ps(this->sseVec, _mm_set1_ps(other));
		return _SSE_Vec4_float(x * other, y * other, z * other, w * other);
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator/(const float other) const
	{
		if (SSE_ENABLED) return _mm_div_ps(this->sseVec, _mm_set1_ps(other));
		return _SSE_Vec4_float(x / other, y / other, z / other, w / other);
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator+=(const float other)
	{
		*this = *this + other;
		return *this;
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator-=(const float other)
	{
		*this = *this - other;
		return *this;
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator*=(const float other)
	{
		*this = *this * other;
		return *this;
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator/=(const float other)
	{
		*this = *this / other;
		return *this;
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator+(const _SSE_Vec4_float other) const
	{
		if (SSE_ENABLED) return _mm_add_ps(this->sseVec, other.sseVec);
		return _SSE_Vec4_float(x + other.x, y + other.y, z + other.z, w + other.w);
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator-(const _SSE_Vec4_float other) const
	{
		if (SSE_ENABLED) return _mm_sub_ps(this->sseVec, other.sseVec);
		return _SSE_Vec4_float(x - other.x, y - other.y, z - other.z, w - other.w);
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator*(const _SSE_Vec4_float other) const
	{
		if (SSE_ENABLED) return _mm_mul_ps(this->sseVec, other.sseVec);
		return _SSE_Vec4_float(x * other.x, y * other.y, z * other.z, w * other.w);
	}

	inline _SSE_Vec4_float _SSE_Vec4_float::operator/(const _SSE_Vec4_float other) const
	{
		if (SSE_ENABLED) return _mm_div_ps(this->sseVec, other.sseVec);
		return _SSE_Vec4_float(x / other.x, y / other.y, z / other.z, w / other.w);
	}

	inline const float& _SSE_Vec4_float::operator[](int i) const
	{
		return this->val[i];
	}

	inline float& _SSE_Vec4_float::operator[](int i)
	{
		return const_cast<float&>(static_cast<const _SSE_Vec4_float&>(*this).val[i]);
	}

	inline _SSE_Vec4_float _SSE_Vec4_float::operator+=(const _SSE_Vec4_float other)
	{
		*this = *this + other;
		return *this;
	}

	inline _SSE_Vec4_float _SSE_Vec4_float::operator-=(const _SSE_Vec4_float other)
	{
		*this = *this - other;
		return *this;
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator*=(const _SSE_Vec4_float other)
	{
		*this = *this * other;
		return *this;
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator/=(const _SSE_Vec4_float other)
	{
		*this = *this / other;
		return *this;
	}


	inline float _SSE_Vec4_float::product() const
	{
		return x * y * z * w;
	}


	inline float _SSE_Vec4_float::len() const
	{
		return sqrt(this->lenSq());
	}


	inline float _SSE_Vec4_float::lenSq() const
	{
		return x * x + y * y + z * z + w * w; //dot product with itself
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator-() const
	{
		return _SSE_Vec4_float(-x, -y, -z, -w);
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::unit() const
	{
		float len = this->len();
		return *this / len;
	}


	inline float _SSE_Vec4_float::dot(const _SSE_Vec4_float other) const
	{
		if (SSE_ENABLED)
		{
			__m128 r1 = _mm_mul_ps(sseVec, other.sseVec);
			__m128 shuf = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(2, 3, 0, 1));
			__m128 sums = _mm_add_ps(r1, shuf);

			shuf = _mm_movehl_ps(shuf, sums);
			sums = _mm_add_ss(sums, shuf);
			return _mm_cvtss_f32(sums);
		}
		return x * other.x + y * other.y + z * other.z + w * other.w;
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::cross3d(const _SSE_Vec4_float other) const
	{
		float cx = y * other.z - z * other.y;
		float cy = z * other.x - x * other.z;
		float cz = x * other.y - y * other.x;
		return _SSE_Vec4_float(cx, cy, cz, 0);
	}

	inline void _SSE_Vec4_float::assert_validX() const
	{
		assert(!isnan(x));
	}
	inline void _SSE_Vec4_float::assert_validY() const
	{
		assert(!isnan(y));
	}
	inline void _SSE_Vec4_float::assert_validZ() const
	{
		assert(!isnan(z));
	}	
	inline void _SSE_Vec4_float::assert_validW() const
	{
		assert(!isnan(w));
	}

}