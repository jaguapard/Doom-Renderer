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
		screenSpaceTriangle.tv[i].worldCoords = tv[i].worldCoords;

		screenSpaceTriangle.tv[i].spaceCoords = context.ctr->screenSpaceToPixels(screenSpaceTriangle.tv[i].spaceCoords);
		screenSpaceTriangle.tv[i].textureCoords.z = zInv;
	}

	//we need to sort by triangle's screen Y (ascending) for later flat top and bottom splits
	screenSpaceTriangle.sortByAscendingSpaceY();
	if (screenSpaceTriangle.tv[2].spaceCoords.y - screenSpaceTriangle.tv[0].spaceCoords.y == 0) return; //avoid divisions by 0. 0 height triangle is nonsensical anyway

	real splitAlpha = (screenSpaceTriangle.tv[1].spaceCoords.y - screenSpaceTriangle.tv[0].spaceCoords.y) / (screenSpaceTriangle.tv[2].spaceCoords.y - screenSpaceTriangle.tv[0].spaceCoords.y);
	TexVertex splitVertex = lerp(screenSpaceTriangle.tv[0], screenSpaceTriangle.tv[2], splitAlpha);

	TexVertex v2_copy = screenSpaceTriangle.tv[2];
	screenSpaceTriangle.tv[2] = splitVertex;
	screenSpaceTriangle.addToRenderQueueFinal(context, false); //flat bottom part

	//screenSpaceTriangle.tv = { screenSpaceTriangle.tv[1], splitVertex, v2_copy };
	screenSpaceTriangle.tv[0] = splitVertex;
	screenSpaceTriangle.tv[2] = v2_copy;
	screenSpaceTriangle.addToRenderQueueFinal(context, true); //flat top part
}

void Triangle::addToRenderQueueFinal(const TriangleRenderContext& context, bool flatTop) const
{
	RenderJob rj;
	rj.flatTop = flatTop;
	rj.t = *this;
	rj.lightMult = context.lightMult;
	rj.textureIndex = context.textureIndex;
	context.renderJobs->push_back(rj);
}


void Triangle::drawSlice(const TriangleRenderContext& context, const RenderJob& renderJob, int zoneMinY, int zoneMaxY) const
{
	//Scanline rasterization algorithm
	if (tv[0].spaceCoords.y >= zoneMaxY || tv[2].spaceCoords.y < zoneMinY) return;
	real yBeg = std::clamp<real>(tv[0].spaceCoords.y, zoneMinY, zoneMaxY);
	real yEnd = std::clamp<real>(tv[2].spaceCoords.y, zoneMinY, zoneMaxY);

	real ySpan = tv[2].spaceCoords.y - tv[0].spaceCoords.y; //since this function draws only flat top or flat bottom triangles, either y1 == y2 or y2 == y3. y3-y1 ensures we don't get 0. 0 height triangles are culled in previous stage 
	real yp = (yBeg - tv[0].spaceCoords.y) / ySpan;
	real ypStep = 1.0 / ySpan;

	const Texture& texture = context.textureManager->getTextureByIndex(renderJob.textureIndex);
	bool flatTop = renderJob.flatTop;
	auto& frameBuf = *context.frameBuffer;
	auto& lightBuf = *context.lightBuffer;
	auto& depthBuf = *context.zBuffer;
	int bufW = frameBuf.getW(); //save to avoid constant memory reads. Buffers don't change in size while rendering.

	FloatPack16 sequence_float = FloatPack16::sequence();
	FloatPack16 lightMult = renderJob.lightMult;

	for (real y = yBeg; y < yEnd; ++y, yp += ypStep) //draw flat bottom part
	{
		TexVertex leftTv = lerp(tv[0], tv[flatTop + 1], yp); //flat top and flat bottom triangles require different interpolation points
		TexVertex rightTv = lerp(tv[flatTop], tv[2], yp); //using a flag passed from the "cooking" step seems to be the best option for maintainability and performance
		if (leftTv.spaceCoords.x > rightTv.spaceCoords.x) std::swap(leftTv, rightTv);

		real original_xBeg = leftTv.spaceCoords.x;
		real original_xEnd = rightTv.spaceCoords.x;
		FloatPack16 xBeg = std::clamp<real>(original_xBeg, 0, context.framebufW);
		FloatPack16 xEnd = std::clamp<real>(original_xEnd, 0, context.framebufW);
		real xSpan = original_xEnd - original_xBeg;

		real xpStep = 1.0 / xSpan;
		FloatPack16 xp = (xBeg - original_xBeg) / xSpan;

		VectorPack16 interpolatedDividedUv = VectorPack16(leftTv.textureCoords).lerp(rightTv.textureCoords, xp);
		VectorPack16 interpolatedDividedUvStep = (rightTv.textureCoords - leftTv.textureCoords) * xpStep;
		interpolatedDividedUv += interpolatedDividedUvStep * sequence_float;
		interpolatedDividedUvStep *= 16;

		size_t pixelIndex = size_t(y) * bufW + size_t(xBeg[0]); //all buffers have the same size, so we can use a single index

		//the loop increment section is fairly busy because it's body can be interrupted at various steps, but all increments must always happen
		for (FloatPack16 x = sequence_float + xBeg; 
			Mask16 loopBoundsMask = x < xEnd; 
			x += 16, pixelIndex += 16, xp += xpStep,
			interpolatedDividedUv.x += interpolatedDividedUvStep.x,
			interpolatedDividedUv.y += interpolatedDividedUvStep.y,
			interpolatedDividedUv.z += interpolatedDividedUvStep.z)
		{
			FloatPack16 currDepthValues = &depthBuf[pixelIndex];
			Mask16 visiblePointsMask = loopBoundsMask & currDepthValues > interpolatedDividedUv.z;
			if (!visiblePointsMask) continue; //if all points are occluded, then skip

			VectorPack16 worldCoords = VectorPack16(leftTv.worldCoords).lerp(rightTv.worldCoords, xp);

			VectorPack16 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z;
			VectorPack16 texturePixels = texture.gatherPixels512(uvCorrected.x, uvCorrected.y, visiblePointsMask);
			Mask16 opaquePixelsMask = visiblePointsMask & texturePixels.a > 0.0f;

			FloatPack16 distSquared = (worldCoords - context.camPos).lenSq3d();

			if (context.wireframeEnabled)
			{
				__mmask16 visibleEdgeMask = visiblePointsMask & (x <= original_xBeg + 1 | x >= original_xEnd - 1);
				texturePixels.r = _mm512_mask_blend_ps(visibleEdgeMask, texturePixels.r, _mm512_set1_ps(1));
				texturePixels.g = _mm512_mask_blend_ps(visibleEdgeMask, texturePixels.g, _mm512_set1_ps(1));
				texturePixels.b = _mm512_mask_blend_ps(visibleEdgeMask, texturePixels.b, _mm512_set1_ps(1));
				texturePixels.a = _mm512_mask_blend_ps(visibleEdgeMask, texturePixels.a, _mm512_set1_ps(1));
				//lightMult = _mm512_mask_blend_ps(visibleEdgeMask, lightMult, _mm512_set1_ps(1));
			}

			VectorPack16 dynaLight = (VectorPack16(Vec4(1, 0.8235, 0, 1)) * 1e5) / distSquared;
			//VectorPack16 dynaLight = 1;
			texturePixels = texturePixels * (dynaLight + lightMult);
			_mm512_mask_store_ps(&depthBuf[pixelIndex], opaquePixelsMask, interpolatedDividedUv.z);
			frameBuf.storePixels16(pixelIndex, texturePixels, opaquePixelsMask);
		}
	}
}

Vec4 Triangle::getNormalVector() const
{
	return (tv[2].spaceCoords - tv[0].spaceCoords).cross3d(tv[1].spaceCoords - tv[0].spaceCoords);
}