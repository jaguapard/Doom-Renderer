#include "MainFragmentRenderShader.h"
#include "../ShadowMap.h"

void MainFragmentRenderShader::run(MainFragmentRenderInput& input)
{
	for (const auto& it : *input.renderJobs)
	{
		this->drawRenderJobSlice(input.ctx, it, input.zoneMinY, input.zoneMaxY);
	}
}

std::optional<RenderJob::BoundingBox> MainFragmentRenderShader::getRenderJobSliceBoundingBox(const RenderJob& renderJob, int zoneMinY, int zoneMaxY, real xMin, real xMax)
{
	if (renderJob.boundingBox.minY >= zoneMaxY || renderJob.boundingBox.maxY < zoneMinY) return {};
	real yBeg = std::clamp<real>(renderJob.boundingBox.minY, zoneMinY, zoneMaxY - 1);
	real yEnd = std::clamp<real>(renderJob.boundingBox.maxY, zoneMinY, zoneMaxY - 1);
	real xBeg = std::clamp<real>(renderJob.boundingBox.minX, xMin, xMax);
	real xEnd = std::clamp<real>(renderJob.boundingBox.maxX, xMin, xMax);

	RenderJob::BoundingBox adjustedBox;
	adjustedBox.minY = yBeg;
	adjustedBox.maxY = yEnd;
	adjustedBox.minX = xBeg;
	adjustedBox.maxX = xEnd;
	return adjustedBox;
}

void MainFragmentRenderShader::drawRenderJobSlice(const TriangleRenderContext& context, const RenderJob& renderJob, int zoneMinY, int zoneMaxY) const
{
	auto boundingBox = getRenderJobSliceBoundingBox(renderJob, zoneMinY, zoneMaxY, 0, context.framebufW - 1);
	if (!boundingBox) return;

	real yBeg = boundingBox.value().minY;
	real yEnd = boundingBox.value().maxY;
	real xBeg = boundingBox.value().minX;
	real xEnd = boundingBox.value().maxX;

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

			//if (!context.renderingShadowMap)
			{
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
					FloatPack16 zInv = FloatPack16(context.gameSettings.fovMult) / sunWorldPositions.z;
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
			}

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
