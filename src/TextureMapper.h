#pragma once
#include "Vec.h"
#include "Triangle.h"

class TextureMapper
{
public:
	static TexVertex mapRelativeToReferencePoint(const Vec4& pointToMap, const Vec4& referencePoint);
	static Triangle mapTriangleRelativeToMinPoint(Triangle t);
	static Triangle mapTriangleRelativeToFirstVertex(Triangle t);
	static TexVertex mapWanted(const Vec4& pointToMap, const Vec4& wanted);
};