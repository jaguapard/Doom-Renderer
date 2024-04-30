#include "DoomWorldLoader.h"
#include "DoomStructs.h"
#include "TextureManager.h"
#include "helpers.h"
#include "PolygonTriangulator.h"

#include <iostream>

std::vector<std::vector<Model>> DoomWorldLoader::loadMapSectorsAsModels(
	const std::vector<Linedef>& linedefs,
	const std::vector<Vertex>& vertices,
	const std::vector<Sidedef>& sidedefs,
	const std::vector<Sector>& sectors,
	TextureManager& textureManager
)
{
	std::vector<std::vector<Model>> sectorModels(sectors.size());
	std::vector<std::vector<Linedef>> sectorLinedefs(sectors.size());

	for (const auto& linedef : linedefs)
	{
		Vertex sv = vertices[linedef.startVertex];
		Vertex ev = vertices[linedef.endVertex];
		//if ((linedef.startVertex == 124 && linedef.endVertex == 128) || (linedef.startVertex == 128 && linedef.endVertex == 124)) __debugbreak();

		std::array<Vec3, 6> linedef3dVerts;
		linedef3dVerts[0] = { real(sv.x), 0, real(sv.y) }; //quads originated by a linedef will always only differ in height
		linedef3dVerts[1] = { real(ev.x), 0, real(ev.y) }; //0 in y coordinate means that it expects the higher of two heights,
		linedef3dVerts[2] = { real(sv.x), -1, real(sv.y) }; //-1 = lower

		linedef3dVerts[3] = { real(ev.x), 0, real(ev.y) };
		linedef3dVerts[4] = { real(sv.x), -1, real(sv.y) };
		linedef3dVerts[5] = { real(ev.x), -1, real(ev.y) };

		std::array<int, 2> sidedefNumbers = { linedef.frontSidedef, linedef.backSidedef };

		std::vector<SectorInfo> linedefSectors; //gather info about sectors this linedef separates to build triangles later
		for (int i = 0; i < 2; ++i)
		{
			int nSidedef = sidedefNumbers[i];
			if (nSidedef == -1) continue; //1 sided linedef

			const Sidedef& sidedef = sidedefs[nSidedef];
			int nSector = sidedef.facingSector;
			const Sector& sector = sectors[nSector];
			sectorLinedefs[nSector].push_back(linedef); //make a list of sector's linedefs for further floor and ceiling creation

			SectorInfo sectorInfo;
			sectorInfo.sidedefNumber = nSidedef;
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

		std::sort(linedefSectors.begin(), linedefSectors.end(), [](const SectorInfo& si1, const SectorInfo& si2) {return si1.floorHeight < si2.floorHeight; });
		//middle section of the wall is a section between higher floor and lower ceiling
		{
			int higherFloor = linedefSectors[0].floorHeight;
			int lowerCeiling = linedefSectors[0].ceilingHeight;
			if (linedefSectors.size() > 1)
			{
				higherFloor = std::max(higherFloor, linedefSectors[1].floorHeight);
				lowerCeiling = std::min(lowerCeiling, linedefSectors[1].ceilingHeight);
			}
	
			const auto& si = linedefSectors[0];
			auto model = getTrianglesForSectorWallQuads(higherFloor, lowerCeiling, linedef3dVerts, si, si.middleTexture, textureManager);
			auto& target = sectorModels[si.sectorNumber];
			target.push_back(model);
		}

		if (linedefSectors.size() > 1)
		{
			{	//lower sections
				SectorInfo low = linedefSectors[0];
				SectorInfo high = linedefSectors[1];

				auto model = getTrianglesForSectorWallQuads(low.floorHeight, high.floorHeight, linedef3dVerts, high, low.lowerTexture, textureManager); //TODO: should probably do two calls, since the linedef can be double sided
				auto& target = sectorModels[low.sectorNumber]; //TODO: think about which sector to assign this triangles to
				target.push_back(model);
			}

			std::sort(linedefSectors.begin(), linedefSectors.end(), [](const SectorInfo& si1, const SectorInfo& si2) {return si1.ceilingHeight < si2.ceilingHeight; });
			{   //upper sections
				SectorInfo low = linedefSectors[0];
				SectorInfo high = linedefSectors[1];

				auto model = getTrianglesForSectorWallQuads(low.ceilingHeight, high.ceilingHeight, linedef3dVerts, high, high.upperTexture, textureManager); //TODO: should probably do two calls, since the linedef can be double sided
				auto& target = sectorModels[high.sectorNumber]; //TODO: think about which sector to assign this triangles to
				target.push_back(model);
			}
		}
	}

	for (int nSector = 0; nSector < sectors.size(); ++nSector)
	{
		//tessel size of 1 causes a ton of flickering and floor/ceiling disappearing depending on camera movement. TODO: implement varying sizes in orcishTriangulation()
#ifdef NDEBUG
		bool debugEnabled = false;
#else
		bool debugEnabled = true;
#endif

		auto triangulation = triangulateFloorsAndCeilingsForSector(sectors[nSector], sectorLinedefs[nSector], vertices, textureManager, debugEnabled ? -1 : 1); //too slow in debug mode
		auto& target = sectorModels[nSector];
		target.insert(target.end(), triangulation.begin(), triangulation.end());
		std::cout << "Sector " << nSector << " got split into " << triangulation.size() << " triangles.\n";
	}

	return sectorModels;
}

Model DoomWorldLoader::getTrianglesForSectorWallQuads(real bottomHeight, real topHeight, const std::array<Vec3, 6>& quadVerts, const SectorInfo& sectorInfo, const std::string& textureName, TextureManager& textureManager)
{
	Model ret;
	if (textureName.empty() || textureName == "-" || bottomHeight == topHeight) return ret; //nonsensical arrangement or undefined texture

	Vec3 origin;
	if (bottomHeight > topHeight) std::swap(bottomHeight, topHeight);

	std::vector<Triangle> triangles;
	//TODO: add some kind of Z figting prevention for double-sided linedefs
	int textureIndex = textureManager.getTextureIndexByName(textureName);
	for (int i = 0; i < 2; ++i)
	{
		Triangle t;
		for (int j = 0; j < 3; ++j)
		{
			Vec3 cookedVert = quadVerts[i * 3 + j];
			cookedVert.y = cookedVert.y == 0 ? topHeight : bottomHeight;
			t.tv[j].spaceCoords = cookedVert;
			if (i * 3 + j == 0) origin = cookedVert; //if this is the first vertice processed, then save it as an origin for following texture coordinate calculation

			Vec3 worldOffset = t.tv[j].spaceCoords - origin;
			Vec2 uvPrefab;
			uvPrefab.x = std::max(abs(worldOffset.x), abs(worldOffset.z)) == abs(worldOffset.x) ? worldOffset.x : worldOffset.z;
			uvPrefab.y = worldOffset.y;
			Vec2 uv = Vec2(sectorInfo.xTextureOffset, sectorInfo.yTextureOffset) - uvPrefab;
			t.tv[j].textureCoords = { uv.x, uv.y };
		}
		triangles.push_back(t);
	}
	return Model(triangles, textureIndex, textureManager);
}

typedef std::pair<Ved2, Ved2> Line;

struct XRange
{
	int y, minX, maxX; //[minX, maxX)
};

/*returns a vector of triangles. UV and Y world coord MUST BE SET AFTERWARDS BY THE CALLER!
DOOM doesn't really give us too many constraints with it's sector shape, so good algorithms like ear clipping may fail (and I'm too lazy to write it anyway) 
*/
std::vector<Ved2> DoomWorldLoader::orcishTriangulation(std::vector<Linedef> sectorLinedefs, const std::vector<Vertex>& vertices, int tesselSize)
{
	if (tesselSize == -1) return std::vector<Ved2>();
	assert(sectorLinedefs.size() >= 3);

	std::vector<std::pair<Ved2, Ved2>> polygonLines;
	for (const auto& ldf : sectorLinedefs)
	{
		Vertex sv = vertices[ldf.startVertex];
		Vertex ev = vertices[ldf.endVertex];
		polygonLines.push_back(std::make_pair(Ved2(sv.x, sv.y), Ved2(ev.x, ev.y)));
	}

	auto ret = PolygonTriangulator::triangulate(polygonLines);

	return ret;
}

std::vector<Model> DoomWorldLoader::triangulateFloorsAndCeilingsForSector(const Sector& sector, const std::vector<Linedef>& sectorLinedefs, const std::vector<Vertex>& vertices, TextureManager& textureManager, int tesselSize)
{
	std::vector<Triangle> trisFloor, trisCeiling;
	auto polygonSplit = orcishTriangulation(sectorLinedefs, vertices, tesselSize);	
	if (polygonSplit.empty()) return {};

	real minX = std::min_element(polygonSplit.begin(), polygonSplit.end(), [](const Ved2& v1, const Ved2& v2) {return v1.x < v2.x; })->x;
	real minY = std::min_element(polygonSplit.begin(), polygonSplit.end(), [](const Ved2& v1, const Ved2& v2) {return v1.y < v2.y; })->y;
	Vec2 uvOffset = { minX, minY };

	int floorTextureIndex = textureManager.getTextureIndexByName(wadStrToStd(sector.floorTexture));
	int ceilingTextureIndex = textureManager.getTextureIndexByName(wadStrToStd(sector.ceilingTexture));

	for (int i = 0; i < polygonSplit.size(); i += 3)
	{
		Triangle t[2];
		for (int j = 0; j < 6; ++j)
		{
			bool isFloor = j < 3;
			Ved2 _2vert = polygonSplit[i + j % 3];
			Vec3 vert = Vec3(real(_2vert.x), 0, real(_2vert.y));
			Vec3 uv = vert - Vec3(uvOffset.x, uvOffset.y);
			t[j / 3].tv[j % 3].spaceCoords = vert;
			t[j/3].tv[j%3].spaceCoords.y = isFloor ? sector.floorHeight : sector.ceilingHeight;
			t[j / 3].tv[j % 3].textureCoords = Vec3(uv.x, uv.z);
		}

		trisFloor.push_back(t[0]);
		trisCeiling.push_back(t[1]);
	}
	return { Model(trisFloor, floorTextureIndex, textureManager), Model(trisCeiling, ceilingTextureIndex, textureManager) };
}