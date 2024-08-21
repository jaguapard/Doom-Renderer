#pragma once
#include <vector>
#include <array>
#include "Triangle.h"
class Model
{
public:
	Model() = default;
	Model(const std::vector<Triangle>& triangles, int textureIndex);
	void addToRenderQueue(TriangleRenderContext ctx) const;
	int getTriangleCount() const;
	Vec4 getBoundingBoxMidPoint() const;
	const std::vector<Triangle>& getTriangles() const;

	void swapVertexOrder();
private:
	std::vector<Triangle> triangles;
	int textureIndex;
	std::array<Vec4, 8> boundingBox; //8 points to check clipping and collision against
};