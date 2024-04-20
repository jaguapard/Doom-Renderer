#pragma once
#include "Vec.h"
#include "Triangle.h"

class TextureMapper
{
public:
	static TexVertex mapRelativeToReferencePoint(const Vec3& pointToMap, const Vec3& referencePoint);
	static Triangle mapTriangleRelativeToMinPoint(Triangle t);
	static Triangle mapTriangleRelativeToFirstVertex(Triangle t);
	static TexVertex mapWanted(const Vec3& pointToMap, const Vec3& wanted);
};