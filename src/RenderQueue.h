#pragma once
#include <vector>
#include "Threadpool.h"
#include "Triangle.h"

struct DrawInfo
{
	//Triangle flatTop, flatBottom;
	int textureIndex;
	real lightMult;
};

struct RenderJob //RenderJob is a struct that holds stuff needed for a specific triangle's render
{
	Triangle t;
	DrawInfo info;
};



class RenderQueue
{
public:
	RenderQueue(Threadpool& pool);
	void clear();
	void addInitialJob(const Triangle& t, int textureIndex, real lightMult);

	void drawOn(TriangleRenderContext ctx);
private:
	Threadpool& threadpool;
	std::vector<RenderJob> initialJobs, processedJobs;
	//std::vector<std::vector<DrawJob>> drawJobs; //each thread gets it's own storage for drawJobs it produces

	void doTransformations(TriangleRenderContext& ctx);
	void doRotationAndClipping(TriangleRenderContext& ctx);
	void doScreenSpaceTransformAndDraw(TriangleRenderContext& ctx);
};