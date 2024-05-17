#pragma once
#include "Vec.h"
#include "Matrix4.h"
class CoordinateTransformer
{
public:
	CoordinateTransformer() = default;
	CoordinateTransformer(int w, int h);
	void prepare(const Vec3 camPos, const Vec3 camAng);
	Vec3 screenSpaceToPixels(const Vec3 v) const;

	Vec3 rotateAndTranslate(Vec3 v) const;
	Vec3 shift(const Vec3 v) const;

	Matrix4 getCurrentTransformationMatrix() const;
private:
	Matrix4 rotationTranslation;
	Vec3 _shift;
	Vec3 hVec;
};