#pragma once
#include <array>

#include <SDL/SDL.h>

#include "helpers.h"
#include "Vec.h"
#include "CoordinateTransformer.h"
#include "ZBuffer.h"
#include "Texture.h"
#include "TextureManager.h"
#include "Triangle.h"

#include <immintrin.h>

struct alignas(32) TexVertex
{
	union {
		struct {
			Vec3 spaceCoords; //this can mean different things inside different contexts, world or screen space
			Vec3 textureCoords; //this too, but it's either normal uv's, or z-divided ones
		};
		struct {
			real x, y, z, w;
			real u, v, _pad1, _pad2;
		};
		__m256 ymm;
	};

	bool operator<(const TexVertex& b) const
	{
		return spaceCoords.y < b.spaceCoords.y;
	}

	TexVertex getClipedToPlane(const TexVertex& dst, real planeZ) const;
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

struct TriangleRenderContext;
struct RenderJob;

struct alignas(32) Triangle
{
	union {
		std::array<TexVertex, 3> tv;
		struct {
			real x1, y1, z1, w1, u1, v1, _pad01, _pad02;
			real x2, y2, z2, w2, u2, v2, _pad11, _pad12;
			real x3, y3, z3, w3, u3, v3, _pad21, _pad22;
		};
		struct {
			__m256 ymm1, ymm2, ymm3;
		};
	};

	void sortByAscendingSpaceX();
	void sortByAscendingSpaceY();
	void sortByAscendingSpaceZ();
	void sortByAscendingTextureX();
	void sortByAscendingTextureY();

	void addToRenderQueue(const TriangleRenderContext& context) const;
	static std::pair<Triangle, Triangle> pairFromRect(std::array<TexVertex, 4> rectPoints);
	void drawSlice(const TriangleRenderContext& context, const RenderJob& renderJob, int zoneMinY, int zoneMaxY) const;

	Vec3 getNormalVector() const;
private:
	void prepareScreenSpace(const TriangleRenderContext& context) const; //WARNING: this method expects tv to contain rotated (but not yet z-divided coords)!
	void addToRenderQueueFinal(const TriangleRenderContext& context, bool flatBottom) const; //This method expects tv to contain screen space coords in tv.spaceCoords with z holding 1/world z and z divided texture coords in tv.textureCoords
};

struct RenderJob
{
	Triangle t;
	bool flatBottom;
	int textureIndex;
	real lightMult;
};

struct TriangleRenderContext
{
	PixelBuffer<Color>* frameBuffer;
	PixelBuffer<real>* lightBuffer;
	ZBuffer* zBuffer;
	const CoordinateTransformer* ctr;	
	const TextureManager* textureManager;
	const Texture* texture;
	int textureIndex;
	real lightMult;
	real framebufW, framebufH;

	int doomSkyTextureMarkerIndex;
	bool wireframeEnabled;
	bool backfaceCullingEnabled;

	std::vector<RenderJob>* renderJobs;

	real nearPlaneClippingZ = -1;
	real fovMult = 1;
};