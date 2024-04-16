#pragma once
#include "Vec.h"

class Matrix3
{
public:
	Matrix3();
	Matrix3(real e11, real e12, real e13, real e21, real e22, real e23, real e31, real e32, real e33);
	Matrix3 operator*(const Matrix3& other) const;
	Vec3 operator*(const Vec3& v3) const;
	real elements[3][3];
};

Matrix3 rotateX(real theta);
Matrix3 rotateY(real theta);
Matrix3 rotateZ(real theta);
Matrix3 getRotationMatrix(const Vec3& angle);