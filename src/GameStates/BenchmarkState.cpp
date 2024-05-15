#include "BenchmarkState.h"
#include <iostream>
#include <fstream>

BenchmarkState::BenchmarkState(GameStateInitData data, Threadpool* threadpool)
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
	camPos = { 441, 877, -488 };
	camAng = { 0,0,-0.43 };

	performanceMonitorDisplayEnabled = false;
	wireframeEnabled = false;
	mouseCaptured = false;
	fogEnabled = false;
	skyRenderingMode = SPHERE;

	this->changeMapTo(defaultMap);
	performanceMonitor.reset();
	benchmarkTimer.restart();
}

void BenchmarkState::handleInputEvent(SDL_Event& ev)
{
}

void BenchmarkState::postEventPollingRoutine()
{
}

void BenchmarkState::update()
{
	camAng.y = (1 - double(benchmarkModeFrames - benchmarkModeFramesRemaining) / benchmarkModeFrames) * 2 * M_PI;
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
	}
}
