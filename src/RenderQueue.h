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
	bool shouldDraw = true;
};



class RenderQueue
{
public:
	RenderQueue(Threadpool& pool);
	void clear();
	void addInitialJob(const Triangle& t, int textureIndex, real lightMult);

	std::vector<Threadpool::task_id> drawOn(TriangleRenderContext ctx, std::vector<Threadpool::task_id> dependencies);
private:
	Threadpool& threadpool;
	std::vector<std::vector<RenderJob>> threadJobs;
	size_t jobsInQueue = 0;
	//std::vector<std::vector<DrawJob>> drawJobs; //each thread gets it's own storage for drawJobs it produces

	void doRotationAndClipping(TriangleRenderContext& ctx, size_t threadIndex);
	void doScreenSpaceTransform(TriangleRenderContext& ctx, size_t threadIndex);
	void doDraw(TriangleRenderContext& ctx, size_t threadIndex);
	void doScreenBlitting(TriangleRenderContext& ctx, size_t threadIndex);
};