#include "FloatColorBuffer.h"

FloatColorBuffer::FloatColorBuffer(int w, int h) :w(w), h(h)
{
	int pixelCount = w * h;
	r.resize(w * h);
	b.resize(w * h);
	g.resize(w * h);
	a.resize(w * h);
}

VectorPack8 FloatColorBuffer::gatherPixels8(const __m256i& xCoords, const __m256i& yCoords, const __mmask8& mask) const
{
	VectorPack8 ret;
	__m256i rowStart = _mm256_mullo_epi32(yCoords, _mm256_set1_epi32(w));
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
	__m512i rowStart = _mm512_mullo_epi32(yCoords, _mm512_set1_epi32(w));
	__m512i pixelIndices = _mm512_add_epi32(rowStart, xCoords);

	ret.x = _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask, pixelIndices, r.data(), sizeof(float));
	ret.y = _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask, pixelIndices, g.data(), sizeof(float));
	ret.z = _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask, pixelIndices, b.data(), sizeof(float));
	ret.w = _mm512_mask_i32gather_ps(_mm512_setzero_ps(), mask, pixelIndices, a.data(), sizeof(float));
	return ret;
}

void FloatColorBuffer::setPixel(int x, int y, Color color)
{
	r[y * w + x] = color.r;
	g[y * w + x] = color.g;
	b[y * w + x] = color.b;
	a[y * w + x] = color.a;
}

int FloatColorBuffer::getW() const
{
	return w;
}

int FloatColorBuffer::getH() const
{
	return h;
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
