#pragma once
#include <array>

#include <SDL/SDL.h>

#include "helpers.h"
#include "Vec.h"
#include "CoordinateTransformer.h"
#include "ZBuffer.h"
#include "Texture.h"

struct TexVertex
{
	Vec3 worldCoords;
	Vec2 textureCoords;

	bool operator<(const TexVertex& b) const
	{
		return worldCoords.y < b.worldCoords.y;
	}
};

inline TexVertex lerp(const TexVertex& t1, const TexVertex& t2, double amount)
{
	return { t1.worldCoords + (t2.worldCoords - t1.worldCoords) * amount, t1.textureCoords + (t2.textureCoords - t1.textureCoords) * amount };
}

struct Triangle
{
	std::array<TexVertex, 3> tv;
	int textureIndex;

	void drawOn(SDL_Surface* s, const CoordinateTransformer& ctr, ZBuffer& zBuffer, const std::vector<Texture>& textures) const;
private:
	//WARNING: this method expects tv to contain rotated (but not yet z-divided coords)!
	void drawInner(SDL_Surface* s, const CoordinateTransformer& ctr, ZBuffer& zBuffer, const std::vector<Texture>& textures) const;
};