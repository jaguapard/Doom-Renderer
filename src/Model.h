#pragma once
#include <vector>
#include <array>
#include "Triangle.h"
class Model
{
public:
	Model() = default;
	Model(const std::vector<Triangle>& triangles, int textureIndex, TextureManager& textureManager);
	void draw(TriangleRenderContext ctx) const;
	int getTriangleCount() const;
	Vec3 getBoundingBoxMidPoint() const;
private:
	std::vector<Triangle> triangles;
	int textureIndex;
	std::array<Vec3, 8> boundingBox; //8 points to check clipping and collision against
};