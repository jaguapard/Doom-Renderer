#include "TextureMapper.h"

TexVertex TextureMapper::mapRelativeToReferencePoint(const Vec3& pointToMap, const Vec3& referencePoint)
{
    Vec3 delta = pointToMap - referencePoint;
    return mapWanted(pointToMap, delta);
}

TexVertex TextureMapper::mapWanted(const Vec3& pointToMap, const Vec3& wanted)
{
    constexpr real eps = 1e-6;
    const real& wx = wanted.x;
    const real& wy = wanted.y;
    const real& wz = wanted.z;
    Vec3 n = wanted / wanted.len();

    if (abs(n.x) > eps && abs(n.y) > eps) //if both wx and wy are non-zero, just map it directly, wx to "u", wy to "v"
    {
        return { pointToMap, wanted };
    }
    else
    {
        if (abs(n.x) < eps)
        {
            return { pointToMap, {wz, wy, wz} };
        }
        else
        {
            return { pointToMap, {wx,wz,wz} };
        }
    }
}
