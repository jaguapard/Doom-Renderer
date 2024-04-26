#include "Triangle.h"
#include "Statsman.h"

#include <functional>
constexpr real planeZ = -1;

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

		if (rt.z > planeZ)
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
		TexVertex clipped1 = rot[i].getClipedToPlane(v1);
		TexVertex clipped2 = rot[i].getClipedToPlane(v2);
				
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
		clipped.tv[v1_ind] = rot[v1_ind].getClipedToPlane(rot[i]);
		clipped.tv[v2_ind] = rot[v2_ind].getClipedToPlane(rot[i]);
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
	std::array<TexVertex, 3> fullyTransformed;
	for (int i = 0; i < 3; ++i)
	{
		real zInv = 1.0 / tv[i].spaceCoords.z;
		Vec3 zDividedWorld = tv[i].spaceCoords * zInv;
		Vec3 zDividedUv = tv[i].textureCoords * zInv;
		zDividedUv.z = zInv;
		fullyTransformed[i] = { context.ctr->screenSpaceToPixels(zDividedWorld), zDividedUv };
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

	flatTop.addToRenderQueueFinal(context, false);
	flatBottom.addToRenderQueueFinal(context, true);
}

void Triangle::addToRenderQueueFinal(const TriangleRenderContext& context, bool flatBottom) const
{
	/*Main idea: we are interpolating between lines of the triangle. All the next mathy stuff can be imagined as walking from a to b,
	"mixing" (linearly interpolating) between two values. */
	//if (yBeg < yEnd)
	{
		RenderJob rj;
		rj.flatBottom = flatBottom;
		rj.t = *this;
		rj.lightMult = context.lightMult;
		rj.textureIndex = context.textureIndex;
		context.renderJobs->push_back(rj);
	}
}

void Triangle::drawSlice(const TriangleRenderContext & context, const RenderJob& renderJob, int zoneMinY, int zoneMaxY) const
{
	real x1 = tv[0].spaceCoords.x, x2 = tv[1].spaceCoords.x, x3 = tv[2].spaceCoords.x, y1 = tv[0].spaceCoords.y, y2 = tv[1].spaceCoords.y, y3 = tv[2].spaceCoords.y;
	real original_yBeg = y1;
	real original_yEnd = y3;

	real yBeg = std::clamp<real>(y1, 0, context.framebufH);
	real yEnd = std::clamp<real>(y3, 0, context.framebufH);
	if (yBeg >= zoneMaxY || yEnd < zoneMinY) return;
	yBeg = std::max<real>(yBeg, zoneMinY);
	yEnd = std::min<real>(yEnd, zoneMaxY - 1e-6);

	real ySpan = y3 - y1; //since this function draws only flat top or flat bottom triangles, either y1 == y2 or y2 == y3. y3-y1 ensures we don't get 0, unless triangle is 0 thick, then it will be killed by loop conditions before division by 0 can occur  
	//const Texture& texture = *context.texture;
	const Texture& texture = context.textureManager->getTextureByIndex(renderJob.textureIndex);
	bool flatBottom = renderJob.flatBottom;

	const TexVertex& lerpDst1 = flatBottom ? tv[1] : tv[2]; //flat top and flat bottom triangles require different interpolation points
	const TexVertex& lerpSrc2 = flatBottom ? tv[0] : tv[1]; //using a flag passed from the "cooking" step seems to be the best option for maintainability and performance

	real ypStep = 1.0 / ySpan;
	real yp = (yBeg - y1) / ySpan;
	for (real y = yBeg; y < yEnd; ++y, yp += ypStep) //draw flat bottom part
	{
		TexVertex scanlineTv1 = lerp(tv[0], lerpDst1, yp); 
		TexVertex scanlineTv2 = lerp(lerpSrc2, tv[2], yp);
		const TexVertex* left = &scanlineTv1;
		const TexVertex* right = &scanlineTv2;
		if (left->spaceCoords.x > right->spaceCoords.x) std::swap(left, right);
		
		real original_xBeg = left->spaceCoords.x;
		real original_xEnd = right->spaceCoords.x;
		real xBeg = std::clamp<real>(left->spaceCoords.x, 0, context.framebufW);
		real xEnd = std::clamp<real>(right->spaceCoords.x, 0, context.framebufW);
		real xSpan = right->spaceCoords.x - left->spaceCoords.x;

		real xpStep = 1.0 / xSpan;
		real xp = (xBeg - left->spaceCoords.x) / xSpan;
		//xBeg = ceil(xBeg + 0.5);
		//xEnd = ceil(xEnd + 0.5);
		
		Vec3 interpolatedDividedUvStep = (right->textureCoords - left->textureCoords) * xpStep;
		Vec3 interpolatedDividedUv = lerp(left->textureCoords, right->textureCoords, xp);
		int bufW = context.frameBuffer->getW(); //save to avoid constant memory reads. Buffers don't change in size while rendering.
		int yInt = int(y);
		int xInt = int(xBeg);
		for (real x = xBeg; x < xEnd; ++x, xp += xpStep, ++xInt)
		{			
			interpolatedDividedUv += interpolatedDividedUvStep;
			Vec3 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z; //TODO: 3rd division is useless

			int pixelIndex = yInt * bufW + xInt; //all buffers have the same size, so we can use a single index
			bool occluded = (*(context.zBuffer))[pixelIndex] <= interpolatedDividedUv.z;
			if (occluded) continue;

			Color texturePixel = texture.getPixel(uvCorrected.x, uvCorrected.y);
			bool notFullyTransparent = texturePixel.a > 0;
			
			auto lightMult = renderJob.lightMult;
			if (context.wireframeEnabled)
			{
				int rx = x, ry = y, oxb = original_xBeg, oxe = original_xEnd, oyb = original_yBeg, oye = original_yEnd;
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
				(*context.frameBuffer)[pixelIndex] = texturePixel;
				(*context.lightBuffer)[pixelIndex] = lightMult;
				(*context.zBuffer)[pixelIndex] = interpolatedDividedUv.z;
			}
		}
	}
}

TexVertex TexVertex::getClipedToPlane(const TexVertex& dst) const
{
	real alpha = inverse_lerp(spaceCoords.z, dst.spaceCoords.z, planeZ);
	assert(alpha >= 0 && alpha <= 1);
	return lerp(*this, dst, alpha);
}
