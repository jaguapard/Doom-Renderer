#pragma once
#include <unordered_map>
#include <string>

#include "Texture.h"

class TextureManager
{
public:
	TextureManager();
	int getTextureIndexByName(std::string name); //a new texture will be created and returned if it does not exist
	int getTextureIndexByPath(std::string path); //a new texture will be created and returned if it does not exist
	const Texture& getTextureByIndex(int index, bool multithreadProtectionEnabled = false) const; //multithreadProtectionEnabled set to false will disable the mutex locking for this call. Use only if you are sure that no other thread is going to write this object
	const Texture& getTextureByName(const std::string& name);
private:
	std::vector<Texture> textures;
	std::unordered_map<std::string, int> textureNameToIndexMap;
	std::unordered_map<std::string, std::string> textureNameTranslation;

	std::unordered_map<std::string, std::string> loadTextureTranslation();

	mutable std::recursive_mutex mtx;
};