#pragma once
#include <vector>
#include "VectorPack.h"
#include "Color.h"

class FloatColorBuffer
{
public:
	FloatColorBuffer() = default;
	FloatColorBuffer(int w, int h);

	VectorPack8 gatherPixels8(const __m256i& xCoords, const __m256i& yCoords, const __mmask8& mask) const;
	VectorPack16 gatherPixels16(const __m512i& xCoords, const __m512i& yCoords, const __mmask16& mask) const;

	void setPixel(int x, int y, Color color);

	int getW() const;
	int getH() const;
private:
	std::vector<float> r, g, b, a;
	int w, h;
};