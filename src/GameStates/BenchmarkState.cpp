#include "BenchmarkState.h"
#include <iostream>
#include <fstream>
#include <sstream>

BenchmarkState::BenchmarkState(GameStateInitData data)
{
	this->threadpool = threadpool;
	initData = data;
	defaultMap = "MAP15";
	init();

	benchmarkModeFrames = data.argc > 2 ? atoi(data.argv[2]) : -1;
	if (data.argc > 3) benchmarkPassName = data.argv[3];

	if (benchmarkModeFrames < 1)
	{
		std::cout << "Benchmark mode frame count not set. Defaulting to 1000.\n";
		benchmarkModeFrames = 1000;
	}
	std::cout << "Benchmark mode selected. Running...\n";

	//override some settings from the MainGame state init
	benchmarkModeFramesRemaining = benchmarkModeFrames;
	this->camera.pos = { 441, 877, -488 };
	this->camera.angle = { 0,0,-0.43 };

	settings.performanceMonitorDisplayEnabled = false;
	settings.wireframeEnabled = false;
	settings.mouseCaptured = false;
	settings.fogEnabled = false;
	settings.skyRenderingMode = SkyRenderingMode::SPHERE;

	this->changeMapTo(defaultMap);
	performanceMonitor.reset();
	benchmarkTimer.restart();
}

void BenchmarkState::handleInput() //benchmark mode doesn't care about input
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {}; //avoid window being marked "not responding"
}


void BenchmarkState::update()
{
	this->camera.angle.y = (1 - double(benchmarkModeFrames - benchmarkModeFramesRemaining) / benchmarkModeFrames) * 2 * M_PI;
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
		throw GameStateSwitch();
	}
}
