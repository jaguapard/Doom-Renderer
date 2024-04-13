#pragma once
#include <cstdint>
class Statsman
{
public:
	static constexpr bool enabled = true;

	struct ZBuffer
	{
		uint64_t
			depthTests = 0,
			negativeDepthDiscards = 0,
			outOfBoundsAccesses = 0,
			occlusionDiscards = 0,
			writes = 0;
	};
	struct Triangles
	{
		uint64_t
			tripleVerticeOutOfScreenDiscards = 0,
			doubleVertexOutOfScreenSplits = 0,
			singleVertexOutOfScreenSplits = 0,
			zeroVerticesOutsideDraws = 0;
	};
	struct Textures
	{
		uint64_t
			pixelFetches = 0,
			optimizedXreads = 0,
			optimizedYreads = 0;
	};

	ZBuffer zBuffer;
	Triangles triangles;
	Textures textures;
private:
};

extern Statsman statsman;

#define StatCount(expression) if (Statsman::enabled) {expression;}
//#define StatCount(expression, ) if (Statsman::enabled) {expression;}