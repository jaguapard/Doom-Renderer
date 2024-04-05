#include <SDL\SDL.h>
#include <SDL\SDL_image.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

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

struct Vec3
{
	double x, y, z;
};

struct Texture
{
	SDL_Surface* surf;
	Texture(std::string name)
	{
		std::string path = "D:/Games/GZDoom/Doom2_unpacked/" + name + ".png";
		surf = IMG_Load(path.c_str());
		//SDL_PixelFormat fmt = SDL_PixelFormatEnum::SDL_PIXELFORMAT_RGBA8888;
	}
	uint32_t getPixel(int x, int y)
	{
		x %= surf->w;
		y %= surf->h;
		uint32_t* px = (uint32_t*)surf->pixels;
		return px[y * surf->w + x];
	}

	~Texture()
	{
		SDL_FreeSurface(surf);
	}
};
//void setPixel()

int getTextureIndexByName(std::string name, std::vector<Texture>& textures, std::unordered_map<std::string, int>& textureNameToIndexMap)
{
	if (textureNameToIndexMap.find(name) != textureNameToIndexMap.end()) return textureNameToIndexMap[name];

	textures.emplace_back(name);
	textureNameToIndexMap[name] = textures.size() - 1;
	return textures.size() - 1;
}

struct DrawingPrimitive
{
	Vec3 vertices[6];
	int textureIndex;
	int xTextureOffset = 0, yTextureOffset = 0;
};

void main()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	loadPwad("D:/Games/GZDoom/STUPID_copy.wad");

	std::vector<std::vector<Sidedef*>> sectorSidedefs(sectors.size());
	std::vector<std::vector<Linedef*>> sidedefLinedefs(sidedefs.size());
	std::vector<std::vector<Vec3>> sectorVertices(sectors.size());
	std::vector<Texture> textures;
	std::unordered_map<std::string, int> textureNameToIndexMap;

	for (int i = 0; i < sectors.size(); ++i)
	{
		for (auto& it : sidedefs)
		{
			if (it.facingSector == i)
			{
				sectorSidedefs[i].push_back(&it);
			}
		}
	}	

	for (int i = 0; i < sidedefs.size(); ++i)
	{
		for (auto& it : linedefs)
		{
			if (it.frontSidedef == i || it.backSidedef == i)
			{
				sidedefLinedefs[i].push_back(&it);
			}
		}
	}

	std::vector<std::vector<DrawingPrimitive>> sectorPrimitives(sectors.size());
	for (int i = 0; i < sectors.size(); ++i)
	{
		for (int j = 0; j < sectorSidedefs[i].size(); ++j)
		{
			for (int k = 0; k < sidedefLinedefs[j].size(); ++k)
			{
				Linedef* ldf = sidedefLinedefs[j][k];
				DrawingPrimitive p;
				Vertex sv = vertices[ldf->startVertex];
				Vertex ev = vertices[ldf->endVertex];
				double fh = sectors[i].floorHeight;
				double ch = sectors[i].ceilingHeight;

				p.vertices[0] = { double(sv.x), fh, double(sv.y) }; //z is supposed to be depth, so swap with y? i.e. doom takes depth as y, height as z, we take y as height
				p.vertices[1] = { double(ev.x), fh, double(sv.y) };
				p.vertices[2] = { double(sv.x), fh, double(ev.y) };
				p.vertices[3] = { double(ev.x), fh, double(ev.y) };
				p.textureIndex = getTextureIndexByName(sectors[i].floorTexture, textures, textureNameToIndexMap);
				sectorPrimitives[i].push_back(p);

				p.vertices[0] = { double(sv.x), ch, double(sv.y) };
				p.vertices[1] = { double(ev.x), ch, double(sv.y) };
				p.vertices[2] = { double(sv.x), ch, double(ev.y) };
				p.vertices[3] = { double(ev.x), ch, double(ev.y) };
				p.textureIndex = getTextureIndexByName(sectors[i].ceilingTexture, textures, textureNameToIndexMap);
				sectorPrimitives[i].push_back(p);

				p.vertices[0] = { double(sv.x), ch, double(sv.y) };
				p.vertices[1] = { double(ev.x), ch, double(sv.y) };
				p.vertices[2] = { double(ev.x), fh, double(sv.y) };
				p.vertices[3] = { double(sv.x), fh, double(sv.y) };
				p.textureIndex = getTextureIndexByName(sectorSidedefs[i][j]->middleTexture, textures, textureNameToIndexMap);
				p.xTextureOffset = sectorSidedefs[i][j]->xTextureOffset;
				p.yTextureOffset = sectorSidedefs[i][j]->yTextureOffset;
				sectorPrimitives[i].push_back(p);
			}
		}
	}


	
	SDL_Window* wnd = SDL_CreateWindow("Doom Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, 0);
	SDL_Surface* wndSurf = SDL_GetWindowSurface(wnd);
	SDL_Surface* framebuf = SDL_CreateRGBSurface(0, 320, 200, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0xFF);
	while (true)
	{
		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{

		}

		for (int i = 0; i < sectors.size(); ++i)
		{
			for (const auto& v : sectorVertices[i])
			{
				int x = v.x / 4 + 32;
				int y = v.y / 4 + 32;
				uint32_t* px = (uint32_t*)framebuf->pixels;
				px[framebuf->w * y + x] = 0xFFFFFFFF;
			}
		}

		SDL_UpperBlitScaled(framebuf, nullptr, wndSurf, nullptr);
		SDL_UpdateWindowSurface(wnd);
		SDL_Delay(10);
	}
	for (int i = 0; i < sectors.size(); ++i)
	{
		const Sector& me = sectors[i];
		std::vector<Sidedef*> mySidedefs;
		std::vector<Linedef*> myLinedefs;
		std::vector<Vertex> myVertices;

		//gather all stuff required to make a draw packet 
	}

	system("pause");
}