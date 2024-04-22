#pragma once
#include <vector>
#include <array>
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
	std::array<Vec3, 8> boundingBox; //8 points to check clipping and collision against
};