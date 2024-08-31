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
#include "misc/GameSettings.h"

struct TriangleRenderContext;
struct RenderJob;
class Model;

struct Triangle
{
	//std::array<TexVertex, 3> tv;
	TexVertex tv[3];

	void sortByAscendingSpaceX();
	void sortByAscendingSpaceY();
	void sortByAscendingSpaceZ();
	void sortByAscendingTextureX();
	void sortByAscendingTextureY();

	void addToRenderQueue(const TriangleRenderContext& context, const Model* pModel) const;
	static std::pair<Triangle, Triangle> pairFromRect(std::array<TexVertex, 4> rectPoints);

	Vec4 getNormalVector() const;

	VectorPack16 interpolateSpaceCoords(const FloatPack16& alpha, const FloatPack16& beta, const FloatPack16& gamma) const;
	VectorPack16 interpolateTextureCoords(const FloatPack16& alpha, const FloatPack16& beta, const FloatPack16& gamma) const;
	VectorPack16 interpolateWorldCoords(const FloatPack16& alpha, const FloatPack16& beta, const FloatPack16& gamma) const;
private:
	void prepareScreenSpace(const TriangleRenderContext& context, const Model* pModel) const; //WARNING: this method expects tv to contain rotated (but not yet z-divided coords)!
	void addToRenderQueueFinal(const TriangleRenderContext& context, const Model* pModel) const; //This method expects tv to contain screen space coords in tv.spaceCoords with z holding 1/world z and z divided texture coords in tv.textureCoords
};

struct TriangleRenderContext
{
	FloatColorBuffer* frameBuffer = nullptr;
	PixelBuffer<real>* lightBuffer = nullptr;
	ZBuffer* zBuffer = nullptr;
	FloatColorBuffer* pixelWorldPos = nullptr;

	std::vector<RenderJob>* renderJobs;
	const std::vector<PointLight>* pointLights;
	const std::vector<ShadowMap>* shadowMaps;

	const CoordinateTransformer* ctr;

	real framebufW, framebufH;
	Vec4 camPos;

	GameSettings gameSettings;
	int doomSkyTextureMarkerIndex;
};