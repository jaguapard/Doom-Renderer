#include "ShadowMap.h"
#include "Threadpool.h"
#include "shaders/MainFragmentRenderShader.h"
#include "Renderers/RasterizationRenderer.h"

ShadowMap::ShadowMap(int w, int h, const Camera& cam)
{
	this->depthBuffer = { w,h };
	this->pov = cam;
	this->ctr = { w,h };
	this->ctr.prepare(cam.pos, cam.angle);
}

void ShadowMap::render(const std::vector<Model>& models, const GameSettings& gameSettings, Threadpool& threadpool)
{	
	RasterizationRenderer rend(this->depthBuffer.getW(), this->depthBuffer.getH(), threadpool, true);
	std::vector<const Model*> modelPtrs;
	for (const auto& it : models) if (it.textureIndex != 0) modelPtrs.push_back(&it); //TODO: remove this hardcode (sky textured level geometry in DOOM)
	rend.drawScene(modelPtrs, nullptr, gameSettings, this->pov, true);
	this->depthBuffer = rend.getDepthBuffer();
}
