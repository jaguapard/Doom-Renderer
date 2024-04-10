#include "ZBuffer.h"

ZBuffer::ZBuffer(int w, int h) : w(w), h(h)
{
	values.resize(w * h);
}

bool ZBuffer::testAndSet(int x, int y, double depth)
{
	bool cmp = depth < values[y * w + h];
	if (cmp) values[y * w + h] = depth;
	return cmp;
}

void ZBuffer::clear()
{
	for (auto& it : values) it = std::numeric_limits<double>::infinity();
}
