#pragma once
#include <vector>
#include <array>
#include "Triangle.h"
class Model
{
public:
	Model() = default;
	Model(const std::vector<Triangle>& triangles, int textureIndex, const TextureManager& textureManager);
	int getTriangleCount() const;
	Vec4 getBoundingBoxMidPoint() const;
	const std::vector<Triangle>& getTriangles() const;

	void swapVertexOrder();
	std::optional<real> lightMult;
	int textureIndex;
	bool noBackfaceCulling = false;
private:
	std::vector<Triangle> triangles;	
	std::array<Vec4, 8> boundingBox; //8 points to check clipping and collision against
};