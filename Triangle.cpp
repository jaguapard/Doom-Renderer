#include "Triangle.h"

void Triangle::drawOn(SDL_Surface* s, const CoordinateTransformer& ctr, ZBuffer& zBuffer, const std::vector<Texture>& textures) const
{
	drawInner(s, ctr, zBuffer, textures);
}

void Triangle::drawInner(SDL_Surface* s, const CoordinateTransformer& ctr, ZBuffer& zBuffer, const std::vector<Texture>& textures) const
{
	/*/Vec3 v0 = ctr.rotate(ctr.doCamOffset(tv[0].worldCoords));
	Vec3 v1 = ctr.rotate(ctr.doCamOffset(tv[1].worldCoords));
	Vec3 v2 = ctr.rotate(ctr.doCamOffset(tv[2].worldCoords));
	Vec3 cross = (v1 - v0).cross(v2 - v0);
	Vec3 camPos = -ctr.doCamOffset(Vec3(0, 0, 0));
	if (cross.dot(camPos) > 0) return;*/

	double maxX = s->w, maxY = s->h;

	/*/std::array<Vec3, 3> camOffset;
	for (int i = 0; i < 3; ++i) camOffset[i] = ctr.doCamOffset()*/

	std::array<int, 3> screenIndices = { 0,1,2 };
	std::array<TexVertex, 3> fullyTransformed;
	for (int i = 0; i < 3; ++i) fullyTransformed[i] = { ctr.toScreenCoords(tv[i].worldCoords), tv[i].textureCoords };

	//we need to sort index list by triangle's screen Y (ascending), to later gather the pre-z-divide and fully transformed (to screen space) lists
	std::sort(std::begin(screenIndices), std::end(screenIndices), [&](int a, int b) {return fullyTransformed[a].worldCoords.y < fullyTransformed[b].worldCoords.y; });

	std::array<TexVertex, 3> worldSpace, screenSpace;
	int badZvertices = 0;
	for (int i = 0; i < 3; ++i)
	{
		Vec3 off = ctr.doCamOffset(tv[screenIndices[i]].worldCoords);
		Vec3 rot = ctr.rotate(off);
		badZvertices += rot.z > 1; //clipping plane. TODO: clarify if this should be > or <
		if (badZvertices >= 3) return; //triangle is completely behind the clipping plane, discard it.
		worldSpace[i] = { rot,tv[screenIndices[i]].textureCoords };
		screenSpace[i] = { fullyTransformed[screenIndices[i]] };
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
		for (double x = xBeg; x < xEnd; ++x)
		{
			double xp = (x - xLeft) / (xRight - xLeft);
			Vec3 interpolatedDividedUv = lerp(dividedUvLeft, dividedUvRight, xp);
			Vec3 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z; //TODO: 3rd division is useless
			if (zBuffer.testAndSet(x, y, interpolatedDividedUv.z))
			{
				auto c = textures[textureIndex].getPixel(uvCorrected.x, uvCorrected.y);
				setPixel(s, x, y, c);
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
		for (double x = xBeg; x < xEnd; ++x)
		{
			double xp = (x - xLeft) / (xRight - xLeft);
			Vec3 interpolatedDividedUv = lerp(dividedUvLeft, dividedUvRight, xp);
			Vec3 uvCorrected = interpolatedDividedUv / interpolatedDividedUv.z; //TODO: 3rd division is useless
			if (zBuffer.testAndSet(x, y, interpolatedDividedUv.z))
			{
				auto c = textures[textureIndex].getPixel(uvCorrected.x, uvCorrected.y);
				setPixel(s, x, y, c);
			}
		}
	}
}
