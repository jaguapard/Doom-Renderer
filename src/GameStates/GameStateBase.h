#pragma once
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <memory>
#include <optional>

#include "../C_Input.h"

class Threadpool;
class GameStateBase;

struct GameStateInitData
{
	int argc;
	char** argv;

	SDL_Window* wnd;
	Threadpool* threadpool;	
};

struct GameStateSwitch
{
	std::shared_ptr<GameStateBase> nextState = nullptr;
};

class GameStateBase
{
public:
	virtual GameStateSwitch run();
	virtual void beginNewFrame() = 0;
	virtual void handleInput() = 0;
	//virtual void handleInputEvent(SDL_Event& ev) = 0;
	//virtual void postEventPollingRoutine() = 0;

	virtual void update() = 0;
	virtual void draw() = 0;
	virtual void endFrame() = 0;
protected:
	C_Input input;
};