#include "TextureMapper.h"

TexVertex TextureMapper::mapRelativeToReferencePoint(const Vec4& pointToMap, const Vec4& referencePoint)
{
    Vec4 delta = pointToMap - referencePoint;
    return mapWanted(pointToMap, delta);
}


Triangle TextureMapper::mapTriangleRelativeToMinPoint(Triangle t)
{
    real minX = std::min_element(t.tv.begin(), t.tv.end(), [](const TexVertex& tv1, const TexVertex& tv2) {return tv1.spaceCoords.x < tv2.spaceCoords.x; })->spaceCoords.x;
    real minY = std::min_element(t.tv.begin(), t.tv.end(), [](const TexVertex& tv1, const TexVertex& tv2) {return tv1.spaceCoords.y < tv2.spaceCoords.y; })->spaceCoords.y;
    real minZ = std::min_element(t.tv.begin(), t.tv.end(), [](const TexVertex& tv1, const TexVertex& tv2) {return tv1.spaceCoords.z < tv2.spaceCoords.z; })->spaceCoords.z;
    Vec4 ref = { minX,minY, minZ };
    for (auto& tv : t.tv)
        tv = mapRelativeToReferencePoint(tv.spaceCoords, ref);
    return t;
}

Triangle TextureMapper::mapTriangleRelativeToFirstVertex(Triangle t)
{
    for (auto& tv : t.tv)
        tv = mapRelativeToReferencePoint(tv.spaceCoords, t.tv[0].spaceCoords);
    return t;
}

TexVertex TextureMapper::mapWanted(const Vec4& pointToMap, const Vec4& wanted)
{
    constexpr real eps = 1e-6;
    const real& wx = wanted.x;
    const real& wy = wanted.y;
    const real& wz = wanted.z;
    Vec4 n = wanted / wanted.len();

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
