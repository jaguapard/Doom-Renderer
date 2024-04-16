#pragma once
#include <array>

#include <SDL/SDL.h>

#include "helpers.h"
#include "Vec.h"
#include "CoordinateTransformer.h"
#include "ZBuffer.h"
#include "Texture.h"
#include "TextureManager.h"

struct TexVertex
{
	Vec3 spaceCoords; //this can mean different things inside different contexts, world or screen space
	Vec3 textureCoords; //this too, but it's either normal uv's, or z-divided ones

	bool operator<(const TexVertex& b) const
	{
		return spaceCoords.y < b.spaceCoords.y;
	}

	TexVertex getClipedToPlane(const TexVertex& dst) const;
};

inline TexVertex lerp(const TexVertex& t1, const TexVertex& t2, double amount)
{
	return { t1.spaceCoords + (t2.spaceCoords - t1.spaceCoords) * amount, t1.textureCoords + (t2.textureCoords - t1.textureCoords) * amount };
}

struct TriangleRenderContext
{
	PixelBuffer<Color>* frameBuffer;
	ZBuffer* zBuffer;
	const CoordinateTransformer* ctr;	
	const TextureManager* textureManager;
	double lightMult;
	double framebufW, framebufH;
};

struct Triangle
{
	std::array<TexVertex, 3> tv;
	int textureIndex;

	void drawOn(const TriangleRenderContext& context) const;
private:
	void drawRotationPrepped(const TriangleRenderContext& context) const; //WARNING: this method expects tv to contain rotated (but not yet z-divided coords)!
	void drawScreenSpaceAndUvDividedPrepped(const TriangleRenderContext& context, bool flatBottom) const; //This method expects tv to contain screen space coords in tv.spaceCoords with z holding 1/world z and z divided texture coords in tv.textureCoords
};