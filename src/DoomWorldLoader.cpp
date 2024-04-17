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

		auto triangulation = triangulateFloorsAndCeilingsForSector(sectors[nSector], sectorLinedefs[nSector], vertices, textureManager, debugEnabled && false ? -1 : 1); //too slow in debug mode
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

typedef std::pair<Ved2, Ved2> Line;
bool rayCrossesLine(const Ved2& p, const std::pair<Ved2, Ved2>& lines)
{
	const Ved2& sv = lines.first;
	const Ved2& ev = lines.second;

	//floating point precision is a bitch here, force use doubles everywhere
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
	
	//these are safeguards against rampant global find-and-replace mishaps breaking everything
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

bool isPointInsidePolygon(const Ved2& p, const std::vector<std::pair<Ved2, Ved2>>& lines)
{
	assert(lines.size() >= 3);
	int intersections = 0;
	for (const auto& it : lines) intersections += rayCrossesLine(p, it);
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
			bitmap[y * w + x] = isPointInsidePolygon(Vec2(x + minX, y + minY), polygonLines);
		}
	}

	static int count = -1;
	++count;
	saveBitmap(bitmap, w, h, "sectors_debug/" + std::to_string(count) + "_initial.png");

	std::vector<Ved2> ret;

	for (int i = 0; i < polygonLines.size(); ++i)
	{
		Line line = polygonLines[i];
		if (line.first.x == line.second.x || line.first.y == line.second.y) continue; //skip perfectly vertical or perfectly horizontal lines

		if (line.first.y > line.second.y) std::swap(line.first, line.second); //enforce increasing Y
		double ySpan = line.second.y - line.first.y;

		int pass = 0;
	look:
		++pass;
		assert(pass == 1 || pass == 2);
		double expansionY = line.first.y;
		for (; expansionY < line.second.y; ++expansionY) //try growing y until we find first y that makes our candidate triangle to spill outside the polygon
		{
			double yp = (expansionY - line.first.y) / ySpan;
			double xv = lerp<double>(line.first.x, line.second.x, yp); //we carve only right angle triangles, so only 1 lerp is needed
			double xLeft = xv;
			double xRight = pass == 1 ? line.first.x : line.second.x;
			if (xLeft > xRight) std::swap(xLeft, xRight);
			for (double x = xLeft; x < xRight; ++x) //or <=?
			{
				if (!bitmap[int(expansionY - minY) * w + int(x - minX)]) // point outside polygon, stop expansion attempts. Doubles must be truncated, since y*w may become intermediate integers between floor(y)*w and ceiling(y)*w
				{
					goto triangleOutsidePolygon;
				}
			}
		}

	triangleOutsidePolygon:
		if (expansionY == line.first.y)
		{
			//if (pass == 1) goto look;
			continue; //if the first expansion attempt failed, just go to the next line
		}
		//else, shear expanded line and mark points as occupied
		for (double ny = line.first.y; ny < expansionY; ++ny)
		{
			double yp = (ny - line.first.y) / ySpan;
			double xv = lerp<double>(line.first.x, line.second.x, yp);
			double xLeft = xv;
			double xRight = line.first.x;
			if (xLeft > xRight) std::swap(xLeft, xRight);
			for (double x = xLeft; x < xRight; ++x) //or <=?
			{
				bitmap[int(ny - minY) * w + int(x - minX)] = false;
			}
		}

		double expansionAlpha = (expansionY - line.first.y) / ySpan;
		double xv = lerp<double>(line.first.x, line.second.x, expansionAlpha);
		Ved2 newVert = { line.first.x, expansionY };
		Ved2 shearEnd = { xv, expansionY };

		ret.push_back(line.first);
		ret.push_back(newVert);
		ret.push_back(shearEnd);
		//polygonLines[i] = std::make_pair(shearedStart, shearedEnd);
		if (expansionY < line.second.y)
		{
			polygonLines[i--] = std::make_pair(shearEnd, line.second); //if line was not fully consumed, shear it
			polygonLines.push_back({ line.first, newVert });
			polygonLines.push_back({ newVert, line.second });
		}
		else //else remove it
		{
			std::swap(polygonLines[i--], polygonLines.back());
			polygonLines.pop_back();
		}
	}
	saveBitmap(bitmap, w, h, "sectors_debug/" + std::to_string(count) + "_zcarved.png");

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
			auto lineBeg = bitmap.begin() + y * w + startingPoint.x;
			auto lineEnd = bitmap.begin() + y * w + endX;
			if (std::find(lineBeg, lineEnd, false) != lineEnd) break; // this line has points outside of the polygon, can't expand

			r.h++;
			std::fill(lineBeg, lineEnd, false);
		}
		r.x += minX;
		r.y += minY;
		rects.push_back(r);
	}

	for (const auto& it : rects)
	{
		ret.push_back(Ved2(it.x, it.y));
		ret.push_back(Ved2(it.x + it.w, it.y));
		ret.push_back(Ved2(it.x + it.w, it.y + it.h));

		ret.push_back(Ved2(it.x + it.w, it.y + it.h));
		ret.push_back(Ved2(it.x, it.y + it.h));
		ret.push_back(Ved2(it.x, it.y));
	}
	return ret;
}

std::vector<Triangle> DoomWorldLoader::triangulateFloorsAndCeilingsForSector(const Sector& sector, const std::vector<Linedef>& sectorLinedefs, const std::vector<Vertex>& vertices, TextureManager& textureManager, int tesselSize)
{
	std::vector<Triangle> ret;
	auto polygonSplit = orcishTriangulation(sectorLinedefs, vertices, tesselSize);	
	if (polygonSplit.empty()) return ret;

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
			Vec3 uv = vert - uvOffset;
			t[j / 3].tv[j % 3].spaceCoords = vert;
			t[j/3].tv[j%3].spaceCoords.y = isFloor ? sector.floorHeight : sector.ceilingHeight;
			t[j / 3].tv[j % 3].textureCoords = Vec2(uv.x, uv.z);
			t[j/3].textureIndex = isFloor ? floorTextureIndex : ceilingTextureIndex;
		}

		ret.push_back(t[0]);
		ret.push_back(t[1]);
	}
	return ret;
}