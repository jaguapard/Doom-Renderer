#pragma once
#include <vector>
#include "Triangle.h"
class Model
{
public:
	Model() = default;
	Model(const std::vector<Triangle>& triangles, int textureIndex);
	void draw(TriangleRenderContext ctx) const;
private:
	std::vector<Triangle> triangles;
	int textureIndex;
};