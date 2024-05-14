#include "Matrix3.h"
#include <cmath>
Matrix3::Matrix3()
{
	for (unsigned char x = 0; x < 4; ++x)
	{
		for (unsigned char y = 0; y < 4; ++y)
		{
			this->elements[x][y] = 0.0;
		}
	}
}

Matrix3::Matrix3(real e11, real e12, real e13, real e21, real e22, real e23, real e31, real e32, real e33)
{
	this->elements[0][0] = e11;
	this->elements[0][1] = e12;
	this->elements[0][2] = e13;
	this->elements[1][0] = e21;
	this->elements[1][1] = e22;
	this->elements[1][2] = e23;
	this->elements[2][0] = e31;
	this->elements[2][1] = e32;
	this->elements[2][2] = e33;
}

Matrix3 Matrix3::operator*(const Matrix3& other) const
{
	Matrix3 ret;
	ret.elements[0][0] = this->elements[0][0] * other.elements[0][0] + this->elements[0][1] * other.elements[1][0] + this->elements[0][2] * other.elements[2][0];
	ret.elements[1][0] = this->elements[1][0] * other.elements[0][0] + this->elements[1][1] * other.elements[1][0] + this->elements[1][2] * other.elements[2][0];
	ret.elements[2][0] = this->elements[2][0] * other.elements[0][0] + this->elements[2][1] * other.elements[1][0] + this->elements[2][2] * other.elements[2][0];
	ret.elements[0][1] = this->elements[0][0] * other.elements[0][1] + this->elements[0][1] * other.elements[1][1] + this->elements[0][2] * other.elements[2][1];
	ret.elements[1][1] = this->elements[1][0] * other.elements[0][1] + this->elements[1][1] * other.elements[1][1] + this->elements[1][2] * other.elements[2][1];
	ret.elements[2][1] = this->elements[2][0] * other.elements[0][1] + this->elements[2][1] * other.elements[1][1] + this->elements[2][2] * other.elements[2][1];
	ret.elements[0][2] = this->elements[0][0] * other.elements[0][2] + this->elements[0][1] * other.elements[1][2] + this->elements[0][2] * other.elements[2][2];
	ret.elements[1][2] = this->elements[1][0] * other.elements[0][2] + this->elements[1][1] * other.elements[1][2] + this->elements[1][2] * other.elements[2][2];
	ret.elements[2][2] = this->elements[2][0] * other.elements[0][2] + this->elements[2][1] * other.elements[1][2] + this->elements[2][2] * other.elements[2][2];
	return ret;
}

Vec3 Matrix3::operator*(const Vec3& v3) const
{
	return{
		v3.x * this->elements[0][0] + v3.y * this->elements[1][0] + v3.z * this->elements[2][0],
		v3.x * this->elements[0][1] + v3.y * this->elements[1][1] + v3.z * this->elements[2][1],
		v3.x * this->elements[0][2] + v3.y * this->elements[1][2] + v3.z * this->elements[2][2]
	};
}

Matrix3 Matrix3::transposed() const
{
	Matrix3 ret;
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			ret.elements[i][j] = this->elements[j][i];
		}
	}
	return ret;
}

Vec3 Matrix3::multiplyByTransposed(const Vec3& v3) const
{
#ifdef __AVX2__
	__m256 zeros = _mm256_setzero_ps();
	__m256 vv = _mm256_broadcast_ps(&v3.sseVec);
	__m256 m12 = _mm256_mul_ps(vv, *reinterpret_cast<const __m256*>(&elements[0]));
	__m256 m34 = _mm256_mul_ps(vv, *reinterpret_cast<const __m256*>(&elements[2]));

	m12 = _mm256_hadd_ps(m12, zeros);
	m12 = _mm256_hadd_ps(m12, zeros); //2 horizontal adds in sequence crush everything into bits [0..31] and [128..159]. Only they have nonzero values
	m34 = _mm256_hadd_ps(m34, zeros);
	m34 = _mm256_hadd_ps(m34, zeros); //now lower 32 bits of each 128 lane have our final vector values

	__m256i permMask1 = _mm256_set_epi32(7, 7, 7, 7, 7, 7, 4, 0); //permute 2 ret values into bits [0..63]
	__m256i permMask2 = _mm256_set_epi32(7, 7, 7, 7, 4, 0, 7, 7); //permute 2 ret values into bits [64..127]
	__m256 perm_m12 = _mm256_permutevar8x32_ps(m12, permMask1);
	__m256 perm_m34 = _mm256_permutevar8x32_ps(m34, permMask2);
	__m256 ret = _mm256_blend_ps(perm_m12, perm_m34, 0b1100); //blend the final result into one 128 bit vector
	return _mm256_castps256_ps128(ret);
#elif 0
	//mn = v3 * elements[n]. Ret should be: (add up everything in m1, add up everything in m2 ...)
	__m128 zeros = _mm_setzero_ps();
	__m128 m1 = _mm_mul_ps(v3.sseVec, *reinterpret_cast<const __m128*>(&elements[0]));
	m1 = _mm_hadd_ps(m1, zeros);
	m1 = _mm_hadd_ps(m1, zeros);

	__m128 m2 = _mm_mul_ps(v3.sseVec, *reinterpret_cast<const __m128*>(&elements[1]));
	m2 = _mm_hadd_ps(m2, zeros);
	m2 = _mm_hadd_ps(m2, zeros);

	__m128 m3 = _mm_mul_ps(v3.sseVec, *reinterpret_cast<const __m128*>(&elements[2]));
	m3 = _mm_hadd_ps(m3, zeros);
	m3 = _mm_hadd_ps(m3, zeros);

	__m128 m4 = _mm_mul_ps(v3.sseVec, *reinterpret_cast<const __m128*>(&elements[3]));
	m4 = _mm_hadd_ps(m4, zeros);
	m4 = _mm_hadd_ps(m4, zeros);

	__m128i ret2 = _mm_bslli_si128(_mm_castps_si128(m2), 4);
	__m128i ret3 = _mm_bslli_si128(_mm_castps_si128(m3), 8);
	__m128i ret4 = _mm_bslli_si128(_mm_castps_si128(m4), 12);
	
	__m128i ret12 = _mm_or_si128(_mm_castps_si128(m1), ret2);
	__m128i ret34 = _mm_or_si128(ret3, ret4);
	
	return _mm_castsi128_ps(_mm_or_si128(ret12, ret34));
#else
	return {
		v3.x * elements[0][0] + v3.y * elements[0][1] + v3.z * elements[0][2],
		v3.x * elements[1][0] + v3.y * elements[1][1] + v3.z * elements[1][2],
		v3.x * elements[2][0] + v3.y * elements[2][1] + v3.z * elements[2][2],
	};
#endif
}

Matrix3 rotateX(real theta)
{
	const real sinTheta = sin(theta);
	const real cosTheta = cos(theta);
	Matrix3 ret;	
	real e[4][4] = {
		{cosTheta, sinTheta, 0.0, 0.0},
		{-sinTheta, cosTheta, 0.0, 0.0},
		{0.0,    0.0,   1.0, 0.0},
		{0.0,0.0,0.0,0.0},
	};
	memcpy(&ret.elements[0][0], &e[0][0], sizeof(e));
	return ret;
}

Matrix3 rotateY(real theta)
{
	const real sinTheta = sin(theta);
	const real cosTheta = cos(theta);
	Matrix3 ret;
	real e[4][4] = {
		{cosTheta, 0.0,-sinTheta, 0.0},
		{0.0,   1.0, 0.0, 0.0 },
		{ sinTheta, 0.0, cosTheta, 0.0 },
		{0.0, 0.0, 0.0, 0.0},
	};
	memcpy(&ret.elements[0][0], &e[0][0], sizeof(e));
	return ret;
}

Matrix3 rotateZ(real theta)
{
	const real sinTheta = sin(theta);
	const real cosTheta = cos(theta);

	Matrix3 ret;
	real e[4][4] = {
		{1.0, 0.0,   0.0, 0.0},
		{0.0, cosTheta, sinTheta, 0.0},
		{0.0,-sinTheta, cosTheta, 0.0 },
		{0.0, 0.0, 0.0, 0.0},
	};
	memcpy(&ret.elements[0][0], &e[0][0], sizeof(e));
	return ret;
}


Matrix3 getRotationMatrix(const Vec3& angle)
{
	return rotateX(angle.x) * rotateY(angle.y) * rotateZ(angle.z);
}
