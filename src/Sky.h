#pragma once
#include <vector>
#include "Triangle.h"
#include "Model.h"

class Sky
{
public:
	Sky() = default;
	Sky(std::string textureName, TextureManager& textureManager);
	void addToRenderQueue(TriangleRenderContext ctx);
	const std::vector<Triangle>& getTriangles() const;
private:
	Model skyModel;	
};