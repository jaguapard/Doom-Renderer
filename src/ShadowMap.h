#pragma once
#include "ZBuffer.h"
#include "CoordinateTransformer.h"
#include "Triangle.h"
#include "Model.h"

struct ShadowMapTriangleAddendum
{
	std::array<Vec4, 3> screenCoords;
	bool isVisibleToMap;
};

struct ShadowMap
{
	ZBuffer depthBuffer;
	CoordinateTransformer ctr;

	ShadowMap(int w, int h, const CoordinateTransformer& ctr);
	void render(const std::vector<const Model*>& models, const GameSettings& gameSettings, const std::vector<ShadowMap>& shadowMaps);
};