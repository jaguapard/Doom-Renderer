#include "CoordinateTransformer.h"

CoordinateTransformer::CoordinateTransformer(int w, int h)
{
	this->w = w;
	this->h = h;
	real widthToHeightAspectRatio = real(w) / h;
	this->_shift = { widthToHeightAspectRatio / 2, 0.5, 0 };
}

void CoordinateTransformer::prepare(const Vec3 camPos, const Matrix4& rotation)
{
	//this->camPos = camPos;

	Matrix4 translation = Matrix4::identity();
	translation.elements[0][3] = -camPos.x;
	translation.elements[1][3] = -camPos.y;
	translation.elements[2][3] = -camPos.z;
	translation.elements[3][3] = 1;
	this->translationRotation = rotation * translation;
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

Vec3 CoordinateTransformer::translateAndRotate(Vec3 v) const
{
	v.w = 1;
	Vec3 interm = translationRotation * v;
	return interm;
}

Vec3 CoordinateTransformer::shift(const Vec3 v) const
{
	assert(this->_shift.z == 0.0); //ensure to not touch z
	return v + this->_shift;
}
