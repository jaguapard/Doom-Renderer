#pragma once
#include <vector>

#include "Triangle.h"

class DoomWorldLoader
{
public:
	static std::vector<std::vector<Triangle>> loadTriangles();
private:
	struct SectorInfo //info about the sector in relation to linedef being processed. This struct is for internal use
	{
		int sidedefNumber, floorHeight, ceilingHeight, sectorNumber, xTextureOffset, yTextureOffset;
		std::string upperTexture, middleTexture, lowerTexture;
		std::vector<Triangle> triangles;
	};

	static std::vector<Triangle> getTrianglesForSectorQuads(double bottomHeight, double topHeight, const std::array<Vec3, 6>& quadVerts, const SectorInfo& sectorInfo, const std::string& textureName);
};