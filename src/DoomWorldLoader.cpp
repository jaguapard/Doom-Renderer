#include "DoomWorldLoader.h"
#include "DoomStructs.h"
#include "TextureManager.h"
#include "helpers.h"

#include <iostream>

std::vector<std::vector<Triangle>> DoomWorldLoader::loadTriangles(
	const std::vector<Linedef>& linedefs,
	const std::vector<Vertex>& vertices,
	const std::vector<Sidedef>& sidedefs,
	const std::vector<Sector>& sectors,
	TextureManager& textureManager
)
{
	std::vector<std::vector<Triangle>> sectorTriangles(sectors.size());
	std::vector<std::vector<Linedef>> sectorLinedefs(sectors.size());

	for (const auto& linedef : linedefs)
	{
		Vertex sv = vertices[linedef.startVertex];
		Vertex ev = vertices[linedef.endVertex];

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
		for (const auto& si : linedefSectors) //first add middle sections of walls 
		{
			auto newTris = getTrianglesForSectorWallQuads(si.floorHeight, si.ceilingHeight, linedef3dVerts, si, si.middleTexture, textureManager);
			auto& target = sectorTriangles[si.sectorNumber];
			target.insert(target.end(), newTris.begin(), newTris.end());
		}

		if (linedefSectors.size() > 1)
		{
			{	//lower sections
				SectorInfo low = linedefSectors[0];
				SectorInfo high = linedefSectors[1];

				auto newTris = getTrianglesForSectorWallQuads(low.floorHeight, high.floorHeight, linedef3dVerts, high, low.lowerTexture, textureManager); //TODO: should probably do two calls, since the linedef can be real sided
				auto& target = sectorTriangles[low.sectorNumber]; //TODO: think about which sector to assign this triangles to
				target.insert(target.end(), newTris.begin(), newTris.end());
			}

			std::sort(linedefSectors.begin(), linedefSectors.end(), [](const SectorInfo& si1, const SectorInfo& si2) {return si1.ceilingHeight < si2.ceilingHeight; });
			{   //upper sections
				SectorInfo low = linedefSectors[0];
				SectorInfo high = linedefSectors[1];

				auto newTris = getTrianglesForSectorWallQuads(low.ceilingHeight, high.ceilingHeight, linedef3dVerts, high, high.upperTexture, textureManager); //TODO: should probably do two calls, since the linedef can be real sided
				auto& target = sectorTriangles[high.sectorNumber]; //TODO: think about which sector to assign this triangles to
				target.insert(target.end(), newTris.begin(), newTris.end());
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
		auto& target = sectorTriangles[nSector];
		target.insert(target.end(), triangulation.begin(), triangulation.end());
		std::cout << "Sector " << nSector << " got split into " << triangulation.size() << " triangles.\n";
	}

	return sectorTriangles;
}

std::vector<Triangle> DoomWorldLoader::getTrianglesForSectorWallQuads(real bottomHeight, real topHeight, const std::array<Vec3, 6>& quadVerts, const SectorInfo& sectorInfo, const std::string& textureName, TextureManager& textureManager)
{
	std::vector<Triangle> ret;
	if (textureName.empty() || textureName == "-" || bottomHeight == topHeight) return ret; //nonsensical arrangement or undefined texture

	Vec3 origin;
	if (bottomHeight > topHeight) std::swap(bottomHeight, topHeight);

	//TODO: add some kind of Z figting prevention for double-sided linedefs
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
			t.tv[j].textureCoords = uv;			
		}
		t.textureIndex = textureManager.getTextureIndexByName(textureName);
		ret.push_back(t);
	}
	return ret;
}

real scalarCross2d(Vec2 a, Vec2 b)
{
	return a.x * b.y - a.y * b.x;
}

bool rayCrossesLine(const Ved2& p, const Linedef& line, const std::vector<Vertex>& vertices)
{
	Vertex _sv = vertices[line.startVertex];
	Vertex _ev = vertices[line.endVertex];

	//floating point precision is a bitch here, force use doubles everywhere
	Ved2 sv = { double(_sv.x), double(_sv.y) };
	Ved2 ev = Ved2(double(_ev.x), double(_ev.y));

	Ved2 v1 = p - sv;
	Ved2 v2 = ev - sv;
	Ved2 v3 = { -sqrt(2), sqrt(3) }; //using irrational values to never get pucked by linedefs being perfectly collinear to our ray by chance
	v3 /= v3.len(); //idk, seems like a good measure
	double dot = v2.dot(v3);
	//if (abs(dot) < 1e-9) return false;

	double t1 = scalarCross2d(v2, v1) / dot;
	double t2 = v1.dot(v3) / dot;

	//we are fine with false positives (point is considered inside when it's not), but false negatives are absolutely murderous
	if (t1 >= 1e-9 && (t2 >= 1e-9 && t2 <= 1.0 - 1e-9))
		return true;
	return false;
	
	//this is a safeguard against rampant global find-and-replace
	static_assert(sizeof(sv) == 16);
	static_assert(sizeof(ev) == 16);
	static_assert(sizeof(v1) == 16);
	static_assert(sizeof(v2) == 16);
	static_assert(sizeof(v3) == 16);
	static_assert(sizeof(dot) == 8);
	static_assert(sizeof(t1) == 8);
	static_assert(sizeof(t2) == 8);
	static_assert(sizeof(p) == 16);
}

bool isPointInsidePolygon(const Ved2& p, const std::vector<Linedef>& lines, const std::vector<Vertex>& vertices)
{
	int intersections = 0;
	for (const auto& it : lines) intersections += rayCrossesLine(p, it, vertices);
	return intersections % 2 == 1;
}

struct XRange
{
	int y, minX, maxX; //[minX, maxX)
};

void saveBitmap(const std::vector<bool>& bitmap, int w, int h, std::string fName)
{
	std::vector<uint32_t> pixels(w * h);
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			pixels[y * w + x] = bitmap[y * w + x] ? 0xFFFFFFFF : 0x000000FF;
		}
	}
	SDL_Surface* png = SDL_CreateRGBSurfaceWithFormatFrom(&pixels.front(), w, h, 32, w * 4, SDL_PIXELFORMAT_ABGR32);
	IMG_SavePNG(png, fName.c_str());
	SDL_FreeSurface(png);
}


/*returns a vector of triangles. UV and Y world coord MUST BE SET AFTERWARDS BY THE CALLER!
DOOM doesn't really give us too many constraints with it's sector shape, so good algorithms like ear clipping may fail (and I'm too lazy to write it anyway) 
*/
std::vector<Vec3> DoomWorldLoader::orcishTriangulation(std::vector<Linedef> sectorLinedefs, const std::vector<Vertex>& vertices, int tesselSize)
{
	if (tesselSize == -1) return std::vector<Vec3>();
	assert(sectorLinedefs.size() >= 3);

	/*
	The gyst: draw lines on a bitmap, then flood fill and turn all "pixels" into pairs of triangles.
	*/
	std::vector<Vertex> vs;
	int minX = INT32_MAX;
	int maxX = INT32_MIN;
	int minY = INT32_MAX;
	int maxY = INT32_MIN;
	for (const auto& it : sectorLinedefs) //scan all linedefs to find offsets and size for bitmap
	{
		Vertex sv = vertices[it.startVertex];
		Vertex ev = vertices[it.endVertex];
		minX = std::min<int>(minX, std::min(sv.x, ev.x));
		minY = std::min<int>(minY, std::min(sv.y, ev.y));
		maxX = std::max<int>(maxX, std::max(sv.x, ev.x));
		maxY = std::max<int>(maxY, std::max(sv.y, ev.y));
	}

	int w = maxX - minX + 1;
	int h = maxY - minY + 1;
	std::vector<bool> bitmap(w * h, false);
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			bitmap[y * w + x] = isPointInsidePolygon(Vec2(x + minX, y + minY), sectorLinedefs, vertices);
		}
	}

	static int count = -1;
	++count;
	saveBitmap(bitmap, w, h, "sectors_debug/" + std::to_string(count) + "_initial.png");

	std::vector<Vec3> ret;
	if (false)
	{
		//carve out right angle triangles
		for (const auto& it : sectorLinedefs)
		{
			Vertex sv = vertices[it.startVertex];
			Vertex ev = vertices[it.endVertex];
			if (sv.x == ev.x || sv.y == ev.y) continue; //it is a horizontal or vertical line, we want only slopes

			//a triangle will have two vertices from the affected linedef, and a third will be calculated from them to reside inside the sector and have right angle. 
			// There are only 2 such possible arrangments
			Vec2 candidate(sv.x, ev.y);
			Vec3 v[3];
			if (!isPointInsidePolygon(candidate, sectorLinedefs, vertices)) candidate = Vec2(ev.x, sv.y);
			ret.push_back(v[0] = Vec3(sv.x, 0, sv.y));
			ret.push_back(v[1] = Vec3(ev.x, 0, ev.y));
			ret.push_back(v[2] = Vec3(candidate.x, 0, candidate.y));

			std::sort(std::begin(v), std::end(v), [](const Vec3& a, const Vec3& b) {return a.z < b.z; });
			real span = v[2].z - v[0].z;
			assert(span > 0);
			bool leftStraight = v[0].x == v[2].x;
			bool rightStraight = v[1].x == v[2].x;
			for (real y = v[0].z; y < v[2].z; ++y)
			{
				real yp = (y - v[0].z) / span;
				assert(yp >= 0 && yp <= 1);
				real xLeft = lerp(v[0].x, v[1].x, rightStraight ? 0 : leftStraight ? 1 : yp); //breaks if x0 == x2 and NOT rightStraight (always == 2)
				real xRight = lerp(v[0].x, v[2].x, rightStraight ? 1 : yp); //and this also always == 0 if previous conditions are met
				if (xLeft > xRight) std::swap(xLeft, xRight);
				for (real x = xLeft; x < xRight; ++x)
				{
					bitmap[int(y - minY) * w + int(x - minX)] = false;
				}
			}
		}
		saveBitmap(bitmap, w, h, "sectors_debug/" + std::to_string(count) + "_post_tri_carve.png");
	}
	


	std::vector<SDL_Rect> rects;
	while (true)
	{
		auto firstFreeIt = std::find(bitmap.begin(), bitmap.end(), true);
		if (firstFreeIt == bitmap.end()) break;

		int firstFreePos = firstFreeIt - bitmap.begin();

		SDL_Point startingPoint = { firstFreePos % w, firstFreePos / w };
		int initialWidth = std::find(firstFreeIt, bitmap.begin() + (startingPoint.y + 1) * w, false) - firstFreeIt;
		assert(initialWidth > 0);
		int endX = startingPoint.x + initialWidth;
		std::fill(firstFreeIt, firstFreeIt + initialWidth, false);

		SDL_Rect r = { startingPoint.x, startingPoint.y, initialWidth, 1 };
		for (int y = startingPoint.y + 1; y < h; ++y)
		{
			bool lineGoodForExpansion = true;
			for (int x = startingPoint.x; x < endX; ++x)
			{
				if (!bitmap[y * w + x])
				{
					lineGoodForExpansion = false;
					break;
				}
			}

			if (lineGoodForExpansion)
			{
				r.h++;
				std::fill(bitmap.begin() + y * w + startingPoint.x, bitmap.begin() + y * w + endX, false);
			}
			else
			{
				break;
			}
		}
		r.x += minX;
		r.y += minY;
		rects.push_back(r);
	}

	for (const auto& it : rects)
	{
		ret.push_back(Vec3(it.x, 0, it.y));
		ret.push_back(Vec3(it.x + it.w, 0, it.y));
		ret.push_back(Vec3(it.x + it.w, 0, it.y + it.h));

		ret.push_back(Vec3(it.x + it.w, 0, it.y + it.h));
		ret.push_back(Vec3(it.x, 0, it.y + it.h));
		ret.push_back(Vec3(it.x, 0, it.y));
	}
	return ret;
}

std::vector<Triangle> DoomWorldLoader::triangulateFloorsAndCeilingsForSector(const Sector& sector, const std::vector<Linedef>& sectorLinedefs, const std::vector<Vertex>& vertices, TextureManager& textureManager, int tesselSize)
{
	std::vector<Triangle> ret;
	auto polygonSplit = orcishTriangulation(sectorLinedefs, vertices, tesselSize);	
	if (polygonSplit.empty()) return ret;

	real minX = std::min_element(polygonSplit.begin(), polygonSplit.end(), [](const Vec3& v1, const Vec3& v2) {return v1.x < v2.x; })->x;
	real minZ = std::min_element(polygonSplit.begin(), polygonSplit.end(), [](const Vec3& v1, const Vec3& v2) {return v1.z < v2.z; })->z;
	Vec3 uvOffset = { minX, minZ, 0 };

	int floorTextureIndex = textureManager.getTextureIndexByName(wadStrToStd(sector.floorTexture));
	int ceilingTextureIndex = textureManager.getTextureIndexByName(wadStrToStd(sector.ceilingTexture));

	for (int i = 0; i < polygonSplit.size(); i += 3)
	{
		Triangle t[2];
		for (int j = 0; j < 6; ++j)
		{
			bool isFloor = j < 3;
			Vec3 uv = polygonSplit[i + j%3] - uvOffset;
			t[j/3].tv[j%3].spaceCoords = polygonSplit[i + j%3];
			t[j/3].tv[j%3].spaceCoords.y = isFloor ? sector.floorHeight : sector.ceilingHeight;
			t[j / 3].tv[j % 3].textureCoords = Vec2(uv.x, uv.z);
			t[j/3].textureIndex = isFloor ? floorTextureIndex : ceilingTextureIndex;
		}

		ret.push_back(t[0]);
		ret.push_back(t[1]);
	}
	return ret;
}