#include "RasterizationRenderer.h"


RasterizationRenderer::RasterizationRenderer(int w, int h, Threadpool& threadpool) : threadpool(threadpool)
{
	zBuffer = { w,h };
	frameBuf = { w,h };
}

void RasterizationRenderer::drawScene(const std::vector<Model>& models, SDL_Surface* dstSurf, const GameSettings& gameSettings)
{
}
