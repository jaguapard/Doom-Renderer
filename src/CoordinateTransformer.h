#pragma once
#include "Vec.h"
#include "Matrix4.h"
class CoordinateTransformer
{
public:
	CoordinateTransformer(int w, int h);
	void prepare(const Vec3 camPos, const Matrix4& rotation);
	Vec3 screenSpaceToPixels(const Vec3 v) const;

	Vec3 translateAndRotate(Vec3 v) const;
	Vec3 shift(const Vec3 v) const;

	Matrix4 getCurrentTransformationMatrix() const;
private:
	int w, h;

	Matrix4 translationRotation;
	Vec3 _shift;
	real widthToHeightAspectRatio;
};