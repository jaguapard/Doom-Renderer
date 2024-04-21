#include "DoomMap.h"
#include "DoomWorldLoader.h"

std::vector<std::vector<Triangle>> DoomMap::getTriangles(TextureManager& tm)
{
    auto tris = DoomWorldLoader::loadTriangles(linedefs, vertices, sidedefs, sectors, tm);

    return tris;
}
