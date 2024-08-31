#pragma once
#include "../Model.h"

struct GameSettings;
struct Camera;

class RendererBase
{
public:
	virtual void drawScene(const std::vector<const Model*>& models, SDL_Surface* dstSurf, const GameSettings& gameSettings, const Camera& pov) = 0;
	virtual std::vector<std::pair<std::string, std::string>> getAdditionalOSDInfo();
	//virtual size_t getInternalRenderWidth() = 0;
	//virtual size_t getInternalRenderHeight() = 0;
};