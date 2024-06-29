#include "CoordinateTransformer.h"

CoordinateTransformer::CoordinateTransformer(int w, int h)
{
	real widthToHeightAspectRatio = real(w) / h;
	this->_shift = { widthToHeightAspectRatio / 2, 0.5, 0 };
	this->hVec = Vec4(h, h, 1, 1);
}

void CoordinateTransformer::prepare(const Vec4 camPos, const Vec4 camAng)
{
	//this->camPos = camPos;
	Matrix4 rotation = Matrix4::rotationXYZ(camAng);
	Matrix4 translation = Matrix4::identity();
	translation[0][3] = -camPos.x;
	translation[1][3] = -camPos.y;
	translation[2][3] = -camPos.z;
	translation[3][3] = 1;

	this->rotationTranslation = rotation * translation;
	this->inverseRotationTranslation = rotationTranslation.inverse();
	//this->translationRotation = translation * rotation;
}

/*Vec4 CoordinateTransformer::toScreenCoords(const Vec4 v) const
{
	assert(this->_shift.z == 0.0); //ensure to not touch z
	Vec4 camOffset = v - camPos;
	Vec4 rot = rotation.multiplyByTransposed(camOffset);
	Vec4 perspective = rot / rot.z; //screen space coords of vector

	Vec4 shifted = perspective + this->_shift; //convert so (0,0) in `perspective` corresponds to center of the screen
	Vec4 final = shifted * h;
	return final;
}*/

Vec4 CoordinateTransformer::screenSpaceToPixels(const Vec4 v) const
{
	assert(this->_shift.z == 0.0); //ensure to not touch z
	return (v + this->_shift) * hVec;
}

static const __m128 wOne = _mm_set1_ps(1);
Vec4 CoordinateTransformer::rotateAndTranslate(Vec4 v) const
{
	v = _mm_blend_ps(v, wOne, 0b1000); //same as v.w = 1, but it's slightly faster
	Vec4 interm = rotationTranslation * v;
	return interm;
}

Vec4 CoordinateTransformer::shift(const Vec4 v) const
{
	assert(this->_shift.z == 0.0); //ensure to not touch z
	return v + this->_shift;
}

VectorPack16 CoordinateTransformer::pixelsToWorld16(const VectorPack16& px) const
{
	FloatPack16 z = FloatPack16(-1) / px.z;

	VectorPack16 screenSpace = px / hVec - _shift;
	VectorPack16 rotatedTranslated = screenSpace * z;
	return rotatedTranslated; //TODO: reverse rotation-translation
}

Matrix4 CoordinateTransformer::getCurrentTransformationMatrix() const
{
	return rotationTranslation;
}
