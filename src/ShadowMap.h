#pragma once
#include "ZBuffer.h"
#include "CoordinateTransformer.h"
#include "Triangle.h"
#include "Model.h"

struct ShadowMap
{
	ZBuffer depthBuffer;
	CoordinateTransformer ctr;

	ShadowMap(int w, int h, const CoordinateTransformer& ctr);
	void render(const std::vector<const Model*>& models, const GameSettings& gameSettings);
};