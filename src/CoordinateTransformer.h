#pragma once
#include "Vec.h"
#include "Matrix4.h"
#include "VectorPack.h"

class CoordinateTransformer
{
public:
	CoordinateTransformer() = default;
	CoordinateTransformer(int w, int h);
	void prepare(const Vec4 camPos, const Vec4 camAng);
	Vec4 screenSpaceToPixels(const Vec4 v) const;
	VectorPack16 screenSpaceToPixels(const VectorPack16& v) const;

	Vec4 rotateAndTranslate(Vec4 v) const;
	Vec4 shift(const Vec4 v) const;

	VectorPack16 pixelsToWorld16(const VectorPack16& px) const;

	Matrix4 getCurrentTransformationMatrix() const;
	Matrix4 getCurrentInverseTransformationMatrix() const;
private:
	Matrix4 rotationTranslation;
	Matrix4 inverseRotationTranslation;
	Vec4 _shift;
	Vec4 hVec;
};