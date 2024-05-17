#include "Matrix4.h"
#include <cmath>
#include <sstream>
#include <iomanip>

Matrix4::Matrix4(const std::initializer_list<bob::_SSE_Vec4_float> lst)
{
	assert(lst.size() == 4);
	for (int i = 0; i < 4; ++i) this->val[i] = *(lst.begin() + i);
}

Matrix4 Matrix4::operator*(const Matrix4& other) const
{
	Matrix4 ret = Matrix4::zeros();
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			for (int k = 0; k < 4; ++k)
			{
				ret.elements[i][j] += this->elements[i][k] * other.elements[k][j];
			}
		}
	}
	return ret;
}

Vec3 Matrix4::operator*(const Vec3 v) const
{
#ifdef __AVX2__
	__m256 vv = _mm256_broadcast_ps(&v.sseVec);
	__m256 preSum_xy = _mm256_mul_ps(vv, *reinterpret_cast<const __m256*>(&val[0])); //sum elements 0-3 to get result x, 4-7 for y
	__m256 preSum_zw = _mm256_mul_ps(vv, *reinterpret_cast<const __m256*>(&val[2])); //sum elements 0-3 to get result z, 4-7 for w

	__m256 preSum_xyzw = _mm256_hadd_ps(preSum_xy, preSum_zw); //after this we need to sum pairs of elements to get final result
	__m256 shuffled_xyzw = _mm256_hadd_ps(preSum_xyzw, preSum_xyzw); //after this we have result. xy are in elements 0, 4 and zw is in 1, 5. Other elements are just duplicated copies of them
	__m256 ret = _mm256_permutevar8x32_ps(shuffled_xyzw, _mm256_set_epi32(7, 7, 7, 7, 5, 1, 4, 0)); //this permute moves the elements into proper order
	return _mm256_castps256_ps128(ret);
#elif 0 //too much stuff, scalar may be faster
	//mn = v3 * elements[n]. Ret should be: (add up everything in m1, add up everything in m2 ...)
	__m128 zeros = _mm_setzero_ps();
	__m128 m1 = _mm_mul_ps(v.sseVec, *reinterpret_cast<const __m128*>(&val[0]));
	m1 = _mm_hadd_ps(m1, zeros);
	m1 = _mm_hadd_ps(m1, zeros);

	__m128 m2 = _mm_mul_ps(v.sseVec, *reinterpret_cast<const __m128*>(&val[1]));
	m2 = _mm_hadd_ps(m2, zeros);
	m2 = _mm_hadd_ps(m2, zeros);

	__m128 m3 = _mm_mul_ps(v.sseVec, *reinterpret_cast<const __m128*>(&val[2]));
	m3 = _mm_hadd_ps(m3, zeros);
	m3 = _mm_hadd_ps(m3, zeros);

	__m128 m4 = _mm_mul_ps(v.sseVec, *reinterpret_cast<const __m128*>(&val[3]));
	m4 = _mm_hadd_ps(m4, zeros);
	m4 = _mm_hadd_ps(m4, zeros);

	__m128i ret2 = _mm_bslli_si128(_mm_castps_si128(m2), 4);
	__m128i ret3 = _mm_bslli_si128(_mm_castps_si128(m3), 8);
	__m128i ret4 = _mm_bslli_si128(_mm_castps_si128(m4), 12);

	__m128i ret12 = _mm_or_si128(_mm_castps_si128(m1), ret2);
	__m128i ret34 = _mm_or_si128(ret3, ret4);

	return _mm_castsi128_ps(_mm_or_si128(ret12, ret34));
#else
	Vec3 ret(0, 0, 0, 0);
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			ret.val[i] += (*this)[i][j] * v[j];
		}
	}
	return ret;
#endif
}

Matrix4 Matrix4::transposed() const
{
	Matrix4 ret;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			ret.elements[i][j] = this->elements[j][i];
		}
	}
	return ret;
}

Vec3 Matrix4::multiplyByTransposed(const Vec3 v) const
{
	__m128 x = _mm_set1_ps(v.x);
	__m128 y = _mm_set1_ps(v.y);
	__m128 z = _mm_set1_ps(v.z);
	__m128 w = _mm_set1_ps(v.w);

	__m128 p1 = _mm_mul_ps(x, this->val[0].sseVec);
	__m128 p2 = _mm_mul_ps(y, this->val[1].sseVec);
	__m128 p3 = _mm_mul_ps(z, this->val[2].sseVec);
	__m128 p4 = _mm_mul_ps(w, this->val[3].sseVec);

	return _mm_add_ps(_mm_add_ps(p1, p2), _mm_add_ps(p3, p4));
}

const bob::_SSE_Vec4_float& Matrix4::operator[](int i) const
{
	return val[i];
}

bob::_SSE_Vec4_float& Matrix4::operator[](int i)
{
	return const_cast<bob::_SSE_Vec4_float&>(const_cast<const Matrix4*>(this)->operator[](i));
}

std::string Matrix4::toString(int precision) const
{
	std::stringstream ss;
	ss.precision(precision);

	std::string s[16];
	int sInd = 0;
	int maxLen = -1;

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			ss << elements[i][j];
			s[sInd++] = ss.str();
			ss.str("");
			maxLen = std::max<int>(s[sInd - 1].length(), maxLen);
		}
	}

	std::string ret;
	for (int i = 0; i < 16; ++i)
	{
		ret += std::string(7+precision - s[i].size(), ' ') + s[i];
		if (i % 4 == 3) ret += '\n';
	}
	return ret;
}

Matrix4 Matrix4::rotationX(float theta)
{
	const real sinTheta = sin(theta);
	const real cosTheta = cos(theta);
	return {
		bob::_SSE_Vec4_float(cosTheta,	sinTheta, 0.0, 0.0),
		bob::_SSE_Vec4_float(-sinTheta, cosTheta, 0.0, 0.0),
		bob::_SSE_Vec4_float(0.0,		0.0,	  1.0, 0.0),
		bob::_SSE_Vec4_float(0.0,		0.0,	  0.0, 1.0),
	};
}

Matrix4 Matrix4::rotationY(float theta)
{
	const real sinTheta = sin(theta);
	const real cosTheta = cos(theta);
	return {
		bob::_SSE_Vec4_float(cosTheta,  0.0,	-sinTheta,	0.0),
		bob::_SSE_Vec4_float(0.0,		1.0,	0.0,		0.0),
		bob::_SSE_Vec4_float(sinTheta,  0.0,	cosTheta,	0.0),
		bob::_SSE_Vec4_float(0.0,		0.0,	0.0,		1.0),
	};
}

Matrix4 Matrix4::rotationZ(float theta)
{
	const real sinTheta = sin(theta);
	const real cosTheta = cos(theta);
	return {
		bob::_SSE_Vec4_float(1.0,	0.0,		0.0,		0.0),
		bob::_SSE_Vec4_float(0.0,	cosTheta,	sinTheta,	0.0),
		bob::_SSE_Vec4_float(0.0,	-sinTheta,	cosTheta,	0.0),
		bob::_SSE_Vec4_float(0.0,	0.0,		0.0,		1.0),
	};
}

Matrix4 Matrix4::rotationXYZ(const Vec3& angle)
{
	return rotationZ(angle.z) * rotationY(angle.y) * rotationX(angle.x);
	//return rotationX(angle.x) * rotationY(angle.y) * rotationZ(angle.z);
	//return (rotationZ(angle.z) * rotationY(angle.y) * rotationX(angle.x)).transposed();
}

Matrix4 Matrix4::identity(float value, int dim)
{
	assert(dim > 0 && dim <= 4);
	Matrix4 ret = Matrix4::zeros();
	for (int i = 0; i < dim; ++i) ret.elements[i][i] = value;
	return ret;
}

Matrix4 Matrix4::zeros()
{
	Matrix4 ret;
	memset(&ret, 0, sizeof(ret));
	return ret;
}
