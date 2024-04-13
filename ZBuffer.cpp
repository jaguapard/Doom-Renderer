#include "ZBuffer.h"

ZBuffer::ZBuffer(int w, int h) : PixelBuffer<double>(w, h)
{
}

bool ZBuffer::testAndSet(int x, int y, double depth)
{
	if (depth > 0) return false; //disallow stuff from behind the camera to show up, kinda?
	if (isOutOfBounds(x, y)) return false;
	bool cmp = depth < this->getPixelUnsafe(x, y);
	if (cmp) this->setPixelUnsafe(x, y, depth);
	return cmp;
}