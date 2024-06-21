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
    return _mm256_sub_ps(*this, other);
}

TexVertex TexVertex::operator+(const TexVertex& other) const
{
    return _mm256_add_ps(*this,other);
}

TexVertex TexVertex::operator*(const float m) const
{
    return _mm256_mul_ps(*this, _mm256_set1_ps(m));
}

bool TexVertex::operator<(const TexVertex& b) const
{
	return spaceCoords.y < b.spaceCoords.y;
}

TexVertex::operator __m256() const
{
    return _mm256_loadu_ps((float*)this);
}

TexVertex::TexVertex(__m256 v)
{
    _mm256_storeu_ps((float*)this, v);
    //*reinterpret_cast<__m256*>(this) = v;
}

TexVertex::TexVertex(const Vec4 space, const Vec4 texture)
{
    spaceCoords=space;
    textureCoords=texture;
}

TexVertex lerp(const TexVertex& t1, const TexVertex& t2, real amount)
{
#ifdef __AVX__
	__m256 t = _mm256_broadcast_ss(&amount);
	__m256 diff = _mm256_sub_ps(t2, t1); //result = tv1 + diff*amount
	return _mm256_fmadd_ps(diff, t, t1); //a*b+c
#endif
	return { lerp(t1.spaceCoords, t2.spaceCoords, amount), lerp(t1.textureCoords, t2.textureCoords,amount) };
}
