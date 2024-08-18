#include "Model.h"
#include <functional>
#include <numeric>
#include "Statsman.h"

Model::Model(const std::vector<Triangle>& triangles, int textureIndex, TextureManager& textureManager)
{
	//assert(triangles.size() > 0);
	this->textureIndex = textureIndex;

	std::function inf = std::numeric_limits<real>::infinity;
	Vec4 min(inf(), inf(), inf()), max(-inf(), -inf(), -inf());
	for (const auto& it : triangles)
	{
		for (auto& tv : it.tv)
		{
			min.x = std::min(min.x, tv.spaceCoords.x);
			min.y = std::min(min.y, tv.spaceCoords.y);
			min.z = std::min(min.z, tv.spaceCoords.z);
			max.x = std::max(max.x, tv.spaceCoords.x);
			max.y = std::max(max.y, tv.spaceCoords.y);
			max.z = std::max(max.z, tv.spaceCoords.z);
		}
	}

	boundingBox = { //TODO: verify this
		Vec4(min.x, min.y, min.z),
		Vec4(min.x, min.y, max.z),
		Vec4(max.x, min.y, min.z),
		Vec4(max.x, min.y, max.z), //all lower done

		Vec4(min.x, max.y, min.z),
		Vec4(max.x, max.y, min.z),
		Vec4(min.x, max.y, max.z),
		Vec4(max.x, max.y, max.z),
	};

	for (const auto& it : triangles)
	{
		this->triangles.push_back(it);
	}
	assert(this->triangles.size() > 0);
}

void Model::addToRenderQueue(TriangleRenderContext ctx) const
{
	if (this->textureIndex == ctx.doomSkyTextureMarkerIndex) return; //skip sky textured level geometry

	/*/for (const auto& p : boundingBox)
	{
		Vec4 screen = ctx.ctr->toScreenCoords(p);
		if (screen.x )
		if (isInRange(screen.x, real(0), ctx.framebufW)) goto render; //if any points of the bounding box are on the screen, then the model may be visible, proceed to rendering
		if (isInRange(screen.y, real(0), ctx.framebufH)) goto render;
	}
	StatCount(statsman.models.boundingBoxDiscards++);
	return; //if we got here, it means that all points are outside the screen, so no need to draw this model
	*/
	render:
	const auto& texture = ctx.textureManager->getTextureByIndex(textureIndex);
	ctx.textureIndex = textureIndex;
	ctx.gameSettings.backfaceCullingEnabled &= texture.hasOnlyOpaquePixels(); //stuff with transparency requires having backface culling disabled to be properly rendered
	for (const auto& it : triangles) it.addToRenderQueue(ctx);
}

int Model::getTriangleCount() const
{
	return triangles.size();
}

Vec4 Model::getBoundingBoxMidPoint() const
{
	return std::accumulate(boundingBox.begin(), boundingBox.end(), Vec4(0, 0, 0)) / boundingBox.size();
}

const std::vector<Triangle>& Model::getTriangles() const
{
	return triangles;
}

void Model::swapVertexOrder()
{
	for (auto& it : triangles) std::swap(it.tv[1], it.tv[2]);
}
