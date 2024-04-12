#include "ZBuffer.h"

ZBuffer::ZBuffer(int w, int h) : w(w), h(h)
{
	values.resize(w * h);
}

bool ZBuffer::testAndSet(int x, int y, double depth)
{
	if (depth > 0) return false; //disallow stuff from behind the camera to show up, kinda?
	if (x < 0 || y < 0 || x >= w || y >= h) return false;
	bool cmp = depth < values[y * w + x];
	if (cmp) values[y * w + x] = depth;
	return cmp;
}

void ZBuffer::clear()
{
	for (auto& it : values) it = std::numeric_limits<double>::infinity();
}