#pragma once
#include <immintrin.h>
class LehmerRNG
{
public:
	LehmerRNG();
	__m512i next();
private:
	__m512i state1, state2;
};