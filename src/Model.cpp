#include "Model.h"

Model::Model(const std::vector<Triangle>& triangles, int textureIndex)
{
	this->triangles = triangles;
	this->textureIndex = textureIndex;
}

void Model::draw(TriangleRenderContext ctx) const
{
	ctx.texture = &ctx.textureManager->getTextureByIndex(textureIndex);
	for (const auto& it : triangles) it.drawOn(ctx);
}
