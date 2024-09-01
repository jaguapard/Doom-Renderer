#include "AssetLoader.h"


#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Vec.h"
#include <sstream>
#include <iostream>
#include <fstream>

#pragma comment(lib, "assimp-vc143-mt.lib")

Vec4 aiToBob(aiVector3D ai)
{
	return { ai.x, ai.y, ai.z, 0 };
}

std::string getFolderFromPath(std::string path, bool addTrailingSlash = false)
{
	size_t lastSlash = path.rfind('/');
	std::string pathStr;
	if (lastSlash != path.npos) pathStr = path.substr(0, lastSlash);
	else if (lastSlash != path.npos) pathStr = path.substr(0, lastSlash);
	else throw std::runtime_error("Could not extract folder from path string: " + path);

	if (addTrailingSlash) return pathStr + "/";
	else return pathStr;
}
AssetLoader::AssetLoader()
{

}

template <typename T>
void writeVarToFile(const T& var, std::ofstream& file, size_t sizeOverride = 0)
{
	if (!sizeOverride) file.write((const char*)(&var), sizeof(var));
	else file.write((const char*)(&var), sizeOverride);
}

std::vector<Model> AssetLoader::loadObj(std::string path, TextureManager& textureManager, std::string convertToSavePath)
{
	const auto pScene = this->importer.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_MakeLeftHanded | aiProcess_GenUVCoords);
	std::stringstream ss;
	ss << "Error while loading scene " << path << ": ";
	if (!pScene)
	{
		ss << this->importer.GetErrorString();
		throw std::runtime_error(ss.str());
	}
	
	std::vector<Model> models;
	std::ofstream convertedSavedModel;
	if (convertToSavePath.length() > 0) convertedSavedModel = std::ofstream(convertToSavePath, std::ios::binary);

	for (size_t i = 0; i < pScene->mNumMeshes; i++)
	{
		aiMesh* mesh = pScene->mMeshes[i];
		aiMaterial* material = pScene->mMaterials[mesh->mMaterialIndex];
		aiString texturePath;
		material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);

		std::string textureRelPath = texturePath.C_Str();
		std::string textureFullPath = getFolderFromPath(path, true) + textureRelPath;
		int textureIndex = textureManager.getTextureIndexByPath(textureFullPath);

		std::vector<Triangle> tris;
		for (size_t j = 0; j < mesh->mNumFaces; ++j)
		{
			aiFace face = mesh->mFaces[j];
			if (face.mNumIndices != 3)
			{
				ss << "Mesh " << i << " face " << j << " has unexpected vertice count: " << face.mNumIndices;
				throw std::runtime_error(ss.str());
			}

			Triangle& t = tris.emplace_back();
			for (int k = 0; k < 3; ++k)
			{
				int vertIndex = face.mIndices[k];
				aiVector3D aiVertice = mesh->mVertices[vertIndex];
				aiVector3D aiUVs = mesh->mTextureCoords[0][vertIndex];
				aiUVs.y *= -1;
				t.tv[k].spaceCoords = aiToBob(aiVertice);
				t.tv[k].worldCoords = aiToBob(aiVertice);
				t.tv[k].textureCoords = aiToBob(aiUVs);
			}
		}
		models.push_back(Model(tris, textureIndex, textureManager));

		if (convertToSavePath.length() > 0)
		{
			float saveData[15];
			uint64_t modelSize = tris.size() * sizeof(saveData); //size of a single model entry in bytes, not counting size member and path. 
			writeVarToFile(modelSize, convertedSavedModel);
			convertedSavedModel.write(textureRelPath.c_str(), textureRelPath.length() + 1);
			for (const auto& it : tris)
			{				
				for (int k = 0; k < 3; ++k)
				{
					saveData[5 * k] = it.tv[k].worldCoords.x;
					saveData[5 * k + 1] = it.tv[k].worldCoords.y;
					saveData[5 * k + 2] = it.tv[k].worldCoords.z;
					saveData[5 * k + 3] = it.tv[k].textureCoords.x;
					saveData[5 * k + 4] = it.tv[k].textureCoords.y;
				}
				writeVarToFile(saveData, convertedSavedModel);
			}
		}
	}
	
	size_t triangleCount = 0;
	for (auto& it : models) triangleCount += it.getTriangleCount();
	std::cout << "Scene " << path << ": loaded " << triangleCount << " triangles\n";
	return models;
}
