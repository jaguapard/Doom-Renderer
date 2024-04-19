#include "DoomMap.h"
#include "DoomWorldLoader.h"

std::vector<std::vector<Triangle>> DoomMap::getTriangles(TextureManager& tm)
{
    return DoomWorldLoader::loadTriangles(linedefs, vertices, sidedefs, sectors, tm);
}
