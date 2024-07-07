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
#include "PointLight.h"

struct TriangleRenderContext;
struct RenderJob;

struct Triangle
{
	//std::array<TexVertex, 3> tv;
	TexVertex tv[3];

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
	void addToRenderQueueFinal(const TriangleRenderContext& context) const; //This method expects tv to contain screen space coords in tv.spaceCoords with z holding 1/world z and z divided texture coords in tv.textureCoords
};

struct RenderJob
{
	Triangle originalTriangle;

	int textureIndex;
	real lightMult;

	real minX, minY, maxX, maxY;
	real rcpSignedArea;
};

struct TriangleRenderContext
{
	FloatColorBuffer* frameBuffer;
	PixelBuffer<real>* lightBuffer;
	ZBuffer* zBuffer;
    FloatColorBuffer* pixelWorldPos;

	const CoordinateTransformer* ctr;	
	const TextureManager* textureManager;
	const Texture* texture;
	int textureIndex;
	real lightMult;
	real framebufW, framebufH;
	Vec4 camPos;

	int doomSkyTextureMarkerIndex;
	bool wireframeEnabled;
	bool backfaceCullingEnabled;
	bool ditheringEnabled;

	std::vector<RenderJob>* renderJobs;
    const std::vector<PointLight>* pointLights;

	real nearPlaneClippingZ = -1;
	real fovMult = 1;
};