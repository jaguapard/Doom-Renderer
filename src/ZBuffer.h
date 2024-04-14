#pragma once
#include <vector>

#include "PixelBuffer.h"

class ZBuffer : public PixelBuffer<double>
{
public:
	ZBuffer(int w, int h);
	bool test(int x, int y, double depth);
	bool testAndSet(int x, int y, double depth);
};