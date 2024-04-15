#include "Triangle.h"
#include "Statsman.h"

constexpr double planeZ = -1;

void Triangle::drawOn(PixelBuffer<Color>& buf, const CoordinateTransformer& ctr, ZBuffer& zBuffer, TextureManager& textureManager, double lightMult) const
{
	std::array<TexVertex,3> rot;
	bool vertexOutside[3] = { false };
	int outsideVertexCount = 0;
	for (int i = 0; i < 3; ++i)
	{
		Vec3 off = ctr.doCamOffset(tv[i].worldCoords);
		Vec3 rt = ctr.rotate(off);		

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
	
	if (outsideVertexCount == 0) //all vertices are in front of camera, prepare data for drawInner and proceed
	{
		StatCount(statsman.triangles.zeroVerticesOutsideDraws++);
		Triangle prepped = *this;
		prepped.tv = rot;
		return prepped.drawInner(buf, ctr, zBuffer, textureManager, lightMult);
	}
	
	if (outsideVertexCount == 1) //if 1 vertice is outside, then 1 triangle gets turned into two
	{
		if (Statsman::enabled) statsman.triangles.singleVertexOutOfScreenSplits++;
		for (int i = 0; i < 3; ++i)
		{
			if (vertexOutside[i])
			{
				int v1_ind = i > 0 ? i - 1 : 2; //preserve vertice order of the original triangle and prevent out of bounds
				int v2_ind = i < 2 ? i + 1 : 0; //we only "change" the existing vertex

				const TexVertex& v1 = rot[v1_ind];
				const TexVertex& v2 = rot[v2_ind];
				Triangle t1 = *this, t2 = *this;
				TexVertex clipped1 = rot[i].getClipedToPlane(v1);
				TexVertex clipped2 = rot[i].getClipedToPlane(v2);
				
				t1.tv = { v1,clipped1, v2 };
				t2.tv = { clipped1, clipped2, v2};

				t1.drawInner(buf, ctr, zBuffer, textureManager, lightMult);
				t2.drawInner(buf, ctr, zBuffer, textureManager, lightMult);
				return;
			}
		}
	}

	if (outsideVertexCount == 2) //in case there are 2 vertices that are outside, the triangle just gets clipped (no new triangles needed)
	{
		StatCount(statsman.triangles.doubleVertexOutOfScreenSplits++);
		for (int i = 0; i < 3; ++i) //look for outside vertices and beat them into shape
		{
			if (!vertexOutside[i])
			{
				int v1_ind = i > 0 ? i - 1 : 2; //preserve vertice order of the original triangle and prevent out of bounds
				int v2_ind = i < 2 ? i + 1 : 0; //we only "change" the existing vertex

				Triangle clipped = *this;
				clipped.tv[v1_ind] = rot[v1_ind].getClipedToPlane(rot[i]);
				clipped.tv[v2_ind] = rot[v2_ind].getClipedToPlane(rot[i]);
				clipped.tv[i] = rot[i];
				return clipped.drawInner(buf, ctr, zBuffer, textureManager, lightMult);
			}
		}
		

	}
}

//WARNING: this method expects tv to contain rotated (but not yet z-divided coords)!
void Triangle::drawInner(PixelBuffer<Color>& buf, const CoordinateTransformer& ctr, ZBuffer& zBuffer, TextureManager& textureManager, double lightMult) const
{
	/*/Vec3 v0 = ctr.rotate(ctr.doCamOffset(tv[0].worldCoords));
	Vec3 v1 = ctr.rotate(ctr.doCamOffset(tv[1].worldCoords));
	Vec3 v2 = ctr.rotate(ctr.doCamOffset(tv[2].worldCoords));
	Vec3 cross = (v1 - v0).cross(v2 - v0);
	Vec3 camPos = -ctr.doCamOffset(Vec3(0, 0, 0));
	if (cross.dot(camPos) > 0) return;*/

	double maxX = buf.getW(), maxY = buf.getH();

	std::array<TexVertex, 3> fullyTransformed;
	for (int i = 0; i < 3; ++i)
	{
		//double zInv = 1.0 / zDivided[i].worldCoords.z;
		Vec3 zDivided = tv[i].worldCoords / tv[i].worldCoords.z;
		fullyTransformed[i] = { ctr.screenSpaceToPixels(zDivided), tv[i].textureCoords };
	}

	std::array<int, 3> screenIndices = { 0,1,2 };
	//we need to sort index list by triangle's screen Y (ascending), to later gather the pre-z-divide and fully transformed (to screen space) lists
	std::sort(std::begin(screenIndices), std::end(screenIndices), [&](int a, int b) {return fullyTransformed[a].worldCoords.y < fullyTransformed[b].worldCoords.y; });
	std::array<TexVertex, 3> worldSpace, screenSpace;
	for (int i = 0; i < 3; ++i)
	{
		worldSpace[i] = tv[screenIndices[i]];
		screenSpace[i] = fullyTransformed[screenIndices[i]];
	}
	
	std::array<Vec3, 3> uvDividedByZ;
	for (int i = 0; i < 3; ++i)
	{
		double zInv = 1.0 / worldSpace[i].worldCoords.z;
		Vec2 dividedUv = worldSpace[i].textureCoords * zInv;
		uvDividedByZ[i] = Vec3(dividedUv.x, dividedUv.y, zInv);
	}

	/*Main idea: we are interpolating between lines of the triangle. All the next mathy stuff can be imagined as walking from a to b,
	"mixing" (linearly interpolating) between two values. */
	double x1 = screenSpace[0].worldCoords.x, x2 = screenSpace[1].worldCoords.x, x3 = screenSpace[2].worldCoords.x, y1 = screenSpace[0].worldCoords.y, y2 = screenSpace[1].worldCoords.y, y3 = screenSpace[2].worldCoords.y;
	double splitAlpha = (y2 - y1) / (y3 - y1); //how far along original triangle's y is the split line? 0 = extreme top, 1 = extreme bottom
	double split_xend = lerp(x1, x3, splitAlpha); //last x of splitting line
	Vec3 split_dividedUvEnd = lerp(uvDividedByZ[0], uvDividedByZ[2], splitAlpha);

	double yBeg = std::max(0.0, y1);
	double yEnd = std::min(maxY, y2);
	const Texture& texture = textureManager.getTextureByIndex(this->textureIndex);

	for (double y = yBeg; y < yEnd; ++y) //draw flat bottom part
	{
		double yp = (y - y1) / (y2 - y1); //this is the "progress" along the flat bottom part, not whole triangle!
		double xLeft = lerp(x1, x2, yp);
		double xRight = lerp(x1, x3, splitAlpha * yp);

		Vec3 dividedUvLeft = lerp(uvDividedByZ[0], uvDividedByZ[1], yp);
		Vec3 dividedUvRight = lerp(uvDividedByZ[0], uvDividedByZ[2], splitAlpha * yp);

		if (xLeft > xRight)
		{
			std::swap(xLeft, xRight); //enforce non-decreasing x for next loop.
			std::swap(dividedUvLeft, dividedUvRight); //If we swap x, then uv also has to go.
		}

		double xBeg = std::max(0.0, xLeft);
		double xEnd = std::min(maxX, xRight);
		//xBeg = ceil(xBeg + 0.5);
		//xEnd = ceil(xEnd + 0.5);
		for (double x = xBeg; x < xEnd; ++x)
		{
			double xp = (x - xLeft) / (xRight - xLeft);
			Vec3 interpolatedDividedUv = lerp(dividedUvLeft, dividedUvRight, xp);
			Vec3 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z; //TODO: 3rd division is useless
			if (zBuffer.testAndSet(x, y, interpolatedDividedUv.z))
			{
				auto c = texture.getPixel(uvCorrected.x, uvCorrected.y, lightMult);
				buf.setPixelUnsafe(x, y, c);
			}
		}
	}

	yBeg = std::max(0.0, y2);
	yEnd = std::min(maxY, y3);
	for (double y = yBeg; y < yEnd; ++y) //draw flat top part
	{
		double yp = (y - y2) / (y3 - y2); //this is the "progress" along the flat top part, not whole triangle!
		double xLeft = lerp(x2, x3, yp);
		double xRight = lerp(split_xend, x3, yp);

		Vec3 dividedUvLeft = lerp(uvDividedByZ[1], uvDividedByZ[2], yp);
		Vec3 dividedUvRight = lerp(split_dividedUvEnd, uvDividedByZ[2], yp);

		if (xLeft > xRight)
		{
			std::swap(xLeft, xRight); //enforce non-decreasing x for next loop.
			std::swap(dividedUvLeft, dividedUvRight); //If we swap x, then uv also has to go.
		}

		double xBeg = std::max(0.0, xLeft);
		double xEnd = std::min(maxX, xRight);
		//xBeg = ceil(xBeg + 0.5);
		//xEnd = ceil(xEnd + 0.5);
		for (double x = xBeg; x < xEnd; ++x)
		{
			double xp = (x - xLeft) / (xRight - xLeft);
			Vec3 interpolatedDividedUv = lerp(dividedUvLeft, dividedUvRight, xp);
			Vec3 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z; //TODO: 3rd division is useless
			if (zBuffer.testAndSet(x, y, interpolatedDividedUv.z))
			{
				auto c = texture.getPixel(uvCorrected.x, uvCorrected.y, lightMult);
				buf.setPixelUnsafe(x, y, c);
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
