#pragma once
#include "MainFragmentRenderShader.h"

class DepthTextureRenderShader : public MainFragmentRenderShader
{
public:
	virtual void run(MainFragmentRenderInput& input);
private:
	void drawRenderJobSlice(const TriangleRenderContext& context, const RenderJob& renderJob, real zoneMinY, real zoneMaxY) const;
};