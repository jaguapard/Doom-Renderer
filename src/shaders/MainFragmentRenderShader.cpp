#include "MainFragmentRenderShader.h"
#include "../ShadowMap.h"

void MainFragmentRenderShader::run(MainFragmentRenderInput& input)
{
	assert(input.zoneMinY == floor(input.zoneMinY));
	assert(input.zoneMaxY == floor(input.zoneMaxY));

	RenderJob::BoundingBox threadBounds;
	threadBounds.minX = 0;
	threadBounds.minY = input.zoneMinY;
	threadBounds.maxX = input.ctx.framebufW - 1;
	threadBounds.maxY = input.zoneMaxY;
	bool depthOnly = input.renderDepthTextureOnly;
	for (const auto& it : *input.renderJobs)
	{
		auto boundingBox = getRenderJobSliceBoundingBox(it, threadBounds);
		if (!boundingBox) continue;
		if (!depthOnly) this->drawRenderJobSlice(input.ctx, it, boundingBox.value());
		else this->drawRenderJobSliceDepthOnly(input.ctx, it, boundingBox.value());
	}
}

std::optional<RenderJob::BoundingBox> MainFragmentRenderShader::getRenderJobSliceBoundingBox(const RenderJob& renderJob, const RenderJob::BoundingBox& threadBounds)
{
	if (renderJob.boundingBox.minY >= threadBounds.maxY || renderJob.boundingBox.maxY < threadBounds.minY) return {};

	//real minX, minY, maxX, maxY;
	__m128 original = _mm_loadu_ps(&renderJob.boundingBox.minX);
	__m128 tb = _mm_loadu_ps(&threadBounds.minX);
	__m128 lows = _mm_shuffle_ps(tb, tb, _MM_SHUFFLE(1, 0, 1, 0));
	__m128 highs = _mm_shuffle_ps(tb, tb, _MM_SHUFFLE(3, 2, 3, 2));

	__m128 clampedLow = _mm_max_ps(lows, original);
	__m128 clampedHigh = _mm_min_ps(highs, clampedLow);
	RenderJob::BoundingBox adjustedBox;
	_mm_storeu_ps(&adjustedBox.minX, clampedHigh);
	return adjustedBox;
}

void MainFragmentRenderShader::drawRenderJobSlice(const TriangleRenderContext& context, const RenderJob& renderJob, const RenderJob::BoundingBox& boundingBoxOverride) const
{
	real yBeg = boundingBoxOverride.minY;
	real yEnd = boundingBoxOverride.maxY;
	real xBeg = boundingBoxOverride.minX;
	real xEnd = boundingBoxOverride.maxX;

	const Texture& texture = context.gameSettings.textureManager->getTextureByIndex(renderJob.pModel->textureIndex);
	auto& depthBuf = *context.zBuffer;
	const auto& tv = renderJob.originalTriangle.tv;
	real adjustedLight = renderJob.pModel->lightMult ? powf(renderJob.pModel->lightMult.value(), context.gameSettings.gamma) : context.gameSettings.gamma;

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
			FloatPack16 currDepthValues = depthBuf.getPixels16(xInt, yInt);
			Mask16 visiblePointsMask = pointsInsideTriangleMask & currDepthValues > interpolatedDividedUv.z;
			if (!visiblePointsMask) continue; //if all points are occluded, then skip

			VectorPack16 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z;
			VectorPack16 texturePixels = texture.gatherPixels512(uvCorrected.x, uvCorrected.y, visiblePointsMask);
			Mask16 opaquePixelsMask = visiblePointsMask & texturePixels.a > 0.0f;

			VectorPack16 worldCoords = VectorPack16(tv[0].worldCoords) * alpha + VectorPack16(tv[1].worldCoords) * beta + VectorPack16(tv[2].worldCoords) * gamma;
			worldCoords /= interpolatedDividedUv.z;
			worldCoords.w = 1;

			VectorPack16 dynaLight = 0;
			if (false)
			{
				for (const auto& it : *context.pointLights)
				{
					FloatPack16 distSquared = (worldCoords - it.pos).lenSq3d();
					Vec4 power = it.color * it.intensity;
					dynaLight += VectorPack16(power) / distSquared;
				}
			}

			Vec4 shadowLightColorMults = Vec4(1, 1, 1) * 1.5;
			Vec4 shadowDarkColorMults = shadowLightColorMults * 0.2;
			VectorPack16 shadowColorMults = 0;

			for (const auto& currentShadowMap : *context.shadowMaps)
			{
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
			if (context.gameSettings.wireframeEnabled)
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

			context.frameBuffer->setPixels16(xInt, yInt, texturePixels, opaquePixelsMask);
			if (context.gameSettings.fogEnabled) context.pixelWorldPos->setPixels16(xInt, yInt, worldCoords, opaquePixelsMask);

			depthBuf.setPixels16(xInt, yInt, interpolatedDividedUv.z, opaquePixelsMask);
		}
	}
}

void MainFragmentRenderShader::drawRenderJobSliceDepthOnly(const TriangleRenderContext& context, const RenderJob& renderJob, const RenderJob::BoundingBox& boundingBoxOverride) const
{
	real yBeg = boundingBoxOverride.minY;
	real yEnd = boundingBoxOverride.maxY;
	real xBeg = boundingBoxOverride.minX;
	real xEnd = boundingBoxOverride.maxX;

	const Texture& texture = context.gameSettings.textureManager->getTextureByIndex(renderJob.pModel->textureIndex); //texture is still required - need to know whether there are transparent pixels
	auto& depthBuf = *context.zBuffer;
	const auto& tv = renderJob.originalTriangle.tv;

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
			FloatPack16 currDepthValues = depthBuf.getPixels16(xInt, yInt);
			Mask16 visiblePointsMask = pointsInsideTriangleMask & currDepthValues > interpolatedDividedUv.z;
			if (!visiblePointsMask) continue; //if all points are occluded, then skip

			VectorPack16 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z;
			VectorPack16 texturePixels = texture.gatherPixels512(uvCorrected.x, uvCorrected.y, visiblePointsMask);
			Mask16 opaquePixelsMask = visiblePointsMask & texturePixels.a > 0.0f;

			depthBuf.setPixels16(xInt, yInt, interpolatedDividedUv.z, opaquePixelsMask);
		}
	}
}

std::tuple<FloatPack16, FloatPack16, FloatPack16> RenderHelpers::calculateBarycentricCoordinates(const VectorPack16& r, const Vec4& r1, const Vec4& r2, const Vec4& r3, const real& rcpSignedArea)
{
	return {
		(r - r3).cross2d(r2 - r3) * rcpSignedArea,
		(r - r3).cross2d(r3 - r1) * rcpSignedArea,
		(r - r1).cross2d(r1 - r2) * rcpSignedArea //do NOT change this to 1-alpha-beta or 1-(alpha+beta). That causes wonkiness in textures
	};
}
