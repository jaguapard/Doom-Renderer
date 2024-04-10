#pragma once
#include <vector>
class ZBuffer
{
public:
	ZBuffer(int w, int h);
	
	bool testAndSet(int x, int y, double depth);
	void clear();
private:
	std::vector<double> values;
	int w, h;
};