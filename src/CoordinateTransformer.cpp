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

	this->rotationTranslation = (rotation * translation).transposed();
	//this->inverseRotationTranslation = rotationTranslation.inverse();
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

VectorPack16 CoordinateTransformer::screenSpaceToPixels(const VectorPack16& v) const
{
	return (v + this->_shift) * hVec;
}

Vec4 CoordinateTransformer::rotateAndTranslate(Vec4 v) const
{
	Vec4 interm = rotationTranslation.multiplyByTransposed(v);
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
	rotatedTranslated.z = z;
	rotatedTranslated.w = 1;
	return inverseRotationTranslation * rotatedTranslated; //TODO: reverse rotation-translation
}

Matrix4 CoordinateTransformer::getCurrentTransformationMatrix() const
{
	return rotationTranslation.transposed();
}

Matrix4 CoordinateTransformer::getCurrentInverseTransformationMatrix() const
{
	return inverseRotationTranslation;
}
