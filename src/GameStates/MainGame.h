#pragma once

#include <vector>
#include <map>
#include <string>
#include <array>

#include <SDL/SDL.h>

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

#include "../misc/Enums.h"
#include "../PointLight.h"
#include "../misc/GameSettings.h"
#include "../ShadowMap.h"
#include "../KeepApartVector.h"
#include "../Camera.h"
#include "../Renderers/RendererBase.h"

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
	GameStateInitData initData;
	SDL_Window* wnd;
	SDL_Surface* wndSurf;
	Threadpool* threadpool;

    std::vector<PointLight> pointLights;

	std::vector<Vec4> camPosAndAngArchieve;
	int activeCamPosAndAngle = 2;
	Camera camera;

	std::vector<ShadowMap> shadowMaps;

	GameSettings settings;
	PerformanceMonitor performanceMonitor;	

	std::vector<Model> sceneModels;

	TextureManager textureManager;

	Sky sky;

	int currSkyTextureIndex = 2;
	std::vector<std::string> skyTextures;

	task_id windowUpdateTaskId = 0;

	std::map<std::string, DoomMap> maps;
	std::string defaultMap;
	DoomMap* currentMap = nullptr;
	std::string warpTo;

	std::array<uint32_t, 4> shifts;

	std::unique_ptr<RendererBase> renderer;
	void init();
	void changeMapTo(std::string mapName);

	void adjustSsaaMult(int add);
};