#include "DoomMap.h"
#include "DoomWorldLoader.h"

std::vector<std::vector<Model>> DoomMap::getMapGeometryModels(TextureManager& tm)
{
    auto tris = DoomWorldLoader::loadMapSectorsAsModels(linedefs, vertices, sidedefs, sectors, tm);

    return tris;
}
