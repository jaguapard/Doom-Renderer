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
#include "TextureMapper.h"
#include "Sky.h"

#include "DoomStructs.h"
#include "DoomWorldLoader.h"
#include "WadLoader.h"
#include "Model.h"

#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2_image.lib")
#pragma comment(lib,"SDL2_ttf.lib")
#undef main

Statsman statsman;

void* operator new(size_t n)
{
	StatCount(statsman.memory.allocsByNew++);
	return _aligned_malloc(n, 64);
}

void operator delete(void* block)
{
	StatCount(statsman.memory.freesByDelete++);
	return _aligned_free(block);
}

enum SkyRenderingMode
{
	NONE,
	ROTATING,
	SPHERE,
	COUNT
};
void program(int argc, char** argv)
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

	int framebufW = 1920;
	int framebufH = 1080;
	int screenW = 1920;
	int screenH = 1080;
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
	bool mouseCaptured = false;
	bool wireframeEnabled = false;

	PerformanceMonitor performanceMonitor;
	bool performanceMonitorDisplayEnabled = true;

	std::string warpTo;

	real camAngAdjustmentSpeed_Mouse = 1e-3;
	real camAngAdjustmentSpeed_Keyboard = 3e-2;

	std::vector<std::vector<Model>> sectorWorldModels;
	TextureManager textureManager;
	DoomMap* currentMap = nullptr;

	SkyRenderingMode skyRenderingMode = SPHERE;
	Sky sky("RSKY1", textureManager);
	int currSkyTextureIndex = 3;
	std::vector<std::string> skyTextures;
	skyTextures.push_back("RSKY1");
	skyTextures.push_back("RSKY2");
	skyTextures.push_back("RSKY3");
	{
		std::ifstream f("data/sky_textures.txt");
		std::string line;
		while (std::getline(f, line))
		{
			skyTextures.push_back(line);
		}
	}

	bool benchmarkMode = argc > 1 && (!strcmpi(argv[1], "benchmark"));
	int benchmarkModeFrames = argc > 2 ? atoi(argv[2]) : -1;
	int benchmarkModeFramesRemaining;
	std::string benchmarkPassName;
	if (argc > 3) benchmarkPassName = argv[3];

	if (benchmarkMode)
	{
		if (benchmarkModeFrames < 1)
		{
			std::cout << "Benchmark mode frame count not set. Defaulting to 1000.\n";
			benchmarkModeFrames = 1000;
		}
		std::cout << "Benchmark mode selected. Running...\n";

		benchmarkModeFramesRemaining = benchmarkModeFrames;
		camPos = { 441, 877, -488 };
		camAng = { 0,0,0.43 };
		
		currentMap = &maps.at("MAP15");
		sectorWorldModels = currentMap->getMapGeometryModels(textureManager);

		performanceMonitorDisplayEnabled = false;
		wireframeEnabled = false;
		mouseCaptured = false;
		fogEnabled = false;
		skyRenderingMode = SPHERE;
	}

	bob::Timer benchmarkTimer;
	performanceMonitor.reset();
	while (true)
	{
		performanceMonitor.registerFrameBegin();
		framebuf.clear();
		//framebuf = skyBuff;
		SDL_FillRect(wndSurf, nullptr, Color(0,0,0));
    	zBuffer.clear();
		lightBuf.clear(1);

		Vec3 camAdd = { 0,0,0 };
		if (!benchmarkMode) //benchmark mode will supress any user input
		{
			input.beginNewFrame();
			SDL_Event ev;
			while (SDL_PollEvent(&ev))
			{
				input.handleEvent(ev);
				if (mouseCaptured && ev.type == SDL_MOUSEMOTION)
				{
					camAng += { 0, ev.motion.xrel * -camAngAdjustmentSpeed_Mouse, ev.motion.yrel* camAngAdjustmentSpeed_Mouse};

					camAng.z = std::clamp<real>(camAng.z, -M_PI / 2 + 0.01, M_PI / 2 + 0.01); //no real need for + 0.01, but who knows
				}
				if (ev.type == SDL_MOUSEWHEEL)
				{
					flySpeed *= pow(1.05, ev.wheel.y);
				}

				if (ev.type == SDL_QUIT) return;
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
				sectorWorldModels = currentMap->getMapGeometryModels(textureManager);

				warpTo.clear();
				performanceMonitor.reset();
			}

			//xoring with 1 == toggle true->false or false->true
			if (input.wasCharPressedOnThisFrame('G')) fogEnabled ^= 1;
			if (input.wasCharPressedOnThisFrame('P')) performanceMonitorDisplayEnabled ^= 1;
			if (input.wasCharPressedOnThisFrame('J')) skyRenderingMode = static_cast<SkyRenderingMode>((skyRenderingMode + 1) % (SkyRenderingMode::COUNT));
			if (input.wasCharPressedOnThisFrame('O')) wireframeEnabled ^= 1;

			if (input.wasButtonPressedOnThisFrame(SDL_SCANCODE_LCTRL))
			{
				mouseCaptured ^= 1;
				SDL_SetRelativeMouseMode(mouseCaptured ? SDL_TRUE : SDL_FALSE);
			}

			bool skyChanged = false;
			if (input.wasButtonPressedOnThisFrame(SDL_SCANCODE_LEFT))
			{
				currSkyTextureIndex--;
				skyChanged = true;
			}
			else if (input.wasButtonPressedOnThisFrame(SDL_SCANCODE_RIGHT))
			{
				currSkyTextureIndex++;
				skyChanged = true;
			}

			if (currSkyTextureIndex < 0) currSkyTextureIndex = 0;
			else currSkyTextureIndex %= skyTextures.size();

			if (skyChanged) sky = Sky(skyTextures[currSkyTextureIndex], textureManager);


			//camAng += { camAngAdjustmentSpeed_Keyboard * input.isButtonHeld(SDL_SCANCODE_R), camAngAdjustmentSpeed_Keyboard* input.isButtonHeld(SDL_SCANCODE_T), camAngAdjustmentSpeed_Keyboard* input.isButtonHeld(SDL_SCANCODE_Y)};
			//camAng -= { camAngAdjustmentSpeed_Keyboard * input.isButtonHeld(SDL_SCANCODE_F), camAngAdjustmentSpeed_Keyboard* input.isButtonHeld(SDL_SCANCODE_G), camAngAdjustmentSpeed_Keyboard* input.isButtonHeld(SDL_SCANCODE_H)};
			if (input.isButtonHeld(SDL_SCANCODE_C)) camPos = { 0.1,32.1,370 };
			if (input.isButtonHeld(SDL_SCANCODE_V)) camAng = { 0,0,0 };
			gamma += 0.1 * (input.isButtonHeld(SDL_SCANCODE_EQUALS) - input.isButtonHeld(SDL_SCANCODE_MINUS));
			fogMaxIntensityDist += 10 * (input.isButtonHeld(SDL_SCANCODE_B) - input.isButtonHeld(SDL_SCANCODE_N));

			camAdd = -Vec3({ real(input.isButtonHeld(SDL_SCANCODE_D)), real(input.isButtonHeld(SDL_SCANCODE_X)), real(input.isButtonHeld(SDL_SCANCODE_W)) });
			camAdd += { real(input.isButtonHeld(SDL_SCANCODE_A)), real(input.isButtonHeld(SDL_SCANCODE_Z)), real(input.isButtonHeld(SDL_SCANCODE_S))};

			camAng.x = fmod(camAng.x, M_PI);
			camAng.y = fmod(camAng.y, 2 * M_PI);
			//camAng.z = fmod(camAng.z, 2*M_PI);
		}
		else
		{
			SDL_Event ev;
			while (SDL_PollEvent(&ev)) {}; //prevent window from freezing
			camAng.y = double(benchmarkModeFrames-benchmarkModeFramesRemaining) / benchmarkModeFrames * 2 * M_PI;
			if (!benchmarkModeFramesRemaining--)
			{
				auto info = performanceMonitor.getPercentileInfo();
				std::stringstream ss;
				ss << benchmarkModeFrames << " frames rendered in " << benchmarkTimer.getTime() << " s\n";
				ss << info.fps_avg << " avg, " << "1% low: " << info.fps_1pct_low << ", " << "0.1% low: " << info.fps_point1pct_low << "\n";
				if (!benchmarkPassName.empty()) ss << "Comment: " << benchmarkPassName << "\n";

				std::cout << ss.str();
				std::ofstream f("benchmarks.txt", std::ios::app);
				f << ss.str() << "\n";
				break;
			}
		}

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

		TriangleRenderContext ctx;
		ctx.ctr = &ctr;
		ctx.frameBuffer = &framebuf;
		ctx.lightBuffer = &lightBuf;
		ctx.textureManager = &textureManager;
		ctx.zBuffer = &zBuffer;
		ctx.framebufW = framebufW - 1;
		ctx.framebufH = framebufH - 1;
		ctx.doomSkyTextureMarkerIndex = textureManager.getTextureIndexByName("F_SKY1"); //Doom uses F_SKY1 to mark sky. Any models with this texture will exit their rendering immediately
		ctx.wireframeEnabled = wireframeEnabled;

		if (currentMap)
		{			
			for (int nSector = 0; nSector < sectorWorldModels.size(); ++nSector)
			{
				for (const auto& model : sectorWorldModels[nSector])
				{
					ctx.lightMult = pow(currentMap->sectors[nSector].lightLevel / 256.0, gamma);
					model.draw(ctx);
				}
			}
		}
		if (skyRenderingMode == SPHERE) sky.draw(ctx); //a 3D sky can be drawn after everything else. In fact, it's better, since a large part of it may already be occluded.

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
		assert(screenW * screenH == framebufW * framebufH);
		Color::multipliyByLightInPlace(&lightBuf[0], &framebuf[0], framebufW * framebufH);
		Color::toSDL_Uint32(&framebuf[0], reinterpret_cast<Uint32*>(wndSurf->pixels), framebufW * framebufH, shifts);

		performanceMonitor.registerFrameDone();
		PerformanceMonitor::OptionalInfo info;
		info.camPos = camPos;
		info.camAng = camAng;
		if (performanceMonitorDisplayEnabled) performanceMonitor.drawOn(wndSurf, { 0,0 }, &info);

		SDL_UpdateWindowSurface(wnd);
		//std::cout << "Frame " << frames++ << " done\n";
	}
}

int main(int argc, char** argv)
{
	try
	{
		program(argc, argv);
	}
	catch (const std::exception& e)
	{
		std::string msg = e.what();
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "An error has occured", msg.c_str(), nullptr);
	}
	return 0;
}