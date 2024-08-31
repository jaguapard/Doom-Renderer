#pragma once
#include "../Model.h"

struct GameSettings;

class RendererBase
{
public:
	virtual void drawScene(const std::vector<Model>& models, SDL_Surface* dstSurf, const GameSettings& gameSettings) = 0;
};