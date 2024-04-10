#include "ZBuffer.h"

ZBuffer::ZBuffer(int w, int h) : w(w), h(h)
{
	values.resize(w * h);
}

bool ZBuffer::testAndSet(int x, int y, double depth)
{
	if (x < 0 || y < 0 || x >= w || y >= h) return false;
	bool cmp = depth < values[y * w + h];
	if (cmp) values[y * w + h] = depth;
	return cmp;
}

void ZBuffer::clear()
{
	for (auto& it : values) it = std::numeric_limits<double>::infinity();
}
