#pragma once
#include <initializer_list>
#include <cassert>
#include <iterator>
#include <bit>

#include <immintrin.h>
#include <smmintrin.h>
#include <cmath>

namespace bob
{
	struct alignas(16) _SSE_Vec4_float
	{
        float x,y,z,w;
	private:
#if _M_IX86_FP >= 2 || defined(_M_AMD64) || defined(_M_X64) || defined(__AVX__) //TODO: this is not a good enough way. It only detects SSE2 or AVX, we use SSE4.1 somewhere as well. Say "thanks" to Macro$haft, all other compilers just define __SSE4_1__
#define SSE_VER 42
		static constexpr bool SSE_ENABLED = true;
#else
		static constexpr bool SSE_ENABLED = false;
#endif
	public:
		_SSE_Vec4_float() = default;
		//_SSE_Vec4_float(const float x);
		//_SSE_Vec4_float(const float x, const float y);
		//_SSE_Vec4_float(const float x, const float y, const float z);
		_SSE_Vec4_float(const float x, const float y = 0, const float z = 0, const float w = 0);
		_SSE_Vec4_float(const __m128 v);
		//_SSE_Vec4_float(const std::initializer_list<float> list);

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

        /*
		void assert_validX() const;
		void assert_validY() const;
		void assert_validZ() const;
		void assert_validW() const;*/

		float& operator[](int i);
		const float& operator[](int i) const;
		operator __m128() const;

		float product() const;
		float len() const;
		float lenSq() const;

		_SSE_Vec4_float operator-() const;
		_SSE_Vec4_float unit() const;

		float dot(const _SSE_Vec4_float other) const;
		float cross2d(const _SSE_Vec4_float other) const;
		_SSE_Vec4_float cross3d(const _SSE_Vec4_float other) const;		
	};

	//inline _SSE_Vec4_float::_SSE_Vec4_float(const float x) : x(x), y(0), z(0), w(0) {}
	//inline _SSE_Vec4_float::_SSE_Vec4_float(const float x, const float y) : x(x), y(y), z(0), w(0) {}
	//inline _SSE_Vec4_float::_SSE_Vec4_float(const float x, const float y, const float z) : x(x), y(y), z(z), w(0) {}
	inline _SSE_Vec4_float::_SSE_Vec4_float(const float x, const float y, const float z, const float w)
	{
		*this = _mm_setr_ps(x, y, z, w);
	}

	inline _SSE_Vec4_float::_SSE_Vec4_float(const __m128 v)
    {
        *reinterpret_cast<__m128*>(this) = v;
    }

	/*
	inline _SSE_Vec4_float::_SSE_Vec4_float(const std::initializer_list<float> list)
	{
		x = list.size() >= 1 ? *(list.begin()) : 0;
		y = list.size() >= 2 ? *(list.begin() + 1) : 0;
		z = list.size() >= 3 ? *(list.begin() + 2) : 0;
		w = list.size() >= 4 ? *(list.begin() + 3) : 0;
	}*/

	inline _SSE_Vec4_float _SSE_Vec4_float::operator+(const float other) const
	{
		if (SSE_ENABLED) return _mm_add_ps(*this, _mm_set1_ps(other));
		return _SSE_Vec4_float(x + other, y + other, z + other, w + other);
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator-(const float other) const
	{
		if (SSE_ENABLED) return _mm_sub_ps(*this, _mm_set1_ps(other));
		return _SSE_Vec4_float(x - other, y - other, z - other, w - other);
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator*(const float other) const
	{
		if (SSE_ENABLED) return _mm_mul_ps(*this, _mm_set1_ps(other));
		return _SSE_Vec4_float(x * other, y * other, z * other, w * other);
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator/(const float other) const
	{
		if (SSE_ENABLED) return _mm_div_ps(*this, _mm_set1_ps(other));
		float reciprocal = 1.0 / other;
		return *this * reciprocal;
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
		if (SSE_ENABLED) return _mm_add_ps(*this, other);
		return _SSE_Vec4_float(x + other.x, y + other.y, z + other.z, w + other.w);
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator-(const _SSE_Vec4_float other) const
	{
		if (SSE_ENABLED) return _mm_sub_ps(*this, other);
		return _SSE_Vec4_float(x - other.x, y - other.y, z - other.z, w - other.w);
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator*(const _SSE_Vec4_float other) const
	{
		if (SSE_ENABLED) return _mm_mul_ps(*this, other);
		return _SSE_Vec4_float(x * other.x, y * other.y, z * other.z, w * other.w);
	}

	inline _SSE_Vec4_float _SSE_Vec4_float::operator/(const _SSE_Vec4_float other) const
	{
		if (SSE_ENABLED) return _mm_div_ps(*this, other);
		return _SSE_Vec4_float(x / other.x, y / other.y, z / other.z, w / other.w);
	}

	inline const float& _SSE_Vec4_float::operator[](int i) const
	{
        assert(i >= 0 && i <= 4);
		return reinterpret_cast<const float*>(this)[i];
	}

	inline _SSE_Vec4_float::operator __m128() const
    {
		return std::bit_cast<__m128>(*this);
	}

	inline float& _SSE_Vec4_float::operator[](int i)
	{
		return const_cast<float&>(static_cast<const _SSE_Vec4_float&>(*this)[i]);
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
		return dot(*this);
	}


	inline _SSE_Vec4_float _SSE_Vec4_float::operator-() const
	{
		if (SSE_ENABLED) return _mm_sub_ps(_mm_setzero_ps(), *this);
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
			__m128 r1 = _mm_mul_ps(*this, other);
			__m128 shuf = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(2, 3, 0, 1));
			__m128 sums = _mm_add_ps(r1, shuf);

			shuf = _mm_movehl_ps(shuf, sums);
			sums = _mm_add_ss(sums, shuf);
			return _mm_cvtss_f32(sums);
		}
		return x * other.x + y * other.y + z * other.z + w * other.w;
	}

	inline float _SSE_Vec4_float::cross2d(const _SSE_Vec4_float other) const
	{
#if SSE_VER >= 10
		_SSE_Vec4_float os = _mm_shuffle_ps(other, other, _MM_SHUFFLE(3, 2, 0, 1));
		_SSE_Vec4_float prefab = *this * os;
		return prefab.x - prefab.y;
#endif
		return x * other.y - y * other.x;
	}

	inline _SSE_Vec4_float _SSE_Vec4_float::cross3d(const _SSE_Vec4_float other) const
	{
#if __AVX2__ && 0 //seems like SSE version is even with this one
		__m256 p = _mm256_permutevar8x32_ps(_mm256_castps128_ps256(*this), _mm256_setr_epi32(1, 2, 0, 3, 2, 0, 1, 3));
		__m256 q = _mm256_permutevar8x32_ps(_mm256_castps128_ps256(other), _mm256_setr_epi32(2, 0, 1, 3, 1, 2, 0, 3));
		__m256 pq = _mm256_mul_ps(p, q);
		return _mm_sub_ps(_mm256_extractf128_ps(pq, 0), _mm256_extractf128_ps(pq, 1));
#elif SSE_VER >= 10
		_SSE_Vec4_float p1 = _mm_shuffle_ps(*this, *this, _MM_SHUFFLE(3, 0, 2, 1));
		_SSE_Vec4_float q1 = _mm_shuffle_ps(other, other, _MM_SHUFFLE(3, 1, 0, 2));
		_SSE_Vec4_float p2 = _mm_shuffle_ps(*this, *this, _MM_SHUFFLE(3, 1, 0, 2));
		_SSE_Vec4_float q2 = _mm_shuffle_ps(other, other, _MM_SHUFFLE(3, 0, 2, 1));
		return p1 * q1 - p2 * q2;
#else
		float cx = y * other.z - z * other.y;
		float cy = z * other.x - x * other.z;
		float cz = x * other.y - y * other.x;
		return _SSE_Vec4_float(cx, cy, cz, 0);
#endif
	}

    /*
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
	}*/

}