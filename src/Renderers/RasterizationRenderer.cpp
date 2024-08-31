#include "RasterizationRenderer.h"
#include "../Threadpool.h"
#include "../Camera.h"
#include "../Statsman.h"
#include "../shaders/MainFragmentRenderShader.h"
#include "../blitting.h"
#include <sstream>
#include "../ShadowMap.h"

RasterizationRenderer::RasterizationRenderer(int w, int h, Threadpool& threadpool, bool depthOnly)
{
	this->zBuffer = { w,h };
	if (!depthOnly)
	{
		this->frameBuf = { w,h };
		this->pixelWorldPosBuf = { w,h };
	}
	this->threadpool = &threadpool;
	this->ctr = { w,h };

	this->renderJobs.resize(threadpool.getThreadCount());
}

void RasterizationRenderer::drawScene(const std::vector<const Model*>& models, SDL_Surface* dstSurf, const GameSettings& gameSettings, const Camera& pov)
{
	this->drawScene(models, dstSurf, gameSettings, pov, false); //can't add a default value to virtual function, so just overload it
}

void RasterizationRenderer::drawScene(const std::vector<const Model*>& models, SDL_Surface* dstSurf, const GameSettings& gameSettings, const Camera& pov, bool depthOnly)
{
	this->currFrameGameSettings = gameSettings; 
	size_t threadCount = threadpool->getThreadCount();
	std::vector<ModelSlice> distributedSlices = this->distributeTrianglesForWorkers(models, threadCount);
	this->ctr.prepare(pov.pos, pov.angle);
	std::array<uint32_t, 4> surfaceShifts;
	if (dstSurf) surfaceShifts = this->getShiftsForSurface(dstSurf);

	std::vector<task_id> transformTasks, drawTasks;
	for (int tNum = 0; tNum < threadCount; ++tNum)
	{
		taskfunc_t f = [&, tNum]() {
			for (const auto& slice : distributedSlices)
			{
				if (slice.workerNumber == tNum) this->addTriangleRangeToRenderQueue(slice.pTrianglesBegin, slice.pTrianglesEnd, slice.pModel, tNum);
			}
		};
		transformTasks.push_back(threadpool->addTask(f));
	}
	threadpool->waitForMultipleTasks(transformTasks);
	
	for (int tNum = 0; tNum < threadCount; ++tNum)
	{
		//is is crucial to capture some stuff by value [=], else function risks getting garbage values when the task starts. 
		//It is, however, assumed that renderJobs vector remains in a valid state until all tasks are completed.
		taskfunc_t f = [=]() {
			int ssaaMult = this->currFrameGameSettings.ssaaMult;
			int outputHeight = (depthOnly ? this->zBuffer.getH() : this->frameBuf.getH()) / ssaaMult;
			auto lim = threadpool->getLimitsForThread(tNum, 0, outputHeight);
			int outputMinY = lim.first; //truncate limits to avoid fighting
			int outputMaxY = lim.second;
			int renderMinY = outputMinY * ssaaMult; //it is essential to caclulate rendering zone limits like this, to ensure there are no intersections in different threads
			int renderMaxY = outputMaxY * ssaaMult;

			this->zBuffer.clearRows(renderMinY, renderMaxY); //Z buffer has to be cleared, else only pixels closer than previous frame will draw
			if (this->currFrameGameSettings.bufferCleaningEnabled)
			{
				//this->zBuffer.clearRows(renderMinY, renderMaxY);
				//this->zBuffer.clearRows(renderMinY, renderMaxY, 1);
			}

			BoundingBox threadBox;
			threadBox.minX = 0;
			threadBox.minY = renderMinY;
			threadBox.maxX = (depthOnly ? this->zBuffer.getW() : this->frameBuf.getW()) - 1;
			threadBox.maxY = renderMaxY - 1;

			for (const auto& it : this->renderJobs)
			{
				for (const auto& rj : it)
				{
					this->drawRenderJobSlice(rj, threadBox, depthOnly);
				}
			}			

			//if (this->currFrameGameSettings.fogEnabled) blitting::applyFog(*ctx.frameBuffer, *ctx.pixelWorldPos, camPos, settings.fogIntensity / settings.fovMult, Vec4(0.7, 0.7, 0.7, 1), renderMinY, renderMaxY, settings.fogEffectVersion); //divide by fovMult to prevent FOV setting from messing with fog intensity
			//threadpool->waitUntilTaskCompletes(windowUpdateTaskId);
			if (dstSurf) blitting::frameBufferIntoSurface(this->frameBuf, dstSurf, outputMinY, outputMaxY, surfaceShifts, this->currFrameGameSettings.ditheringEnabled, ssaaMult);
		};

		drawTasks.push_back(threadpool->addTask(f));
	}

	threadpool->waitForMultipleTasks(drawTasks);
	for (auto& it : this->renderJobs) it.clear();
}

std::vector<std::pair<std::string, std::string>> RasterizationRenderer::getAdditionalOSDInfo()
{
	return {
		{"Render resolution", (std::stringstream() << this->frameBuf.getW() << "x" << this->frameBuf.getH() << " (" << this->currFrameGameSettings.ssaaMult << "x)").str()}
	};
}

void RasterizationRenderer::saveBuffers()
{
	std::string s = std::to_string(__rdtsc());
	//this->frameBuf.saveToFile("screenshots/" + s + "_framebuf.png");
	this->zBuffer.saveToFile("screenshots/" + s + "_zbuf.png");
	this->shadowMaps[0]->depthBuffer.saveToFile("screenshots/" + s + "_shadow_map.png");
}

const ZBuffer& RasterizationRenderer::getDepthBuffer() const
{
	return this->zBuffer;
}

std::vector<RasterizationRenderer::ModelSlice> RasterizationRenderer::distributeTrianglesForWorkers(const std::vector<const Model*>& sceneModels, size_t threadCount)
{
	std::vector<ModelSlice> modelSlices;
	size_t totalTriangles = 0;
	for (const auto& model : sceneModels)
	{
		size_t count = model->getTriangleCount();
		totalTriangles += count;

		ModelSlice& slice = modelSlices.emplace_back();
		slice.pTrianglesBegin = model->getTriangles().data();
		slice.pTrianglesEnd = slice.pTrianglesBegin + count;
		slice.pModel = model;
	}

	size_t trianglesBeforeDistribution = 0, trianglesAfterDistribution = 0;
	for (const auto& it : modelSlices) trianglesBeforeDistribution += it.pTrianglesEnd - it.pTrianglesBegin;
	assert(trianglesBeforeDistribution == totalTriangles);

	size_t sliceIndex = 0;
	for (size_t i = 0; i < threadCount; ++i)
	{
		size_t myTriangleCount = 0;
		auto [limLow, limHigh] = threadpool->getLimitsForThread(i, 0, totalTriangles, threadCount);
		size_t myTriangleLimit = limHigh - limLow;
		while (myTriangleLimit > 0 && sliceIndex < modelSlices.size())
		{
			size_t currentSliceSize = modelSlices[sliceIndex].pTrianglesEnd - modelSlices[sliceIndex].pTrianglesBegin;
			if (currentSliceSize <= myTriangleLimit)
			{
				myTriangleLimit -= currentSliceSize;
				modelSlices[sliceIndex].workerNumber = i;
			}
			else
			{
				const Triangle* pBorder = modelSlices[sliceIndex].pTrianglesBegin + myTriangleLimit;
				ModelSlice& newSlice = modelSlices.emplace_back(); //maybe should insert in the same place?

				newSlice.pModel = modelSlices[sliceIndex].pModel;
				newSlice.pTrianglesEnd = modelSlices[sliceIndex].pTrianglesEnd;
				newSlice.pTrianglesBegin = pBorder;

				modelSlices[sliceIndex].pTrianglesEnd = pBorder;
				modelSlices[sliceIndex].workerNumber = i;
				myTriangleLimit = 0;
			}
			++sliceIndex;
		}
	}
	modelSlices.back().workerNumber = threadCount - 1; //TODO: not a good way
	for (const auto& it : modelSlices) trianglesAfterDistribution += it.pTrianglesEnd - it.pTrianglesBegin;
	assert(trianglesBeforeDistribution == trianglesAfterDistribution);

	for (auto& it : modelSlices) assert(it.workerNumber != -1);
	return modelSlices;
}


int doTriangleClipping(const Triangle& triangleToClip, real clippingZ, Triangle* trianglesOut, int* outsideVertexCount)
{
	std::array<bool, 3> vertexOutside = { false };
	*outsideVertexCount = 0;
	for (int i = 0; i < 3; ++i)
	{
		if (triangleToClip.tv[i].spaceCoords.z > clippingZ)
		{
			vertexOutside[i] = true;
			(*outsideVertexCount)++;
		}
	}

	//search for one of a kind vertex (outside if outsideVertexCount==1, else inside)
	int i = std::find(std::begin(vertexOutside), std::end(vertexOutside), *outsideVertexCount == 1) - std::begin(vertexOutside);
	int v1_ind = i > 0 ? i - 1 : 2; //preserve vertice order of the original triangle and prevent out of bounds lookups
	int v2_ind = i < 2 ? i + 1 : 0;
	const TexVertex& v1 = triangleToClip.tv[v1_ind];
	const TexVertex& v2 = triangleToClip.tv[v2_ind];

	switch (*outsideVertexCount)
	{
	case 0:
		trianglesOut[0] = triangleToClip;
		return 1;
	case 1: //if 1 vertice is outside, then 1 triangle gets turned into two
		TexVertex clipped1 = triangleToClip.tv[i].getClipedToPlane(v1, clippingZ);
		TexVertex clipped2 = triangleToClip.tv[i].getClipedToPlane(v2, clippingZ);

		trianglesOut[0] = { v1,       clipped1, v2 };
		trianglesOut[1] = { clipped1, clipped2, v2 };
		return 2;
	case 2:
		Triangle clipped;
		clipped.tv[v1_ind] = v1.getClipedToPlane(triangleToClip.tv[i], clippingZ);
		clipped.tv[v2_ind] = v2.getClipedToPlane(triangleToClip.tv[i], clippingZ);
		clipped.tv[i] = triangleToClip.tv[i];

		trianglesOut[0] = clipped;
		return 1;
	default:
		return 0;
	}
}

real _3min(real a, real b, real c)
{
	return std::min(a, std::min(b, c));
}
real _3max(real a, real b, real c)
{
	return std::max(a, std::max(b, c));
}

void RasterizationRenderer::addTriangleRangeToRenderQueue(const Triangle* pBegin, const Triangle* pEnd, const Model* pModel, size_t workerNumber)
{
	for (auto pTriangle = pBegin; pTriangle < pEnd; ++pTriangle)
	{
		Triangle t[2];
		int trianglesOut = this->doWorldTransformationsAndClipping(*pTriangle, *pModel, (Triangle*)&t);
		for (int i = 0; i < trianglesOut; ++i)
		{
			auto screenSpaceTriangle = this->transformToScreenSpace(t[i]);
			if (!screenSpaceTriangle) continue;

			const Triangle& t = screenSpaceTriangle.value();
			const Vec4 r1 = t.tv[0].spaceCoords;
			const Vec4 r2 = t.tv[1].spaceCoords;
			const Vec4 r3 = t.tv[2].spaceCoords;

			real signedArea = (r1 - r3).cross2d(r2 - r3);
			if (signedArea == 0.0) continue;

			RenderJob& rj = this->renderJobs[workerNumber].emplace_back();
			rj.rcpSignedArea = 1.0 / signedArea;
			rj.transformedTriangle = t;

			real screenMinX = _3min(r1.x, r2.x, r3.x);
			real screenMaxX = _3max(r1.x, r2.x, r3.x);
			real screenMinY = _3min(r1.y, r2.y, r3.y);
			real screenMaxY = _3max(r1.y, r2.y, r3.y);

			rj.boundingBox.minX = floor(screenMinX);
			rj.boundingBox.maxX = ceil(screenMaxX);
			rj.boundingBox.minY = floor(screenMinY);
			rj.boundingBox.maxY = ceil(screenMaxY);
			rj.pModel = pModel;
		}
	}
}

int RasterizationRenderer::doWorldTransformationsAndClipping(const Triangle& triangle, const Model& model, Triangle* trianglesOut) const
{
	Triangle rotated;
	for (int i = 0; i < 3; ++i)
	{
		Vec4 spaceCopy = triangle.tv[i].spaceCoords;
		spaceCopy.w = 1;

		rotated.tv[i].spaceCoords = this->ctr.rotateAndTranslate(spaceCopy);
		rotated.tv[i].textureCoords = triangle.tv[i].textureCoords;
		rotated.tv[i].worldCoords = triangle.tv[i].spaceCoords;
	}

	if (currFrameGameSettings.backfaceCullingEnabled && currFrameGameSettings.textureManager->getTextureByIndex(model.textureIndex).hasOnlyOpaquePixels())
	{
		Vec4 normal = rotated.getNormalVector();
		if (rotated.tv[0].spaceCoords.dot(normal) >= 0) return 0;
	}

	int outsideVertexCount;
	int nTrianglesOut = doTriangleClipping(rotated, currFrameGameSettings.nearPlaneZ, trianglesOut, &outsideVertexCount);
	StatCount(statsman.triangles.verticesOutside[outsideVertexCount]++);
	return nTrianglesOut;
}

std::optional<Triangle> RasterizationRenderer::transformToScreenSpace(const Triangle& t) const
{
	Triangle screenSpaceTriangle;
	for (int i = 0; i < 3; ++i)
	{
		real zInv = currFrameGameSettings.fovMult / t.tv[i].spaceCoords.z;
		screenSpaceTriangle.tv[i].spaceCoords = t.tv[i].spaceCoords * zInv;
		screenSpaceTriangle.tv[i].textureCoords = t.tv[i].textureCoords * zInv;
		screenSpaceTriangle.tv[i].worldCoords = t.tv[i].worldCoords * zInv;
		screenSpaceTriangle.tv[i].spaceCoords = this->ctr.screenSpaceToPixels(screenSpaceTriangle.tv[i].spaceCoords);
		screenSpaceTriangle.tv[i].textureCoords.z = zInv;
	}

	real y1 = screenSpaceTriangle.tv[0].spaceCoords.y;
	real y2 = screenSpaceTriangle.tv[1].spaceCoords.y;
	real y3 = screenSpaceTriangle.tv[2].spaceCoords.y;
	real x1 = screenSpaceTriangle.tv[0].spaceCoords.x;
	real x2 = screenSpaceTriangle.tv[1].spaceCoords.x;
	real x3 = screenSpaceTriangle.tv[2].spaceCoords.x;
	if (_3min(y1, y2, y3) == _3max(y1, y2, y3) || _3min(x1, x2, x3) == _3max(x1, x2, x3)) return {}; //avoid divisions by 0. 0 height triangle is nonsensical anyway

	return screenSpaceTriangle;
}

std::array<uint32_t, 4> RasterizationRenderer::getShiftsForSurface(const SDL_Surface* surf) const
{
	//this is a stupid fix for everything becoming way too blue in debug mode specifically.
	//it tries to find a missing bit shift to put the alpha value into the unused byte, since Color.toSDL_Uint32 expects 4 shifts
	auto* wf = surf->format;
	std::array<uint32_t, 4> shifts = { wf->Rshift, wf->Gshift, wf->Bshift };
	uint32_t missingShift = 24;
	for (int i = 0; i < 3; ++i)
	{
		if (std::find(std::begin(shifts), std::end(shifts), i * 8) == std::end(shifts))
		{
			missingShift = i * 8;
			break;
		}
	}
	shifts[3] = missingShift;
	for (auto& it : shifts) assert(it % 8 == 0);
	return shifts;
}

BoundingBox RasterizationRenderer::clampBoundingBox(const BoundingBox& clampFrom, const BoundingBox& clampBy) const
{
	assert(offsetof(clampFrom, minX) == 0);
	assert(offsetof(clampFrom, minY) == 4);
	assert(offsetof(clampFrom, maxX) == 8);
	assert(offsetof(clampFrom, maxY) == 12);
	assert(sizeof(clampFrom.maxY) == 4);

	//real minX, minY, maxX, maxY;
	BoundingBox ret;
	if (clampFrom.minY >= clampBy.maxY || clampFrom.maxY < clampBy.minY)
	{
		ret.minY = -999;
		ret.maxY = -9999;
		return ret;
	}

	__m128 original = _mm_loadu_ps((const float*)&clampFrom);
	__m128 clampingBox = _mm_loadu_ps((const float*)&clampBy);

	__m128 lows = _mm_shuffle_ps(clampingBox, clampingBox, _MM_SHUFFLE(1, 0, 1, 0));
	__m128 highs = _mm_shuffle_ps(clampingBox, clampingBox, _MM_SHUFFLE(3, 2, 3, 2));

	__m128 clampedLow = _mm_max_ps(lows, original);
	__m128 clampedHigh = _mm_min_ps(highs, clampedLow);
	_mm_storeu_ps((float*)&ret, clampedHigh);
	return ret;
}

void RasterizationRenderer::drawRenderJobSlice(const RenderJob& renderJob, const BoundingBox& threadBox, bool depthOnly)
{
	BoundingBox clampedBox = this->clampBoundingBox(renderJob.boundingBox, threadBox);
	real yBeg = clampedBox.minY;
	real yEnd = clampedBox.maxY;
	real xBeg = clampedBox.minX;
	real xEnd = clampedBox.maxX;

	const Texture& texture = this->currFrameGameSettings.textureManager->getTextureByIndex(renderJob.pModel->textureIndex);
	const auto& tv = renderJob.transformedTriangle.tv;
	real adjustedLight = renderJob.pModel->lightMult ? powf(renderJob.pModel->lightMult.value(), this->currFrameGameSettings.gamma) : this->currFrameGameSettings.gamma;

	for (real y = yBeg; y <= yEnd; ++y)
	{
		size_t yInt = y;
		//the loop increment section is fairly busy because it's body can be interrupted at various steps, but all increments must always happen
		for (FloatPack16 x = FloatPack16::sequence() + xBeg; Mask16 loopBoundsMask = x <= xEnd; x += 16)
		{
			size_t xInt = x[0];
			VectorPack16 r = VectorPack16(x, y, 0.0, 0.0);
			auto [alpha, beta, gamma] = RenderHelpers::calculateBarycentricCoordinates(r, tv[0].spaceCoords, tv[1].spaceCoords, tv[2].spaceCoords, renderJob.rcpSignedArea);

			Mask16 pointsInsideTriangleMask = loopBoundsMask & alpha >= 0.0 & beta >= 0.0 & gamma >= 0.0;
			if (!pointsInsideTriangleMask) continue;

			VectorPack16 interpolatedDividedUv = VectorPack16(tv[0].textureCoords) * alpha + VectorPack16(tv[1].textureCoords) * beta + VectorPack16(tv[2].textureCoords) * gamma;
			FloatPack16 currDepthValues = this->zBuffer.getPixels16(xInt, yInt);
			Mask16 visiblePointsMask = pointsInsideTriangleMask & currDepthValues > interpolatedDividedUv.z;
			if (!visiblePointsMask) continue; //if all points are occluded, then skip

			VectorPack16 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z;
			VectorPack16 texturePixels = texture.gatherPixels512(uvCorrected.x, uvCorrected.y, visiblePointsMask);
			Mask16 opaquePixelsMask = visiblePointsMask & texturePixels.a > 0.0f;

			if (!depthOnly)
			{
				VectorPack16 worldCoords = VectorPack16(tv[0].worldCoords) * alpha + VectorPack16(tv[1].worldCoords) * beta + VectorPack16(tv[2].worldCoords) * gamma;
				worldCoords /= interpolatedDividedUv.z;
				worldCoords.w = 1;

				VectorPack16 dynaLight = 0;
				/*/
				if (false)
				{
					for (const auto& it : *context.pointLights)
					{
						FloatPack16 distSquared = (worldCoords - it.pos).lenSq3d();
						Vec4 power = it.color * it.intensity;
						dynaLight += VectorPack16(power) / distSquared;
					}
				}*/

				Vec4 shadowLightColorMults = Vec4(1, 1, 1) * 1.5;
				Vec4 shadowDarkColorMults = shadowLightColorMults * 0.2;
				VectorPack16 shadowColorMults = 0;

				for (const auto& it :  this->shadowMaps)
				{
					const auto& currentShadowMap = *it;
					VectorPack16 sunWorldPositions = currentShadowMap.ctr.getCurrentTransformationMatrix() * worldCoords;
					FloatPack16 zInv = FloatPack16(currentShadowMap.fovMult) / sunWorldPositions.z;
					VectorPack16 sunScreenPositions = currentShadowMap.ctr.screenSpaceToPixels(sunWorldPositions * zInv);
					sunScreenPositions.z = zInv;

					Mask16 inShadowMapBounds = currentShadowMap.depthBuffer.checkBounds(sunScreenPositions.x, sunScreenPositions.y);
					Mask16 shadowMapDepthGatherMask = inShadowMapBounds & opaquePixelsMask;

					FloatPack16 shadowMapDepths = currentShadowMap.depthBuffer.gatherPixels16(_mm512_cvttps_epi32(sunScreenPositions.x), _mm512_cvttps_epi32(sunScreenPositions.y), shadowMapDepthGatherMask);
					float shadowMapBias = 1.f / 10e6;
					//float shadowMapBias = 0;
					Mask16 pointsInShadow = ~inShadowMapBounds | shadowMapDepths < (sunScreenPositions.z - shadowMapBias);
					shadowColorMults.r += _mm512_mask_blend_ps(pointsInShadow, FloatPack16(shadowLightColorMults.x), FloatPack16(shadowDarkColorMults.x));
					shadowColorMults.g += _mm512_mask_blend_ps(pointsInShadow, FloatPack16(shadowLightColorMults.y), FloatPack16(shadowDarkColorMults.y));
					shadowColorMults.b += _mm512_mask_blend_ps(pointsInShadow, FloatPack16(shadowLightColorMults.z), FloatPack16(shadowDarkColorMults.z));
				}

				texturePixels = (texturePixels * adjustedLight) * (dynaLight + shadowColorMults);
				if (this->currFrameGameSettings.wireframeEnabled)
				{
					Mask16 visibleEdgeMaskAlpha = visiblePointsMask & alpha <= 0.01;
					Mask16 visibleEdgeMaskBeta = visiblePointsMask & beta <= 0.01;
					Mask16 visibleEdgeMaskGamma = visiblePointsMask & gamma <= 0.01;
					Mask16 total = visibleEdgeMaskAlpha | visibleEdgeMaskBeta | visibleEdgeMaskGamma;

					texturePixels.r = _mm512_mask_blend_ps(visibleEdgeMaskAlpha, texturePixels.r, _mm512_set1_ps(1));
					texturePixels.g = _mm512_mask_blend_ps(visibleEdgeMaskBeta, texturePixels.g, _mm512_set1_ps(1));
					texturePixels.b = _mm512_mask_blend_ps(visibleEdgeMaskGamma, texturePixels.b, _mm512_set1_ps(1));
					texturePixels.a = _mm512_mask_blend_ps(total, texturePixels.a, _mm512_set1_ps(1));
					//lightMult = _mm512_mask_blend_ps(visibleEdgeMask, lightMult, _mm512_set1_ps(1));
				}

				this->frameBuf.setPixels16(xInt, yInt, texturePixels, opaquePixelsMask);
				if (this->currFrameGameSettings.fogEnabled) this->pixelWorldPosBuf.setPixels16(xInt, yInt, worldCoords, opaquePixelsMask);
			}

			this->zBuffer.setPixels16(xInt, yInt, interpolatedDividedUv.z, opaquePixelsMask);
		}
	}
}

void RasterizationRenderer::addShadowMap(const ShadowMap& m)
{
	this->shadowMaps.push_back(&m);
}

void RasterizationRenderer::removeShadowMaps()
{
	this->shadowMaps.clear();
}
