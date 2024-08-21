#include "AssetLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#pragma comment(lib, "assimp-vc140-mt.lib")
std::vector<Model> AssetLoader::loadObj(std::string path)
{
	Assimp::Importer imp;
	const auto pScene = imp.ReadFile(path.c_str(), aiProcess_Triangulate);
    return std::vector<Model>();
}
