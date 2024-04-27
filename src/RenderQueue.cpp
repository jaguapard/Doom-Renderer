#include "RenderQueue.h"
#include "Statsman.h"

RenderQueue::RenderQueue(Threadpool& pool)
	:threadpool(pool)
{
	//drawJobs.resize(threadpool.getThreadCount());
}

void RenderQueue::clear()
{
	initialJobs.clear();
	//drawJobs.clear();
}

void RenderQueue::addInitialJob(const Triangle& t, int textureIndex, real lightMult)
{
	RenderJob rj;
	rj.t = t;
	rj.info.textureIndex = textureIndex;
	rj.info.lightMult = lightMult;
	initialJobs.emplace_back(rj);
}

void RenderQueue::drawOn(TriangleRenderContext ctx)
{
	this->doRotationAndClipping(ctx);
	this->doScreenSpaceTransformAndDraw(ctx);

	int myMaxY = 1079;
	int myMinY = 0;

	int myPixelCount = (myMaxY - myMinY) * ctx.framebufW;
	int myStartIndex = myMinY * ctx.framebufW;

	real* lightPtr = ctx.lightBuffer->begin() + myStartIndex;
	Color* framebufPtr = ctx.frameBuffer->begin() + myStartIndex;
	Uint32* wndSurfPtr = reinterpret_cast<Uint32*>(ctx.wndSurf->pixels) + myStartIndex;

	Color::multipliyByLightInPlace(lightPtr, framebufPtr, myPixelCount);
	Color::toSDL_Uint32(framebufPtr, wndSurfPtr, myPixelCount, *ctx.windowBitShifts);

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
	int64_t indexChange = 0;
	for (size_t nRenderJob = 0; nRenderJob < initialJobs.size() - indexChange; ++nRenderJob)
	{
	loopBegin:
		std::array<TexVertex, 3> rot;
		bool vertexOutside[3] = { false };
		int outsideVertexCount = 0;
		RenderJob& currJob = initialJobs[nRenderJob];
		Triangle& currTri = currJob.t;

		for (int i = 0; i < 3; ++i)
		{
			Vec3 off = ctx.ctr->doCamOffset(currTri.tv[i].spaceCoords);
			Vec3 rt = ctx.ctr->rotate(off);

			if (rt.z > -1)
			{
				outsideVertexCount++;
				if (outsideVertexCount == 3)
				{
					StatCount(statsman.triangles.tripleVerticeOutOfScreenDiscards++);
					std::swap(currJob, initialJobs.back());
					initialJobs.pop_back();
					goto loopBegin; //triangle is completely behind the clipping plane, discard
				}
				vertexOutside[i] = true;
			}
			rot[i] = { rt, currTri.tv[i].textureCoords };
		}

		if (outsideVertexCount == 0) //all vertices are in front of camera, prepare data for drawRotationPrepped and proceed
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
			Triangle t1 = *this, t2 = *this;
			TexVertex clipped1 = rot[i].getClipedToPlane(v1);
			TexVertex clipped2 = rot[i].getClipedToPlane(v2);

			t1.tv = { v1,clipped1, v2 };
			t2.tv = { clipped1, clipped2, v2 };

			t1.prepareScreenSpace(context, rj, zoneMinY, zoneMaxY);
			t2.prepareScreenSpace(context, rj, zoneMinY, zoneMaxY);
			return;
		}

		if (outsideVertexCount == 2) //in case there are 2 vertices that are outside, the triangle just gets clipped (no new triangles needed)
		{
			StatCount(statsman.triangles.doubleVertexOutOfScreenSplits++);
			int i = std::find(itBeg, itEnd, false) - itBeg;
			int v1_ind = i > 0 ? i - 1 : 2; //preserve vertice order of the original triangle and prevent out of bounds
			int v2_ind = i < 2 ? i + 1 : 0; //we only "change" the existing vertex

			Triangle clipped = *this;
			clipped.tv[v1_ind] = rot[v1_ind].getClipedToPlane(rot[i]);
			clipped.tv[v2_ind] = rot[v2_ind].getClipedToPlane(rot[i]);
			clipped.tv[i] = rot[i];
			return clipped.prepareScreenSpace(context, rj, zoneMinY, zoneMaxY);
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

		flatBottom.drawSlice(ctx, currJob.info, true, zoneMinY, zoneMaxY); //TODO: no, this will not work when we implement MT
		flatTop.drawSlice(ctx, currJob.info, false, zoneMinY, zoneMaxY);
		//flatTop.addToRenderQueueFinal(context, false);
		//flatBottom.addToRenderQueueFinal(context, true);
	}
}
