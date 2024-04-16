#include "Triangle.h"
#include "Statsman.h"

constexpr double planeZ = -1;

void Triangle::drawOn(const TriangleRenderContext& context) const
{
	std::array<TexVertex,3> rot;
	bool vertexOutside[3] = { false };
	int outsideVertexCount = 0;
	for (int i = 0; i < 3; ++i)
	{
		Vec3 off = context.ctr->doCamOffset(tv[i].worldCoords);
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
	/*/Vec3 v0 = ctr.rotate(ctr.doCamOffset(tv[0].worldCoords));
	Vec3 v1 = ctr.rotate(ctr.doCamOffset(tv[1].worldCoords));
	Vec3 v2 = ctr.rotate(ctr.doCamOffset(tv[2].worldCoords));
	Vec3 cross = (v1 - v0).cross(v2 - v0);
	Vec3 camPos = -ctr.doCamOffset(Vec3(0, 0, 0));
	if (cross.dot(camPos) > 0) return;*/

	double maxX = context.framebufW, maxY = context.framebufH;

	std::array<TexVertex, 3> fullyTransformed;
	for (int i = 0; i < 3; ++i)
	{
		double zInv = 1.0 / tv[i].worldCoords.z;
		Vec3 zDivided = tv[i].worldCoords * zInv;
		zDivided.z = zInv;
		fullyTransformed[i] = { context.ctr->screenSpaceToPixels(zDivided), tv[i].textureCoords };
	}

	//we need to sort by triangle's screen Y (ascending) for later flat top and bottom splits
	std::sort(fullyTransformed.begin(), fullyTransformed.end());

	std::array<Vec3, 3> uvDividedByZ;
	for (int i = 0; i < 3; ++i)
	{
		double zInv = fullyTransformed[i].worldCoords.z;
		Vec2 dividedUv = fullyTransformed[i].textureCoords * zInv;
		fullyTransformed[i].textureCoords = Vec3(dividedUv.x, dividedUv.y, zInv);
	}

	double splitAlpha = (fullyTransformed[1].worldCoords.y - fullyTransformed[0].worldCoords.y) / (fullyTransformed[2].worldCoords.y - fullyTransformed[0].worldCoords.y);
	TexVertex splitVertex = lerp(fullyTransformed[0], fullyTransformed[2], splitAlpha);
	
	Triangle flatBottom;// = { {fullyTransformed[0], fullyTransformed[1], splitVertex}, {fullyTransformed[1], splitVertex, fullyTransformed[2]} }, this->textureIndex
	flatBottom.tv = { fullyTransformed[0], fullyTransformed[1], splitVertex };
	flatBottom.textureIndex = this->textureIndex;
	
	Triangle flatTop;
	//flatTop = { fullyTransformed[1], splitVertex, fullyTransformed[2] };
	
	//flatTop = { splitVertex, fullyTransformed[1], fullyTransformed[2]};
	//flatTop = { splitVertex, fullyTransformed[2], fullyTransformed[1] };
	//flatTop = { fullyTransformed[2], splitVertex, fullyTransformed[1] };
	//flatTop = { fullyTransformed[2], fullyTransformed[1], splitVertex };
	bool splitOnLeft = splitVertex.worldCoords.x < fullyTransformed[1].worldCoords.x;
	flatTop = { splitOnLeft ? splitVertex : fullyTransformed[1], splitOnLeft ? fullyTransformed[1] : splitVertex, fullyTransformed[2] };
	flatTop.textureIndex = this->textureIndex;

	flatTop.drawScreenSpaceAndUvDividedPrepped(context);
	//flatBottom.drawScreenSpaceAndUvDividedPrepped(context);
}

void Triangle::drawScreenSpaceAndUvDividedPrepped(const TriangleRenderContext& context) const
{
	/*Main idea: we are interpolating between lines of the triangle. All the next mathy stuff can be imagined as walking from a to b,
	"mixing" (linearly interpolating) between two values. */
	double x1 = tv[0].worldCoords.x, x2 = tv[1].worldCoords.x, x3 = tv[2].worldCoords.x, y1 = tv[0].worldCoords.y, y2 = tv[1].worldCoords.y, y3 = tv[2].worldCoords.y;

	double yBeg = std::max(0.0, y1);
	double yEnd = std::min(context.framebufH, y3);
	double ySpan = y3 - y1;
	const Texture& texture = context.textureManager->getTextureByIndex(this->textureIndex);

	for (double y = yBeg; y < yEnd; ++y) //draw flat bottom part
	{
		double yp = (y - y1) / ySpan;
		//const TexVertex* tLeft = &scanlineTv1;
		//const TexVertex* right = &scanlineTv2;
		TexVertex scanlineTv1 = lerp(tv[0], tv[1], yp);
		TexVertex scanlineTv2 = lerp(tv[0], tv[2], yp);
		/*/const TexVertex& left = scanlineTv1.worldCoords.x <= scanlineTv2.worldCoords.x ? scanlineTv1 : scanlineTv2;
		const TexVertex& right = scanlineTv1.worldCoords.x <= scanlineTv2.worldCoords.x ? scanlineTv2 : scanlineTv1;*/
		const TexVertex* left = &scanlineTv1;
		const TexVertex* right = &scanlineTv2;
		if (left->worldCoords.x > right->worldCoords.x) std::swap(left, right);
		
		double xBeg = std::max(0.0, left->worldCoords.x);
		double xEnd = std::min(context.framebufW, right->worldCoords.x);
		double tv_xSpan = right->worldCoords.x - left->worldCoords.x;
		//xBeg = ceil(xBeg + 0.5);
		//xEnd = ceil(xEnd + 0.5);
		for (double x = xBeg; x < xEnd; ++x)
		{
			double xp = (x - left->worldCoords.x) / tv_xSpan;
			Vec3 interpolatedDividedUv = lerp(left->textureCoords, right->textureCoords, xp);
			Vec3 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z; //TODO: 3rd division is useless

			Color texturePixel = texture.getPixel(uvCorrected.x, uvCorrected.y);
			bool notFullyTransparent = texturePixel.a > 0;
			if (notFullyTransparent && context.zBuffer->testAndSet(x, y, interpolatedDividedUv.z, notFullyTransparent)) //fully transparent pixels do not need to be considered for drawing
			{
				Color c = texturePixel.multipliedByLight(context.lightMult);
				context.frameBuffer->setPixelUnsafe(x, y, c);
			}
		}
	}
}

TexVertex TexVertex::getClipedToPlane(const TexVertex& dst) const
{
	double alpha = inverse_lerp(worldCoords.z, dst.worldCoords.z, planeZ);
	assert(alpha >= 0 && alpha <= 1);
	return lerp(*this, dst, alpha);
}
