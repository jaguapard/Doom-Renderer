#pragma once
#include <array>
#include <immintrin.h>

#include <SDL/SDL.h>

#include "helpers.h"
#include "Vec.h"
#include "CoordinateTransformer.h"
#include "ZBuffer.h"
#include "Texture.h"
#include "TextureManager.h"
#include "TexVertex.h"

struct TriangleRenderContext;
struct RenderJob;

struct alignas(32) Triangle
{
	union {
		std::array<TexVertex, 3> tv;
		struct {
			struct alignas(32) { real x1, y1, z1, w1, u1, v1, zInv1; };
			struct alignas(32) { real x2, y2, z2, w2, u2, v2, zInv2; };
			struct alignas(32) { real x3, y3, z3, w3, u3, v3, zInv3; };
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

	Vec4 getNormalVector() const;
private:
	void prepareScreenSpace(const TriangleRenderContext& context) const; //WARNING: this method expects tv to contain rotated (but not yet z-divided coords)!
	void addToRenderQueueFinal(const TriangleRenderContext& context, bool flatTop) const; //This method expects tv to contain screen space coords in tv.spaceCoords with z holding 1/world z and z divided texture coords in tv.textureCoords
};

struct RenderJob
{
	Triangle t;
	bool flatTop;
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