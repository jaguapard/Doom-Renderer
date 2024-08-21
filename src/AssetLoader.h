#pragma once
#include <vector>
#include "Model.h"
class AssetLoader
{
public:
	static std::vector<Model> loadObj(std::string path);
};