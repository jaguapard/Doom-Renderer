#include "RendererBase.h"

class Threadpool;

struct BoundingBox
{
	real minX, minY, maxX, maxY;
};


class ShadowMap;

class RasterizationRenderer : public RendererBase
{
public:
	RasterizationRenderer(int w, int h, Threadpool& threadpool);
	virtual void drawScene(const std::vector<const Model*>& models, SDL_Surface* dstSurf, const GameSettings& gameSettings, const Camera& pov);
	virtual std::vector<std::pair<std::string, std::string>> getAdditionalOSDInfo();
private:
	Threadpool* threadpool;

	ZBuffer zBuffer;
	FloatColorBuffer frameBuf;
	FloatColorBuffer pixelWorldPosBuf;

	GameSettings currFrameGameSettings;
	CoordinateTransformer ctr;

	struct RenderJob
	{
		Triangle transformedTriangle;

		const Model* pModel;
		real rcpSignedArea;

		BoundingBox boundingBox;

		RenderJob() {};
	};

	struct ModelSlice
	{
		const Triangle* pTrianglesBegin;
		const Triangle* pTrianglesEnd;
		const Model* pModel;
		int workerNumber = -1;
	};

	std::vector<std::vector<RenderJob>> renderJobs;

	std::vector<ModelSlice> distributeTrianglesForWorkers(const std::vector<const Model*>& sceneModels, size_t threadCount);
	void addTriangleRangeToRenderQueue(const Triangle* pBegin, const Triangle* pEnd, const Model* pModel, size_t workerNumber);

	int doWorldTransformationsAndClipping(const Triangle& triangle, const Model& model, Triangle* trianglesOut) const;
	std::optional<Triangle> transformToScreenSpace(const Triangle& t) const;

	std::array<uint32_t, 4> getShiftsForSurface(const SDL_Surface* surf) const;
};