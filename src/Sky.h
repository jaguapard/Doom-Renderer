#pragma once
#include <vector>
#include "Triangle.h"
#include "Model.h"

class Sky
{
public:
	Sky() = default;
	Sky(std::string textureName, TextureManager& textureManager);
	void draw(TriangleRenderContext ctx);
private:
	Model skyModel;	
};