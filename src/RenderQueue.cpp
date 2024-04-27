#include "RenderQueue.h"

RenderQueue::RenderQueue(Threadpool& pool)
	:threadpool(pool)
{
	drawJobs.resize(threadpool.getThreadCount());
}

void RenderQueue::clear()
{
	initialJobs.clear();
	drawJobs.clear();
}

void RenderQueue::addInitialJob(const Triangle& t, int textureIndex, real lightMult)
{
	initialJobs.emplace_back(t, textureIndex, lightMult);
}

void RenderQueue::drawOn(TriangleRenderContext ctx)
{
	size_t nThreads = threadpool.getThreadCount();
	for (size_t i = 0; i < nThreads; ++i)
	{
		std::function func = [=, &drawJobs]() {
			//int myMinY, myMaxY;
			int myThreadNum = i;
			auto limits = threadpool.getLimitsForThread(myThreadNum, 0, ctx.framebufH);
			
			int myMinY = limits.first;
			int myMaxY = limits.second;
			//if (myThreadNum == threadCount - 1) myMaxY = ctx.framebufH - 1; //avoid going out of bounds

			for (size_t i = 0; i < drawJobs[myThreadNum].size(); ++i)
			{
				const DrawJob& myJob = drawJobs[myThreadNum][i];
				myJob.flatBottom.drawSlice(ctx, myJob, myMinY, myMaxY);
				//myJob.t->startRender(ctx, myJob, myMinY, myMaxY);
			}

			int myPixelCount = (myMaxY - myMinY) * ctx.framebufW;
			int myStartIndex = myMinY * ctx.framebufW;

			real* lightPtr = lightStart + myStartIndex;
			Color* framebufPtr = framebufStart + myStartIndex;
			Uint32* wndSurfPtr = wndSurfStart + myStartIndex;

			Color::multipliyByLightInPlace(lightPtr, framebufPtr, myPixelCount);
			Color::toSDL_Uint32(framebufPtr, wndSurfPtr, myPixelCount, shifts);
		};
	}
}
