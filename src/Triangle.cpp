#include "Triangle.h"
#include "Statsman.h"

constexpr real planeZ = -1;

void Triangle::drawOn(const TriangleRenderContext& context) const
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
		return prepped.drawRotationPrepped(context);
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

		t1.drawRotationPrepped(context);
		t2.drawRotationPrepped(context);
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
		return clipped.drawRotationPrepped(context);
	}
}

//WARNING: this method expects tv to contain rotated (but not yet z-divided coords)!
void Triangle::drawRotationPrepped(const TriangleRenderContext& context) const
{
	/*/Vec3 v0 = ctr.rotate(ctr.doCamOffset(tv[0].spaceCoords));
	Vec3 v1 = ctr.rotate(ctr.doCamOffset(tv[1].spaceCoords));
	Vec3 v2 = ctr.rotate(ctr.doCamOffset(tv[2].spaceCoords));
	Vec3 cross = (v1 - v0).cross(v2 - v0);
	Vec3 camPos = -ctr.doCamOffset(Vec3(0, 0, 0));
	if (cross.dot(camPos) > 0) return;*/

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

	flatTop.drawScreenSpaceAndUvDividedPrepped(context, false);
	flatBottom.drawScreenSpaceAndUvDividedPrepped(context, true);
}

void Triangle::drawScreenSpaceAndUvDividedPrepped(const TriangleRenderContext& context, bool flatBottom) const
{
	/*Main idea: we are interpolating between lines of the triangle. All the next mathy stuff can be imagined as walking from a to b,
	"mixing" (linearly interpolating) between two values. */
	real x1 = tv[0].spaceCoords.x, x2 = tv[1].spaceCoords.x, x3 = tv[2].spaceCoords.x, y1 = tv[0].spaceCoords.y, y2 = tv[1].spaceCoords.y, y3 = tv[2].spaceCoords.y;

	real yBeg = std::clamp<real>(y1, 0, context.framebufH);
	real yEnd = std::clamp<real>(y3, 0, context.framebufH);
	real ySpan = y3 - y1; //since this function draws only flat top or flat bottom triangles, either y1 == y2 or y2 == y3. y3-y1 ensures we don't get 0, unless triangle is 0 thick, then it will be killed by loop conditions before division by 0 can occur  
	const Texture& texture = *context.texture;

	const TexVertex& lerpDst1 = flatBottom ? tv[1] : tv[2]; //flat top and flat bottom triangles require different interpolation points
	const TexVertex& lerpSrc2 = flatBottom ? tv[0] : tv[1]; //using a flag passed from the "cooking" step seems to be the best option for maintainability and performance
	for (real y = yBeg; y < yEnd; ++y) //draw flat bottom part
	{
		real yp = (y - y1) / ySpan;
		TexVertex scanlineTv1 = lerp(tv[0], lerpDst1, yp); 
		TexVertex scanlineTv2 = lerp(lerpSrc2, tv[2], yp);
		const TexVertex* left = &scanlineTv1;
		const TexVertex* right = &scanlineTv2;
		if (left->spaceCoords.x > right->spaceCoords.x) std::swap(left, right);
		
		real xBeg = std::clamp<real>(left->spaceCoords.x, 0, context.framebufW);
		real xEnd = std::clamp<real>(right->spaceCoords.x, 0, context.framebufW);
		real xSpan = right->spaceCoords.x - left->spaceCoords.x;
		//xBeg = ceil(xBeg + 0.5);
		//xEnd = ceil(xEnd + 0.5);
		for (real x = xBeg; x < xEnd; ++x)
		{
			real xp = (x - left->spaceCoords.x) / xSpan;
			Vec3 interpolatedDividedUv = lerp(left->textureCoords, right->textureCoords, xp);
			Vec3 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z; //TODO: 3rd division is useless

			Color texturePixel = texture.getPixel(uvCorrected.x, uvCorrected.y);
			bool notFullyTransparent = texturePixel.a > 0;
			if (notFullyTransparent && context.zBuffer->testAndSet(x, y, interpolatedDividedUv.z, notFullyTransparent)) //fully transparent pixels do not need to be considered for drawing
			{
				context.frameBuffer->setPixelUnsafe(x, y, texturePixel);
				context.lightBuffer->setPixelUnsafe(x, y, context.lightMult);
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
