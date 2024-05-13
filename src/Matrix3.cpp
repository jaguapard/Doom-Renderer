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

Matrix3::Matrix3(real e11, real e12, real e13, real e21, real e22, real e23, real e31, real e32, real e33)
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

Matrix3 Matrix3::transposed() const
{
	Matrix3 ret;
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			ret.elements[i][j] = this->elements[j][i];
		}
	}
	return ret;
}

Vec3 Matrix3::multiplyByTransposed(const Vec3& v3) const
{
	return {
		v3.x * elements[0][0] + v3.y * elements[0][1] + v3.z * elements[0][2],
		v3.x * elements[1][0] + v3.y * elements[1][1] + v3.z * elements[1][2],
		v3.x * elements[2][0] + v3.y * elements[2][1] + v3.z * elements[2][2],
	};
}

Matrix3 rotateX(real theta)
{
	const real sinTheta = sin(theta);
	const real cosTheta = cos(theta);
	return{
		cosTheta, sinTheta, 0.0,
		-sinTheta, cosTheta, 0.0,
		0.0,    0.0,   1.0
	};
}

Matrix3 rotateY(real theta)
{
	const real sinTheta = sin(theta);
	const real cosTheta = cos(theta);
	return{
		cosTheta, 0.0,-sinTheta,
		0.0,   1.0, 0.0,
		sinTheta, 0.0, cosTheta
	};
}

Matrix3 rotateZ(real theta)
{
	const real sinTheta = sin(theta);
	const real cosTheta = cos(theta);
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
