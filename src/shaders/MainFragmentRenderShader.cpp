#include "../Vec.h"
#include "../VectorPack.h"
#include "MainFragmentRenderShader.h"

std::tuple<FloatPack16, FloatPack16, FloatPack16> RenderHelpers::calculateBarycentricCoordinates(const VectorPack16& r, const Vec4& r1, const Vec4& r2, const Vec4& r3, const real& rcpSignedArea)
{
	return {
		(r - r3).cross2d(r2 - r3) * rcpSignedArea,
		(r - r3).cross2d(r3 - r1) * rcpSignedArea,
		(r - r1).cross2d(r1 - r2) * rcpSignedArea //do NOT change this to 1-alpha-beta or 1-(alpha+beta). That causes wonkiness in textures
	};
}
