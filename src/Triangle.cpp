#include "Triangle.h"
#include "Statsman.h"

#include <functional>

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

void Triangle::addToRenderQueue(const TriangleRenderContext& context) const
{
	Triangle rotated;
	bool vertexOutside[3] = { false };
	int outsideVertexCount = 0;
	for (int i = 0; i < 3; ++i)
	{
		rotated.tv[i].spaceCoords = context.ctr->rotateAndTranslate(tv[i].spaceCoords);
		rotated.tv[i].textureCoords = tv[i].textureCoords;
		rotated.tv[i].worldCoords = tv[i].spaceCoords;

		if (rotated.tv[i].spaceCoords.z > context.nearPlaneClippingZ)
		{
			outsideVertexCount++;
			vertexOutside[i] = true;
		}
	}

	StatCount(statsman.triangles.verticesOutside[outsideVertexCount]++);
	if (outsideVertexCount == 3) return;

	if (context.backfaceCullingEnabled)
	{
		Vec4 normal = rotated.getNormalVector();
		if (rotated.tv[0].spaceCoords.dot(normal) >= 0)
		{
			StatCount(statsman.triangles.verticesOutside[outsideVertexCount]--); //if a triangle is culled by backface culling, it will not get a chance to be split, so stats will be wrong
			return;
		}
	}

	if (outsideVertexCount == 0) return rotated.prepareScreenSpace(context); //all vertices are in front of camera, prepare data for drawRotationPrepped and proceed

	//search for one of a kind vertex (outside if outsideVertexCount==1, else inside)
	int i = std::find(std::begin(vertexOutside), std::end(vertexOutside), outsideVertexCount == 1) - std::begin(vertexOutside);
	int v1_ind = i > 0 ? i - 1 : 2; //preserve vertice order of the original triangle and prevent out of bounds lookups
	int v2_ind = i < 2 ? i + 1 : 0;
	const TexVertex& v1 = rotated.tv[v1_ind];
	const TexVertex& v2 = rotated.tv[v2_ind];

	if (outsideVertexCount == 1) //if 1 vertice is outside, then 1 triangle gets turned into two
	{
		TexVertex clipped1 = rotated.tv[i].getClipedToPlane(v1, context.nearPlaneClippingZ);
		TexVertex clipped2 = rotated.tv[i].getClipedToPlane(v2, context.nearPlaneClippingZ);

		Triangle t1 = { v1,       clipped1, v2 };
		Triangle t2 = { clipped1, clipped2, v2 };

		t1.prepareScreenSpace(context);
		t2.prepareScreenSpace(context);
	}

	if (outsideVertexCount == 2) //in case there are 2 vertices that are outside, the triangle just gets clipped (no new triangles needed)
	{
		Triangle clipped;
		clipped.tv[v1_ind] = v1.getClipedToPlane(rotated.tv[i], context.nearPlaneClippingZ);
		clipped.tv[v2_ind] = v2.getClipedToPlane(rotated.tv[i], context.nearPlaneClippingZ);
		clipped.tv[i] = rotated.tv[i];
		clipped.prepareScreenSpace(context);
	}
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

//WARNING: this method expects tv to contain rotated (but not yet z-divided coords)!
void Triangle::prepareScreenSpace(const TriangleRenderContext& context) const
{
	Triangle screenSpaceTriangle;
	for (int i = 0; i < 3; ++i)
	{
		real zInv = context.fovMult / tv[i].spaceCoords.z;
		screenSpaceTriangle.tv[i].spaceCoords = tv[i].spaceCoords * zInv;
		screenSpaceTriangle.tv[i].textureCoords = tv[i].textureCoords * zInv;

		screenSpaceTriangle.tv[i].worldCoords = tv[i].worldCoords * zInv;

		screenSpaceTriangle.tv[i].spaceCoords = context.ctr->screenSpaceToPixels(screenSpaceTriangle.tv[i].spaceCoords);
		screenSpaceTriangle.tv[i].textureCoords.z = zInv;
		
	}

	//we need to sort by triangle's screen Y (ascending) for later flat top and bottom splits
	screenSpaceTriangle.sortByAscendingSpaceY();
	if (screenSpaceTriangle.tv[2].spaceCoords.y - screenSpaceTriangle.tv[0].spaceCoords.y == 0) return; //avoid divisions by 0. 0 height triangle is nonsensical anyway
	
	screenSpaceTriangle.addToRenderQueueFinal(context);
}

void Triangle::addToRenderQueueFinal(const TriangleRenderContext& context) const
{
	RenderJob rj;
	rj.tStart = tv[0];
	rj.span1 = tv[1] - tv[0];
	rj.span2 = tv[2] - tv[0];
	rj.signedArea = rj.span1.spaceCoords.cross2d(rj.span2.spaceCoords);
	rj.originalTriangle = *this;

	Triangle copy = *this;
	copy.sortByAscendingSpaceX();
	rj.minX = copy.tv[0].spaceCoords.x;
	rj.maxX = copy.tv[2].spaceCoords.x;

	copy.sortByAscendingSpaceY();
	rj.minY = copy.tv[0].spaceCoords.y;
	rj.maxY = copy.tv[2].spaceCoords.y;

	rj.lightMult = context.lightMult;
	rj.textureIndex = context.textureIndex;
	context.renderJobs->push_back(rj);
}


void Triangle::drawSlice(const TriangleRenderContext& context, const RenderJob& renderJob, int zoneMinY, int zoneMaxY) const
{
	if (renderJob.minY >= zoneMaxY || renderJob.maxY < zoneMinY) return;
	real yBeg = std::clamp<real>(renderJob.minY, zoneMinY, zoneMaxY);
	real yEnd = std::clamp<real>(renderJob.maxY, zoneMinY, zoneMaxY);
	real xBeg = std::clamp<real>(renderJob.minX, 0, context.framebufW);
	real xEnd = std::clamp<real>(renderJob.maxX, 0, context.framebufW);

	const Texture& texture = context.textureManager->getTextureByIndex(renderJob.textureIndex);
	auto& frameBuf = *context.frameBuffer;
	auto& lightBuf = *context.lightBuffer;
	auto& depthBuf = *context.zBuffer;
	int bufW = frameBuf.getW(); //save to avoid constant memory reads. Buffers don't change in size while rendering.

	const Vec4 r1 = tv[0].spaceCoords;
	const Vec4 r2 = tv[1].spaceCoords;
	const Vec4 r3 = tv[2].spaceCoords;

	real signedArea = (r1 - r3).cross2d(r2 - r3);
	if (signedArea == 0.0) return;

	for (real y = yBeg; y < yEnd; ++y)
	{
		size_t pixelIndex = size_t(y) * bufW + size_t(xBeg); //all buffers have the same size, so we can use a single index

		//the loop increment section is fairly busy because it's body can be interrupted at various steps, but all increments must always happen
		for (FloatPack16 x = FloatPack16::sequence() + floor(xBeg);
			Mask16 loopBoundsMask = x < ceil(xEnd); 
			x += 16, pixelIndex += 16)
		{
			VectorPack16 r = VectorPack16(x, y, 0.0, 0.0);
			FloatPack16 alpha = (r - r3).cross2d(r2 - r3) / signedArea;
			FloatPack16 beta = (r - r3).cross2d(r3 - r1) / signedArea;
			FloatPack16 gamma = (r - r1).cross2d(r1 - r2) / signedArea;
			Mask16 pointsInsideTriangleMask = loopBoundsMask & alpha >= 0.0 & beta >= 0.0 & gamma >= 0.0;
			if (!pointsInsideTriangleMask) continue;

			VectorPack16 interpolatedDividedUv = VectorPack16(tv[0].textureCoords) * alpha + VectorPack16(tv[1].textureCoords) * beta + VectorPack16(tv[2].textureCoords) * gamma;

			FloatPack16 currDepthValues = &depthBuf[pixelIndex];
			Mask16 visiblePointsMask = pointsInsideTriangleMask & currDepthValues > interpolatedDividedUv.z;
			if (!visiblePointsMask) continue; //if all points are occluded, then skip

			VectorPack16 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z;
			VectorPack16 texturePixels = texture.gatherPixels512(uvCorrected.x, uvCorrected.y, visiblePointsMask);
			Mask16 opaquePixelsMask = visiblePointsMask & texturePixels.a > 0.0f;
			
			VectorPack16 worldCoords = VectorPack16(tv[0].worldCoords) * alpha + VectorPack16(tv[1].worldCoords) * beta + VectorPack16(tv[2].worldCoords) * gamma;
            worldCoords /= interpolatedDividedUv.z;
			//FloatPack16 distSquared = (worldCoords - context.camPos).lenSq3d();
			//FloatPack16 distSquared = (worldCoords - Vec4(580, 250, -1015)).lenSq3d(); //expected result: light hanging in the air at this pos

			


			//VectorPack16 dynaLight = (VectorPack16(Vec4(1, 0.7, 0.4, 1)) * 1e6) / distSquared;
			VectorPack16 dynaLight = 0;
            for (const auto& it : *context.pointLights)
            {
                FloatPack16 distSquared = (worldCoords - it.pos).lenSq3d();
                Vec4 power = it.color * it.intensity;
                dynaLight += VectorPack16(power) / distSquared;
            }

			texturePixels = texturePixels * (dynaLight + renderJob.lightMult);
            if (context.wireframeEnabled)
            {
                __mmask16 visibleEdgeMaskAlpha = visiblePointsMask & alpha <= 0.01;
                __mmask16 visibleEdgeMaskBeta = visiblePointsMask & beta <= 0.01;
                __mmask16 visibleEdgeMaskGamma = visiblePointsMask & gamma <= 0.01;
                __mmask16 total = visibleEdgeMaskAlpha | visibleEdgeMaskBeta | visibleEdgeMaskGamma;

                texturePixels.r = _mm512_mask_blend_ps(visibleEdgeMaskAlpha, texturePixels.r, _mm512_set1_ps(1));
                texturePixels.g = _mm512_mask_blend_ps(visibleEdgeMaskBeta, texturePixels.g, _mm512_set1_ps(1));
                texturePixels.b = _mm512_mask_blend_ps(visibleEdgeMaskGamma, texturePixels.b, _mm512_set1_ps(1));
                texturePixels.a = _mm512_mask_blend_ps(total, texturePixels.a, _mm512_set1_ps(1));
                //lightMult = _mm512_mask_blend_ps(visibleEdgeMask, lightMult, _mm512_set1_ps(1));
            }

			_mm512_mask_store_ps(&depthBuf[pixelIndex], opaquePixelsMask, interpolatedDividedUv.z);
			frameBuf.storePixels16(pixelIndex, texturePixels, opaquePixelsMask);
		}
	}
}

Vec4 Triangle::getNormalVector() const
{
	return (tv[2].spaceCoords - tv[0].spaceCoords).cross3d(tv[1].spaceCoords - tv[0].spaceCoords);
}