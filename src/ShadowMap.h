#pragma once
#include "ZBuffer.h"
#include "CoordinateTransformer.h"
#include "Triangle.h"
#include "Model.h"

class Threadpool;
struct ShadowMapTriangleAddendum
{
	std::array<Vec4, 3> screenCoords;
	bool isVisibleToMap;
};

struct ShadowMap
{
	CoordinateTransformer ctr;
	ZBuffer depthBuffer;	
	real fovMult = 1;

	ShadowMap(int w, int h, const CoordinateTransformer& ctr);
	void render(const std::vector<Model>& models, const GameSettings& gameSettings, const std::vector<ShadowMap>& shadowMaps, Threadpool& threadpool);
};