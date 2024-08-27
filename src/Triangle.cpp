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

	rj.boundingBox.minX = floor(screenMinX);
	rj.boundingBox.maxX = ceil(screenMaxX);
	rj.boundingBox.minY = floor(screenMinY);
	rj.boundingBox.maxY = ceil(screenMaxY);
	rj.lightMult = context.lightMult;
	rj.textureIndex = context.textureIndex;

	real xSpan = screenMaxX - screenMinX;
	real ySpan = screenMaxY - screenMinY;	
}

Vec4 Triangle::getNormalVector() const
{
	return (tv[2].spaceCoords - tv[0].spaceCoords).cross3d(tv[1].spaceCoords - tv[0].spaceCoords);
}
