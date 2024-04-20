#include <SDL\SDL.h>
#include <SDL\SDL_image.h>
#include <SDL\SDL_ttf.h>

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
#include "PerformanceMonitor.h"

#include "DoomStructs.h"
#include "DoomWorldLoader.h"
#include "WadLoader.h"
#include "TextureMapper.h"

#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2_image.lib")
#pragma comment(lib,"SDL2_ttf.lib")
#undef main

Statsman statsman;

std::vector<Triangle> generateSphereMesh(int horizontalDivisions, int verticalDivisions, real radius)
{
	assert(horizontalDivisions * (verticalDivisions-1) % 3 == 0);
	std::vector<Triangle> ret;
	std::vector<Vec3> world;
	for (int m = 0; m < horizontalDivisions; m++)
	{
		for (int n = 0; n < verticalDivisions - 1; n++)
		{
			real x = sin(M_PI * m / horizontalDivisions) * cos(2 * M_PI * n / verticalDivisions);
			real y = sin(M_PI * m / horizontalDivisions) * sin(2 * M_PI * n / verticalDivisions);
			real z = cos(M_PI * m / horizontalDivisions);
			world.push_back(Vec3(x, y, z));			
			
			x = sin(M_PI * (m+1) / horizontalDivisions) * cos(2 * M_PI * n / verticalDivisions);
			y = sin(M_PI * (m+1) / horizontalDivisions) * sin(2 * M_PI * n / verticalDivisions);
			z = cos(M_PI * (m+1) / horizontalDivisions);
			world.push_back(Vec3(x, y, z));			
			
			x = sin(M_PI * (m+1) / horizontalDivisions) * cos(2 * M_PI * (n+1) / verticalDivisions);
			y = sin(M_PI * (m+1) / horizontalDivisions) * sin(2 * M_PI * (n+1) / verticalDivisions);
			z = cos(M_PI * (m+1) / horizontalDivisions);
			world.push_back(Vec3(x, y, z));
		}
	}

	assert(world.size() % 3 == 0);
	for (int i = 0; i < world.size(); i += 3)
	{		
		Triangle t;
		for (int j = 0; j < 3; ++j)
		{
			Vec3 textureCoords = (world[i + j] - Vec3(0, 0, 1))*128;
			t.tv[j] = { world[i + j]*radius, textureCoords };
		}

		ret.push_back(t);
	}
	return ret;
}

enum SkyRenderingMode
{
	NONE,
	ROTATING,
	CUBE,
	SPHERE,
	COUNT
};
void program()
{
	if (SDL_Init(SDL_INIT_EVERYTHING)) throw std::runtime_error(std::string("Failed to initialize SDL: ") + SDL_GetError());
	if (TTF_Init()) throw std::runtime_error(std::string("Failed to initialize SDL TTF: ") + TTF_GetError());
	//if (IMG_Init()) throw std::runtime_error(std::string("Failed to initialize SDL image: ") + IMG_GetError());

	auto maps = WadLoader::loadWad("D:/Games/GZDoom/DOOM2.wad"); //can't redistribute commercial wads!
	//loadWad("data/test_maps/STUPID.wad");
	//loadWad("data/test_maps/HEXAGON.wad");
	//loadWad("data/test_maps/RECT.wad");

	Vec3 camPosAndAngArchieve[] = {
		{ 0,0,0 }, {0,0,0},
		{ 0.1,32.1,370 }, {0,0,0}, //default view
		{-96, 70, 784}, {0,0,0}, //doom 2 map 01 player start
	};

	int activeCamPosAndAngle = 2;
	Vec3 camPos = camPosAndAngArchieve[activeCamPosAndAngle * 2];
	Vec3 camAng = camPosAndAngArchieve[activeCamPosAndAngle * 2 + 1];

	int framebufW = 1280;
	int framebufH = 720;
	int screenW = 1280;
	int screenH = 720;
	SDL_Window* wnd = SDL_CreateWindow("Doom Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenW, screenH, 0);
	SDL_Surface* wndSurf = SDL_GetWindowSurface(wnd);

	PixelBuffer<Color> framebuf(framebufW, framebufH);
	auto skyBuff = framebuf;
	PixelBuffer<real> lightBuf(framebufW, framebufH);
	ZBuffer zBuffer(framebufW, framebufH);

	C_Input input;
	CoordinateTransformer ctr(framebufW, framebufH);
	
	uint64_t frames = 0;
	real flySpeed = 15;
	real gamma = 1.3;
	bool fogEnabled = false;
	real fogMaxIntensityDist = 600;

	PerformanceMonitor performanceMonitor;
	bool performanceMonitorDisplayEnabled = true;

	std::string warpTo;

	real camAngAdjustmentSpeed_Mouse = 1e-3;
	real camAngAdjustmentSpeed_Keyboard = 3e-2;

	std::vector<std::vector<Triangle>> sectorTriangles;
	TextureManager textureManager;
	DoomMap* currentMap = nullptr;
	std::vector<Triangle> skyCube, skySphere;

	{
		const Texture& sky = textureManager.getTextureByName("RSKY1");
		int skyTextureIndex = textureManager.getTextureIndexByName("RSKY1");
		double scaleX = 256 / double(framebufW); //TODO: remove hardcoded const
		double scaleY = 128 / double(framebufH);
		for (int y = 0; y < framebufH; ++y) //boring sky
		{
			for (int x = 0; x < framebufW; ++x)
			{
				double fx = scaleX * x;
				double fy = scaleY * y;
				skyBuff.setPixel(x,y, sky.getPixel(fx, fy));
			}
		}

		double skyCubeSide = 131072;
		/*std::vector<Vec3> cubeVerts =  //connect: 0->1->2 and 2->3->0
		{
			{-1, -1, -1},  { 1, -1, -1},  { 1,  1, -1},  {-1,  1, -1}, //neg z
			{-1, -1,  1},  { 1, -1,  1},  { 1,  1,  1},  {-1,  1,  1}, //pos z

			{-1, -1, -1},  { 1, -1, -1},  { 1, -1,  1},  {-1, -1,  1}, //neg y
			{-1,  1, -1},  { 1,  1, -1},  { 1,  1,  1},  {-1,  1,  1}, //pos y

			{-1,  1, -1},  {-1, -1, -1},  {-1, -1,  1},  {-1,  1,  1}, //neg x
			{1,   1, -1},  { 1, -1, -1},  { 1, -1,  1},  {1,   1,  1}, //pos x
		};*/
		std::vector<TexVertex> cubeVerts =  //connect: 0->1->2 and 2->3->0
		{
			{ {-1, -1, -1}, {0,0}}, { { 1, -1, -1 }, {1,0}},  {{ 1,  1, -1 }, {1,1}}, {{ -1,  1, -1 }, {0, 1}}, //neg z
			{ {-1, -1,  1}, {0,0}}, { { 1, -1,  1 }, {1,0}}, { {1,  1,  1 }, {1,1}}, { { -1,  1,  1 }, {0,1}}, //pos z

			{ {-1, -1, -1}, {0,0}}, { { 1, -1, -1 }, {1,0}}, {{ 1, -1,  1 }, {1,1}},  {{-1, -1,  1}, {0,1}},  //neg y
			{{-1,  1, -1}, {0,0}}, {{ 1,  1, -1 }, {1,0}}, {{ 1,  1,  1 }, {1,1}}, {{ -1,  1,  1 },{0,1}}, //pos y

			{ { -1,  1, -1 }, {1,1} }, {{ -1, -1, -1 }, {1,0}}, {{ -1, -1,  1 }, {0,0}}, {{ -1,  1,  1 }, {0,1}}, //neg x
			{{1,   1, -1}, {1,1}}, {{ 1, -1, -1 }, {1,0}}, {{ 1, -1,  1 }, {0,0}}, {{ 1,   1,  1 }, {0,1}}, //pos x
		};

		Vec3 cubeSizeMult = { 1, 0.5, 1 };
		std::vector<TexVertex> tv(cubeVerts.size());

		for (int i = 0; i < cubeVerts.size(); ++i)
		{
			
			//TODO: remove hardcoded const
			tv[i] = { cubeVerts[i].spaceCoords * 128, cubeVerts[i].textureCoords * 128 };
		}

		for (int i = 0; i < tv.size(); i += 4)
		{
			Triangle t;
			t.textureIndex = skyTextureIndex;

			//connect: 0->1->2 and 2->3->0
			t.tv[0] = tv[i + 0];
			t.tv[1] = tv[i + 1];
			t.tv[2] = tv[i + 2];
			//t = TextureMapper::mapTriangleRelativeToFirstVertex(t);
			skyCube.push_back(t);

			t.tv[0] = tv[i + 2];
			t.tv[1] = tv[i + 3];
			t.tv[2] = tv[i + 0];
			//t = TextureMapper::mapTriangleRelativeToFirstVertex(t);
			skyCube.push_back(t);
		}

		for (auto& tri : skyCube)
			for (auto& tv : tri.tv)
				tv.spaceCoords *= skyCubeSide / 128;

		skySphere = generateSphereMesh(60, 30, 65536);
		for (auto& it : skySphere) it.textureIndex = skyTextureIndex;
	}

	SkyRenderingMode skyRenderingMode = ROTATING;
	while (true)
	{
		performanceMonitor.registerFrameBegin();
		framebuf.clear();
		//framebuf = skyBuff;
		SDL_FillRect(wndSurf, nullptr, Color(0,0,0));
		zBuffer.clear();
		lightBuf.clear(1);



		input.beginNewFrame();
		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			input.handleEvent(ev);
			if (input.isMouseButtonHeld(SDL_BUTTON_LEFT) && ev.type == SDL_MOUSEMOTION)
			{
				camAng += { 0, ev.motion.xrel * camAngAdjustmentSpeed_Mouse, ev.motion.yrel * -camAngAdjustmentSpeed_Mouse};
			}
			if (ev.type == SDL_MOUSEWHEEL)
			{
				flySpeed *= pow(1.05, ev.wheel.y);
			}
		}

		for (char c = '0'; c <= '9'; ++c)
		{
			if (input.wasCharPressedOnThisFrame(c))
			{
				warpTo += c;
				break;
			}
		}

		if (warpTo.length() == 2)
		{
			std::string mapToLoad = "MAP" + warpTo;
			std::cout << "Loading map " << mapToLoad << "...\n";

			currentMap = &maps[mapToLoad];
			sectorTriangles = currentMap->getTriangles(textureManager);

			warpTo.clear();
			performanceMonitor.reset();
		}

		//xoring with 1 == toggle true->false or false->true
		if (input.wasCharPressedOnThisFrame('G')) fogEnabled ^= 1;
		if (input.wasCharPressedOnThisFrame('P')) performanceMonitorDisplayEnabled ^= 1;
		if (input.wasCharPressedOnThisFrame('J')) skyRenderingMode = static_cast<SkyRenderingMode>((skyRenderingMode + 1) % (SkyRenderingMode::COUNT + 1));

		//camAng += { camAngAdjustmentSpeed_Keyboard * input.isButtonHeld(SDL_SCANCODE_R), camAngAdjustmentSpeed_Keyboard* input.isButtonHeld(SDL_SCANCODE_T), camAngAdjustmentSpeed_Keyboard* input.isButtonHeld(SDL_SCANCODE_Y)};
		//camAng -= { camAngAdjustmentSpeed_Keyboard * input.isButtonHeld(SDL_SCANCODE_F), camAngAdjustmentSpeed_Keyboard* input.isButtonHeld(SDL_SCANCODE_G), camAngAdjustmentSpeed_Keyboard* input.isButtonHeld(SDL_SCANCODE_H)};
		if (input.isButtonHeld(SDL_SCANCODE_C)) camPos = { 0.1,32.1,370 };
		if (input.isButtonHeld(SDL_SCANCODE_V)) camAng = { 0,0,0 };
		gamma += 0.1 * (input.isButtonHeld(SDL_SCANCODE_EQUALS) - input.isButtonHeld(SDL_SCANCODE_MINUS));
		fogMaxIntensityDist += 10 * (input.isButtonHeld(SDL_SCANCODE_B) - input.isButtonHeld(SDL_SCANCODE_N));

		Vec3 camAdd = -Vec3({ real(input.isButtonHeld(SDL_SCANCODE_D)), real(input.isButtonHeld(SDL_SCANCODE_X)), real(input.isButtonHeld(SDL_SCANCODE_W)) });
		camAdd += { real(input.isButtonHeld(SDL_SCANCODE_A)), real(input.isButtonHeld(SDL_SCANCODE_Z)), real(input.isButtonHeld(SDL_SCANCODE_S))};

		camAng.x = fmod(camAng.x, 2*M_PI);
		camAng.y = fmod(camAng.y, 2*M_PI);
		camAng.z = fmod(camAng.z, 2*M_PI);

		if (skyRenderingMode == ROTATING)
		{
			const Texture& skyTexture = textureManager.getTextureByName("RSKY1");
			int skyTextureWidth = skyTexture.getW();
			int skyTextureHeight = skyTexture.getH();
			real totalSkySize = framebufW; //asuming fov is 90 degrees
			real skyTextureScale = real(skyTextureWidth) / totalSkySize;
			real skyTextureOffset = (2 * M_PI - camAng.y) / (2 * M_PI) * skyTextureWidth;
			
			for (int y = 0; y < framebufH; ++y) //rotating sky
			{
				for (int x = 0; x < framebufW; ++x)
				{
					real skyX = skyTextureOffset + x * skyTextureScale;
					real skyY = 0 + y * skyTextureScale;
					Color skyColor = skyTexture.getPixel(skyX, skyY);
					framebuf.setPixelUnsafe(x, y, skyColor);
				}
			}
		}

		Matrix3 transformMatrix = getRotationMatrix(camAng);
		if (real l = camAdd.len() > 0)
		{
			camAdd /= camAdd.len();
			camAdd = getRotationMatrix(-camAng) * camAdd;
			camPos += camAdd * flySpeed;
		}
		ctr.prepare(camPos, transformMatrix);

		if (currentMap)
		{
			TriangleRenderContext ctx;
			ctx.ctr = &ctr;
			ctx.frameBuffer = &framebuf;
			ctx.lightBuffer = &lightBuf;
			ctx.textureManager = &textureManager;
			ctx.zBuffer = &zBuffer;
			ctx.framebufW = framebufW - 1;
			ctx.framebufH = framebufH - 1;
			if (skyRenderingMode == CUBE) for (const auto& it : skyCube) it.drawOn(ctx);
			if (skyRenderingMode == SPHERE) for (const auto& it : skySphere) it.drawOn(ctx);
			for (int nSector = 0; nSector < sectorTriangles.size(); ++nSector)
			{
				for (const auto& tri : sectorTriangles[nSector])
				{
					ctx.lightMult = pow(currentMap->sectors[nSector].lightLevel / 256.0, gamma);
					tri.drawOn(ctx);
				}
			}
		}

		if (fogEnabled)
		{
			double* zBuffPixels = zBuffer.getRawPixels();
			Color* framebufPixels = framebuf.getRawPixels();
			//Color fogColor = { 150, 42, 36 255 };
			Color fogColor = { 255, 255, 255, 255 };
			int pxCount = framebufW * framebufH;
			
			for (int i = 0; i < pxCount; ++i)
			{
				real depth = -zBuffPixels[i];
				Color c = framebufPixels[i];
				real lerpT = depth == 0.0 ? 1 : std::clamp((1.0/depth) / fogMaxIntensityDist, 0.0, 1.0); //z buffer stores 1/z, so need to get the real z
				c.r = lerp(c.r, fogColor.r, lerpT);
				c.g = lerp(c.g, fogColor.g, lerpT);
				c.b = lerp(c.b, fogColor.b, lerpT);
				framebufPixels[i] = c;
			}
		}

		//this is a stupid fix for everything becoming way too blue in debug mode specifically.
		//it tries to find a missing bit shift to put the alpha value into the unused byte, since Color.toSDL_Uint32 expects 4 shifts
		Uint32* px = reinterpret_cast<Uint32*>(wndSurf->pixels);
		auto* wf = wndSurf->format;
		std::array<uint32_t, 4> shifts = {wf->Rshift, wf->Gshift, wf->Bshift};
		uint32_t missingShift = 24;
		for (int i = 0; i < 3; ++i)
		{
			if (std::find(std::begin(shifts), std::end(shifts), i * 8) == std::end(shifts))
			{
				missingShift = i * 8;
				break;
			}
		}
		shifts[3] = missingShift;

		for (int y = 0; y < screenH; ++y)
		{
			for (int x = 0; x < screenW; ++x)
			{
				int fx = real(x) / screenW * framebufW;
				int fy = real(y) / screenH * framebufH;
				real lightMult = lightBuf.getPixelUnsafe(fx, fy);
				px[y * screenW + x] = framebuf.getPixelUnsafe(fx, fy).multipliedByLight(lightMult).toSDL_Uint32(shifts);
			}
		}

		performanceMonitor.registerFrameDone();
		PerformanceMonitor::OptionalInfo info;
		info.camPos = camPos;
		info.camAng = camAng;
		if (performanceMonitorDisplayEnabled) performanceMonitor.drawOn(wndSurf, { 0,0 }, &info);

		SDL_UpdateWindowSurface(wnd);
		//std::cout << "Frame " << frames++ << " done\n";
	}

	system("pause");
}

int main()
{
	try
	{
		program();
	}
	catch (const std::exception& e)
	{
		std::string msg = e.what();
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error has occured", msg.c_str(), nullptr);
	}
	return 0;
}