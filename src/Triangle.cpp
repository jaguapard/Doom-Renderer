#include "Triangle.h"
#include "Statsman.h"

#include <functional>
#include "ShadowMap.h"

void Triangle::sortByAscendingSpaceX()
{
	std::sort(std::begin(tv), std::end(tv), [&](TexVertex& tv1, TexVertex& tv2) {return tv1.spaceCoords.x < tv2.spaceCoords.x; });
}

void Triangle::sortByAscendingSpaceY()
{
	std::sort(std::begin(tv), std::end(tv), [&](TexVertex& tv1, TexVertex& tv2) {return tv1.spaceCoords.y < tv2.spaceCoords.y; });
}

void Triangle::sortByAscendingSpaceZ()
{
	std::sort(std::begin(tv), std::end(tv), [&](TexVertex& tv1, TexVertex& tv2) {return tv1.spaceCoords.z < tv2.spaceCoords.z; });
}

void Triangle::sortByAscendingTextureX()
{
	std::sort(std::begin(tv), std::end(tv), [&](TexVertex& tv1, TexVertex& tv2) {return tv1.textureCoords.x < tv2.textureCoords.x; });
}

void Triangle::sortByAscendingTextureY()
{
	std::sort(std::begin(tv), std::end(tv), [&](TexVertex& tv1, TexVertex& tv2) {return tv1.textureCoords.y < tv2.textureCoords.y; });
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

void Triangle::addToRenderQueue(const TriangleRenderContext& context) const
{
	Triangle rotated;
	for (int i = 0; i < 3; ++i)
	{
		Vec4 spaceCopy = tv[i].spaceCoords;
		spaceCopy.w = 1;

		rotated.tv[i].spaceCoords = context.ctr->rotateAndTranslate(spaceCopy);
		rotated.tv[i].textureCoords = tv[i].textureCoords;
		rotated.tv[i].worldCoords = tv[i].spaceCoords;
	}

	if (context.gameSettings.backfaceCullingEnabled)
	{
		Vec4 normal = rotated.getNormalVector();
		if (rotated.tv[0].spaceCoords.dot(normal) >= 0) return;
	}

	Triangle t[2];
	int outsideVertexCount;
	int trianglesOut = doTriangleClipping(rotated, context.gameSettings.nearPlaneZ, (Triangle*)&t, &outsideVertexCount);
	StatCount(statsman.triangles.verticesOutside[outsideVertexCount]++);
	for (int i = 0; i < trianglesOut; ++i) t[i].prepareScreenSpace(context);
}

std::pair<Triangle, Triangle> Triangle::pairFromRect(std::array<TexVertex, 4> rectPoints)
{
	Triangle t[2];
	constexpr int vertInds[2][3] = {{0,1,2}, {0,3,2}};

	std::function sortFunc = [&](const TexVertex& tv1, const TexVertex& tv2) {return tv1.spaceCoords.x < tv2.spaceCoords.x; }; 
	//TODO: make sure this guarantees no overlaps
	std::sort(rectPoints.begin(), rectPoints.end(), sortFunc);
	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			t[i].tv[j] = rectPoints[vertInds[i][j]];
		}
	}
	return std::make_pair(t[0], t[1]);
}

real _3min(real a, real b, real c)
{
	return std::min(a, std::min(b, c));
}
real _3max(real a, real b, real c)
{
	return std::max(a, std::max(b, c));
}

//WARNING: this method expects tv to contain rotated (but not yet z-divided coords)!
void Triangle::prepareScreenSpace(const TriangleRenderContext& context) const
{
	Triangle screenSpaceTriangle;
	for (int i = 0; i < 3; ++i)
	{
		real zInv = context.gameSettings.fovMult / tv[i].spaceCoords.z;
		screenSpaceTriangle.tv[i].spaceCoords = tv[i].spaceCoords * zInv;
		screenSpaceTriangle.tv[i].textureCoords = tv[i].textureCoords * zInv;
		screenSpaceTriangle.tv[i].worldCoords = tv[i].worldCoords * zInv;
		screenSpaceTriangle.tv[i].spaceCoords = context.ctr->screenSpaceToPixels(screenSpaceTriangle.tv[i].spaceCoords);
		screenSpaceTriangle.tv[i].textureCoords.z = zInv;
	}

	real y1 = screenSpaceTriangle.tv[0].spaceCoords.y;
	real y2 = screenSpaceTriangle.tv[1].spaceCoords.y;
	real y3 = screenSpaceTriangle.tv[2].spaceCoords.y;
	if (_3min(y1, y2, y3) == _3max(y1, y2, y3)) return; //avoid divisions by 0. 0 height triangle is nonsensical anyway
	
	screenSpaceTriangle.addToRenderQueueFinal(context);
}

void Triangle::addToRenderQueueFinal(const TriangleRenderContext& context) const
{
	const Vec4 r1 = tv[0].spaceCoords;
	const Vec4 r2 = tv[1].spaceCoords;
	const Vec4 r3 = tv[2].spaceCoords;

	real signedArea = (r1 - r3).cross2d(r2 - r3);
	if (signedArea == 0.0) return;

	RenderJob& rj = context.renderJobs->emplace_back();
	rj.rcpSignedArea = 1.0 / signedArea;
	rj.originalTriangle = *this;

	real screenMinX = _3min(r1.x, r2.x, r3.x);
	real screenMaxX = _3max(r1.x, r2.x, r3.x);
	real screenMinY = _3min(r1.y, r2.y, r3.y);
	real screenMaxY = _3max(r1.y, r2.y, r3.y);

	rj.minX = floor(screenMinX);
	rj.maxX = ceil(screenMaxX);
	rj.minY = floor(screenMinY);
	rj.maxY = ceil(screenMaxY);
	rj.lightMult = context.lightMult;
	rj.textureIndex = context.textureIndex;

	real xSpan = screenMaxX - screenMinX;
	real ySpan = screenMaxY - screenMinY;	
}

inline std::tuple<FloatPack16, FloatPack16, FloatPack16> calculateBarycentricCoordinates(const VectorPack16& r, const Vec4& r1, const Vec4& r2, const Vec4& r3, const real& rcpSignedArea)
{
	return {
		(r - r3).cross2d(r2 - r3) * rcpSignedArea,
		(r - r3).cross2d(r3 - r1) * rcpSignedArea,
		(r - r1).cross2d(r1 - r2) * rcpSignedArea //do NOT change this to 1-alpha-beta or 1-(alpha+beta). That causes wonkiness in textures
	};
}

void Triangle::drawSlice(const TriangleRenderContext& context, const RenderJob& renderJob, int zoneMinY, int zoneMaxY) const
{
	if (renderJob.minY >= zoneMaxY || renderJob.maxY < zoneMinY) return;
	real yBeg = std::clamp<real>(renderJob.minY, zoneMinY, zoneMaxY - 1);
	real yEnd = std::clamp<real>(renderJob.maxY, zoneMinY, zoneMaxY - 1);
	real xBeg = std::clamp<real>(renderJob.minX, 0, context.framebufW - 1);
	real xEnd = std::clamp<real>(renderJob.maxX, 0, context.framebufW - 1);

	const Texture& texture = context.gameSettings.textureManager->getTextureByIndex(renderJob.textureIndex);
	auto& depthBuf = *context.zBuffer;

	for (real y = yBeg; y <= yEnd; ++y)
	{
		size_t yInt = y;
		//the loop increment section is fairly busy because it's body can be interrupted at various steps, but all increments must always happen
		for (FloatPack16 x = FloatPack16::sequence() + xBeg; Mask16 loopBoundsMask = x <= xEnd; x += 16)
		{
			size_t xInt = x[0];
			VectorPack16 r = VectorPack16(x, y, 0.0, 0.0);
			auto [alpha, beta, gamma] = calculateBarycentricCoordinates(r, tv[0].spaceCoords, tv[1].spaceCoords, tv[2].spaceCoords, renderJob.rcpSignedArea);

			Mask16 pointsInsideTriangleMask = loopBoundsMask & alpha >= 0.0 & beta >= 0.0 & gamma >= 0.0;
			if (!pointsInsideTriangleMask) continue;

			VectorPack16 interpolatedDividedUv = this->interpolateTextureCoords(alpha, beta, gamma);
			FloatPack16 currDepthValues = depthBuf.getPixels16(xInt, yInt);
			Mask16 visiblePointsMask = pointsInsideTriangleMask & currDepthValues > interpolatedDividedUv.z;
			if (!visiblePointsMask) continue; //if all points are occluded, then skip

			VectorPack16 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z;
			VectorPack16 texturePixels = texture.gatherPixels512(uvCorrected.x, uvCorrected.y, visiblePointsMask);
			Mask16 opaquePixelsMask = visiblePointsMask & texturePixels.a > 0.0f;

			if (!context.renderingShadowMap)
			{
				VectorPack16 worldCoords = this->interpolateWorldCoords(alpha, beta, gamma);
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
				dynaLight.a = 1;
	
				Vec4 shadowLightColorMults = Vec4(1, 1, 1) * 1.5;
				Vec4 shadowDarkColorMults = shadowLightColorMults * 0.2;
				VectorPack16 shadowColorMults = 0;
				//shadowColorMults.a = 0.f;

				const ShadowMap& currentShadowMap = (*context.shadowMaps)[0];
				//if (s1.z != 0 && s2.z != 0 && s3.z != 0)
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
				shadowColorMults.a = 1;

				texturePixels = (texturePixels * renderJob.lightMult) * (dynaLight + shadowColorMults);
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

				context.frameBuffer->setPixels16(xInt,yInt, texturePixels, opaquePixelsMask);
				if (context.gameSettings.fogEnabled) context.pixelWorldPos->setPixels16(xInt,yInt, worldCoords, opaquePixelsMask);
			}

			depthBuf.setPixels16(xInt, yInt, interpolatedDividedUv.z, opaquePixelsMask);
		}
	}
}

Vec4 Triangle::getNormalVector() const
{
	return (tv[2].spaceCoords - tv[0].spaceCoords).cross3d(tv[1].spaceCoords - tv[0].spaceCoords);
}

VectorPack16 Triangle::interpolateSpaceCoords(const FloatPack16& alpha, const FloatPack16& beta, const FloatPack16& gamma) const
{
	return VectorPack16(tv[0].spaceCoords) * alpha + VectorPack16(tv[1].spaceCoords) * beta + VectorPack16(tv[2].spaceCoords) * gamma;
}


VectorPack16 Triangle::interpolateTextureCoords(const FloatPack16& alpha, const FloatPack16& beta, const FloatPack16& gamma) const
{
	return VectorPack16(tv[0].textureCoords) * alpha + VectorPack16(tv[1].textureCoords) * beta + VectorPack16(tv[2].textureCoords) * gamma;
}

VectorPack16 Triangle::interpolateWorldCoords(const FloatPack16& alpha, const FloatPack16& beta, const FloatPack16& gamma) const
{
	return VectorPack16(tv[0].worldCoords) * alpha + VectorPack16(tv[1].worldCoords) * beta + VectorPack16(tv[2].worldCoords) * gamma;
}

