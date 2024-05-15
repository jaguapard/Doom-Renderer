#pragma once
#include "GameStateBase.h"

class MainGame : public GameStateBase
{
	virtual void beginNewFrame();
	virtual void handleInputEvent(SDL_Event& ev);
	virtual void update();
	virtual void draw();
	virtual void endFrame();
};