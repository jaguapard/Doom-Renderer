#pragma once
#include "ZBuffer.h"
#include "CoordinateTransformer.h"
#include "Triangle.h"
#include "Model.h"
#include "Camera.h"

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
	Camera pov;

	ShadowMap(int w, int h, const Camera& pov);
	void render(const std::vector<Model>& models, const GameSettings& gameSettings, Threadpool& threadpool);
};