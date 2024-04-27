#include "RenderQueue.h"
#include "Statsman.h"

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
	this->doRotationAndClipping(ctx);
	this->doScreenSpaceTransformAndDraw(ctx);

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

void RenderQueue::doRotationAndClipping(TriangleRenderContext& ctx)
{
	for (size_t nRenderJob = 0; nRenderJob < initialJobs.size(); ++nRenderJob)
	{
	loopBegin:
		std::array<TexVertex, 3> rot;
		bool vertexOutside[3] = { false };
		int outsideVertexCount = 0;
		RenderJob& currJob = initialJobs[nRenderJob];
		Triangle& currTriangle = currJob.t;

		for (int i = 0; i < 3; ++i)
		{
			Vec3 off = ctx.ctr->doCamOffset(currTriangle.tv[i].spaceCoords);
			Vec3 rt = ctx.ctr->rotate(off);

			if (rt.z > ctx.nearPlaneClippingZ)
			{
				outsideVertexCount++;
				if (outsideVertexCount == 3) //triangle is completely behind the clipping plane, discard
				{
					StatCount(statsman.triangles.tripleVerticeOutOfScreenDiscards++);
					std::swap(initialJobs[nRenderJob--], initialJobs.back());
					initialJobs.pop_back();
					goto loopBegin; //can't use continue here, it will not skip upper loop iteration
				}
				vertexOutside[i] = true;
			}
			currTriangle.tv[i].spaceCoords = rt;
		}

		if (outsideVertexCount == 0) //if all vertices are in front of camera, this triangle doesn't need clipping
		{
			StatCount(statsman.triangles.zeroVerticesOutsideDraws++);
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
			Triangle newTriangle;
			TexVertex clipped1 = rot[i].getClipedToPlane(v1);
			TexVertex clipped2 = rot[i].getClipedToPlane(v2);

			currTriangle.tv = { v1,clipped1, v2 };
			newTriangle.tv = { clipped1, clipped2, v2 };
			initialJobs.push_back({ newTriangle, currJob.info });
			continue;
		}

		if (outsideVertexCount == 2) //in case there are 2 vertices that are outside, the triangle just gets clipped (no new triangles needed)
		{
			StatCount(statsman.triangles.doubleVertexOutOfScreenSplits++);
			int i = std::find(itBeg, itEnd, false) - itBeg;
			int v1_ind = i > 0 ? i - 1 : 2; //preserve vertice order of the original triangle and prevent out of bounds
			int v2_ind = i < 2 ? i + 1 : 0; //we only "change" the existing vertex

			currTriangle.tv[v1_ind] = rot[v1_ind].getClipedToPlane(rot[i]);
			currTriangle.tv[v2_ind] = rot[v2_ind].getClipedToPlane(rot[i]);
			currTriangle.tv[i] = rot[i];
			continue;
		}
	}
}

void RenderQueue::doScreenSpaceTransformAndDraw(TriangleRenderContext& ctx)
{
	int zoneMinY = 0; //TODO: testing values, remove!
	int zoneMaxY = 1079;

	for (size_t nRenderJob = 0; nRenderJob < initialJobs.size(); ++nRenderJob)
	{
		RenderJob& currJob = initialJobs[nRenderJob];
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
		if (fullyTransformed[0].spaceCoords.y == fullyTransformed[1].spaceCoords.y && fullyTransformed[1].spaceCoords.y == fullyTransformed[2].spaceCoords.y) return; //sadly, this doesn't get caught by loop conditions, since NaN wrecks havok there
		//we need to sort by triangle's screen Y (ascending) for later flat top and bottom splits
		std::sort(fullyTransformed.begin(), fullyTransformed.end());

		real splitAlpha = (fullyTransformed[1].spaceCoords.y - fullyTransformed[0].spaceCoords.y) / (fullyTransformed[2].spaceCoords.y - fullyTransformed[0].spaceCoords.y);
		TexVertex splitVertex = lerp(fullyTransformed[0], fullyTransformed[2], splitAlpha);

		Triangle flatBottom;
		flatBottom.tv = { fullyTransformed[0], fullyTransformed[1], splitVertex };

		Triangle flatTop;
		flatTop = { fullyTransformed[1], splitVertex, fullyTransformed[2] };

		flatBottom.drawSlice(ctx, currJob.info, true, zoneMinY, zoneMaxY);
		flatTop.drawSlice(ctx, currJob.info, false, zoneMinY, zoneMaxY);
		//flatTop.addToRenderQueueFinal(context, false);
		//flatBottom.addToRenderQueueFinal(context, true);
	}
}
