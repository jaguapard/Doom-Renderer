#include "DoomWorldLoader.h"
#include "DoomStructs.h"
#include "TextureManager.h"

std::vector<std::vector<Triangle>> DoomWorldLoader::loadTriangles(
	const std::vector<Linedef>& linedefs,
	const std::vector<Vertex>& vertices,
	const std::vector<Sidedef>& sidedefs,
	const std::vector<Sector>& sectors,
	TextureManager& textureManager
)
{
	std::vector<std::vector<Triangle>> sectorTriangles(sectors.size());
	for (const auto& linedef : linedefs)
	{
		Vertex sv = vertices[linedef.startVertex];
		Vertex ev = vertices[linedef.endVertex];

		std::array<Vec3, 6> linedef3dVerts;
		linedef3dVerts[0] = { double(sv.x), 0, double(sv.y) }; //quads originated by a linedef will always only differ in height
		linedef3dVerts[1] = { double(ev.x), 0, double(ev.y) }; //0 in y coordinate means that it expects the higher of two heights,
		linedef3dVerts[2] = { double(sv.x), -1, double(sv.y) }; //-1 = lower

		linedef3dVerts[3] = { double(ev.x), 0, double(ev.y) };
		linedef3dVerts[4] = { double(sv.x), -1, double(sv.y) };
		linedef3dVerts[5] = { double(ev.x), -1, double(ev.y) };

		std::array<int, 2> sidedefNumbers = { linedef.frontSidedef, linedef.backSidedef };
		std::vector<SectorInfo> linedefSectors; //gather info about sectors this linedef separates to build triangles later
		for (int i = 0; i < 2; ++i)
		{
			int nSidedef = sidedefNumbers[i];
			if (nSidedef == -1) continue; //1 sided linedef

			const Sidedef& sidedef = sidedefs[nSidedef];
			int nSector = sidedef.facingSector;
			const Sector& sector = sectors[nSector];

			SectorInfo sectorInfo;
			sectorInfo.floorHeight = sector.floorHeight;
			sectorInfo.ceilingHeight = sector.ceilingHeight;
			sectorInfo.sectorNumber = nSector;

			sectorInfo.lowerTexture = wadStrToStd(sidedef.lowerTexture);
			sectorInfo.middleTexture = wadStrToStd(sidedef.middleTexture);
			sectorInfo.upperTexture = wadStrToStd(sidedef.upperTexture);

			sectorInfo.xTextureOffset = sidedef.xTextureOffset;
			sectorInfo.yTextureOffset = sidedef.yTextureOffset;

			linedefSectors.push_back(sectorInfo);
		}

		std::vector<Triangle> triangles;

		if (linedefSectors.size() == 1) //if there's only 1 sector, then just add the triangles to the list
		{
			const SectorInfo& si = linedefSectors[0];
			double bottom = si.floorHeight;
			double top = si.ceilingHeight;
			for (auto& it : getTrianglesForSectorQuads(bottom, top, linedef3dVerts, linedefSectors[0], si.middleTexture, textureManager))
				sectorTriangles[si.sectorNumber].push_back(it);
		}		
	}

	return sectorTriangles;
}

std::vector<Triangle> DoomWorldLoader::getTrianglesForSectorQuads(double bottomHeight, double topHeight, const std::array<Vec3, 6>& quadVerts, const SectorInfo& sectorInfo, const std::string& textureName, TextureManager& textureManager)
{
	std::vector<Triangle> ret;
	Vec3 origin;

	for (int i = 0; i < 2; ++i)
	{
		Triangle t;
		for (int j = 0; j < 3; ++j)
		{
			Vec3 cookedVert = quadVerts[i * 3 + j];
			cookedVert.y = cookedVert.y == 0 ? topHeight : bottomHeight;
			t.tv[j].worldCoords = cookedVert;
			if (i * 3 + j == 0) origin = cookedVert; //if this is the first vertice processed, then save it as an origin for following texture coordinate calculation

			Vec3 worldOffset = t.tv[j].worldCoords - origin;
			Vec2 uvPrefab;
			uvPrefab.x = std::max(abs(worldOffset.x), abs(worldOffset.z)) == abs(worldOffset.x) ? worldOffset.x : worldOffset.z;
			uvPrefab.y = worldOffset.y;
			Vec2 uv = Vec2(sectorInfo.xTextureOffset, sectorInfo.yTextureOffset) - uvPrefab;
			t.tv[j].textureCoords = uv;			
		}
		t.textureIndex = textureManager.getTextureIndexByName(textureName);
		ret.push_back(t);
	}
	return ret;
}
