#include <SDL\SDL.h>
#include <SDL\SDL_image.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <algorithm>
#include <array>

#include <adm/strings.h>

#include "C_Input.h"
#include "CoordinateTransformer.h"
#include "Matrix3.h"
#include "Texture.h"
#include "ZBuffer.h"
#include "Triangle.h"

#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2_image.lib")
#undef main

#pragma pack(push, 1)
struct Vertex
{
	int16_t x, y;
};

struct Linedef
{
	int16_t startVertex;
	int16_t endVertex;
	int16_t flags;
	int16_t specialType;
	int16_t sectorTag;
	int16_t frontSidedef;
	int16_t backSidedef;
};

struct Sidedef
{
	int16_t xTextureOffset;
	int16_t yTextureOffset;
	char upperTexture[8];
	char lowerTexture[8];
	char middleTexture[8];
	int16_t facingSector;
};

struct Sector
{
	int16_t floorHeight;
	int16_t ceilingHeight;
	char floorTexture[8];
	char ceilingTexture[8];
	int16_t lightLevel;
	int16_t specialType;
	int16_t tagNumber;
};
#pragma pack(pop)

std::vector<Vertex> vertices;
std::vector<Linedef> linedefs;
std::vector<Sidedef> sidedefs;
std::vector<Sector> sectors;

template <typename T>
void readRaw(T& ret, const void* bytes)
{
	ret = *reinterpret_cast<const T*>(bytes);
}

template <typename T>
T readRaw(const void* bytes)
{
	return *reinterpret_cast<const T*>(bytes);
}

template <typename T> 
std::vector<T> getVectorFromWad(int32_t lumpDataOffset, int32_t lumpSizeBytes, const std::vector<char>& wadBytes)
{
	std::vector<T> ret;
	for (int i = 0; i < lumpSizeBytes; i += sizeof(T))
	{
		T v = readRaw<T>(&wadBytes[lumpDataOffset + i]);
		ret.push_back(v);
	}
	return ret;
}

void loadPwad(std::string path)
{
	std::ifstream f(path, std::ios::binary);
	std::vector<char> wadBytes;
	while (f)
	{
		char b;
		f.read(&b, 1);
		wadBytes.push_back(b);
	}

	if (memcmp(&wadBytes.front(), "PWAD", 4))
		throw std::exception("Invalid PWAD file: header mismatch");

	int32_t numFiles = readRaw<int32_t>(&wadBytes[4]);
	int32_t FAToffset = readRaw<int32_t>(&wadBytes[8]);

	int32_t filePtr = FAToffset;
	for (int i = 0; i < numFiles; ++i)
	{
		char name[9] = { 0 };
		int32_t lumpDataOffset = readRaw<int32_t>(&wadBytes[filePtr]);
		int32_t lumpSizeBytes = readRaw<int32_t>(&wadBytes[filePtr + 4]);
		memcpy(name, &wadBytes[filePtr + 8], 8);
		std::cout << "Found WAD file: " << name << ", data offset: " << lumpDataOffset << ", size: " << lumpSizeBytes << "\n";

		if (!strcmp(name, "VERTEXES")) vertices = getVectorFromWad<Vertex>(lumpDataOffset, lumpSizeBytes, wadBytes);
		if (!strcmp(name, "LINEDEFS")) linedefs = getVectorFromWad<Linedef>(lumpDataOffset, lumpSizeBytes, wadBytes);
		if (!strcmp(name, "SIDEDEFS")) sidedefs = getVectorFromWad<Sidedef>(lumpDataOffset, lumpSizeBytes, wadBytes);
		if (!strcmp(name, "SECTORS")) sectors = getVectorFromWad<Sector>(lumpDataOffset, lumpSizeBytes, wadBytes);
		filePtr += 16;
	}
}

int getTextureIndexByName(std::string name, std::vector<Texture>& textures, std::unordered_map<std::string, int>& textureNameToIndexMap, const std::unordered_map<std::string,std::string>& textureNameTranslation)
{
	auto it = textureNameTranslation.find(name);
	std::string fileName = it == textureNameTranslation.end() ? name : it->second;
	if (textureNameToIndexMap.find(fileName) != textureNameToIndexMap.end()) return textureNameToIndexMap[fileName]; //find by file name

	textures.emplace_back(fileName);
	textureNameToIndexMap[fileName] = textures.size() - 1;
	return textures.size() - 1;
}

std::vector<Texture> textures;

std::unordered_map<std::string, std::string> loadTextureTranslation()
{
	std::unordered_map<std::string, std::string> ret;
	std::ifstream f("D:/Games/GZDoom/MappingTests/TEXTURES.txt");

	std::string line, textureName, fileName;
	while (std::getline(f, line))
	{
		if (line.empty() || line[0] == '/') continue;

		auto parts = adm::strings::split_copy(line, " ");
		if (parts[0] == "WallTexture") textureName = std::string(parts[1], 1, parts[1].length() - 3);
		if (parts[0].find("Patch") != parts[0].npos)
		{
			fileName = std::string(parts[1], 1, parts[1].length() - 3);
			ret[textureName] = fileName;
		}
		int a = 1;
	}

	return ret;
}

void main()
{
	auto textureNameTranslation = loadTextureTranslation();
	SDL_Init(SDL_INIT_EVERYTHING);
	//loadPwad("D:/Games/GZDoom/STUPID.wad");
	loadPwad("D:/Games/GZDoom/MappingTests/D2_MAP01.wad");

	std::vector<std::vector<int>> sectorSidedefIndices(sectors.size());
	std::vector<std::vector<int>> sidedefLinedefIndices(sidedefs.size());
	std::vector<std::vector<Vec3>> sectorVertices(sectors.size());
	std::unordered_map<std::string, int> textureNameToIndexMap;

	Vec3 camPosAndAngArchieve[] = {
		{ 0,0,0 }, {0,0,0},
		{ 0.1,32.1,370 }, {0,0,0}, //default view
		{ -45.9, 175.1, 145 }, { 0,0.086, 0.518 }, //buggy mess 1
		{0.1, 124.1, 188}, {0,0.012, 0.349}, //buggy mess 2
		{-96, 70, 784}, {0,0,0}, //doom 2 map 01 player start
	};

	int activeCamPosAndAngle = 4;
	Vec3 camPos = camPosAndAngArchieve[activeCamPosAndAngle * 2];
	Vec3 camAng = camPosAndAngArchieve[activeCamPosAndAngle * 2 + 1];

	for (int i = 0; i < sectors.size(); ++i)
	{
		for (auto& it : sidedefs)
		{
			if (it.facingSector == i)
			{
				sectorSidedefIndices[i].push_back(&it - &sidedefs.front());
			}
		}
	}	

	for (int i = 0; i < sidedefs.size(); ++i)
	{
		for (auto& it : linedefs)
		{
			if (it.frontSidedef == i || it.backSidedef == i)
			{
				sidedefLinedefIndices[i].push_back(&it - &linedefs.front());
			}
		}
	}

	std::vector<std::vector<Triangle>> sectorTriangles(sectors.size());

	for (int nSector = 0; nSector < sectors.size(); ++nSector)
	{
		const Sector& sector = sectors[nSector];
		for (int sidedefIndex : sectorSidedefIndices[nSector])
		{
			const Sidedef& sidedef = sidedefs[sidedefIndex];
			for (int linedefIndex : sidedefLinedefIndices[sidedefIndex])
			{
				const Linedef& linedef = linedefs[linedefIndex];
				char nameBuf[9] = { 0 };
				memcpy(nameBuf, sidedef.middleTexture, 8);
				if (!strcmp(nameBuf, "-")) continue; //skip sidedef if it has no middle texture

				Vertex sv = vertices[linedef.startVertex];
				Vertex ev = vertices[linedef.endVertex];
				double fh = sector.floorHeight;
				double ch = sector.ceilingHeight;
				bool sidedefIsBack = linedef.backSidedef == sidedefIndex;

				Vec3 verts[3][4];
				int textureIndices[3];
				
				verts[0][0] = {double(sv.x), fh, double(sv.y)}; //z is supposed to be depth, so swap with y. Doom uses y for depth, height as z, we take y as height
				verts[0][1] = { double(ev.x), fh, double(sv.y) };
				verts[0][2] = { double(ev.x), fh, double(ev.y) };
				verts[0][3] = { double(sv.x), fh, double(ev.y) };
				memset(nameBuf, 0, 9);
				memcpy(nameBuf, sector.floorTexture, 8);
				textureIndices[0] = getTextureIndexByName(nameBuf, textures, textureNameToIndexMap, textureNameTranslation);				

				verts[1][0] = { double(sv.x), ch, double(sv.y) };
				verts[1][1] = { double(ev.x), ch, double(sv.y) };
				verts[1][2] = { double(ev.x), ch, double(ev.y) };
				verts[1][3] = { double(sv.x), ch, double(ev.y) };
				memset(nameBuf, 0, 9);
				memcpy(nameBuf, sector.ceilingTexture, 8);
				textureIndices[1] = getTextureIndexByName(nameBuf, textures, textureNameToIndexMap, textureNameTranslation);

				verts[2][0] = { double(sv.x), ch, double(sv.y) };
				verts[2][1] = { double(ev.x), ch, double(ev.y) };
				verts[2][2] = { double(sv.x), fh, double(sv.y) };
				verts[2][3] = { double(ev.x), fh, double(ev.y) };	
				memset(nameBuf, 0, 9);
				memcpy(nameBuf, sidedef.middleTexture, 8);
				textureIndices[2] = getTextureIndexByName(nameBuf, textures, textureNameToIndexMap, textureNameTranslation);

				for (int quadNum = 0; quadNum < 3; ++quadNum)
				{
					for (int i = 0; i < 2; ++i)
					{
						Triangle t;
						t.textureIndex = textureIndices[quadNum];
						for (int j = 0; j < 3; ++j)
						{
							t.tv[j].worldCoords = verts[quadNum][j + i];
							Vec3 uv3 = verts[quadNum][j + i] - verts[quadNum][0];
							t.tv[j].textureCoords = { uv3.x + sidedef.xTextureOffset, uv3.y + sidedef.yTextureOffset };
						}
						sectorTriangles[nSector].emplace_back(t);
					}
				}
			}
		}
	}


	
	SDL_Window* wnd = SDL_CreateWindow("Doom Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, 0);
	SDL_Surface* wndSurf = SDL_GetWindowSurface(wnd);

	int framebufW = 320;
	int framebufH = 180;
	SDL_Surface* framebuf = SDL_CreateRGBSurface(0, framebufW, framebufH, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0xFF);
	C_Input input;
	CoordinateTransformer ctr(framebufW, framebufH);

	ZBuffer zBuffer(framebufW, framebufH);
	uint64_t frames = 0;
	while (true)
	{
		SDL_FillRect(framebuf, nullptr, 0);
		SDL_FillRect(wndSurf, nullptr, 0);
		zBuffer.clear();

		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			input.handleEvent(ev);
			if (input.isMouseButtonHeld(SDL_BUTTON_LEFT) && ev.type == SDL_MOUSEMOTION)
			{
				camAng += { 0, ev.motion.xrel * 1e-3, ev.motion.yrel * -1e-3};
			}
		}

		camPos -= { 1.0 * input.isButtonHeld(SDL_SCANCODE_D), 1.0 * input.isButtonHeld(SDL_SCANCODE_X), 1.0 * input.isButtonHeld(SDL_SCANCODE_W)};
		camPos += { 1.0 * input.isButtonHeld(SDL_SCANCODE_A), 1.0 * input.isButtonHeld(SDL_SCANCODE_Z), 1.0 * input.isButtonHeld(SDL_SCANCODE_S)};

		camAng += { 1e-2 * input.isButtonHeld(SDL_SCANCODE_R), 1e-2 * input.isButtonHeld(SDL_SCANCODE_T), 1e-2 * input.isButtonHeld(SDL_SCANCODE_Y)};
		camAng -= { 1e-2 * input.isButtonHeld(SDL_SCANCODE_F), 1e-2 * input.isButtonHeld(SDL_SCANCODE_G), 1e-2 * input.isButtonHeld(SDL_SCANCODE_H)};
		if (input.isButtonHeld(SDL_SCANCODE_C)) camPos = { 0.1,32.1,370 };
		if (input.isButtonHeld(SDL_SCANCODE_V)) camAng = { 0,0,0 };

		Matrix3 transformMatrix = getRotationMatrix(camAng);
		ctr.prepare(camPos, transformMatrix);

		for (int i = 0; i < sectors.size(); ++i)
		{
			//Vec3 testTri[3] = {{10,15,20}, {7, 20, 10}, {12, 11, 24}};
			for (int j = 0; j < sectorTriangles[i].size(); ++j) sectorTriangles[i][j].drawOn(framebuf, ctr, zBuffer, textures);
		}
		std::cout << "Frame " << frames++ << " done\n";

		SDL_UpperBlitScaled(framebuf, nullptr, wndSurf, nullptr);
		SDL_UpdateWindowSurface(wnd);
		SDL_Delay(10);
	}

	system("pause");
}