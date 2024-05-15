#pragma once
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

struct GameStateInitData
{
	SDL_Window* wnd;
	int argc;
	char** argv;
};

class GameStateBase
{
public:
	virtual void beginNewFrame() = 0;
	virtual void handleInputEvent(SDL_Event& ev) = 0;
	virtual void postEventPollingRoutine() = 0;

	virtual void update() = 0;
	virtual void draw() = 0;
	virtual void endFrame() = 0;
protected:
};