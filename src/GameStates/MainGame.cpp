#include "MainGame.h"
#include <fstream>
#include <iostream>

#include "../blitting.h"
#include "../EnumclassHelper.h"
#include "../AssetLoader.h"
#include "../shaders/MainFragmentRenderShader.h"
#include "../Renderers/RasterizationRenderer.h"

MainGame::MainGame(GameStateInitData data)
{
	initData = data;
	defaultMap = "MAP00";
	init();

	this->changeMapTo(defaultMap);
	performanceMonitor.reset();
}

void MainGame::beginNewFrame()
{
	performanceMonitor.registerFrameBegin();
	input.beginNewFrame();
}

void MainGame::handleInput()
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev))
	{
		input.handleEvent(ev);
		if (settings.mouseCaptured && ev.type == SDL_MOUSEMOTION)
		{
			this->camera.angle.y += ev.motion.xrel * settings.camAngAdjustmentSpeed_Mouse;
			this->camera.angle.z -= ev.motion.yrel * settings.camAngAdjustmentSpeed_Mouse;
		}
		if (ev.type == SDL_MOUSEWHEEL)
		{
			if (settings.wheelAdjMod == WheelAdjustmentMode::FLY_SPEED) settings.flySpeed *= pow(1.05, ev.wheel.y);
			if (settings.wheelAdjMod == WheelAdjustmentMode::FOV) settings.fovMult *= pow(1.05, ev.wheel.y);
		}
		if (ev.type == SDL_QUIT) throw GameStateSwitch();
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

		this->changeMapTo(mapToLoad);
		warpTo.clear();
	}

	//xoring with 1 == toggle true->false or false->true
	if (input.wasCharPressedOnThisFrame('G')) settings.fogEnabled ^= 1;
	if (input.wasCharPressedOnThisFrame('P')) settings.performanceMonitorDisplayEnabled ^= 1;
	if (input.wasCharPressedOnThisFrame('J')) settings.skyRenderingMode = EnumclassHelper::next(settings.skyRenderingMode);
	if (input.wasCharPressedOnThisFrame('O')) settings.wireframeEnabled ^= 1;
	if (input.wasCharPressedOnThisFrame('C')) this->camera.pos = { -96, 70, 784 };
	if (input.wasCharPressedOnThisFrame('K')) settings.wheelAdjMod = EnumclassHelper::next(settings.wheelAdjMod);
	if (input.wasCharPressedOnThisFrame('L')) settings.fovMult = 1;
	if (input.wasCharPressedOnThisFrame('V')) this->camera.angle = { 0,0,0 };
	if (input.wasCharPressedOnThisFrame('R')) settings.backfaceCullingEnabled ^= 1;
	if (input.wasCharPressedOnThisFrame('U')) settings.fogEffectVersion = EnumclassHelper::next(settings.fogEffectVersion);
	if (input.wasCharPressedOnThisFrame('Y')) settings.ditheringEnabled ^= 1;
	if (input.wasCharPressedOnThisFrame('Q') && settings.ssaaMult > 1) this->adjustSsaaMult(settings.ssaaMult - 1);
	if (input.wasCharPressedOnThisFrame('E')) this->adjustSsaaMult(settings.ssaaMult + 1);
	if (input.wasCharPressedOnThisFrame('H')) this->renderer->saveBuffers();

	if (input.wasButtonPressedOnThisFrame(SDL_SCANCODE_LCTRL))
	{
		settings.mouseCaptured ^= 1;
		SDL_SetRelativeMouseMode(settings.mouseCaptured ? SDL_TRUE : SDL_FALSE);
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

	settings.gamma += 0.1 * (input.isButtonHeld(SDL_SCANCODE_EQUALS) - input.isButtonHeld(SDL_SCANCODE_MINUS));
	settings.fogIntensity += 10 * (input.isButtonHeld(SDL_SCANCODE_B) - input.isButtonHeld(SDL_SCANCODE_N));

	//camAng.z = fmod(camAng.z, M_PI);
	this->camera.angle.y = fmod(this->camera.angle.y, 2 * M_PI);
	this->camera.angle.z = std::clamp<real>(this->camera.angle.z, -M_PI / 2 + 0.01, M_PI / 2 - 0.01); //no real need for 0.01, but who knows
	Matrix4 rotationOnlyMatrix = Matrix4::rotationXYZ(this->camera.angle);

	const Vec4 forward = Vec4(0, 0, -1);
	const Vec4 right = Vec4(1, 0, 0);
	const Vec4 up = Vec4(0, 1, 0);
	//don't touch this arcanery - it somehow works
	Vec4 newForward = Vec4(-rotationOnlyMatrix[2][0], -rotationOnlyMatrix[2][1], -rotationOnlyMatrix[2][2]);
	Vec4 newRight = Vec4(-rotationOnlyMatrix[0][0], -rotationOnlyMatrix[0][1], -rotationOnlyMatrix[0][2]);
	Vec4 newUp = up; //don't transform up for now
	Vec4 camAdd = Vec4(0, 0, 0);

	camAdd += newForward * real(input.isButtonHeld(SDL_SCANCODE_W));
	camAdd -= newForward * real(input.isButtonHeld(SDL_SCANCODE_S));
	camAdd += newRight * real(input.isButtonHeld(SDL_SCANCODE_D));
	camAdd -= newRight * real(input.isButtonHeld(SDL_SCANCODE_A));
	camAdd += newUp * real(input.isButtonHeld(SDL_SCANCODE_Z));
	camAdd -= newUp * real(input.isButtonHeld(SDL_SCANCODE_X));

	if (real len = camAdd.len() > 0)
	{
		camAdd /= len;
		this->camera.pos += camAdd * settings.flySpeed;

		for (int i = 0; i < 4; ++i) pointLights[i].pos += camAdd * settings.flySpeed;
	}
}

void MainGame::init()
{
	threadpool = initData.threadpool;
	wnd = initData.wnd;
	wndSurf = SDL_GetWindowSurface(wnd);
	int w = wndSurf->w;
	int h = wndSurf->h;

	camPosAndAngArchieve = {
	   { 0,0,0 }, {0,0,0},
	   { 0.1,32.1,370 }, {0,0,0}, //default view
	   {-96, 70, 784}, {0,0,0}, //doom 2 map 01 player start
	};

	this->camera.pos = camPosAndAngArchieve[activeCamPosAndAngle * 2];
	this->camera.angle = camPosAndAngArchieve[activeCamPosAndAngle * 2 + 1];

	settings.ssaaMult = 1;

	settings.textureManager = &textureManager;
	sky = { "RSKY1", textureManager };

	skyTextures = { "RSKY2", "RSKY3", "RSKY1" };
	{
		std::ifstream f("data/sky_textures.txt");
		std::string line;
		while (std::getline(f, line))
		{
			skyTextures.push_back(line);
		}
	}

	maps = WadLoader::loadWad("doom2.wad"); //can't redistribute commercial wads!

	pointLights = {
		{this->camera.pos, Vec4(1,0.7,0.4,1), 2e4},
		{this->camera.pos + Vec4(-200, 100, 200), Vec4(1,0,0.4,1), 2e4},
		{this->camera.pos + Vec4(200, 100, 200), Vec4(0,0,1,1), 2e4},
		{this->camera.pos + Vec4(100, 50, 100), Vec4(0,1,0,1), 2e4},

		//{Vec4(0,300,0), Vec4(1,1,1,1), 1e5},
		//{Vec4(500,300,0), Vec4(0.5,0.7,1,1), 2e5},
		//{Vec4(-500,300,0), Vec4(0.1,0.5,1,1), 2e5},
	};

	this->renderer = std::make_unique<RasterizationRenderer>(w, h, *threadpool);
}

std::string vecToStr(const Vec4& v)
{
	return std::to_string(v.x) + " " + std::to_string(v.y) + " " + std::to_string(v.z);
}

std::string boolToStr(bool b)
{
	return b ? "enabled" : "disabled";
}

void MainGame::draw()
{
	std::vector<const Model*> modelPtrs;
	int skyTextureMarker = textureManager.getTextureIndexByName("F_SKY1");
	for (const auto& it : sceneModels) if (it.textureIndex != skyTextureMarker) modelPtrs.push_back(&it);
	if (settings.skyRenderingMode == SkyRenderingMode::SPHERE) modelPtrs.push_back(&sky.getModel());

	threadpool->waitUntilTaskCompletes(windowUpdateTaskId);
	renderer->drawScene(modelPtrs, wndSurf, settings, camera);

	windowUpdateTaskId = threadpool->addTask([&, this]() {
		performanceMonitor.registerFrameDone();
		if (settings.performanceMonitorDisplayEnabled || performanceMonitor.getFrameNumber() % 1024 == 0)
		{
			std::vector<std::pair<std::string, std::string>> perfmonInfo = {
				{"Cam pos", vecToStr(this->camera.pos)},
				{"Cam ang", vecToStr(this->camera.angle)},
				{"Fly speed", std::to_string(settings.flySpeed) + "/frame"},
				{"Backface culling", settings.backfaceCullingEnabled ? "enabled" : "disabled"},
				{"Buffer cleaning", settings.bufferCleaningEnabled ? "enabled" : "disabled"},
				{"FOV", std::to_string(2 * atan(1 / settings.fovMult) * 180 / M_PI) + " degrees"},
				{"Fog", !settings.fogEnabled ? "disabled" : ("version " + std::to_string(int(settings.fogEffectVersion)) + ", intensity " + std::to_string(settings.fogIntensity))},
				{"Dithering", settings.ditheringEnabled ? "enabled" : "disabled"},
				{"Gamma", std::to_string(settings.gamma)},
				{"Output resolution", std::to_string(wndSurf->w) + "x" + std::to_string(wndSurf->h)},

				//{"Transformation matrix", "\n" + ctr.getCurrentTransformationMatrix().toString()},
				//{"Inverse transformation matrix", "\n" + ctr.getCurrentInverseTransformationMatrix().toString()},
			};

			auto additionalOSDinfo = renderer->getAdditionalOSDInfo();
			for (auto& it : additionalOSDinfo) perfmonInfo.push_back(it);

			if (settings.performanceMonitorDisplayEnabled) performanceMonitor.drawOn(wndSurf, { 0,0 }, perfmonInfo);
			else std::cout << performanceMonitor.composeString(perfmonInfo) << "\n";
		}
		if (SDL_UpdateWindowSurface(initData.wnd)) throw std::runtime_error(SDL_GetError());
	});
}

void MainGame::endFrame()
{
}

void MainGame::update()
{
}

#ifdef NDEBUG
bool debug = false;
#else
bool debug = true;
#endif


void MainGame::changeMapTo(std::string mapName)
{
	if (mapName != "MAP00")
	{
		currentMap = &maps.at(mapName);
		auto sectorWorldModels = currentMap->getMapGeometryModels(textureManager);
		sceneModels.clear();

		for (int nSector = 0; nSector < sectorWorldModels.size(); ++nSector)
		{
			for (auto& model : sectorWorldModels[nSector])
			{
				model.lightMult = currentMap->sectors[nSector].lightLevel / 256.0;
				sceneModels.push_back(model);
			}
		}
	}
	else
	{
		currentMap = nullptr;
		AssetLoader loader;
		//GLTF and FBX load fine
		//sceneModels = loader.loadObj("scenes/Sponza/sponza.obj", textureManager, "H:/Sponza goodies/old_sponza/old_sponza.bmdl");
		//loader.loadObj("H:/Sponza goodies/pkg_c1_trees/NewSponza_CypressTree_FBX_YUp.fbx", textureManager, "H:/Sponza goodies/tree.bmdl");
		//loader.loadObj("H:/Sponza goodies/pkg_b_ivy/NewSponza_IvyGrowth_FBX_YUp.fbx", textureManager, "H:/Sponza goodies/pkg_b_ivy/ivy.bmdl");
		//loader.loadObj("H:/Sponza goodies/pkg_a_curtains/NewSponza_Curtains_FBX_YUp.fbx", textureManager, "H:/Sponza goodies/curtains.bmdl");
		//loader.loadObj("H:/Sponza goodies/main1_sponza/NewSponza_Main_Yup_003.fbx", textureManager, "H:/Sponza goodies/new_sponza.bmdl");
		//sceneModels = AssetLoader::loadObj("H:/Sponza goodies/pkg_a_curtains/NewSponza_Curtains_FBX_YUp.fbx", textureManager);
		//sceneModels = AssetLoader::loadObj("H:/Sponza goodies/main1_sponza/NewSponza_Main_Yup_003.fbx", textureManager);
		//return;
		std::string paths[] = {
			"H:/Sponza goodies/main1_sponza/new_sponza.bmdl",
			"H:/Sponza goodies/pkg_a_curtains/curtains.bmdl",
			"H:/Sponza goodies/pkg_c1_trees/tree.bmdl",	
			"H:/Sponza goodies/pkg_b_ivy/ivy.bmdl",
		};

		size_t modelsToLoad = std::size(paths);
		std::vector<task_id> loaderTasks;
		std::mutex mtx;

		for (int i = 0; i < modelsToLoad; ++i)
		{
			taskfunc_t f = [&, i]() {
				auto returnedModels = AssetLoader::loadBmdl(paths[i], textureManager);
				std::lock_guard lck(mtx);
				this->sceneModels.insert(this->sceneModels.end(), returnedModels.begin(), returnedModels.end());
			};

			loaderTasks.push_back(this->threadpool->addTask(f));
		}
		this->threadpool->waitForMultipleTasks(loaderTasks);
		this->camera.pos = Vec4(-1305.55, 175.75, 67.645);
		this->camera.angle = Vec4(0, -1.444047, -0.125);
	}

	size_t triangleCount = 0;
	for (auto& it : sceneModels) triangleCount += it.getTriangleCount();
	std::cout << "Total triangles loaded: " << triangleCount << "\n";

	auto *r = dynamic_cast<RasterizationRenderer*>(this->renderer.get());
	if (r)
	{
		r->removeShadowMaps();
		int shadowMapW = debug ? 192 : 19200;
		int shadowMapH = debug ? 108 : 10800;
		//Camera shadowMapPov = { .pos = Vec4(-1846, 2799, 568), .angle = Vec4(0, -1.2869, -0.6689) };
		//Camera shadowMapPov = { .pos = Vec4(-288.22, 2493.0354, -519.728333), .angle = Vec4(0, 3.685142, -1.213774) };
		Camera shadowMapPov = { .pos = Vec4(843.313965, 3009.328857, -55.578117), .angle = Vec4(0, -4.721983, -1.09903) };
		this->shadowMaps = { ShadowMap(shadowMapW,shadowMapH,shadowMapPov) };

		for (auto& it : shadowMaps)
		{
			it.render(sceneModels, this->settings, *threadpool);
			r->addShadowMap(it);
		}		
	}
	performanceMonitor.reset();
}

void MainGame::adjustSsaaMult(int newMult)
{
	int w = wndSurf->w * newMult;
	int h = wndSurf->h * newMult;
	settings.ssaaMult = newMult;
	this->renderer = std::make_unique<RasterizationRenderer>(w, h, *threadpool);
	for (auto& it : shadowMaps) dynamic_cast<RasterizationRenderer*>(this->renderer.get())->addShadowMap(it);
}