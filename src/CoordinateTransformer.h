#pragma once
#include "Vec.h"
#include "Matrix4.h"
class CoordinateTransformer
{
public:
	CoordinateTransformer(int w, int h);
	void prepare(const Vec3 camPos, const Matrix4& rotation);
	Vec3 toScreenCoords(const Vec3 v) const;
	Vec3 screenSpaceToPixels(const Vec3 v) const;

	Vec3 doCamOffset(const Vec3 v) const;
	Vec3 rotate(const Vec3 v) const;
	Vec3 shift(const Vec3 v) const;
private:
	int w, h;
	Vec3 camPos;
	Matrix4 rotation;
	Vec3 _shift;
	real widthToHeightAspectRatio;
};