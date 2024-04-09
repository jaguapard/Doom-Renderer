#include "CoordinateTransformer.h"

CoordinateTransformer::CoordinateTransformer(int w, int h)
{
	this->w = w;
	this->h = h;
	double widthToHeightAspectRatio = double(w) / h;
	shift = { widthToHeightAspectRatio / 2, 0.5, 0 };
}

void CoordinateTransformer::prepare(const Vec3& camPos, const Matrix3& rotation)
{
	this->camPos = camPos;
	this->rotation = rotation;
}

Vec3 CoordinateTransformer::toScreenCoords(const Vec3& v) const
{
	Vec3 camOffset = v - camPos;
	Vec3 rot = rotation * camOffset;
	Vec3 perspective = rot / rot.z; //screen space coords of vector

	Vec3 shifted = perspective + this->shift; //convert so (0,0) in `perspective` corresponds to center of the screen
	Vec3 final = shifted * h;
	return final;
}
