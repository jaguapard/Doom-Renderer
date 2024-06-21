#include "MainGame.h"
#include <fstream>
#include <iostream>

#include "../blitting.h"

MainGame::MainGame(GameStateInitData data)
{
	initData = data;
	defaultMap = "MAP01";
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
			camAng.y += ev.motion.xrel * settings.camAngAdjustmentSpeed_Mouse;
			camAng.z -= ev.motion.yrel * settings.camAngAdjustmentSpeed_Mouse;
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
	if (input.wasCharPressedOnThisFrame('J')) settings.skyRenderingMode = static_cast<SkyRenderingMode>((settings.skyRenderingMode + 1) % (SkyRenderingMode::COUNT));
	if (input.wasCharPressedOnThisFrame('O')) settings.wireframeEnabled ^= 1;
	if (input.wasCharPressedOnThisFrame('C')) camPos = { -96, 70, 784 };
	if (input.wasCharPressedOnThisFrame('K')) settings.wheelAdjMod = static_cast<WheelAdjustmentMode>((uint32_t(settings.wheelAdjMod) + 1) % uint32_t(WheelAdjustmentMode::COUNT));
	if (input.wasCharPressedOnThisFrame('L')) settings.fovMult = 1;
	if (input.wasCharPressedOnThisFrame('V')) camAng = { 0,0,0 };
	if (input.wasCharPressedOnThisFrame('R')) settings.backfaceCullingEnabled ^= 1;

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
	settings.fogMaxIntensityDist += 10 * (input.isButtonHeld(SDL_SCANCODE_B) - input.isButtonHeld(SDL_SCANCODE_N));

	//camAng.z = fmod(camAng.z, M_PI);
	camAng.y = fmod(camAng.y, 2 * M_PI);
	camAng.z = std::clamp<real>(camAng.z, -M_PI / 2 + 0.01, M_PI / 2 - 0.01); //no real need for 0.01, but who knows
	Matrix4 rotationOnlyMatrix = Matrix4::rotationXYZ(camAng);

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
		camPos += camAdd * settings.flySpeed;
	}
}

void MainGame::init()
{
	threadpool = initData.threadpool;
	wnd = initData.wnd;
	wndSurf = SDL_GetWindowSurface(wnd);
	shifts = getShiftsForWindow();

	camPosAndAngArchieve = {
	   { 0,0,0 }, {0,0,0},
	   { 0.1,32.1,370 }, {0,0,0}, //default view
	   {-96, 70, 784}, {0,0,0}, //doom 2 map 01 player start
	};

	camPos = camPosAndAngArchieve[activeCamPosAndAngle * 2];
	camAng = camPosAndAngArchieve[activeCamPosAndAngle * 2 + 1];

	int framebufW = wndSurf->w;
	int framebufH = wndSurf->h;

	framebuf = { framebufW, framebufH };
	lightBuf = { framebufW, framebufH };
	zBuffer = { framebufW, framebufH };


	this->ctr = { framebufW, framebufH };
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
	int threadCount = threadpool->getThreadCount();
	
	TriangleRenderContext ctx = makeTriangleRenderContext();
	fillRenderJobsList(ctx, renderJobs);

	real* lightStart = lightBuf.begin();
	Color* framebufStart = framebuf.begin();
	Uint32* wndSurfStart = reinterpret_cast<Uint32*>(wndSurf->pixels);

	std::vector<ThreadpoolTask> taskBatch;

	for (int tNum = 0; tNum < threadCount; ++tNum)
	{
		//is is crucial to capture some stuff by value [=], else function risks getting garbage values when the task starts. 
		//It is, however, assumed that renderJobs vector remains in a valid state until all tasks are completed.
		taskfunc_t f = [=]() {
			auto lim = threadpool->getLimitsForThread(tNum, 0, ctx.framebufH);
			int myMinY = lim.first; //truncate limits to avoid fighting
			int myMaxY = lim.second;			
	
			ctx.zBuffer->clearRows(myMinY, myMaxY); //Z buffer has to be cleared, else only pixels closer than previous frame will draw
			if (settings.bufferCleaningEnabled)
			{
				ctx.frameBuffer->clearRows(myMinY, myMaxY);
				ctx.lightBuffer->clearRows(myMinY, myMaxY, 1);
			}

			for (int i = 0; i < renderJobs.size(); ++i)
			{
				const RenderJob& myJob = renderJobs[i];
				myJob.t.drawSlice(ctx, myJob, myMinY, myMaxY);
			}

			blitting::lightIntoFrameBuffer(*ctx.frameBuffer, *ctx.lightBuffer, myMinY, myMaxY);
			threadpool->waitUntilTaskCompletes(windowUpdateTaskId);
			blitting::frameBufferIntoSurface(*ctx.frameBuffer, wndSurf, myMinY, myMaxY, shifts);
		};

		ThreadpoolTask task;
		task.func = f;
		taskBatch.emplace_back(task);
	}

	std::vector<task_id> renderTaskIds = threadpool->addTaskBatch(taskBatch);
	threadpool->waitForMultipleTasks(renderTaskIds);
	renderJobs.clear();

	windowUpdateTaskId = threadpool->addTask([&, this]() {
		performanceMonitor.registerFrameDone();
		if (settings.performanceMonitorDisplayEnabled || performanceMonitor.getFrameNumber() % 1024 == 0)
		{
			std::map<std::string, std::string> perfmonInfo;
			perfmonInfo["Cam pos"] = vecToStr(camPos);
			perfmonInfo["Cam ang"] = vecToStr(camAng);
			perfmonInfo["Fly speed"] = std::to_string(settings.flySpeed) + "/frame";
			perfmonInfo["Backface culling"] = settings.backfaceCullingEnabled ? "enabled" : "disabled";
			perfmonInfo["Buffer cleaning"] = settings.bufferCleaningEnabled ? "enabled" : "disabled";
			perfmonInfo["FOV"] = std::to_string(2 * atan(1 / settings.fovMult) * 180 / M_PI) + " degrees";

			perfmonInfo["Transformation matrix"] = "\n" + ctr.getCurrentTransformationMatrix().toString();
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

void MainGame::changeMapTo(std::string mapName)
{
	if (mapName != "MAP00")
	{
		currentMap = &maps.at(mapName);
		sectorWorldModels = currentMap->getMapGeometryModels(textureManager);
	}
	else
	{
		currentMap = nullptr;
		sectorWorldModels = {};
	}
	performanceMonitor.reset();
}

void MainGame::fillRenderJobsList(TriangleRenderContext ctx, std::vector<RenderJob>& renderJobs)
{
	ctx.renderJobs = &renderJobs;

	if (currentMap)
	{
		for (int nSector = 0; nSector < sectorWorldModels.size(); ++nSector)
		{
			for (const auto& model : sectorWorldModels[nSector])
			{
				ctx.lightMult = pow(currentMap->sectors[nSector].lightLevel / 256.0, settings.gamma);
				model.addToRenderQueue(ctx);
			}
		}
	}
	if (settings.skyRenderingMode == SPHERE) sky.addToRenderQueue(ctx); //a 3D sky can be drawn after everything else. In fact, it's better, since a large part of it may already be occluded.

	
}

TriangleRenderContext MainGame::makeTriangleRenderContext()
{
	TriangleRenderContext ctx;
	ctr.prepare(camPos, camAng);
	ctx.ctr = &ctr;
	ctx.frameBuffer = &framebuf;
	ctx.lightBuffer = &lightBuf;
	ctx.textureManager = &textureManager;
	ctx.zBuffer = &zBuffer;
	ctx.framebufW = framebuf.getW();
	ctx.framebufH = framebuf.getH();
	ctx.doomSkyTextureMarkerIndex = textureManager.getTextureIndexByName("F_SKY1"); //Doom uses F_SKY1 to mark sky. Any models with this texture will exit their rendering immediately
	ctx.wireframeEnabled = settings.wireframeEnabled;
	ctx.backfaceCullingEnabled = settings.backfaceCullingEnabled;
	ctx.nearPlaneClippingZ = settings.nearPlaneZ;
	ctx.fovMult = settings.fovMult;

	return ctx;
}

std::array<uint32_t, 4> MainGame::getShiftsForWindow()
{
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
	for (auto& it : shifts) assert(it % 8 == 0);
	return shifts;
}
