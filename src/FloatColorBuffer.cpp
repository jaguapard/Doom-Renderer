#include "FloatColorBuffer.h"

FloatColorBuffer::FloatColorBuffer(int w, int h)
{
	int pixelCount = w * h;
	r.resize(w * h);
	b.resize(w * h);
	g.resize(w * h);
	a.resize(w * h);
	size = FloatColorBufferSize(w, h);
}

VectorPack8 FloatColorBuffer::gatherPixels8(const __m256i& xCoords, const __m256i& yCoords, const __mmask8& mask) const
{
	VectorPack8 ret;
	__m256i rowStart = _mm256_mullo_epi32(yCoords, _mm256_set1_epi32(size.w));
	__m256i pixelIndices = _mm256_add_epi32(rowStart, xCoords);

	ret.x = _mm256_mmask_i32gather_ps(_mm256_setzero_ps(), mask, pixelIndices, r.data(), sizeof(float));
	ret.y = _mm256_mmask_i32gather_ps(_mm256_setzero_ps(), mask, pixelIndices, g.data(), sizeof(float));
	ret.z = _mm256_mmask_i32gather_ps(_mm256_setzero_ps(), mask, pixelIndices, b.data(), sizeof(float));
	ret.w = _mm256_mmask_i32gather_ps(_mm256_setzero_ps(), mask, pixelIndices, a.data(), sizeof(float));
	return ret;
}

VectorPack16 FloatColorBuffer::gatherPixels16(const __m512i& xCoords, const __m512i& yCoords, const __mmask16& mask) const
{
	VectorPack16 ret;
	__m512i rowStart = _mm512_mullo_epi32(yCoords, _mm512_set1_epi32(size.w));
	__m512i pixelIndices = _mm512_add_epi32(rowStart, xCoords);

	ret.x = _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask, pixelIndices, r.data(), sizeof(float));
	ret.y = _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask, pixelIndices, g.data(), sizeof(float));
	ret.z = _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask, pixelIndices, b.data(), sizeof(float));
	ret.w = _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask, pixelIndices, a.data(), sizeof(float));
	return ret;
}

void FloatColorBuffer::scatterPixels16(const __m512i& xCoords, const __m512i& yCoords, const __mmask16& mask, const VectorPack16& pixels)
{
	__m512i rowStart = _mm512_mullo_epi32(yCoords, _mm512_set1_epi32(size.w));
	__m512i pixelIndices = _mm512_add_epi32(rowStart, xCoords);

	_mm512_mask_i32scatter_ps(r.data(), mask, pixelIndices, pixels.r, sizeof(float));
	_mm512_mask_i32scatter_ps(g.data(), mask, pixelIndices, pixels.g, sizeof(float));
	_mm512_mask_i32scatter_ps(b.data(), mask, pixelIndices, pixels.b, sizeof(float));
	_mm512_mask_i32scatter_ps(a.data(), mask, pixelIndices, pixels.a, sizeof(float));
}

VectorPack16 FloatColorBuffer::getPixelLine16(int xStart, int y) const
{
	VectorPack16 ret;
	size_t index = y * size.w + xStart;
	ret.r = &r[index];
	ret.g = &g[index];
	ret.b = &b[index];
	ret.a = &a[index];
	return ret;
}

VectorPack16 FloatColorBuffer::getPixelsStartingFrom16(size_t index) const
{
	VectorPack16 ret;
	ret.r = &r[index];
	ret.g = &g[index];
	ret.b = &b[index];
	ret.a = &a[index];
	return ret;
}

void FloatColorBuffer::storePixels16(int pixelIndex, const VectorPack16& pixels, __mmask16 mask)
{
	_mm512_mask_store_ps(&r[pixelIndex], mask, pixels.r);
	_mm512_mask_store_ps(&g[pixelIndex], mask, pixels.g);
	_mm512_mask_store_ps(&b[pixelIndex], mask, pixels.b);
	_mm512_mask_store_ps(&a[pixelIndex], mask, pixels.a);
}

void FloatColorBuffer::setPixel(int x, int y, Color color)
{
	size_t ind = y * size.w + x;
	r[ind] = (float(color.r) + 1) / 256.0f; //avoid 0.0 in color channels
	g[ind] = (float(color.g) + 1) / 256.0f;
	b[ind] = (float(color.b) + 1) / 256.0f;
	a[ind] = color.a / 255.0f;
}

int FloatColorBuffer::getW() const
{
	return size.w;
}

int FloatColorBuffer::getH() const
{
	return size.h;
}

float* FloatColorBuffer::getp_R()
{
	return r.data();
}

float* FloatColorBuffer::getp_G()
{
	return g.data();
}

float* FloatColorBuffer::getp_B()
{
	return b.data();
}

float* FloatColorBuffer::getp_A()
{
	return a.data();
}

const FloatColorBufferSize& FloatColorBuffer::getSize() const
{
	return size;
}

VectorPack16 FloatColorBuffer::gatherPixels16(const __m512i &indices, const __mmask16 &mask) const
{
    VectorPack16 ret;
    ret.x = _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask, indices, r.data(), sizeof(float));
    ret.y = _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask, indices, g.data(), sizeof(float));
    ret.z = _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask, indices, b.data(), sizeof(float));
    ret.w = _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask, indices, a.data(), sizeof(float));
    return ret;
}
