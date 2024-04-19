#include <iostream>
#include <fstream>

#include <adm/strings.h>

#include "TextureManager.h"

TextureManager::TextureManager()
{
	this->textureNameTranslation = this->loadTextureTranslation();
}

int TextureManager::getTextureIndexByName(std::string name)
{
	auto it = textureNameTranslation.find(name);
	std::string fileName = it == textureNameTranslation.end() ? name : it->second;
	if (textureNameToIndexMap.find(fileName) != textureNameToIndexMap.end()) return textureNameToIndexMap[fileName]; //find by file name

	textures.emplace_back(fileName);
	textureNameToIndexMap[fileName] = textures.size() - 1;
	return textures.size() - 1;
}

const Texture& TextureManager::getTextureByIndex(int index) const
{
	return textures[index];
}

const Texture& TextureManager::getTextureByName(const std::string& name)
{
	return this->getTextureByIndex(this->getTextureIndexByName(name));
}

std::unordered_map<std::string, std::string> TextureManager::loadTextureTranslation()
{
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