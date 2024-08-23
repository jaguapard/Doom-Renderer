#include "ShadowMap.h"
#include "Threadpool.h"

ShadowMap::ShadowMap(int w, int h, const CoordinateTransformer& ctr)
{
	this->depthBuffer = { w,h };
	this->ctr = ctr;
}

void ShadowMap::render(const std::vector<const Model*>& models, const GameSettings& gameSettings, const std::vector<ShadowMap>& shadowMaps, Threadpool& threadpool)
{
	TriangleRenderContext ctx;
	depthBuffer.clear();

	ctx.renderingShadowMap = true;
	ctx.shadowMaps = &shadowMaps;

	ctx.zBuffer = &depthBuffer;
	ctx.frameBuffer = nullptr;
	ctx.pixelWorldPos = nullptr;
	ctx.lightBuffer = nullptr;
	ctx.pointLights = nullptr;
	ctx.ctr = &ctr;

	ctx.framebufW = this->depthBuffer.getW();
	ctx.framebufH = this->depthBuffer.getH();
	ctx.gameSettings = gameSettings;

	std::vector<RenderJob> renderJobs;
	ctx.renderJobs = &renderJobs;
	ctx.doomSkyTextureMarkerIndex = 0; //TODO: remove this hardcode
	
	for (auto& it : models)
	{
		it->addToRenderQueue(ctx);
	}

	int threads = threadpool.getThreadCount();
	std::vector<task_id> ids;
	for (int i = 0; i < threads; ++i)
	{
		taskfunc_t task = [&, i]() {
			auto [limLow, limHigh] = threadpool.getLimitsForThread(i, 0, depthBuffer.getH(), threads);
			for (auto& job : renderJobs)
			{
				job.originalTriangle.drawSlice(ctx, job, limLow, limHigh);
			}
		};

		ids.push_back(threadpool.addTask(task));
	}
	threadpool.waitForMultipleTasks(ids);	
}
