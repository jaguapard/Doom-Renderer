#include "RenderQueue.h"
#include "Statsman.h"

RenderQueue::RenderQueue(Threadpool& pool)
	:threadpool(pool)
{
	threadJobs.resize(threadpool.getThreadCount());
}

void RenderQueue::clear()
{
	assert(threadJobs.size() > 0);
	for (auto& it : threadJobs) it.clear();
	jobsInQueue = 0;
}

void RenderQueue::addInitialJob(const Triangle& t, int textureIndex, real lightMult)
{
	RenderJob rj;
	rj.t = t;
	rj.info.textureIndex = textureIndex;
	rj.info.lightMult = lightMult;
	threadJobs[(jobsInQueue++) % threadJobs.size()].emplace_back(rj); //somewhat even split of triangles

	t.assertNoNans();
}

void RenderQueue::drawOn(TriangleRenderContext ctx)
{
	std::vector<Threadpool::task_id> taskIds;

	for (size_t tInd = 0; tInd < threadpool.getThreadCount(); ++tInd)
	{
		Threadpool::task_t task = [&, tInd]()
		{
			this->doRotationAndClipping(ctx, tInd);
			this->doScreenSpaceTransform(ctx, tInd);
		};
		
		taskIds.push_back(threadpool.addTask(task));
	}

	threadpool.waitForMultipleTasks(taskIds); //wait until all transformations are done
	taskIds.clear();


	for (size_t tInd = 0; tInd < threadpool.getThreadCount(); ++tInd)
	{
		Threadpool::task_t task = [&, tInd]()
			{
				this->doDraw(ctx, tInd);
				this->doScreenBlitting(ctx, tInd);
			};

		taskIds.push_back(threadpool.addTask(task));
	}
	threadpool.waitForMultipleTasks(taskIds);
	

	/*size_t nThreads = threadpool.getThreadCount();
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

			real* lightPtr = ctx.lightBuffer->begin() + myStartIndex;
			Color* framebufPtr = ctx.frameBuffer->begin() + myStartIndex;
			Uint32* wndSurfPtr = reinterpret_cast<Uint32*>(ctx.wndSurf->pixels) + myStartIndex;

			Color::multipliyByLightInPlace(lightPtr, framebufPtr, myPixelCount);
			Color::toSDL_Uint32(framebufPtr, wndSurfPtr, myPixelCount, *ctx.windowBitShifts);
		};
	}*/
}

void RenderQueue::doRotationAndClipping(TriangleRenderContext& ctx, size_t threadIndex)
{
	std::vector<RenderJob>& initialJobs = threadJobs[threadIndex];
	int64_t indexChange = 0;
	for (size_t nRenderJob = 0; nRenderJob < initialJobs.size()-indexChange; ++nRenderJob)
	{
		std::array<TexVertex, 3> rot;
		bool vertexOutside[3] = { false };
		int outsideVertexCount = 0;
		RenderJob& currJob = initialJobs[nRenderJob];
		Triangle& currTri = currJob.t;

		for (int i = 0; i < 3; ++i)
		{
			Vec3 off = ctx.ctr->doCamOffset(currTri.tv[i].spaceCoords);
			Vec3 rt = ctx.ctr->rotate(off);
			off.assert_validX();
			off.assert_validY();
			off.assert_validZ();
			rt.assert_validX();
			rt.assert_validY();
			rt.assert_validZ();

			if (rt.z > -1)
			{
				outsideVertexCount++;
				if (outsideVertexCount == 3)
				{
					StatCount(statsman.triangles.tripleVerticeOutOfScreenDiscards++);
					//triangle is completely behind the clipping plane, discard. We can't use any swapping techiniques, since end elements may be populated with correct triangles (results from clipping). That's why we just mark this job as bad
					currJob.shouldDraw = false;
					break;
				}
				vertexOutside[i] = true;
			}
			rot[i] = { rt, currTri.tv[i].textureCoords };
			rot[i].assertNoNans();
		}

		if (outsideVertexCount == 3) continue;
		if (outsideVertexCount == 0) //all vertices are in front of camera, just save results
		{
			StatCount(statsman.triangles.zeroVerticesOutsideDraws++);
			currTri.tv = rot;
			continue;
		}

		auto itBeg = std::begin(vertexOutside);
		auto itEnd = std::end(vertexOutside);
		if (outsideVertexCount == 1) //if 1 vertice is outside, then 1 triangle gets turned into two
		{
			StatCount(statsman.triangles.singleVertexOutOfScreenSplits++);
			int i = std::find(itBeg, itEnd, true) - itBeg;
			int v1_ind = i > 0 ? i - 1 : 2; //preserve vertice order of the original triangle and prevent out of bounds
			int v2_ind = i < 2 ? i + 1 : 0; //we only "change" the existing vertex

			const TexVertex& v1 = rot[v1_ind];
			const TexVertex& v2 = rot[v2_ind];
			Triangle t1 = currTri, t2 = currTri;
			TexVertex clipped1 = rot[i].getClipedToPlane(v1);
			TexVertex clipped2 = rot[i].getClipedToPlane(v2);

			t1.tv = { v1,clipped1, v2 };
			t2.tv = { clipped1, clipped2, v2 };

			currTri = t1;
			initialJobs.push_back({ .t = t2, .info = currJob.info });

			t1.assertNoNans();
			t2.assertNoNans();

			indexChange++; //avoid attempting to clip the same triangle twice
			continue;
		}

		if (outsideVertexCount == 2) //in case there are 2 vertices that are outside, the triangle just gets clipped (no new triangles needed)
		{
			StatCount(statsman.triangles.doubleVertexOutOfScreenSplits++);
			int i = std::find(itBeg, itEnd, false) - itBeg;
			int v1_ind = i > 0 ? i - 1 : 2; //preserve vertice order of the original triangle and prevent out of bounds
			int v2_ind = i < 2 ? i + 1 : 0; //we only "change" the existing vertex

			Triangle clipped = currTri;
			clipped.tv[v1_ind] = rot[v1_ind].getClipedToPlane(rot[i]);
			clipped.tv[v2_ind] = rot[v2_ind].getClipedToPlane(rot[i]);
			clipped.tv[i] = rot[i];
			currTri = clipped;
			clipped.assertNoNans();
			continue;
		}
	}
}

void RenderQueue::doScreenSpaceTransform(TriangleRenderContext& ctx, size_t threadIndex)
{
	std::vector<RenderJob>& initialJobs = threadJobs[threadIndex];
	auto zone = threadpool.getLimitsForThread(threadIndex, 0, ctx.framebufH);
	int zoneMinY = zone.first;
	int zoneMaxY = zone.second;

	for (size_t nRenderJob = 0; nRenderJob < initialJobs.size(); ++nRenderJob)
	{
		RenderJob& currJob = initialJobs[nRenderJob];
		if (!currJob.shouldDraw) continue;

		Triangle& currentTriangle = currJob.t;
		std::array<TexVertex, 3> fullyTransformed;
		for (int i = 0; i < 3; ++i)
		{
			real zInv = 1.0 / currentTriangle.tv[i].spaceCoords.z;
			Vec3 zDividedWorld = currentTriangle.tv[i].spaceCoords * zInv;
			Vec3 zDividedUv = currentTriangle.tv[i].textureCoords * zInv;
			zDividedUv.z = zInv;
			fullyTransformed[i] = { ctx.ctr->screenSpaceToPixels(zDividedWorld), zDividedUv };
		}
		
		for (auto& it : fullyTransformed) it.assertNoNans();
		if (fullyTransformed[0].spaceCoords.y == fullyTransformed[1].spaceCoords.y && fullyTransformed[1].spaceCoords.y == fullyTransformed[2].spaceCoords.y)
		{
			continue; //sadly, this doesn't get caught by drawing loop conditions, since NaN wrecks havok there
		}

		//we need to sort by triangle's screen Y (ascending) for later flat top and bottom splits
		std::sort(fullyTransformed.begin(), fullyTransformed.end());
		currentTriangle.tv = fullyTransformed;
	}
}

void RenderQueue::doDraw(TriangleRenderContext& ctx, size_t threadIndex)
{
	auto zone = threadpool.getLimitsForThread(threadIndex, 0, ctx.framebufH);
	int zoneMinY = zone.first;
	int zoneMaxY = zone.second;

	for (auto& currJobBlock : threadJobs)
	{
		for (auto& currJob : currJobBlock)
		{
			if (!currJob.shouldDraw) continue;
			const Triangle& t = currJob.t;
			real splitAlpha = (t.tv[1].spaceCoords.y - t.tv[0].spaceCoords.y) / (t.tv[2].spaceCoords.y - t.tv[0].spaceCoords.y);
			TexVertex splitVertex = lerp(t.tv[0], t.tv[2], splitAlpha);

			Triangle flatBottom;
			flatBottom.tv = { t.tv[0], t.tv[1], splitVertex };

			Triangle flatTop;
			flatTop = { t.tv[1], splitVertex, t.tv[2] };

			flatBottom.drawSlice(ctx, currJob.info, true, zoneMinY, zoneMaxY);
			flatTop.drawSlice(ctx, currJob.info, false, zoneMinY, zoneMaxY);
		}
	}
	
}

void RenderQueue::doScreenBlitting(TriangleRenderContext& ctx, size_t threadIndex)
{
	auto zone = threadpool.getLimitsForThread(threadIndex, 0, ctx.framebufH);
	int myMinY = zone.first;
	int myMaxY = zone.second;	

	int myPixelCount = (myMaxY - myMinY) * ctx.framebufW;
	int myStartIndex = myMinY * ctx.framebufW;

	real* lightPtr = ctx.lightBuffer->begin() + myStartIndex;
	Color* framebufPtr = ctx.frameBuffer->begin() + myStartIndex;
	Uint32* wndSurfPtr = reinterpret_cast<Uint32*>(ctx.wndSurf->pixels) + myStartIndex;

	Color::multipliyByLightInPlace(lightPtr, framebufPtr, myPixelCount);
	Color::toSDL_Uint32(framebufPtr, wndSurfPtr, myPixelCount, *ctx.windowBitShifts);
}
