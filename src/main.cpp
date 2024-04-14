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
#include <deque>
#include <map>
#include <set>

#include <adm/strings.h>

#include "C_Input.h"
#include "CoordinateTransformer.h"
#include "Matrix3.h"
#include "Texture.h"
#include "ZBuffer.h"
#include "Triangle.h"
#include "Statsman.h"

#include "DoomStructs.h"
#include "DoomWorldLoader.h"

#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2_image.lib")
#undef main



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

void loadWad(std::string path)
{
	std::ifstream f(path, std::ios::binary);
	std::vector<char> wadBytes;
	while (f)
	{
		char b;
		f.read(&b, 1);
		wadBytes.push_back(b);
	}

	std::string header = wadStrToStd(&wadBytes.front(), 4);
	if (header != "IWAD" && header != "PWAD")
		throw std::exception("Invalid WAD file: header mismatch");

	int32_t numFiles = readRaw<int32_t>(&wadBytes[4]);
	int32_t FAToffset = readRaw<int32_t>(&wadBytes[8]);

	int32_t filePtr = FAToffset;
	for (int i = 0; i < numFiles; ++i)
	{
		int32_t lumpDataOffset = readRaw<int32_t>(&wadBytes[filePtr]);
		int32_t lumpSizeBytes = readRaw<int32_t>(&wadBytes[filePtr + 4]);
		std::string name = wadStrToStd(&wadBytes[filePtr + 8]);
		std::cout << "Found WAD file: " << name << ", data offset: " << lumpDataOffset << ", size: " << lumpSizeBytes << "\n";

		if (name == "VERTEXES") vertices = getVectorFromWad<Vertex>(lumpDataOffset, lumpSizeBytes, wadBytes);
		if (name == "LINEDEFS") linedefs = getVectorFromWad<Linedef>(lumpDataOffset, lumpSizeBytes, wadBytes);
		if (name == "SIDEDEFS") sidedefs = getVectorFromWad<Sidedef>(lumpDataOffset, lumpSizeBytes, wadBytes);
		if (name == "SECTORS") sectors = getVectorFromWad<Sector>(lumpDataOffset, lumpSizeBytes, wadBytes);
		filePtr += 16;
	}
}

Statsman statsman;

void main()
{
	double gamma = 1.3;
	TextureManager textureManager;

	SDL_Init(SDL_INIT_EVERYTHING);
	//loadWad("data/test_maps/STUPID.wad");
	//loadWad("D:/Games/GZDoom/MappingTests/D2_MAP01.wad");
	loadWad("D:/Games/GZDoom/MappingTests/D2_MAP15.wad");
	//loadWad("data/test_maps/HEXAGON.wad");
	//loadWad("data/test_maps/RECT.wad");


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

	std::vector<std::vector<Triangle>> sectorTriangles = DoomWorldLoader::loadTriangles(linedefs, vertices, sidedefs, sectors, textureManager);

	int framebufW = 1280;
	int framebufH = 720;
	int screenW = 1280;
	int screenH = 720;
	SDL_Window* wnd = SDL_CreateWindow("Doom Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenW, screenH, 0);
	SDL_Surface* wndSurf = SDL_GetWindowSurface(wnd);

	PixelBuffer<Color> framebuf(framebufW, framebufH);

	C_Input input;
	CoordinateTransformer ctr(framebufW, framebufH);

	ZBuffer zBuffer(framebufW, framebufH);
	uint64_t frames = 0;
	while (true)
	{
		framebuf.clear();
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

		camPos -= { 15.0 * input.isButtonHeld(SDL_SCANCODE_D), 15.0 * input.isButtonHeld(SDL_SCANCODE_X), 15.0 * input.isButtonHeld(SDL_SCANCODE_W)};
		camPos += { 15.0 * input.isButtonHeld(SDL_SCANCODE_A), 15.0 * input.isButtonHeld(SDL_SCANCODE_Z), 15.0 * input.isButtonHeld(SDL_SCANCODE_S)};

		camAng += { 3e-2 * input.isButtonHeld(SDL_SCANCODE_R), 3e-2 * input.isButtonHeld(SDL_SCANCODE_T), 3e-2 * input.isButtonHeld(SDL_SCANCODE_Y)};
		camAng -= { 3e-2 * input.isButtonHeld(SDL_SCANCODE_F), 3e-2 * input.isButtonHeld(SDL_SCANCODE_G), 3e-2 * input.isButtonHeld(SDL_SCANCODE_H)};
		if (input.isButtonHeld(SDL_SCANCODE_C)) camPos = { 0.1,32.1,370 };
		if (input.isButtonHeld(SDL_SCANCODE_V)) camAng = { 0,0,0 };
		gamma += 0.1 * (input.isButtonHeld(SDL_SCANCODE_EQUALS) - input.isButtonHeld(SDL_SCANCODE_MINUS));

		Matrix3 transformMatrix = getRotationMatrix(camAng);
		ctr.prepare(camPos, transformMatrix);

		for (int i = 0; i < sectors.size(); ++i)
		{
			for (int j = 0; j < sectorTriangles[i].size(); ++j) sectorTriangles[i][j].drawOn(framebuf, ctr, zBuffer, textureManager, pow(sectors[i].lightLevel / 256.0, gamma));
		}
		std::cout << "Frame " << frames++ << " done\n";

		if (true)
		{
			double* zBuffPixels = zBuffer.getRawPixels();
			Color* framebufPixels = framebuf.getRawPixels();
			int pxCount = framebufW * framebufH;

			double fogMaxIntensityDist = 600;
			for (int i = 0; i < pxCount; ++i)
			{
				double depth = -zBuffPixels[i];
				Color c = framebufPixels[i];
				double lerpT = std::clamp((1.0/depth) / fogMaxIntensityDist, 0.0, 1.0); //z buffer stores 1/z, so need to get the real z
				c.r = lerp(c.r, 1.0, lerpT);
				c.g = lerp(c.g, 1.0, lerpT);
				c.b = lerp(c.b, 1.0, lerpT);
				framebufPixels[i] = c;
			}
		}

		Uint32* px = (Uint32*)(wndSurf->pixels);
		auto* wf = wndSurf->format;
		uint32_t shifts[4] = { wf->Rshift, wf->Gshift, wf->Bshift, 32 }; //window does not support transparency, so kill alpha by shifting it out of range of uint32
		for (int y = 0; y < screenH; ++y)
		{
			for (int x = 0; x < screenW; ++x)
			{
				double fx = double(x) / screenW * framebufW;
				double fy = double(y) / screenH * framebufH;
				px[y * screenW + x] = framebuf.getPixel(fx, fy).toSDL_Uint32(shifts);
			}
		}
		SDL_UpdateWindowSurface(wnd);
	}

	system("pause");
}