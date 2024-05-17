#pragma once
#include "Vec.h"
#include <string>

class alignas(32) Matrix4
{
public:
	union {
		bob::_SSE_Vec4_float val[4];
		float elements[4][4];
		struct { __m128 xmm0, xmm1, xmm2, xmm3; };
		struct { __m256 ymm0, ymm1; };
	};

	Matrix4() = default;
	Matrix4(const std::initializer_list<bob::_SSE_Vec4_float> lst);

	Matrix4 operator*(const Matrix4& other) const; //result = this * other
	Vec3 operator*(const Vec3 v) const;

	Matrix4 transposed() const;
	Vec3 multiplyByTransposed(const Vec3 v) const; //result = A^T * x (multiply transposed matrix by column vector v)

	const bob::_SSE_Vec4_float& operator[](int i) const;
	bob::_SSE_Vec4_float& operator[](int i);

	std::string toString(int precision = 5) const;

	static Matrix4 rotationX(float theta);
	static Matrix4 rotationY(float theta);
	static Matrix4 rotationZ(float theta);
	static Matrix4 rotationXYZ(const Vec3& angle);
	static Matrix4 identity(float value = 1.0, int dim = 4);
	static Matrix4 zeros();
};

