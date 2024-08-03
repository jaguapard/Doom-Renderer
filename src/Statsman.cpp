#include "Statsman.h"
#include <sstream>

Statsman Statsman::operator-(const Statsman& other) const
{
    static_assert(sizeof(*this) % 8 == 0); //won't work 100% of the time, but better than nothing. This won't catch even number of 4 byte entries 

    Statsman ret = *this;
    int members = sizeof(*this) / 8;

    //this will only work if all values in Statsman are uint64_t and *this is greater than other
    //struct padding for this was disabled in the header
    for (int i = 0; i < members; ++i)
    {
        reinterpret_cast<uint64_t*>(&ret)[i] -= reinterpret_cast<const uint64_t*>(&other)[i];
    }

    return ret;
}

std::string Statsman::toString()
{
    std::stringstream ss;

    ss << VAR_PRINT(zBuffer.depthTests) << "\n";
    ss << VAR_PRINT(zBuffer.negativeDepthDiscards) << "\n";
    //ss << VAR_PRINT(zBuffer.outOfBoundsAccesses) << "\n";
    ss << VAR_PRINT(zBuffer.occlusionDiscards) << "\n";
    ss << VAR_PRINT(zBuffer.writes) << "\n";
    ss << VAR_PRINT(zBuffer.writeDisabledTests) << "\n";

    ss << VAR_PRINT(triangles.verticesOutside[0]) << "\n";
    ss << VAR_PRINT(triangles.verticesOutside[1]) << "\n";
    ss << VAR_PRINT(triangles.verticesOutside[2]) << "\n";
    ss << VAR_PRINT(triangles.verticesOutside[3]) << "\n";

    ss << VAR_PRINT(textures.pixelFetches) << "\n";
    ss << VAR_PRINT(textures.gathers) << "\n";

    ss << VAR_PRINT(pixels.nonOpaqueDraws) << "\n";

    ss << VAR_PRINT(memory.allocsByNew) << "\n";
    ss << VAR_PRINT(memory.freesByDelete) << "\n";

    ss << VAR_PRINT(models.boundingBoxDiscards) << "\n";
    return ss.str();
}