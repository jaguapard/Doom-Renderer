#pragma once
#include "Vec.h"
#include "Matrix3.h"
class CoordinateTransformer
{
public:
	CoordinateTransformer(int w, int h);
	void prepare(const Vec3& camPos, const Matrix3& rotation);
	Vec3 toScreenCoords(const Vec3& v);
private:
	int w, h;
	Vec3 camPos;
	Matrix3 rotation;
	Vec3 shift;
	double widthToHeightAspectRatio;
};