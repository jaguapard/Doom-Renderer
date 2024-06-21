#include "Matrix4.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <cstring>

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

Vec4 Matrix4::operator*(const Vec4 v) const
{
#if __AVX512F__
	__m512 broadcasted_v = _mm512_broadcast_f32x4(v);
	__m512 preSum = _mm512_mul_ps(broadcasted_v, zmm); //sum elements 0-3 to get result x, 4-7 for y, 8-11 for z, 12-15 for w
	__m512 parts = _mm512_permutexvar_ps(_mm512_setr_epi32(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15), preSum); //add up each __m128 part to get final answer

	return Vec4(_mm512_extractf32x4_ps(parts, 0)) + Vec4(_mm512_extractf32x4_ps(parts, 1)) + Vec4(_mm512_extractf32x4_ps(parts, 2)) + Vec4(_mm512_extractf32x4_ps(parts, 3));
#elif __AVX2__
	__m256 vv = _mm256_broadcast_ps(&v.xmm);
	__m256 preSum_xy = _mm256_mul_ps(vv, ymm0); //sum elements 0-3 to get result x, 4-7 for y
	__m256 preSum_zw = _mm256_mul_ps(vv, ymm1); //sum elements 0-3 to get result z, 4-7 for w

	__m256 preSum_xyzw = _mm256_hadd_ps(preSum_xy, preSum_zw); //to get final: x = 0+1, z = 2+3, y = 4+5, w = 6+7
	//permute values so x=0+4; y=1+5; z=2+6; w=3+7. This allows us to extract the upper half and just add it to the lower to get the result
	__m256 perm = _mm256_permutevar8x32_ps(preSum_xyzw, _mm256_setr_epi32(0, 4, 2, 6, 1, 5, 3, 7));
	return Vec4(_mm256_extractf128_ps(perm, 0)) + Vec4(_mm256_extractf128_ps(perm, 1));
#elif defined(__SSE2__)
	Vec4 r1 = val[0] * v;
	Vec4 r2 = val[1] * v;
	Vec4 r3 = val[2] * v;
	Vec4 r4 = val[3] * v;

	__m128 res1 = _mm_hadd_ps(r1, r2); //x = 0+1, y = 2+3
	__m128 res2 = _mm_hadd_ps(r3, r4); //z = 0+1, w = 2+3

	return _mm_hadd_ps(res1, res2); 
#else
	Vec4 ret(0, 0, 0, 0);
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

Vec4 Matrix4::multiplyByTransposed(const Vec4 v) const
{
	__m128 x = _mm_set1_ps(v.x);
	__m128 y = _mm_set1_ps(v.y);
	__m128 z = _mm_set1_ps(v.z);
	__m128 w = _mm_set1_ps(v.w);

	__m128 p1 = _mm_mul_ps(x, xmm0);
	__m128 p2 = _mm_mul_ps(y, xmm1);
	__m128 p3 = _mm_mul_ps(z, xmm2);
	__m128 p4 = _mm_mul_ps(w, xmm3);

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

Matrix4 Matrix4::rotationXYZ(const Vec4& angle)
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
