#pragma once

#include <cstdint>
#include <string>
#include "helpers.h"

#define VAR_PRINT(x) (std::string(#x) + ": " + toThousandsSeparatedString(x))

#pragma pack(push, 1)
class Statsman
{
public:
	static constexpr bool enabled = false;

	struct ZBuffer
	{
		uint64_t
			depthTests = 0,
			negativeDepthDiscards = 0,
			outOfBoundsAccesses = 0,
			occlusionDiscards = 0,
			writes = 0,
			writeDisabledTests = 0;
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
			pixelFetches = 0;
	};

	struct Pixels
	{
		uint64_t
			nonOpaqueDraws = 0;
	};

	struct Memory
	{
		uint64_t
			allocsByNew = 0,
			freesByDelete = 0;
	};

	struct Models
	{
		uint64_t boundingBoxDiscards = 0;
	};

	ZBuffer zBuffer;
	Triangles triangles;
	Textures textures;
	Pixels pixels;
	Memory memory;
	Models models;

	Statsman operator-(const Statsman& other) const;

	std::string toString();
private:
};
#pragma pack(pop)

extern Statsman statsman;

#define StatCount(expression) if (Statsman::enabled) {expression;}
//#define StatCount(expression, ) if (Statsman::enabled) {expression;}