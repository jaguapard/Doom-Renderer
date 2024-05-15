#include "MainGame.h"
#include <fstream>
#include <iostream>

MainGame::MainGame(GameStateInitData data, Threadpool& threadpool) : threadpool(threadpool)
{
	initData = data;
	init();
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
