#include "RendererBase.h"

class Threadpool;

class RasterizationRenderer : public RendererBase
{
public:
	RasterizationRenderer(int w, int h, Threadpool& threadpool);
	virtual void drawScene(const std::vector<Model>& models, SDL_Surface* dstSurf, const GameSettings& gameSettings);
private:
	ZBuffer zBuffer;
	PixelBuffer<Color> frameBuf;
	FloatColorBuffer pixelWorldPosBuf;

	Threadpool& threadpool;

	struct RenderJob
	{

	};
};