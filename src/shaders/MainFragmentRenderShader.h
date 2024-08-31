#pragma once
#include "ShaderBase.h"
#include "../Triangle.h"

struct RenderHelpers
{
	static std::tuple<FloatPack16, FloatPack16, FloatPack16> calculateBarycentricCoordinates(const VectorPack16& r, const Vec4& r1, const Vec4& r2, const Vec4& r3, const real& rcpSignedArea);
};