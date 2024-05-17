#include "CoordinateTransformer.h"

CoordinateTransformer::CoordinateTransformer(int w, int h)
{
	this->w = w;
	this->h = h;
	real widthToHeightAspectRatio = real(w) / h;
	this->_shift = { widthToHeightAspectRatio / 2, 0.5, 0 };
}

void CoordinateTransformer::prepare(const Vec3 camPos, const Vec3 camAng)
{
	//this->camPos = camPos;
	Matrix4 rotation = Matrix4::rotationXYZ(camAng);
	Matrix4 translation = Matrix4::identity();
	translation[0][3] = -camPos.x;
	translation[1][3] = -camPos.y;
	translation[2][3] = -camPos.z;
	translation[3][3] = 1;
	this->rotationTranslation = rotation * translation;
	//this->translationRotation = translation * rotation;
}

/*Vec3 CoordinateTransformer::toScreenCoords(const Vec3 v) const
{
	assert(this->_shift.z == 0.0); //ensure to not touch z
	Vec3 camOffset = v - camPos;
	Vec3 rot = rotation.multiplyByTransposed(camOffset);
	Vec3 perspective = rot / rot.z; //screen space coords of vector

	Vec3 shifted = perspective + this->_shift; //convert so (0,0) in `perspective` corresponds to center of the screen
	Vec3 final = shifted * h;
	return final;
}*/

Vec3 CoordinateTransformer::screenSpaceToPixels(const Vec3 v) const
{
	assert(this->_shift.z == 0.0); //ensure to not touch z
	return (v + this->_shift) * h;
}

static const __m128 wOne = _mm_set1_ps(1);
Vec3 CoordinateTransformer::rotateAndTranslate(Vec3 v) const
{
	v = _mm_blend_ps(v.sseVec, wOne, 0b1000); //same as v.w = 1, but it's slightly faster
	Vec3 interm = rotationTranslation * v;
	return interm;
}

Vec3 CoordinateTransformer::shift(const Vec3 v) const
{
	assert(this->_shift.z == 0.0); //ensure to not touch z
	return v + this->_shift;
}

Matrix4 CoordinateTransformer::getCurrentTransformationMatrix() const
{
	return rotationTranslation;
}
