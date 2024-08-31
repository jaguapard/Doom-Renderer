#include "RendererBase.h"
#include "../ShadowMap.h"
#include "../Lehmer.h"

class Threadpool;

struct BoundingBox
{
	real minX, minY, maxX, maxY;
};


class ShadowMap;

class RasterizationRenderer : public RendererBase
{
public:
	RasterizationRenderer(int w, int h, Threadpool& threadpool, bool depthOnly = false);
	virtual void drawScene(const std::vector<const Model*>& models, SDL_Surface* dstSurf, const GameSettings& gameSettings, const Camera& pov);
	virtual void drawScene(const std::vector<const Model*>& models, SDL_Surface* dstSurf, const GameSettings& gameSettings, const Camera& pov, bool depthOnly);
	virtual std::vector<std::pair<std::string, std::string>> getAdditionalOSDInfo();
	virtual void saveBuffers();

	const ZBuffer& getDepthBuffer() const;
	void addShadowMap(const ShadowMap& m);
	void removeShadowMaps();
private:
	Threadpool* threadpool;

	ZBuffer zBuffer;
	FloatColorBuffer frameBuf;
	FloatColorBuffer pixelWorldPosBuf;

	GameSettings currFrameGameSettings;
	CoordinateTransformer ctr;

	std::vector<const ShadowMap*> shadowMaps;

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
	std::vector<std::vector<std::vector<size_t>>> filteredJobIndices;
	std::vector<LehmerRNG> rngSources;

	std::vector<ModelSlice> distributeTrianglesForWorkers(const std::vector<const Model*>& sceneModels, size_t threadCount);
	void addTriangleRangeToRenderQueue(const Triangle* pBegin, const Triangle* pEnd, const Model* pModel, size_t workerNumber);

	int doWorldTransformationsAndClipping(const Triangle& triangle, const Model& model, Triangle* trianglesOut) const;
	std::optional<Triangle> transformToScreenSpace(const Triangle& t) const;

	std::array<uint32_t, 4> getShiftsForSurface(const SDL_Surface* surf) const;

	BoundingBox clampBoundingBox(const BoundingBox& clampFrom, const BoundingBox& clampBy) const;
	void drawRenderJobSlice(const RenderJob& renderJob, const BoundingBox& threadBox, bool depthOnly = false);
};