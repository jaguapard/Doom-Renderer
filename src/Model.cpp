#include "Model.h"
#include <functional>
#include <numeric>
#include "Statsman.h"

Model::Model(const std::vector<Triangle>& triangles, int textureIndex, TextureManager& textureManager)
{
	//assert(triangles.size() > 0);
	this->textureIndex = textureIndex;

	std::function inf = std::numeric_limits<real>::infinity;
	Vec3 min(inf(), inf(), inf()), max(-inf(), -inf(), -inf());
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
		Vec3(min.x, min.y, min.z),
		Vec3(min.x, min.y, max.z),
		Vec3(max.x, min.y, min.z),
		Vec3(max.x, min.y, max.z), //all lower done

		Vec3(min.x, max.y, min.z),
		Vec3(max.x, max.y, min.z),
		Vec3(min.x, max.y, max.z),
		Vec3(max.x, max.y, max.z),
	};

	for (const auto& it : triangles)
	{
		this->triangles.push_back(it);
	}
	assert(this->triangles.size() > 0);
}

void Model::draw(TriangleRenderContext ctx) const
{
	if (this->textureIndex == ctx.doomSkyTextureMarkerIndex) return; //skip sky textured level geometry

	/*/for (const auto& p : boundingBox)
	{
		Vec3 screen = ctx.ctr->toScreenCoords(p);
		if (screen.x )
		if (isInRange(screen.x, real(0), ctx.framebufW)) goto render; //if any points of the bounding box are on the screen, then the model may be visible, proceed to rendering
		if (isInRange(screen.y, real(0), ctx.framebufH)) goto render;
	}
	StatCount(statsman.models.boundingBoxDiscards++);
	return; //if we got here, it means that all points are outside the screen, so no need to draw this model
	*/
	render:
	ctx.texture = &ctx.textureManager->getTextureByIndex(textureIndex);
	for (const auto& it : triangles) it.drawOn(ctx);
}

int Model::getTriangleCount() const
{
	return triangles.size();
}

Vec3 Model::getBoundingBoxMidPoint() const
{
	return std::accumulate(boundingBox.begin(), boundingBox.end(), Vec3(0, 0, 0)) / boundingBox.size();
}
