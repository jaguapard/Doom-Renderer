#include "DoomMap.h"
#include "DoomWorldLoader.h"

#include <functional>
std::vector<std::vector<Model>> DoomMap::getMapGeometryModels(TextureManager& tm)
{
    auto models = DoomWorldLoader::loadMapSectorsAsModels(linedefs, vertices, sidedefs, sectors, tm);
    std::function modelIsEmpty = [&](const Model& m) {return m.getTriangleCount() == 0; };

    for (auto& sectorModels : models)
    {
        auto it = std::remove_if(sectorModels.begin(), sectorModels.end(), modelIsEmpty);
        sectorModels.resize(it - sectorModels.begin());
    }

    return models;
}
