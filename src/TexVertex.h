#pragma once
#include "Vec.h"

struct alignas(32) TexVertex
{
	union {
		struct {
			Vec4 spaceCoords; //this can mean different things inside different contexts, world or screen space
			Vec4 textureCoords; //this too, but it's either normal uv's, or z-divided ones
		};
		struct {
			real x, y, z, w;
			real u, v, _pad1, _pad2;
		};
		__m256 ymm;
	};

	TexVertex operator-(const TexVertex& other) const;
	TexVertex operator+(const TexVertex& other) const;
	TexVertex operator*(const float other) const;

	bool operator<(const TexVertex& b) const;

	TexVertex getClipedToPlane(const TexVertex& dst, real planeZ) const;
};

TexVertex lerp(const TexVertex& t1, const TexVertex& t2, real amount);