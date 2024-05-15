#pragma once

#include "MainGame.h"

class BenchmarkState : public MainGame
{
public:
	BenchmarkState(GameStateInitData data, Threadpool* threadpool);
	virtual void handleInputEvent(SDL_Event& ev);
	virtual void postEventPollingRoutine();
	virtual void update();

protected:
	int benchmarkModeFrames = -1;
	int benchmarkModeFramesRemaining;
	std::string benchmarkPassName;
	bob::Timer benchmarkTimer;
};