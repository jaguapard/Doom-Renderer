#pragma once
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

struct GameStateInitData
{
	SDL_Window* wnd;
};

class GameStateBase
{
public:
	virtual void beginNewFrame();
	virtual void handleInputEvent(SDL_Event& ev);
	virtual void update();
	virtual void draw();
	virtual void endFrame();
protected:
};