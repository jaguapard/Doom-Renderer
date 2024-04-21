#pragma once
#include <array>

#include <SDL/SDL.h>

#include "helpers.h"
#include "Vec.h"
#include "CoordinateTransformer.h"
#include "ZBuffer.h"
#include "Texture.h"
#include "TextureManager.h"

#include <immintrin.h>

struct alignas(32) TexVertex
{
	Vec3 spaceCoords; //this can mean different things inside different contexts, world or screen space
	Vec3 textureCoords; //this too, but it's either normal uv's, or z-divided ones

	bool operator<(const TexVertex& b) const
	{
		return spaceCoords.y < b.spaceCoords.y;
	}

	TexVertex getClipedToPlane(const TexVertex& dst) const;
};

inline TexVertex lerp(const TexVertex& t1, const TexVertex& t2, real amount)
{
#ifdef __AVX__
	__m256 tv1 = _mm256_load_ps(reinterpret_cast<const float*>(&t1));
	__m256 tv2 = _mm256_load_ps(reinterpret_cast<const float*>(&t2));
	__m256 t = _mm256_set1_ps(amount);
	__m256 diff = _mm256_sub_ps(tv2, tv1); //result = tv1 + diff*amount
	__m256 res = _mm256_fmadd_ps(diff, t, tv1); //a*b+c
	return *reinterpret_cast<TexVertex*>(&res);
#endif
	return { lerp(t1.spaceCoords, t2.spaceCoords, amount), lerp(t1.textureCoords, t2.textureCoords,amount) };
}

struct TriangleRenderContext
{
	PixelBuffer<Color>* frameBuffer;
	PixelBuffer<real>* lightBuffer;
	ZBuffer* zBuffer;
	const CoordinateTransformer* ctr;	
	const TextureManager* textureManager;
	const Texture* texture;
	real lightMult;
	real framebufW, framebufH;

	int doomSkyTextureMarkerIndex;
	bool wireframeEnabled = false;

	real nearPlaneClippingZ = -1;
};

struct Triangle
{
	std::array<TexVertex, 3> tv;

	void sortByAscendingSpaceX();
	void sortByAscendingSpaceY();
	void sortByAscendingSpaceZ();
	void sortByAscendingTextureX();
	void sortByAscendingTextureY();

	void drawOn(const TriangleRenderContext& context) const;
private:
	void drawRotationPrepped(const TriangleRenderContext& context) const; //WARNING: this method expects tv to contain rotated (but not yet z-divided coords)!
	void drawScreenSpaceAndUvDividedPrepped(const TriangleRenderContext& context, bool flatBottom) const; //This method expects tv to contain screen space coords in tv.spaceCoords with z holding 1/world z and z divided texture coords in tv.textureCoords
};