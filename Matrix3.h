#include <bob/Vec3.h>

typedef bob::_Vec3<double> Vec3;
class Matrix3
{
public:
	Matrix3();
	Matrix3(double e11, double e12, double e13, double e21, double e22, double e23, double e31, double e32, double e33);
	Matrix3 operator*(const Matrix3& other) const;
	Vec3 operator*(const Vec3& v3) const;
	double elements[3][3];
};

Matrix3 rotateX(double theta);
Matrix3 rotateY(double theta);
Matrix3 rotateZ(double theta);
Matrix3 getRotationMatrix(const Vec3& angle);