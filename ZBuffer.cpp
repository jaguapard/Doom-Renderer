#include "ZBuffer.h"
#include "Statsman.h"

ZBuffer::ZBuffer(int w, int h) : PixelBuffer<double>(w, h)
{
}

bool ZBuffer::testAndSet(int x, int y, double depth)
{
	StatCount(statsman.zBuffer.depthTests++);
	bool cmp = depth < this->getPixelUnsafe(x, y);
	if (cmp)
	{
		StatCount(statsman.zBuffer.writes++);
		this->setPixelUnsafe(x, y, depth);
	}
	else
	{
		StatCount(statsman.zBuffer.occlusionDiscards++);
	}
	return cmp;
}