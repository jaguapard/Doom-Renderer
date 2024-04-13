#include "Color.h"
#include <cassert>

Color::Color(uint32_t u)
{
	r = ((u & 0xFF00) >> 8) / 256.0;
	g = ((u & 0xFF0000) >> 16) / 256.0;
	b = ((u & 0xFF000000) >> 24) / 256.0;
	a = (u & 0xFF) / 256;
}

Color::Color(double r, double g, double b, double a) :r(r), g(g), b(b), a(a)
{
}
