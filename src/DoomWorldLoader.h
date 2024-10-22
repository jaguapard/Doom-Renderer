#pragma once
#include <vector>

#include "Triangle.h"
#include "DoomStructs.h"
#include "TextureManager.h"
#include "Model.h"

class DoomWorldLoader
{
public:
	static std::vector<std::vector<Model>> loadMapSectorsAsModels(const std::vector<Linedef>& linedefs, const std::vector<Vertex>& vertices, const std::vector<Sidedef>& sidedefs, const std::vector<Sector>& sectors, TextureManager& textureManager);
private:
	struct SectorInfo //info about the sector in relation to linedef being processed. This struct is for internal use
	{
		int sidedefNumber, floorHeight, ceilingHeight, sectorNumber, xTextureOffset, yTextureOffset;
		std::string upperTexture, middleTexture, lowerTexture;
		std::vector<Triangle> triangles;
	};

	static Model getTrianglesForSectorWallQuads(real bottomHeight, real topHeight, const std::array<Vec4, 6>& quadVerts, const SectorInfo& sectorInfo, const std::string& textureName, TextureManager& textureManager);
	static std::vector<Model> triangulateFloorsAndCeilingsForSector(const Sector& sector, const std::vector<Linedef>& sectorLinedefs, const std::vector<Vertex>& vertices, TextureManager& textureManager);
};