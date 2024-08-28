#pragma once
#include "ShaderBase.h"
#include "../Triangle.h"

struct MainFragmentRenderInput
{
	TriangleRenderContext ctx;
	const std::vector<RenderJob>* renderJobs;
	real zoneMinY, zoneMaxY;
	bool renderDepthTextureOnly = false;
};

struct RenderHelpers
{
	static std::tuple<FloatPack16, FloatPack16, FloatPack16> calculateBarycentricCoordinates(const VectorPack16& r, const Vec4& r1, const Vec4& r2, const Vec4& r3, const real& rcpSignedArea);
};
class MainFragmentRenderShader : public ShaderBase<MainFragmentRenderInput, void>
{
public:
	virtual void run(MainFragmentRenderInput& input);
protected:
	static std::optional<RenderJob::BoundingBox> getRenderJobSliceBoundingBox(const RenderJob& renderJob, const RenderJob::BoundingBox& threadBounds);
private:
	void drawRenderJobSlice(const TriangleRenderContext& context, const RenderJob& renderJob, const RenderJob::BoundingBox& boundingBoxOverride) const;
	void drawRenderJobSliceDepthOnly(const TriangleRenderContext& context, const RenderJob& renderJob, const RenderJob::BoundingBox& boundingBoxOverride) const;
};