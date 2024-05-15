#pragma once
#include "Vec.h"

class alignas(32) Matrix3
{
public:
	//Matrix3();
	//Matrix3(real e11, real e12, real e13, real e21, real e22, real e23, real e31, real e32, real e33);
	Matrix3 operator*(const Matrix3& other) const;
	Vec3 operator*(const Vec3& v3) const;
	union {
		bob::_SSE_Vec4_float val[4];
		float elements[4][4];
	};

	Matrix3 transposed() const;
	Vec3 multiplyByTransposed(const Vec3& v3) const;

	static Matrix3 rotationX(float theta);
	static Matrix3 rotationY(float theta);
	static Matrix3 rotationZ(float theta);
	static Matrix3 rotationXYZ(const Vec3& angle);
	static Matrix3 identity(float value = 1.0, int dim = 4);
	static Matrix3 zeros();
};

