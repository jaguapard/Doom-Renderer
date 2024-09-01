#pragma once
#include <assimp/Importer.hpp>
#include <vector>
#include "Model.h"

class AssetLoader
{
public:
	AssetLoader();
	std::vector<Model> loadObj(std::string path, TextureManager& textureManager);
	
	//void saveModels(const std::vector<Model>& models, std::string path) const;
private:
	Assimp::Importer importer;
};