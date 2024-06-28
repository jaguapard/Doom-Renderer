#pragma once
#include "Vec.h"

struct TexVertex
{
    TexVertex() = default;
    TexVertex(const Vec4 space, const Vec4 texture);
    TexVertex(const Vec4 space, const Vec4 texture, const Vec4 world);

    Vec4 spaceCoords; //this can mean different things inside different contexts, world or screen space
    Vec4 textureCoords; //this too, but it's either normal uv's, or z-divided ones
    Vec4 worldCoords; //world coordinates for this vertex

    
    //TexVertex(__m256 v);
    //operator __m256() const;

	TexVertex operator-(const TexVertex& other) const;
	TexVertex operator+(const TexVertex& other) const;
	TexVertex operator*(const float other) const;

	bool operator<(const TexVertex& b) const;

	TexVertex getClipedToPlane(const TexVertex& dst, real planeZ) const;
};

TexVertex lerp(const TexVertex& t1, const TexVertex& t2, real amount);