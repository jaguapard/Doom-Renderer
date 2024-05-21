#include "ZBuffer.h"
#include "Statsman.h"
#include <iostream>

ZBuffer::ZBuffer(int w, int h) : PixelBuffer<real>(w, h)
{
}

bool ZBuffer::test(int x, int y, real depth)
{
	StatCount(statsman.zBuffer.depthTests++);
	bool cmp = depth < this->getPixel(x, y);
	return cmp;
}

bool ZBuffer::testAndSet(int x, int y, real depth, bool doWrite)
{
	bool cmp = test(x, y, depth);
	if (!doWrite) StatCount(statsman.zBuffer.writeDisabledTests++);
	if (cmp && doWrite)
	{
		StatCount(statsman.zBuffer.writes++);
		this->setPixel(x, y, depth);
	}
	else
	{
		StatCount(statsman.zBuffer.occlusionDiscards++);
	}
	return cmp;
}

Color ZBuffer::toColor(real value) const
{
	real dist = -1.0/value;
	real intensity = std::clamp<real>(dist, 0, 1800) / 1800;
	uint8_t fv = intensity * 255;
	return Color(fv,fv,fv);
}
