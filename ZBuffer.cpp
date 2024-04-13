#include "ZBuffer.h"
#include "Statsman.h"

ZBuffer::ZBuffer(int w, int h) : PixelBuffer<double>(w, h)
{
}

bool ZBuffer::testAndSet(int x, int y, double depth)
{
	StatCount(statsman.zBuffer.depthTests++);
	if (depth > 0)
	{
		StatCount(statsman.zBuffer.negativeDepthDiscards++);
		return false; //disallow stuff from behind the camera to show up, kinda?
	}
	if (isOutOfBounds(x, y))
	{
		StatCount(statsman.zBuffer.outOfBoundsAccesses++);
		return false;
	}
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