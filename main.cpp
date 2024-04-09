#include <SDL\SDL.h>
#include <SDL\SDL_image.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <algorithm>

#include "C_Input.h"
#include "CoordinateTransformer.h"
#include "Matrix3.h"

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

struct Texture
{
	SDL_Surface* surf;
	std::string name;
	Texture(std::string name)
	{
		this->name = name;
		std::string path = "D:/Games/GZDoom/Doom2_unpacked/graphics/" + name + ".png"; //TODO: doom uses TEXTURES lumps for some dark magic with them, this code does not work for unprepared textures.
		surf = IMG_Load(path.c_str());

		SDL_Surface* old = surf;
		surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_ABGR32, 0);
		SDL_FreeSurface(old);
	}
	uint32_t getPixel(int x, int y)
	{
		x = abs(x);
		y = abs(y);
		x %= surf->w;
		y %= surf->h;
		uint32_t* px = (uint32_t*)surf->pixels;
		return px[y * surf->w + x];
	}

	~Texture()
	{
		//SDL_FreeSurface(surf);
	}
};

int getTextureIndexByName(std::string name, std::vector<Texture>& textures, std::unordered_map<std::string, int>& textureNameToIndexMap)
{
	if (textureNameToIndexMap.find(name) != textureNameToIndexMap.end()) return textureNameToIndexMap[name];

	textures.emplace_back(name);
	textureNameToIndexMap[name] = textures.size() - 1;
	return textures.size() - 1;
}

struct DrawingPrimitive
{
	Vec3 vertices[4];
	Vec2 uv[4]; //notice, that is is a misnomer. Doom doesn't use UV system, texture size change will make it bigger in the world space as well.
	int textureIndex;
	int xTextureOffset = 0, yTextureOffset = 0;
};

void setPixel(SDL_Surface* s, int x, int y, uint32_t color)
{
	if (x >= 0 && y >= 0 && x < s->w && y < s->h)
	{
		uint32_t* px = (uint32_t*)s->pixels;
		px[y * s->w + x] = color;
	}
}

template <typename T>
T naive_lerp(const T& start, const T& end, const T& amount)
{
	return start + (end - start) * amount;
}
std::vector<Texture> textures;

struct TexVertex
{
	Vec3 worldCoords;
	Vec2 textureCoords;

	bool operator<(const TexVertex& b) const
	{
		return worldCoords.y < b.worldCoords.y;
	}
};

struct Triangle
{
	TexVertex tv[3];
	int textureIndex;

	void drawOn(SDL_Surface* s, const CoordinateTransformer& ctr, std::vector<double>& zBuffer) const
	{		
		TexVertex screenSpace[3];
		for (int i = 0; i < 3; ++i) screenSpace[i] = { ctr.toScreenCoords(tv[i].worldCoords), tv[i].textureCoords };
		std::sort(std::begin(screenSpace), std::end(screenSpace)); //sort triangles by screen y (ascending, i.e. going from top to bottom). We also must preserve the texture coords data
		
		/*
		Main idea: we are interpolating between lines of the triangle. All the next mathy stuff can be imagined as walking from a to b, 
		"mixing" (linearly interpolating) between two values. Note, that texture mapping is plain ol' linear stuff, not perspective correct.
		*/
		double x1 = screenSpace[0].worldCoords.x, x2 = screenSpace[1].worldCoords.x, x3 = screenSpace[2].worldCoords.x, y1 = screenSpace[0].worldCoords.y, y2 = screenSpace[1].worldCoords.y, y3 = screenSpace[2].worldCoords.y;
		double splitAlpha = (y2 - y1) / (y3 - y1); //how far along original triangle's y is the split line? 0 = extreme top, 1 = extreme bottom
		double split_xend = naive_lerp(x1, x3, splitAlpha); //last x of splitting line
		double split_uend = naive_lerp(screenSpace[0].textureCoords.x, screenSpace[2].textureCoords.x, splitAlpha); //last u of splitting line

		for (double y = y1; y < y2; ++y) //draw flat bottom part
		{
			double yp = (y - y1) / (y2 - y1); //this is the "progress" along the flat bottom part, not whole triangle!
			double xLeft = naive_lerp(x1, x2, yp);
			double xRight = naive_lerp(x1, x3, splitAlpha * yp); //?

			double uLeft = naive_lerp(screenSpace[0].textureCoords.x, screenSpace[1].textureCoords.x, yp);
			double uRight = naive_lerp(screenSpace[0].textureCoords.x, screenSpace[2].textureCoords.x, splitAlpha * yp);
			double v = naive_lerp(screenSpace[0].textureCoords.y, screenSpace[1].textureCoords.y, yp);

			if (xLeft > xRight)
			{
				std::swap(xLeft, xRight); //enforce non-decreasing x for next loop. TODO: make branchless
				std::swap(uLeft, uRight); //this is not a mistake. If we swap x, then u also has to go.
			}
			
			for (double x = xLeft; x < xRight; ++x)
			{
				double xp = (x - xLeft) / (xRight - xLeft);
				auto c = textures[textureIndex].getPixel(naive_lerp(uLeft, uRight, xp), v);
				setPixel(s, x, y, c);
			}
		}

		for (double y = y2; y < y3; ++y) //draw flat top part
		{
			double yp = (y - y2) / (y3 - y2); //this is the "progress" along the flat top part, not whole triangle!
			double xLeft = naive_lerp(x2, x3, yp);
			double xRight = naive_lerp(split_xend, x3, yp);

			double uLeft = naive_lerp(screenSpace[1].textureCoords.x, screenSpace[2].textureCoords.x, yp);
			double uRight = naive_lerp(split_uend, screenSpace[2].textureCoords.x, yp);
			double v = naive_lerp(screenSpace[1].textureCoords.y, screenSpace[2].textureCoords.y, yp);

			if (xLeft > xRight)
			{
				std::swap(xLeft, xRight); //enforce non-decreasing x for next loop. TODO: make branchless
				std::swap(uLeft, uRight); //this is not a mistake. If we swap x, then u also has to go.
			}

			for (double x = xLeft; x < xRight; ++x)
			{
				double xp = (x - xLeft) / (xRight - xLeft);
				auto c = textures[textureIndex].getPixel(naive_lerp(uLeft, uRight, xp), v);
				setPixel(s, x, y, c);
			}
		}
	}
};


void main()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	loadPwad("D:/Games/GZDoom/STUPID.wad");

	std::vector<std::vector<Sidedef*>> sectorSidedefs(sectors.size());
	std::vector<std::vector<Linedef*>> sidedefLinedefs(sidedefs.size());
	std::vector<std::vector<Vec3>> sectorVertices(sectors.size());
	std::unordered_map<std::string, int> textureNameToIndexMap;

	Vec3 camPos = { 0.1,32.1,370 };
	//Vec3 camPos = { 0,0,0 };
	Vec3 camAng = { 0,0,0 };

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
	//std::vector<Triangle> triangles;
	std::vector<std::vector<Triangle>> sectorTriangles(sectors.size());

	for (int i = 0; i < sectors.size(); ++i)
	{
		for (int j = 0; j < sectorSidedefs[i].size(); ++j)
		{
			for (int k = 0; k < sidedefLinedefs[j].size(); ++k)
			{
				Linedef* ldf = sidedefLinedefs[j][k];
				Vertex sv = vertices[ldf->startVertex];
				Vertex ev = vertices[ldf->endVertex];
				double fh = sectors[i].floorHeight;
				double ch = sectors[i].ceilingHeight;
				Vec3 verts[4];

				char nameBuf[9] = { 0 };
				/*/p.vertices[0] = {double(sv.x), fh, double(sv.y)}; //z is supposed to be depth, so swap with y? i.e. doom takes depth as y, height as z, we take y as height
				p.vertices[1] = { double(ev.x), fh, double(sv.y) };
				p.vertices[2] = { double(ev.x), fh, double(ev.y) };
				p.vertices[3] = { double(sv.x), fh, double(ev.y) };
				memcpy(nameBuf, sectors[i].floorTexture, 8);
				p.textureIndex = getTextureIndexByName(nameBuf, textures, textureNameToIndexMap);
				sectorPrimitives[i].push_back(p);

				p.vertices[0] = { double(sv.x), ch, double(sv.y) };
				p.vertices[1] = { double(ev.x), ch, double(sv.y) };
				p.vertices[2] = { double(ev.x), ch, double(ev.y) };
				p.vertices[3] = { double(sv.x), ch, double(ev.y) };
				memset(nameBuf, 0, 9);
				memcpy(nameBuf, sectors[i].ceilingTexture, 8);
				p.textureIndex = getTextureIndexByName(nameBuf, textures, textureNameToIndexMap);
				sectorPrimitives[i].push_back(p);*/

				verts[0] = { double(sv.x), ch, double(sv.y) };
				verts[1] = { double(ev.x), ch, double(ev.y) };
				verts[2] = { double(sv.x), fh, double(sv.y) };
				verts[3] = { double(ev.x), fh, double(ev.y) };
				
				memset(nameBuf, 0, 9);
				memcpy(nameBuf, sectorSidedefs[i][j]->middleTexture, 8);
				int textureIndex = getTextureIndexByName(nameBuf, textures, textureNameToIndexMap);
				for (int n = 0; n < 2; ++n)
				{
					Triangle t;
					t.textureIndex = textureIndex;					
					for (int m = 0; m < 3; ++m)
					{
						t.tv[m].worldCoords = verts[m + n];
						Vec3 uv3 = verts[m + n] - verts[0];
						t.tv[m].textureCoords = { uv3.x + sectorSidedefs[i][j]->xTextureOffset, uv3.y + sectorSidedefs[i][j]->yTextureOffset };
					}
					sectorTriangles[i].emplace_back(t);
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

	std::vector<double> zBuffer(framebufW * framebufH);

	while (true)
	{
		SDL_FillRect(framebuf, nullptr, 0);
		SDL_FillRect(wndSurf, nullptr, 0);
		for (auto& it : zBuffer) it = std::numeric_limits<double>::infinity();

		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			input.handleEvent(ev);
			if (input.isMouseButtonHeld(SDL_BUTTON_LEFT) && ev.type == SDL_MOUSEMOTION)
			{
				camAng += { 0, ev.motion.xrel * 1e-3, ev.motion.yrel * -1e-3};
			}
		}

		camPos += { 1.0 * input.isButtonHeld(SDL_SCANCODE_D), 1.0 * input.isButtonHeld(SDL_SCANCODE_X), 1.0 * input.isButtonHeld(SDL_SCANCODE_W)};
		camPos -= { 1.0 * input.isButtonHeld(SDL_SCANCODE_A), 1.0 * input.isButtonHeld(SDL_SCANCODE_Z), 1.0 * input.isButtonHeld(SDL_SCANCODE_S)};

		camAng += { 1e-2 * input.isButtonHeld(SDL_SCANCODE_R), 1e-2 * input.isButtonHeld(SDL_SCANCODE_T), 1e-2 * input.isButtonHeld(SDL_SCANCODE_Y)};
		camAng -= { 1e-2 * input.isButtonHeld(SDL_SCANCODE_F), 1e-2 * input.isButtonHeld(SDL_SCANCODE_G), 1e-2 * input.isButtonHeld(SDL_SCANCODE_H)};
		if (input.isButtonHeld(SDL_SCANCODE_C)) camPos = { 0.1,32.1,370 };
		if (input.isButtonHeld(SDL_SCANCODE_V)) camAng = { 0,0,0 };

		Matrix3 transformMatrix = getRotationMatrix(camAng);
		ctr.prepare(camPos, transformMatrix);

		//std::vector<Triangle> cameraOffsets()

		for (int i = 0; i < sectors.size(); ++i)
		{
			for (const auto& t : sectorTriangles[i]) t.drawOn(framebuf, ctr, zBuffer);
			/*for (const auto& p : sectorPrimitives[i])
			{
				Vec3 verts[4];
				for (int i = 0; i < 4; ++i) 
				{
					verts[i] = ctr.toScreenCoords(p.vertices[i]);
				}

				for (int i = 0; i < 2; ++i)
				{
					Triangle t = { verts[i], verts[i + 1], verts[i + 2] };
					t.drawOn(framebuf);
				}
			}
			*/


		}

		/*/Vec3 v[3] = {{10,15,20}, {7, 20, 10}, {12, 11, 24}};

		for (auto& it : v)
		{
			it -= camPos;
			it = transformMatrix * it;
			it /= it.z;
			it *= 200;
		}

		Triangle t = { v[0], v[1],v[2] };
		t.drawOn(framebuf);*/

		SDL_UpperBlitScaled(framebuf, nullptr, wndSurf, nullptr);
		SDL_UpdateWindowSurface(wnd);
		SDL_Delay(10);
	}

	system("pause");
}