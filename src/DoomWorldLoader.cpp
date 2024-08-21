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

		std::array<Vec4, 6> linedef3dVerts;
		linedef3dVerts[0] = { real(sv.x), 0, real(sv.y) }; //quads originated by a linedef will always only differ in height
		linedef3dVerts[1] = { real(sv.x), -1, real(sv.y) }; //-1 = lower
		linedef3dVerts[2] = { real(ev.x), 0, real(ev.y) }; //0 in y coordinate means that it expects the higher of two heights,

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

				auto model = getTrianglesForSectorWallQuads(low.floorHeight, high.floorHeight, linedef3dVerts, high, low.lowerTexture, textureManager);
				auto& target = sectorModels[low.sectorNumber]; //TODO: think about which sector to assign this triangles to
				target.push_back(model);
			}

			auto preSortLinedefSectors = linedefSectors;
			std::sort(linedefSectors.begin(), linedefSectors.end(), [](const SectorInfo& si1, const SectorInfo& si2) {return si1.ceilingHeight < si2.ceilingHeight; });
			{   //upper sections
				SectorInfo low = linedefSectors[0];
				SectorInfo high = linedefSectors[1];

				auto model = getTrianglesForSectorWallQuads(low.ceilingHeight, high.ceilingHeight, linedef3dVerts, high, high.upperTexture, textureManager);
				if (preSortLinedefSectors[0].sectorNumber == linedefSectors[0].sectorNumber) model.swapVertexOrder();
				auto& target = sectorModels[high.sectorNumber]; //TODO: think about which sector to assign this triangles to
				target.push_back(model);
			}
		}
	}

	for (int nSector = 0; nSector < sectors.size(); ++nSector)
	{
		auto triangulation = triangulateFloorsAndCeilingsForSector(sectors[nSector], sectorLinedefs[nSector], vertices, textureManager);
		auto& target = sectorModels[nSector];
		target.insert(target.end(), triangulation.begin(), triangulation.end());

		size_t sum = 0;
		for (auto& it : triangulation) sum += it.getTriangleCount();
		//std::cout << "Sector " << nSector << " got split into " << sum << " triangles.\n";
	}

	return sectorModels;
}

Model DoomWorldLoader::getTrianglesForSectorWallQuads(real bottomHeight, real topHeight, const std::array<Vec4, 6>& quadVerts, const SectorInfo& sectorInfo, const std::string& textureName, TextureManager& textureManager)
{
	Model ret;
	if (textureName.empty() || textureName == "-" || bottomHeight == topHeight) return ret; //nonsensical arrangement or undefined texture

	Vec4 origin;
	if (bottomHeight > topHeight) std::swap(bottomHeight, topHeight);

	std::vector<Triangle> triangles;
	//TODO: add some kind of Z figting prevention for double-sided linedefs
	int textureIndex = textureManager.getTextureIndexByName(textureName);
	const Texture& texture = textureManager.getTextureByIndex(textureIndex);
	for (int i = 0; i < 2; ++i)
	{
		Triangle t;
		for (int j = 0; j < 3; ++j)
		{
			Vec4 cookedVert = quadVerts[i * 3 + j];
			cookedVert.y = cookedVert.y == 0 ? topHeight : bottomHeight;
			t.tv[j].spaceCoords = cookedVert;
			if (i * 3 + j == 0) origin = cookedVert; //if this is the first vertice processed, then save it as an origin for following texture coordinate calculation

			Vec4 worldOffset = t.tv[j].spaceCoords - origin;
			Vec2 uvPrefab;
			uvPrefab.x = std::max(abs(worldOffset.x), abs(worldOffset.z)) == abs(worldOffset.x) ? worldOffset.x : worldOffset.z;
			uvPrefab.y = worldOffset.y;
			Vec2 uv = Vec2(sectorInfo.xTextureOffset, sectorInfo.yTextureOffset) - uvPrefab;
			t.tv[j].textureCoords = Vec4(uv.x, uv.y) / Vec4(texture.getW(), texture.getH());
		}
		triangles.push_back(t);
	}
	return Model(triangles, textureIndex);
}

std::vector<Model> DoomWorldLoader::triangulateFloorsAndCeilingsForSector(const Sector& sector, const std::vector<Linedef>& sectorLinedefs, const std::vector<Vertex>& vertices, TextureManager& textureManager)
{
	std::vector<Triangle> trisFloor, trisCeiling;

	if (sectorLinedefs.size() < 3) return {};
	assert(sectorLinedefs.size() >= 3);

	std::vector<Line> polygonLines;
	for (const auto& ldf : sectorLinedefs)
	{
		Vertex sv = vertices[ldf.startVertex];
		Vertex ev = vertices[ldf.endVertex];
		polygonLines.push_back({ Ved2(sv.x, sv.y), Ved2(ev.x, ev.y) });
	}

	auto polygonSplit = PolygonTriangulator::triangulate(polygonLines);
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
			Vec4 vert = Vec4(real(_2vert.x), 0, real(_2vert.y));
			Vec4 uv = vert - Vec4(uvOffset.x, uvOffset.y);
			t[j / 3].tv[j % 3].spaceCoords = vert;
			t[j/3].tv[j%3].spaceCoords.y = isFloor ? sector.floorHeight : sector.ceilingHeight;

			const Texture& texture = textureManager.getTextureByIndex(isFloor ? floorTextureIndex : ceilingTextureIndex);
			t[j / 3].tv[j % 3].textureCoords = Vec4(uv.x, uv.z) / Vec4(texture.getW(), texture.getH());
		}

		trisFloor.push_back(t[0]);
		trisCeiling.push_back(t[1]);
	}

	assert(trisFloor.size() == trisCeiling.size());
	for (int i = 0; i < trisFloor.size(); ++i)
	{
		const Vec4 up = { 0, 1, 0 };
		Vec4 nf = trisFloor[i].getNormalVector();
		Vec4 nc = trisCeiling[i].getNormalVector();

		//enforce proper vertex order. Floor normals must point up, and ceiling ones must point down
		if (nf.dot(up) <= 0) std::swap(trisFloor[i].tv[1], trisFloor[i].tv[2]);
		if (nc.dot(up) > 0) std::swap(trisCeiling[i].tv[1], trisCeiling[i].tv[2]);
	}
	return { Model(trisFloor, floorTextureIndex), Model(trisCeiling, ceilingTextureIndex) };
}