#include "Matrix3.h"
#include <cmath>
Matrix3::Matrix3()
{
	for (unsigned char x = 0; x < 3; ++x)
	{
		for (unsigned char y = 0; y < 3; ++y)
		{
			this->elements[x][y] = 0.0;
		}
	}
}

Matrix3::Matrix3(double e11, double e12, double e13, double e21, double e22, double e23, double e31, double e32, double e33)
{
	this->elements[0][0] = e11;
	this->elements[0][1] = e12;
	this->elements[0][2] = e13;
	this->elements[1][0] = e21;
	this->elements[1][1] = e22;
	this->elements[1][2] = e23;
	this->elements[2][0] = e31;
	this->elements[2][1] = e32;
	this->elements[2][2] = e33;
}

Matrix3 Matrix3::operator*(const Matrix3& other) const
{
	Matrix3 ret;
	ret.elements[0][0] = this->elements[0][0] * other.elements[0][0] + this->elements[0][1] * other.elements[1][0] + this->elements[0][2] * other.elements[2][0];
	ret.elements[1][0] = this->elements[1][0] * other.elements[0][0] + this->elements[1][1] * other.elements[1][0] + this->elements[1][2] * other.elements[2][0];
	ret.elements[2][0] = this->elements[2][0] * other.elements[0][0] + this->elements[2][1] * other.elements[1][0] + this->elements[2][2] * other.elements[2][0];
	ret.elements[0][1] = this->elements[0][0] * other.elements[0][1] + this->elements[0][1] * other.elements[1][1] + this->elements[0][2] * other.elements[2][1];
	ret.elements[1][1] = this->elements[1][0] * other.elements[0][1] + this->elements[1][1] * other.elements[1][1] + this->elements[1][2] * other.elements[2][1];
	ret.elements[2][1] = this->elements[2][0] * other.elements[0][1] + this->elements[2][1] * other.elements[1][1] + this->elements[2][2] * other.elements[2][1];
	ret.elements[0][2] = this->elements[0][0] * other.elements[0][2] + this->elements[0][1] * other.elements[1][2] + this->elements[0][2] * other.elements[2][2];
	ret.elements[1][2] = this->elements[1][0] * other.elements[0][2] + this->elements[1][1] * other.elements[1][2] + this->elements[1][2] * other.elements[2][2];
	ret.elements[2][2] = this->elements[2][0] * other.elements[0][2] + this->elements[2][1] * other.elements[1][2] + this->elements[2][2] * other.elements[2][2];
	return ret;
}

Vec3 Matrix3::operator*(const Vec3& v3) const
{
	return{
		v3.x * this->elements[0][0] + v3.y * this->elements[1][0] + v3.z * this->elements[2][0],
		v3.x * this->elements[0][1] + v3.y * this->elements[1][1] + v3.z * this->elements[2][1],
		v3.x * this->elements[0][2] + v3.y * this->elements[1][2] + v3.z * this->elements[2][2]
	};
}

Matrix3 rotateX(double theta)
{
	const double sinTheta = sin(theta);
	const double cosTheta = cos(theta);
	return{
		cosTheta, sinTheta, 0.0,
		-sinTheta, cosTheta, 0.0,
		0.0,    0.0,   1.0
	};
}

Matrix3 rotateY(double theta)
{
	const double sinTheta = sin(theta);
	const double cosTheta = cos(theta);
	return{
		cosTheta, 0.0,-sinTheta,
		0.0,   1.0, 0.0,
		sinTheta, 0.0, cosTheta
	};
}

Matrix3 rotateZ(double theta)
{
	const double sinTheta = sin(theta);
	const double cosTheta = cos(theta);
	return{
		1.0, 0.0,   0.0,
		0.0, cosTheta, sinTheta,
		0.0,-sinTheta, cosTheta,
	};
}


Matrix3 getRotationMatrix(const Vec3& angle)
{
	return rotateX(angle.x) * rotateY(angle.y) * rotateZ(angle.z);
}
