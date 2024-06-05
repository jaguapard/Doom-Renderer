#include "TexVertex.h"

#include "helpers.h"

TexVertex TexVertex::getClipedToPlane(const TexVertex& dst, real planeZ) const
{
	real alpha = inverse_lerp(spaceCoords.z, dst.spaceCoords.z, planeZ);
	assert(alpha >= 0 && alpha <= 1);
	return lerp(*this, dst, alpha);
}

TexVertex TexVertex::operator-(const TexVertex& other) const
{
	return { .ymm = _mm256_sub_ps(ymm, other.ymm) };
}

TexVertex TexVertex::operator+(const TexVertex& other) const
{
	return  { .ymm = _mm256_add_ps(ymm, other.ymm) };
}

TexVertex TexVertex::operator*(const float m) const
{
	return { .ymm = _mm256_mul_ps(ymm, _mm256_broadcast_ss(&m)) };
}

bool TexVertex::operator<(const TexVertex& b) const
{
	return spaceCoords.y < b.spaceCoords.y;
}

TexVertex lerp(const TexVertex& t1, const TexVertex& t2, real amount)
{
#ifdef __AVX__
	__m256 t = _mm256_broadcast_ss(&amount);
	__m256 diff = _mm256_sub_ps(t2.ymm, t1.ymm); //result = tv1 + diff*amount
	__m256 res = _mm256_fmadd_ps(diff, t, t1.ymm); //a*b+c
	return { .ymm = res };
#endif
	return { lerp(t1.spaceCoords, t2.spaceCoords, amount), lerp(t1.textureCoords, t2.textureCoords,amount) };
}
