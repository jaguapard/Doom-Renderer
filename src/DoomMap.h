#pragma once
#include <vector>

#include "DoomStructs.h"
#include "Triangle.h"

class DoomMap
{
public:
	DoomMap() = default;
	std::vector<Vertex> vertices;
	std::vector<Linedef> linedefs;
	std::vector<Sidedef> sidedefs;
	std::vector<Sector> sectors;

	std::vector<std::vector<Triangle>> getTriangles(TextureManager& tm);
};