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

	FloatColorBuffer framebuf;
	PixelBuffer<real> lightBuf;
	ZBuffer zBuffer;

	CoordinateTransformer ctr;

	struct GameSettings
	{
		real flySpeed = 6;
		real gamma = 1.3;
		real nearPlaneZ = -1;
		real fovMult = 1;

		real camAngAdjustmentSpeed_Mouse = 1e-3;
		real camAngAdjustmentSpeed_Keyboard = 3e-2;
		
		real fogIntensity = 600;
		int fogEffectVersion = 1;

		WheelAdjustmentMode wheelAdjMod = WheelAdjustmentMode::FLY_SPEED;
		SkyRenderingMode skyRenderingMode = SPHERE;

		bool fogEnabled = false;
		bool mouseCaptured = false;
		bool wireframeEnabled = false;
		bool backfaceCullingEnabled = false;
		bool bufferCleaningEnabled = false;
		bool performanceMonitorDisplayEnabled = true;		
	};

	GameSettings settings;
	PerformanceMonitor performanceMonitor;	

	std::vector<std::vector<Model>> sectorWorldModels;
	TextureManager textureManager;

	Sky sky;

	int currSkyTextureIndex = 2;
	std::vector<std::string> skyTextures;

	task_id windowUpdateTaskId = 0;

	std::map<std::string, DoomMap> maps;
	std::string defaultMap;
	DoomMap* currentMap = nullptr;
	std::string warpTo;

	std::vector<RenderJob> renderJobs;
	std::array<uint32_t, 4> shifts;


	void init();
	void changeMapTo(std::string mapName);

	TriangleRenderContext makeTriangleRenderContext();
	void fillRenderJobsList(TriangleRenderContext ctx, std::vector<RenderJob>& renderJobs);
	std::array<uint32_t, 4> getShiftsForWindow();
};