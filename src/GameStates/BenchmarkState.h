#pragma once

#include "MainGame.h"

class BenchmarkState : public MainGame
{
public:
	BenchmarkState(GameStateInitData data);
	virtual void handleInput();
	virtual void update();

protected:
	int benchmarkModeFrames = -1;
	int benchmarkModeFramesRemaining;
	std::string benchmarkPassName;
	bob::Timer benchmarkTimer;
};