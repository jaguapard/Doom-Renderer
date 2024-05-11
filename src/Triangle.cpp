#include "Triangle.h"
#include "Statsman.h"

#include <functional>

void Triangle::sortByAscendingSpaceX()
{
	std::sort(tv.begin(), tv.end(), [&](TexVertex& tv1, TexVertex& tv2) {return tv1.spaceCoords.x < tv2.spaceCoords.x; });
}

void Triangle::sortByAscendingSpaceY()
{
	std::sort(tv.begin(), tv.end(), [&](TexVertex& tv1, TexVertex& tv2) {return tv1.spaceCoords.y < tv2.spaceCoords.y; });
}

void Triangle::sortByAscendingSpaceZ()
{
	std::sort(tv.begin(), tv.end(), [&](TexVertex& tv1, TexVertex& tv2) {return tv1.spaceCoords.z < tv2.spaceCoords.z; });
}

void Triangle::sortByAscendingTextureX()
{
	std::sort(tv.begin(), tv.end(), [&](TexVertex& tv1, TexVertex& tv2) {return tv1.textureCoords.x < tv2.textureCoords.x; });
}

void Triangle::sortByAscendingTextureY()
{
	std::sort(tv.begin(), tv.end(), [&](TexVertex& tv1, TexVertex& tv2) {return tv1.textureCoords.y < tv2.textureCoords.y; });
}

void Triangle::addToRenderQueue(const TriangleRenderContext& context) const
{
	std::array<TexVertex,3> rot;
	bool vertexOutside[3] = { false };
	int outsideVertexCount = 0;
	for (int i = 0; i < 3; ++i)
	{
		Vec3 off = context.ctr->doCamOffset(tv[i].spaceCoords);
		Vec3 rt = context.ctr->rotate(off);

		if (rt.z > context.nearPlaneClippingZ)
		{
			outsideVertexCount++;
			if (outsideVertexCount == 3) 
			{
				StatCount(statsman.triangles.tripleVerticeOutOfScreenDiscards++);
				return; //triangle is completely behind the clipping plane, discard
			}
			vertexOutside[i] = true;			
		}
		rot[i] = { rt, tv[i].textureCoords };
	}

	if (context.backfaceCullingEnabled)
	{
		Vec3 normal = (rot[2].spaceCoords - rot[0].spaceCoords).cross3d(rot[1].spaceCoords - rot[0].spaceCoords);
		if (rot[0].spaceCoords.dot(normal) >= 0) return;
	}
	
	if (outsideVertexCount == 0) //all vertices are in front of camera, prepare data for drawRotationPrepped and proceed
	{
		StatCount(statsman.triangles.zeroVerticesOutsideDraws++);
		Triangle prepped = *this;
		prepped.tv = rot;
		return prepped.prepareScreenSpace(context);
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
		TexVertex clipped1 = rot[i].getClipedToPlane(v1, context.nearPlaneClippingZ);
		TexVertex clipped2 = rot[i].getClipedToPlane(v2, context.nearPlaneClippingZ);
				
		t1.tv = { v1,clipped1, v2 };
		t2.tv = { clipped1, clipped2, v2};

		t1.prepareScreenSpace(context);
		t2.prepareScreenSpace(context);
		return;
	}

	if (outsideVertexCount == 2) //in case there are 2 vertices that are outside, the triangle just gets clipped (no new triangles needed)
	{
		StatCount(statsman.triangles.doubleVertexOutOfScreenSplits++);
		int i = std::find(itBeg, itEnd, false) - itBeg;		
		int v1_ind = i > 0 ? i - 1 : 2; //preserve vertice order of the original triangle and prevent out of bounds
		int v2_ind = i < 2 ? i + 1 : 0; //we only "change" the existing vertex

		Triangle clipped = *this;
		clipped.tv[v1_ind] = rot[v1_ind].getClipedToPlane(rot[i], context.nearPlaneClippingZ);
		clipped.tv[v2_ind] = rot[v2_ind].getClipedToPlane(rot[i], context.nearPlaneClippingZ);
		clipped.tv[i] = rot[i];
		return clipped.prepareScreenSpace(context);
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
		Vec3 zDividedWorld = tv[i].spaceCoords * zInv;
		Vec3 zDividedUv = tv[i].textureCoords * zInv;
		zDividedUv.z = zInv;
		screenSpaceTriangle.tv[i] = { context.ctr->screenSpaceToPixels(zDividedWorld), zDividedUv };
	}
	if (screenSpaceTriangle.y1 == screenSpaceTriangle.y2 && screenSpaceTriangle.y2 == screenSpaceTriangle.y3) return; //sadly, this doesn't get caught by loop conditions, since NaN wrecks havok there
	//we need to sort by triangle's screen Y (ascending) for later flat top and bottom splits
	screenSpaceTriangle.sortByAscendingSpaceY();

	real splitAlpha = (screenSpaceTriangle.y2 - screenSpaceTriangle.y1) / (screenSpaceTriangle.y3 - screenSpaceTriangle.y1);
	TexVertex splitVertex = lerp(screenSpaceTriangle.tv[0], screenSpaceTriangle.tv[2], splitAlpha);

	TexVertex v2_copy = screenSpaceTriangle.tv[2];
	screenSpaceTriangle.tv[2] = splitVertex;
	screenSpaceTriangle.addToRenderQueueFinal(context, true); //flat bottom part

	//screenSpaceTriangle.tv = { screenSpaceTriangle.tv[1], splitVertex, v2_copy };
	screenSpaceTriangle.tv[0] = splitVertex;
	screenSpaceTriangle.tv[2] = v2_copy;
	screenSpaceTriangle.addToRenderQueueFinal(context, false); //flat top part
}

void Triangle::addToRenderQueueFinal(const TriangleRenderContext& context, bool flatBottom) const
{
	RenderJob rj;
	rj.flatBottom = flatBottom;
	rj.t = *this;
	rj.lightMult = context.lightMult;
	rj.textureIndex = context.textureIndex;
	context.renderJobs->push_back(rj);
}

void Triangle::drawSlice(const TriangleRenderContext & context, const RenderJob& renderJob, int zoneMinY, int zoneMaxY) const
{
	//Scanline rasterization algorithm
	if (y1 >= zoneMaxY || y3 < zoneMinY) return;
	real yBeg = std::clamp<real>(y1, zoneMinY, zoneMaxY);
	real yEnd = std::clamp<real>(y3, zoneMinY, zoneMaxY);

	real ySpan = y3 - y1; //since this function draws only flat top or flat bottom triangles, either y1 == y2 or y2 == y3. y3-y1 ensures we don't get 0. 0 height triangles are culled in previous stage 
	//const Texture& texture = *context.texture;
	const Texture& texture = context.textureManager->getTextureByIndex(renderJob.textureIndex);
	bool flatBottom = renderJob.flatBottom;

	real ypStep = 1.0 / ySpan;
	real yp = (yBeg - y1) / ySpan;
	auto& frameBuf = *context.frameBuffer;
	auto& lightBuf = *context.lightBuffer;
	auto& depthBuf = *context.zBuffer;
	int bufW = frameBuf.getW(); //save to avoid constant memory reads. Buffers don't change in size while rendering.

	for (real y = yBeg; y < yEnd; ++y, yp += ypStep) //draw flat bottom part
	{
		TexVertex leftTv = lerp(tv[0], tv[2 - flatBottom], yp); //flat top and flat bottom triangles require different interpolation points
		TexVertex rightTv = lerp(tv[1 - flatBottom], tv[2], yp); //using a flag passed from the "cooking" step seems to be the best option for maintainability and performance
		if (leftTv.spaceCoords.x > rightTv.spaceCoords.x) std::swap(leftTv, rightTv);
		
		real original_xBeg = leftTv.spaceCoords.x;
		real original_xEnd = rightTv.spaceCoords.x;
		real xBeg = std::clamp<real>(original_xBeg, 0, context.framebufW);
		real xEnd = std::clamp<real>(original_xEnd, 0, context.framebufW);
		real xSpan = original_xEnd - original_xBeg;

		real xpStep = 1.0 / xSpan;
		real xp = (xBeg - original_xBeg) / xSpan;
		//xBeg = ceil(xBeg + 0.5);
		//xEnd = ceil(xEnd + 0.5);
		
		Vec3 interpolatedDividedUvStep = (rightTv.textureCoords - leftTv.textureCoords) * xpStep;
		Vec3 interpolatedDividedUv = lerp(leftTv.textureCoords, rightTv.textureCoords, xp);
		int yInt = int(y);
		int xInt = int(xBeg);
		for (real x = xBeg; x < xEnd; ++x, xp += xpStep, ++xInt)
		{			
			interpolatedDividedUv += interpolatedDividedUvStep;
			Vec3 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z;

			int pixelIndex = yInt * bufW + xInt; //all buffers have the same size, so we can use a single index
			bool occluded = depthBuf[pixelIndex] <= interpolatedDividedUv.z;
			if (occluded) continue;

			Color texturePixel = texture.getPixel(uvCorrected.x, uvCorrected.y);
			bool notFullyTransparent = texturePixel.a > 0;
			
			auto lightMult = renderJob.lightMult;
			if (context.wireframeEnabled)
			{
				int rx = x, ry = y, oxb = original_xBeg, oxe = original_xEnd, oyb = y1, oye = y3;
				bool leftEdge = rx == oxb;
				bool rightEdge = rx == oxe;
				bool topEdge = flatBottom && ry == oyb;
				bool bottomEdge = !flatBottom && ry == oye;
				if (leftEdge || rightEdge || topEdge || bottomEdge)
				{
					texturePixel = Color(255, 255, 255);
					lightMult = 1;
				}
			}
			if (notFullyTransparent) //fully transparent pixels do not need to be considered for drawing
			{
				frameBuf[pixelIndex] = texturePixel;
				lightBuf[pixelIndex] = lightMult;
				depthBuf[pixelIndex] = interpolatedDividedUv.z;
			}
		}
	}
}

Vec3 Triangle::getNormalVector() const
{
	return (tv[2].spaceCoords - tv[0].spaceCoords).cross3d(tv[1].spaceCoords - tv[0].spaceCoords);
}

TexVertex TexVertex::getClipedToPlane(const TexVertex& dst, real planeZ) const
{
	real alpha = inverse_lerp(spaceCoords.z, dst.spaceCoords.z, planeZ);
	assert(alpha >= 0 && alpha <= 1);
	return lerp(*this, dst, alpha);
}
