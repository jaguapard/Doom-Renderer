#pragma once
#include <vector>

#include "PixelBuffer.h"
#include "real.h"

class ZBuffer : public PixelBuffer<real>
{
public:
	ZBuffer() = default;
	ZBuffer(int w, int h);
	bool test(int x, int y, real depth);
	bool testAndSet(int x, int y, real depth, bool doWrite = true); //the doWrite flag is for transparent pixels, they can still be occluded, but will not occlude others

	Color toColor(real value) const override;

	static FloatPack16 toRealDist(const FloatPack16& values); //the Z buffer doesn't store real dist, but it's warped form, this one can be used to get real distance
};