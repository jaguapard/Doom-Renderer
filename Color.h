#pragma once
#include <stdint.h>

struct Color
{
	double r, g, b, a;
	Color(uint32_t u);
	Color(double r, double g, double b, double a);
};