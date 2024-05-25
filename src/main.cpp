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
#include "GameStates/BenchmarkState.h"

#include <thread>
#include <functional>

#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2_image.lib")
#pragma comment(lib,"SDL2_ttf.lib")
#undef main

Statsman statsman;
std::unique_ptr<Threadpool> threadpool;

void* operator new(size_t n)
{
	StatCount(statsman.memory.allocsByNew++);
#ifdef __AVX512__
	constexpr size_t alignmentRequirement = 64;
#elif __AVX__
	constexpr size_t alignmentRequirement = 32;
#else 
	constexpr size_t alignmentRequirement = 16;
#endif

	return _aligned_malloc(n, alignmentRequirement);
}

void operator delete(void* block)
{
	StatCount(statsman.memory.freesByDelete++);
	return _aligned_free(block);
}


std::map<std::string, std::string> parseCmdArgs(int argc, char** argv)
{
	std::cout << "Command line is:\n";
	for (int i = 1; i < argc; ++i) std::cout << argv[i] << " ";
	std::cout << "\n";

	if (argc < 2) return {}; //argv[0] is out exe path, so it's meaningless to try and parse something else

	std::map<std::string, std::string> ret;
	std::unordered_set<std::string> validKeys = { "benchmark", "threads" };
	for (int i = 1; i < argc; ++i)
	{
		std::string argName = argv[i];
		if (validKeys.find(argName) == validKeys.end()) std::cout << "Unknown cmd argument at position " << i << ": " << argName << ", ignoring.\n";
		else 
		{
			if (argName == "threads" && argc > i + 1) ret[argName] = argv[i++ + 1];
			if (argName == "benchmark") ret[argName] = "true";
		}
		
	}
	return ret;
}

void program(int argc, char** argv)
{
	if (SDL_Init(SDL_INIT_EVERYTHING)) throw std::runtime_error(std::string("Failed to initialize SDL: ") + SDL_GetError());
	if (TTF_Init()) throw std::runtime_error(std::string("Failed to initialize SDL TTF: ") + TTF_GetError());
	//if (IMG_Init()) throw std::runtime_error(std::string("Failed to initialize SDL image: ") + IMG_GetError());
	// 	
	//loadWad("data/test_maps/STUPID.wad");
	//loadWad("data/test_maps/HEXAGON.wad");
	//loadWad("data/test_maps/RECT.wad");

	SDL_Window* wnd = SDL_CreateWindow("Doom Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, 0);
	SDL_Surface* wndSurf = SDL_GetWindowSurface(wnd);

	std::shared_ptr<GameStateBase> currGameState;
	auto args = parseCmdArgs(argc, argv);
	bool benchmarkMode = args.find("benchmark") != args.end() && args["benchmark"] == "true";

	std::optional<size_t> threadCount = std::nullopt;
	if (args.find("threads") != args.end()) threadCount = atol(args["threads"].c_str());

	//threadpool = std::unique_ptr<Threadpool>(new Threadpool(threadCount));
	threadpool = std::make_unique<Threadpool>(threadCount);

	GameStateInitData initData;
	initData.wnd = wnd;
	initData.argc = argc;
	initData.argv = argv;
	initData.threadpool = threadpool.get();

	if (benchmarkMode) currGameState = std::make_shared<BenchmarkState>(initData);
	else currGameState = std::make_shared<MainGame>(initData);

	while (true)
	{
		GameStateSwitch sw = currGameState->run();
		if (sw.nextState) currGameState = sw.nextState;
		else break;
			

		/*
		//if (skyRenderingMode == ROTATING)
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
					framebuf.setPixel(x, y, skyColor);
				}
			}
		}*/

		
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
					real pixel = zBuffer.getPixel(x, y);
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