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
	MainGame(GameStateInitData data, Threadpool& threadpool);
	virtual void beginNewFrame();
	virtual void handleInputEvent(SDL_Event& ev);
	virtual void postEventPollingRoutine();
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
	std::vector<Vec3> camPosAndAngArchieve;

	int activeCamPosAndAngle = 2;
	Vec3 camPos;
	Vec3 camAng;

	int framebufW = 1920;
	int framebufH = 1080;
	int screenW = 1920;
	int screenH = 1080;

	PixelBuffer<Color> framebuf;
	PixelBuffer<real> lightBuf;
	ZBuffer zBuffer;

	C_Input input;
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

	Threadpool& threadpool;
	std::vector<Threadpool::task_id> bufferClearingTaskIds;
	Threadpool::task_id windowUpdateTaskId = 0;

	std::map<std::string, DoomMap> maps;




	void init();
	void loadMap(std::string mapName);

	TriangleRenderContext makeTriangleRenderContext();
	std::vector<RenderJob> makeRenderJobsList(TriangleRenderContext ctx);
	std::array<uint32_t, 4> getShiftsForWindow();
};