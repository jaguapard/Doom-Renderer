#pragma once
#include <vector>
#include <memory>

#include "VectorPack.h"
#include "Color.h"
#include "Vec.h"

struct FloatColorBufferSize
{
	float fw, fh;
	int w, h;

	FloatColorBufferSize() = default;
	FloatColorBufferSize(int w, int h)
	{
		this->w = w;
		this->h = h;
		this->fw = w;
		this->fh = h;
	}
};
class FloatColorBuffer
{
public:
	FloatColorBuffer() = default;
	FloatColorBuffer(const FloatColorBuffer& other);
	FloatColorBuffer(int w, int h);

	void operator=(const FloatColorBuffer& other);
	VectorPack8 gatherPixels8(const __m256i& xCoords, const __m256i& yCoords, const __mmask8& mask) const;
	VectorPack16 gatherPixels16(const __m512i& xCoords, const __m512i& yCoords, const __mmask16& mask) const;
	VectorPack16 gatherPixels16(const __m512i& indices, const __mmask16& mask) const;

    void scatterPixels16(const __m512i &xCoords, const __m512i &yCoords, const __mmask16 &mask, const VectorPack16 &pixels);

	VectorPack16 getPixels16(int xStart, int y) const;
	VectorPack16 getPixels16(size_t index) const;

	void setPixels16(int xStart, int y, const VectorPack16& pixels, __mmask16 mask);
	void setPixels16(size_t pixelIndex, const VectorPack16& pixels, __mmask16 mask);

	void setPixel(int x, int y, Color color);

	Vec4 getPixelAsVec4(int x, int y) const;

	int getW() const;
	int getH() const;

	float* getp_R();
	float* getp_G();
	float* getp_B();
	float* getp_A();

	const FloatColorBufferSize& getSize() const;
private:
	FloatColorBufferSize size;
	std::unique_ptr<float[]> r, g, b, a;
};