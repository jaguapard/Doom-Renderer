#include <SDL\SDL.h>
#include <SDL\SDL_image.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

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
	int16_t xOffset;
	int16_t yOffset;
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

	int32_t numFiles = *reinterpret_cast<int32_t*>(&wadBytes[4]);
	int32_t FAToffset = *reinterpret_cast<int32_t*>(&wadBytes[8]);

	int32_t filePtr = FAToffset;
	for (int i = 0; i < numFiles; ++i)
	{
		char name[9] = { 0 };
		int32_t lumpDataOffset;
		int32_t lumpSizeBytes;
		readRaw(lumpDataOffset, &wadBytes[filePtr]);
		readRaw(lumpSizeBytes, &wadBytes[filePtr + 4]);
		memcpy(name, &wadBytes[filePtr + 8], 8);

		std::cout << "Found WAD file: " << name << ", data offset: " << lumpDataOffset << ", size: " << lumpSizeBytes << "\n";

		filePtr += 16;

		if (!strcmp(name, "VERTEXES"))
		{
			for (int i = 0; i < lumpSizeBytes; i += sizeof(Vertex))
			{
				Vertex v = readRaw<Vertex>(&wadBytes[lumpDataOffset + i]);
				vertices.push_back(v);
				std::cout << "Got vertex " << v.x << " " << v.y << "\n";
			}
		}
	}
}

void main()
{
	loadPwad("D:/Games/GZDoom/STUPID_copy.wad");

	system("pause");
}