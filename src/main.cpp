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
#include "Matrix4.h"
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
#include "Threadpool.h"
#include "GameStates/MainGame.h"

#include <thread>
#include <functional>

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

struct CmdArg
{
	std::string originalValue;
	int64_t asInt;
	double asDouble;

	enum class Type {
		INTEGER,
		DOUBLE,
		STRING,
		NONE, //single arg (no value), a switch
	};
};

std::map<std::string, CmdArg> parseCmdArgs(int argc, char** argv)
{
	std::string validKeys = { "benchmark" };
	return {};
}

void program(int argc, char** argv)
{
	Threadpool threadpool;
	if (SDL_Init(SDL_INIT_EVERYTHING)) throw std::runtime_error(std::string("Failed to initialize SDL: ") + SDL_GetError());
	if (TTF_Init()) throw std::runtime_error(std::string("Failed to initialize SDL TTF: ") + TTF_GetError());
	//if (IMG_Init()) throw std::runtime_error(std::string("Failed to initialize SDL image: ") + IMG_GetError());
	

	
	//loadWad("data/test_maps/STUPID.wad");
	//loadWad("data/test_maps/HEXAGON.wad");
	//loadWad("data/test_maps/RECT.wad");

	SDL_Window* wnd = SDL_CreateWindow("Doom Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, 0);
	SDL_Surface* wndSurf = SDL_GetWindowSurface(wnd);

	/*
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
		camAng = { 0,0,-0.43 };
		
		currentMap = &maps.at("MAP15");
		sectorWorldModels = currentMap->getMapGeometryModels(textureManager);

		performanceMonitorDisplayEnabled = false;
		wireframeEnabled = false;
		mouseCaptured = false;
		fogEnabled = false;
		skyRenderingMode = SPHERE;
	}

	bob::Timer benchmarkTimer;
	*/

	GameStateInitData initData;
	initData.wnd = wnd;
	initData.argc = argc;
	initData.argv = argv;
	std::unique_ptr<GameStateBase> currGameState = std::make_unique<MainGame>(initData, threadpool);

	while (true)
	{
		currGameState->beginNewFrame();

		/*
		if (!benchmarkMode) //benchmark mode will supress any user input
		{*/
			SDL_Event ev;
			while (SDL_PollEvent(&ev))
			{
				currGameState->handleInputEvent(ev);
				if (ev.type == SDL_QUIT) return;
			}
			currGameState->postEventPollingRoutine();
			
		/* }
		else
		{
			SDL_Event ev;
			while (SDL_PollEvent(&ev)) {}; //prevent window from freezing
			camAng.y = (1 - double(benchmarkModeFrames-benchmarkModeFramesRemaining) / benchmarkModeFrames) * 2 * M_PI;
			if (!benchmarkModeFramesRemaining--)
			{
				auto info = performanceMonitor.getPercentileInfo();
				std::stringstream ss;
				ss << "\n" << benchmarkModeFrames << " frames rendered in " << benchmarkTimer.getTime() << " s\n";
				ss << info.fps_avg << " avg, " << "1% low: " << info.fps_1pct_low << ", " << "0.1% low: " << info.fps_point1pct_low << "\n";
				if (!benchmarkPassName.empty()) ss << "Comment: " << benchmarkPassName << "\n";

				std::cout << ss.str();
				std::ofstream f("benchmarks.txt", std::ios::app);
				f << ss.str() << "\n";
				break;
			}
		} */

		/*
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
		}*/

		currGameState->update();
		currGameState->draw();
		//assert(screenW * screenH == framebufW * framebufH);
		/*
		if (fogEnabled)
		{
			real* zBuffPixels = zBuffer.getRawPixels();
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

		if (!benchmarkMode && input.wasCharPressedOnThisFrame('U'))
		{
			std::string s = std::to_string(__rdtsc());
			framebuf.saveToFile("screenshots/" + s + "_framebuf.png");
			zBuffer.saveToFile("screenshots/" + s+ "_zbuf.png");
			lightBuf.saveToFile("screenshots/" + s + "_lightbuf.png");

			std::ofstream f("screenshots/" + s + "_zbuffer.txt");
			f.precision(10);
			for (int y = 0; y < framebufH; ++y)
			{
				for (int x = 0; x < framebufW; x++)
				{
					real pixel = zBuffer.getPixelUnsafe(x, y);
					real depth = -1.0 / pixel;
					f << x << " " << y << " " << depth << "\n";
				}
			}
		}

		if (!benchmarkMode && input.wasCharPressedOnThisFrame('U'))
		{
			//framebuf.saveToFile("screenshots/fullframe.png");
		}
		*/

		currGameState->endFrame();
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