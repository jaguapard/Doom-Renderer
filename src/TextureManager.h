#pragma once
#include <unordered_map>
#include <string>

#include "Texture.h"

class TextureManager
{
public:
	TextureManager();
	int getTextureIndexByName(std::string name); //a new texture will be created and returned if it does not exist
	const Texture& getTextureByIndex(int index) const;
	const Texture& getTextureByName(const std::string& name);
private:
	std::vector<Texture> textures;
	std::unordered_map<std::string, int> textureNameToIndexMap;
	std::unordered_map<std::string, std::string> textureNameTranslation;

	std::unordered_map<std::string, std::string> loadTextureTranslation();
};