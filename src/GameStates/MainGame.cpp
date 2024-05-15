#include "MainGame.h"
#include <fstream>
#include <iostream>

MainGame::MainGame(GameStateInitData data, Threadpool& threadpool) : threadpool(threadpool)
{
	initData = data;
	init();
}

void MainGame::beginNewFrame()
{
	input.beginNewFrame();
}

void MainGame::init()
{
	camPosAndAngArchieve = {
	   { 0,0,0 }, {0,0,0},
	   { 0.1,32.1,370 }, {0,0,0}, //default view
	   {-96, 70, 784}, {0,0,0}, //doom 2 map 01 player start
	};

	camPos = camPosAndAngArchieve[activeCamPosAndAngle * 2];
	camAng = camPosAndAngArchieve[activeCamPosAndAngle * 2 + 1];

	framebufW = initData.wnd->surf->w;
	framebufH = initData.wnd->surf->h;

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
	loadMap("MAP01");
	performanceMonitor.reset();
}

void MainGame::loadMap(std::string mapName)
{
	currentMap = &maps["MAP01"];
	sectorWorldModels = currentMap->getMapGeometryModels(textureManager);
	performanceMonitor.reset();
}

std::vector<RenderJob> MainGame::makeRenderJobsList(TriangleRenderContext ctx)
{
	std::vector<RenderJob> renderJobs;

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

	return renderJobs;
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
	ctx.framebufW = framebufW;
	ctx.framebufH = framebufH;
	ctx.doomSkyTextureMarkerIndex = textureManager.getTextureIndexByName("F_SKY1"); //Doom uses F_SKY1 to mark sky. Any models with this texture will exit their rendering immediately
	ctx.wireframeEnabled = wireframeEnabled;
	ctx.backfaceCullingEnabled = backfaceCullingEnabled;
	ctx.nearPlaneClippingZ = nearPlaneZ;
	ctx.fovMult = fovMult;

	return ctx;
}
