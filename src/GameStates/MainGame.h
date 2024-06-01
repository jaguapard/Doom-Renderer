#pragma once

#include <vector>
#include <map>
#include <string>
#include <array>

#include "GameStateBase.h"
#include "../Vec.h"
#include "../PixelBuffer.h"
#include "../ZBuffer.h"
#include "../C_Input.h"
#include "../CoordinateTransformer.h"
#include "../PerformanceMonitor.h"
#include "../Sky.h"
#include "../DoomMap.h"
#include "../TextureManager.h"
#include "../Threadpool.h"
#include "../WadLoader.h"
#include "../Triangle.h"

#include <SDL/SDL.h>

class MainGame : public GameStateBase
{
public:
	MainGame() = default;
	MainGame(GameStateInitData data);
	virtual void beginNewFrame();
	virtual void handleInput();

	virtual void update();
	virtual void draw();
	virtual void endFrame();
protected:
	enum class WheelAdjustmentMode : uint32_t
	{
		FLY_SPEED,
		FOV,
		COUNT,
	};

	enum SkyRenderingMode : uint32_t
	{
		NONE,
		ROTATING,
		SPHERE,
		COUNT
	};

	GameStateInitData initData;
	SDL_Window* wnd;
	SDL_Surface* wndSurf;
	Threadpool* threadpool;

	std::vector<Vec4> camPosAndAngArchieve;
	int activeCamPosAndAngle = 2;
	Vec4 camPos;
	Vec4 camAng;

	PixelBuffer<Color> framebuf;
	PixelBuffer<real> lightBuf;
	ZBuffer zBuffer;

	CoordinateTransformer ctr;

	uint64_t frames = 0;
	real flySpeed = 6;
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
	Sky sky;

	int currSkyTextureIndex = 2;
	std::vector<std::string> skyTextures;

	std::vector<task_id> bufferClearingTaskIds;
	task_id windowUpdateTaskId = 0;

	std::map<std::string, DoomMap> maps;
	std::string defaultMap;

	std::vector<RenderJob> renderJobs;
	std::array<uint32_t, 4> shifts;


	void init();
	void changeMapTo(std::string mapName);

	TriangleRenderContext makeTriangleRenderContext();
	void fillRenderJobsList(TriangleRenderContext ctx, std::vector<RenderJob>& renderJobs);
	std::array<uint32_t, 4> getShiftsForWindow();
};