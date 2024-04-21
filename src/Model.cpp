#include "Model.h"
#include <functional>

Model::Model(const std::vector<Triangle>& triangles, int textureIndex)
{
	this->triangles = triangles;
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
}

void Model::draw(TriangleRenderContext ctx) const
{
	if (this->textureIndex == ctx.doomSkyTextureMarkerIndex) return;

	ctx.texture = &ctx.textureManager->getTextureByIndex(textureIndex);
	for (const auto& it : triangles) it.drawOn(ctx);
}
