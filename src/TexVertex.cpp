#include "TexVertex.h"

#include "helpers.h"

TexVertex TexVertex::getClipedToPlane(const TexVertex& dst, real planeZ) const
{
	real alpha = inverse_lerp(spaceCoords.z, dst.spaceCoords.z, planeZ);
	assert(alpha >= 0 && alpha <= 1);
	return lerp(*this, dst, alpha);
}

TexVertex::TexVertex(const Vec4 space, const Vec4 texture)
{
	spaceCoords = space;
	textureCoords = texture;
	worldCoords = space;
}

TexVertex::TexVertex(const Vec4 space, const Vec4 texture, const Vec4 world)
{
	spaceCoords = space;
	textureCoords = texture;
	worldCoords = world;
}

TexVertex TexVertex::operator-(const TexVertex& other) const
{
	TexVertex ret = *this;
	ret.spaceCoords -= other.spaceCoords;
	ret.textureCoords -= other.textureCoords;
	ret.worldCoords -= other.worldCoords;
	return ret;
}

TexVertex TexVertex::operator+(const TexVertex& other) const
{
	TexVertex ret = *this;
	ret.spaceCoords += other.spaceCoords;
	ret.textureCoords += other.textureCoords;
	ret.worldCoords += other.worldCoords;
	return ret;
}

TexVertex TexVertex::operator*(const float m) const
{
	TexVertex ret = *this;
	ret.spaceCoords *= m;
	ret.textureCoords *= m;
	ret.worldCoords *= m;
	return ret;
}

bool TexVertex::operator<(const TexVertex& b) const
{
	return spaceCoords.y < b.spaceCoords.y;
}

TexVertex lerp(const TexVertex& t1, const TexVertex& t2, real amount)
{
#ifdef __AVX__
	/*
	__m256 t = _mm256_broadcast_ss(&amount);
	__m256 diff = _mm256_sub_ps(t2, t1); //result = tv1 + diff*amount
	return _mm256_fmadd_ps(diff, t, t1); //a*b+c
	*/
#endif
	TexVertex ret;
	ret.spaceCoords = lerp(t1.spaceCoords, t2.spaceCoords, amount);
	ret.textureCoords = lerp(t1.textureCoords, t2.textureCoords, amount);
	ret.worldCoords = lerp(t1.worldCoords, t2.worldCoords, amount);
	ret.sunScreenPos = lerp(t1.sunScreenPos, t2.sunScreenPos, amount);
	return ret;
}
