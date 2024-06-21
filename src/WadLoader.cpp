#include <fstream>
#include <stdexcept>
#include <iostream>

#include "WadLoader.h"
#include "helpers.h"
#include <cstring>

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

std::map<std::string, DoomMap> WadLoader::loadWad(std::string path)
{
	std::ifstream f(path, std::ios::binary);
	if (!f.is_open()) throw std::runtime_error(std::string("Failed to open file ") + path + ": " + strerror(errno));

	f.seekg(0, f.end);
	int wadSize = f.tellg();
	f.seekg(0, f.beg);

	std::vector<char> wadBytes(wadSize);
	f.read(&wadBytes.front(), wadSize);
	if (f.gcount() != wadSize) throw std::runtime_error(std::string("Failed to load file ") + path + ": " + strerror(errno));

	std::string header = wadStrToStd(&wadBytes.front(), 4);
	if (header != "IWAD" && header != "PWAD")
		throw std::runtime_error(std::string("Invalid WAD file: ") + path + ": header mismatch. Got " + header);

	int32_t numFiles = readRaw<int32_t>(&wadBytes[4]);
	int32_t FAToffset = readRaw<int32_t>(&wadBytes[8]);

	int32_t filePtr = FAToffset;
	std::string mapName = "";
	DoomMap map;
	std::map<std::string, DoomMap> maps;
	for (int i = 0; i < numFiles; ++i)
	{
		int32_t lumpDataOffset = readRaw<int32_t>(&wadBytes[filePtr]);
		int32_t lumpSizeBytes = readRaw<int32_t>(&wadBytes[filePtr + 4]);
		std::string name = wadStrToStd(&wadBytes[filePtr + 8]);
		if (false) std::cout << "Found file: " << name << ", data offset: " << lumpDataOffset << ", size: " << lumpSizeBytes << "\n";

		bool isEpisodicMap = name.length() >= 4 && name[0] == 'E' && name[2] == 'M'; //Doom 1 and Heretic
		bool isNonEpisodicMap = name.length() >= 5 && std::string(name.begin(), name.begin() + 3) == "MAP"; //Doom 2, Hexen, Strife
		if (isEpisodicMap || isNonEpisodicMap)
		{
			if (mapName != name && mapName != "") maps[mapName] = map; //if map was in progress of being read, then save it
			mapName = name;
		}

		if (name == "VERTEXES") map.vertices = getVectorFromWad<Vertex>(lumpDataOffset, lumpSizeBytes, wadBytes);
		if (name == "LINEDEFS") map.linedefs = getVectorFromWad<Linedef>(lumpDataOffset, lumpSizeBytes, wadBytes);
		if (name == "SIDEDEFS") map.sidedefs = getVectorFromWad<Sidedef>(lumpDataOffset, lumpSizeBytes, wadBytes);
		if (name == "SECTORS") map.sectors = getVectorFromWad<Sector>(lumpDataOffset, lumpSizeBytes, wadBytes);
		filePtr += 16;
	}

	return maps;
}