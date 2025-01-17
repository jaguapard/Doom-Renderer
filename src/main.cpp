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
#ifdef __AVX512F__
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
	std::unordered_set<std::string> validKeys = { "benchmark", "threads", "wnd_w", "wnd_h"};

	for (int i = 1; i < argc; ++i) //skip our executable path
	{
		std::string argName = argv[i];
		std::stringstream ss;
		if (argName.length() < 2 || argName[0] != '-' || argName[1] != '-' || validKeys.find(argName.substr(2)) == validKeys.end())
		{
			std::cout << "Unknown cmd argument at position " << i << ": " << argName << ", ignoring.\n";
		}
		else 
		{
			argName = argName.substr(2);
			if ((argName == "threads" || argName == "wnd_w" || argName == "wnd_h") && argc > i + 1) ret[argName] = argv[i++ + 1];
			if (argName == "benchmark") ret[argName] = "true";
		}
		
	}
	return ret;
}

void program(int argc, char** argv)
{
	
	Matrix4 m = {
		Vec4(6,3,5,1),
		Vec4(-5,-2,9,-6),
		Vec4(11,2,1,34),
		Vec4(4,3,0,2)
	};

	Matrix4 z = m.inverse();

	Vec4 results[16];
	VectorPack16 pack;
	for (int i = 0; i < 16; ++i)
	{
		Vec4 v = Vec4(1032, -2434, 324.534, 1) + Vec4(i*0.5235, i*-123, i/31, i+3);
		results[i] = m * v;
		pack.x[i] = v.x;
		pack.y[i] = v.y;
		pack.z[i] = v.z;
		pack.w[i] = v.w;
	}

	VectorPack16 res2 = m * pack;
	for (int i = 0; i < 16; ++i)
	{
		assert(abs(res2.x[i] - results[i].x) < 0.00001);
		assert(abs(res2.y[i] - results[i].y) < 0.00001);
		assert(abs(res2.z[i] - results[i].z) < 0.00001);
		assert(abs(res2.w[i] - results[i].w) < 0.00001);
	}
	
	
	

	if (SDL_Init(SDL_INIT_EVERYTHING)) throw std::runtime_error(std::string("Failed to initialize SDL: ") + SDL_GetError());
	if (TTF_Init()) throw std::runtime_error(std::string("Failed to initialize SDL TTF: ") + TTF_GetError());
	//if (IMG_Init()) throw std::runtime_error(std::string("Failed to initialize SDL image: ") + IMG_GetError());
	// 	
	//loadWad("data/test_maps/STUPID.wad");
	//loadWad("data/test_maps/HEXAGON.wad");
	//loadWad("data/test_maps/RECT.wad");

	auto args = parseCmdArgs(argc, argv);
	int wndW = args.find("wnd_w") == args.end() ? 1920 : atol(args["wnd_w"].c_str());
	int wndH = args.find("wnd_h") == args.end() ? 1080 : atol(args["wnd_h"].c_str());

	SDL_Window* wnd = SDL_CreateWindow("Doom Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, wndW, wndH, 0);
	SDL_Surface* wndSurf = SDL_GetWindowSurface(wnd);

	std::shared_ptr<GameStateBase> currGameState;
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