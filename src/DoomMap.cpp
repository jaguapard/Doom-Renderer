#include "DoomMap.h"
#include "DoomWorldLoader.h"

std::vector<std::vector<Triangle>> DoomMap::getTriangles(TextureManager& tm)
{
    auto tris = DoomWorldLoader::loadTriangles(linedefs, vertices, sidedefs, sectors, tm);
    int skyIndex = tm.getTextureIndexByName("F_SKY1");
    for (auto& it : tris) //remove all occurances of triangles with F_SKY1 texture, since that marks sky, which is already rendered
    {
        auto removeIt = std::partition(it.begin(), it.end(), [&](const Triangle& t) {return t.textureIndex != skyIndex; });
        it.resize(removeIt - it.begin());
    }

    return tris;
}
