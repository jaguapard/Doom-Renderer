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
#include "Threadpool.h"

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

enum SkyRenderingMode : uint32_t
{
	NONE,
	ROTATING,
	SPHERE,
	COUNT
};

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

enum class WheelAdjustmentMode : uint32_t
{
	FLY_SPEED,
	FOV,
	COUNT,
};

std::map<std::string, CmdArg> parseCmdArgs(int argc, char** argv)
{
	std::string validKeys = { "benchmark" };
	return {};
}

std::string vecToStr(const Vec3& v)
{
	return std::to_string(v.x) + " " + std::to_string(v.y) + " " + std::to_string(v.z);
}

void program(int argc, char** argv)
{
	if (SDL_Init(SDL_INIT_EVERYTHING)) throw std::runtime_error(std::string("Failed to initialize SDL: ") + SDL_GetError());
	if (TTF_Init()) throw std::runtime_error(std::string("Failed to initialize SDL TTF: ") + TTF_GetError());
	//if (IMG_Init()) throw std::runtime_error(std::string("Failed to initialize SDL image: ") + IMG_GetError());

	auto maps = WadLoader::loadWad("doom2.wad"); //can't redistribute commercial wads!
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
	bool backfaceCullingEnabled = false;
	real nearPlaneZ = -1;
	real fovMult = 1;
	WheelAdjustmentMode wheelAdjMod = WheelAdjustmentMode::FLY_SPEED;

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
	int currSkyTextureIndex = 2;
	std::vector<std::string> skyTextures;
	skyTextures.push_back("RSKY2");
	skyTextures.push_back("RSKY3");
	skyTextures.push_back("RSKY1");
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

	std::vector<RenderJob> renderJobs;
	TriangleRenderContext ctx;

	Threadpool threadpool; //auto number of threads
	std::vector<Threadpool::task_id> taskIds;
	Threadpool::task_id windowUpdateTaskId = 0;

	if (!benchmarkMode)
	{
		currentMap = &maps["MAP01"];
		sectorWorldModels = currentMap->getMapGeometryModels(textureManager);
	}

	const Vec3 forward = Vec3(0, 0, -1);
	const Vec3 right = Vec3(1, 0, 0);
	const Vec3 up = Vec3(0, 1, 0);

	while (true)
	{
		int threadCount = threadpool.getThreadCount();
		taskIds = {
			threadpool.addTask([&]() {framebuf.clear(); }),
			threadpool.addTask([&]() {SDL_FillRect(wndSurf, nullptr, Color(0, 0, 0)); }, {windowUpdateTaskId}), //shouldn't overwrite the surface while window update is in progress
			threadpool.addTask([&]() {zBuffer.clear(); }),
			threadpool.addTask([&]() {lightBuf.clear(1); }),
		};		
		performanceMonitor.registerFrameBegin();

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
					camAng.y -= ev.motion.xrel * camAngAdjustmentSpeed_Mouse;
					camAng.z += ev.motion.yrel * camAngAdjustmentSpeed_Mouse;					
				}
				if (ev.type == SDL_MOUSEWHEEL)
				{
					if (wheelAdjMod == WheelAdjustmentMode::FLY_SPEED) flySpeed *= pow(1.05, ev.wheel.y);
					if (wheelAdjMod == WheelAdjustmentMode::FOV) fovMult *= pow(1.05, ev.wheel.y);
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
			if (input.wasCharPressedOnThisFrame('C')) camPos = { -96, 70, 784 };
			if (input.wasCharPressedOnThisFrame('K')) wheelAdjMod = static_cast<WheelAdjustmentMode>((uint32_t(wheelAdjMod) + 1) % uint32_t(WheelAdjustmentMode::COUNT));
			if (input.wasCharPressedOnThisFrame('L')) fovMult = 1;
			if (input.wasCharPressedOnThisFrame('V')) camAng = { 0,0,0 };			
			if (input.wasCharPressedOnThisFrame('R')) backfaceCullingEnabled ^= 1;

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

			if (currSkyTextureIndex < 0) currSkyTextureIndex = skyTextures.size() - 1;
			else currSkyTextureIndex %= skyTextures.size();

			if (skyChanged) sky = Sky(skyTextures[currSkyTextureIndex], textureManager);


			//camAng += { camAngAdjustmentSpeed_Keyboard * input.isButtonHeld(SDL_SCANCODE_R), camAngAdjustmentSpeed_Keyboard* input.isButtonHeld(SDL_SCANCODE_T), camAngAdjustmentSpeed_Keyboard* input.isButtonHeld(SDL_SCANCODE_Y)};
			//camAng -= { camAngAdjustmentSpeed_Keyboard * input.isButtonHeld(SDL_SCANCODE_F), camAngAdjustmentSpeed_Keyboard* input.isButtonHeld(SDL_SCANCODE_G), camAngAdjustmentSpeed_Keyboard* input.isButtonHeld(SDL_SCANCODE_H)};
			
			gamma += 0.1 * (input.isButtonHeld(SDL_SCANCODE_EQUALS) - input.isButtonHeld(SDL_SCANCODE_MINUS));
			fogMaxIntensityDist += 10 * (input.isButtonHeld(SDL_SCANCODE_B) - input.isButtonHeld(SDL_SCANCODE_N));			
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
				ss << "\n" << benchmarkModeFrames << " frames rendered in " << benchmarkTimer.getTime() << " s\n";
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

		camAng.x = fmod(camAng.x, M_PI);
		camAng.y = fmod(camAng.y, 2 * M_PI);
		camAng.z = std::clamp<real>(camAng.z, -M_PI / 2 + 0.01, M_PI / 2 - 0.01); //no real need for 0.01, but who knows
		Matrix3 transformMatrix = getRotationMatrix(camAng);
		
		//don't touch this arcanery - it somehow works
		Vec3 newForward = Vec3(-transformMatrix.elements[0][2], -transformMatrix.elements[1][2], -transformMatrix.elements[2][2]);
		Vec3 newRight =   Vec3(-transformMatrix.elements[0][0], -transformMatrix.elements[1][0], -transformMatrix.elements[2][0]);
		Vec3 newUp = up; //don't transform up for now
		camAdd = Vec3(0, 0, 0);
		camAdd += newForward * real(input.isButtonHeld(SDL_SCANCODE_W));
		camAdd -= newForward * real(input.isButtonHeld(SDL_SCANCODE_S));
		camAdd += newRight * real(input.isButtonHeld(SDL_SCANCODE_D));
		camAdd -= newRight * real(input.isButtonHeld(SDL_SCANCODE_A));
		camAdd += newUp * real(input.isButtonHeld(SDL_SCANCODE_Z));
		camAdd -= newUp * real(input.isButtonHeld(SDL_SCANCODE_X));
		
		if (real len = camAdd.len() > 0)
		{
			camAdd /= len;
			camPos += camAdd * flySpeed;
		}

		ctr.prepare(camPos, transformMatrix);
		
		ctx.ctr = &ctr;
		ctx.frameBuffer = &framebuf;
		ctx.lightBuffer = &lightBuf;
		ctx.textureManager = &textureManager;
		ctx.zBuffer = &zBuffer;
		ctx.framebufW = framebufW;
		ctx.framebufH = framebufH;
		ctx.doomSkyTextureMarkerIndex = textureManager.getTextureIndexByName("F_SKY1"); //Doom uses F_SKY1 to mark sky. Any models with this texture will exit their rendering immediately
		ctx.wireframeEnabled = wireframeEnabled;
		ctx.backfaceCullingEnabled = backfaceCullingEnabled;
		ctx.renderJobs = &renderJobs;
		ctx.nearPlaneClippingZ = nearPlaneZ;
		ctx.fovMult = fovMult;

		if (currentMap)
		{			
			for (int nSector = 0; nSector < sectorWorldModels.size(); ++nSector)
			{
				for (const auto& model : sectorWorldModels[nSector])
				{
					ctx.lightMult = pow(currentMap->sectors[nSector].lightLevel / 256.0, gamma);
					model.addToRenderQueue(ctx);
				}
			}
		}
		if (skyRenderingMode == SPHERE) sky.addToRenderQueue(ctx); //a 3D sky can be drawn after everything else. In fact, it's better, since a large part of it may already be occluded.
		
		assert(screenW * screenH == framebufW * framebufH);
		int pixelCount = framebufW * framebufH;
		int startPixels = 0;
		int threadStep = pixelCount / threadCount;
		real* lightStart = lightBuf.begin();
		Color* framebufStart = framebuf.begin();
		Uint32* wndSurfStart = reinterpret_cast<Uint32*>(wndSurf->pixels);

		//this is a stupid fix for everything becoming way too blue in debug mode specifically.
		//it tries to find a missing bit shift to put the alpha value into the unused byte, since Color.toSDL_Uint32 expects 4 shifts
		auto* wf = wndSurf->format;
		std::array<uint32_t, 4> shifts = { wf->Rshift, wf->Gshift, wf->Bshift };
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

		auto renderDependencies = taskIds;
		for (int tNum = 0; tNum < threadCount; ++tNum)
		{
			//is is crucial to capture some stuff by value [=], else function risks getting garbage values when the task starts. 
			//It is, however, assumed that renderJobs vector remains in a valid state until all tasks are completed.
			Threadpool::task_t f = [=, &renderJobs]() {
				int myThreadNum = tNum;
				int myMinY = real(ctx.framebufH) / threadCount * myThreadNum;
				int myMaxY = real(ctx.framebufH) / threadCount * (myThreadNum + 1);
				if (myThreadNum == threadCount - 1) myMaxY = ctx.framebufH - 1; //avoid going out of bounds

				for (int i = 0; i < renderJobs.size(); ++i)
				{
					const RenderJob& myJob = renderJobs[i];
					myJob.t.drawSlice(ctx, myJob, myMinY, myMaxY);
				}			

				int myPixelCount = (myMaxY - myMinY) * ctx.framebufW;
				int myStartIndex = myMinY * ctx.framebufW;

				real* lightPtr = lightStart + myStartIndex;
				Color* framebufPtr = framebufStart + myStartIndex;
				Uint32* wndSurfPtr = wndSurfStart + myStartIndex;

				Color::multipliyByLightInPlace(lightPtr, framebufPtr, myPixelCount);
				Color::toSDL_Uint32(framebufPtr, wndSurfPtr, myPixelCount, shifts);
			};
			taskIds.push_back(threadpool.addTask(f, renderDependencies));
		}
		
		for (auto& it : taskIds) threadpool.waitUntilTaskCompletes(it);
		renderJobs.clear();

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

		windowUpdateTaskId = threadpool.addTask([&, camPos, camAng]() {
			performanceMonitor.registerFrameDone();
			if (performanceMonitorDisplayEnabled)
			{
				std::map<std::string, std::string> perfmonInfo;
				perfmonInfo["Cam pos"] = vecToStr(camPos);
				perfmonInfo["Cam ang"] = vecToStr(camAng);
				perfmonInfo["Fly speed"] = std::to_string(flySpeed) + "/frame";
				perfmonInfo["Backface culling"] = backfaceCullingEnabled ? "enabled" : "disabled";
				perfmonInfo["FOV"] = std::to_string(2 * atan(1 / fovMult) * 180 / M_PI) + " degrees";
				performanceMonitor.drawOn(wndSurf, { 0,0 }, perfmonInfo);
			}
			if (SDL_UpdateWindowSurface(wnd)) throw std::runtime_error(SDL_GetError()); 
		});
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