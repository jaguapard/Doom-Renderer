#include "DepthTextureRenderShader.h"

void DepthTextureRenderShader::run(MainFragmentRenderInput& input)
{
	for (const auto& it : *input.renderJobs)
	{
		this->drawRenderJobSlice(input.ctx, it, input.zoneMinY, input.zoneMaxY);
	}
}

void DepthTextureRenderShader::drawRenderJobSlice(const TriangleRenderContext& context, const RenderJob& renderJob, int zoneMinY, int zoneMaxY) const
{
	auto boundingBox = getRenderJobSliceBoundingBox(renderJob, zoneMinY, zoneMaxY, 0, context.framebufW - 1);
	if (!boundingBox) return;

	real yBeg = boundingBox.value().minY;
	real yEnd = boundingBox.value().maxY;
	real xBeg = boundingBox.value().minX;
	real xEnd = boundingBox.value().maxX;

	const Texture& texture = context.gameSettings.textureManager->getTextureByIndex(renderJob.textureIndex); //texture is still required - need to know whether there are transparent pixels
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