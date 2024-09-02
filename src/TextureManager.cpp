#include <iostream>
#include <fstream>

#include <adm/strings.h>

#include "TextureManager.h"

TextureManager::TextureManager()
{
	std::lock_guard lck(this->mtx);
	this->textureNameTranslation = this->loadTextureTranslation();
}

int TextureManager::getTextureIndexByName(std::string name)
{
	std::lock_guard lck(this->mtx);
	auto it = textureNameTranslation.find(name);
	std::string fileName = it == textureNameTranslation.end() ? name : it->second;
	if (textureNameToIndexMap.find(fileName) != textureNameToIndexMap.end()) return textureNameToIndexMap[fileName]; //find by file name

	textures.emplace_back(fileName);
	textureNameToIndexMap[fileName] = textures.size() - 1;
	return textures.size() - 1;
}

int TextureManager::getTextureIndexByPath(std::string path)
{
	std::lock_guard lck(this->mtx);
	if (textureNameToIndexMap.find(path) != textureNameToIndexMap.end()) return textureNameToIndexMap[path];

	textures.emplace_back(path, true);
	textureNameToIndexMap[path] = textures.size() - 1;
	return textures.size() - 1;
}

const Texture& TextureManager::getTextureByIndex(int index, bool multithreadProtectionEnabled) const
{
	if (multithreadProtectionEnabled)
	{
		std::lock_guard lck(this->mtx);
		return textures[index];
	}
	return textures[index];
}

const Texture& TextureManager::getTextureByName(const std::string& name)
{
	std::lock_guard lck(this->mtx);
	return this->getTextureByIndex(this->getTextureIndexByName(name));
}

std::unordered_map<std::string, std::string> TextureManager::loadTextureTranslation()
{
	std::lock_guard lck(this->mtx);
	std::unordered_map<std::string, std::string> ret;
	std::ifstream f("data/TEXTURES_DOOM2.txt");

	std::string line, textureName, fileName;
	while (std::getline(f, line))
	{
		if (line.empty() || line[0] == '/') continue;

		auto parts = adm::strings::split_copy(line, " ");
		if (parts[0] == "WallTexture") textureName = std::string(parts[1], 1, parts[1].length() - 3);
		if (parts[0].find("Patch") != parts[0].npos)
		{
			fileName = std::string(parts[1], 1, parts[1].length() - 3);
			ret[textureName] = fileName;
		}
	}

	return ret;
}