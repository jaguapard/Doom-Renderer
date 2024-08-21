#include "AssetLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Vec.h"
#include <sstream>

#pragma comment(lib, "assimp-vc143-mt.lib")

Vec4 aiToBob(aiVector3D ai)
{
	return { ai.x, ai.y, ai.z, 0 };
}

std::vector<Model> AssetLoader::loadObj(std::string path, TextureManager& textureManager)
{
	Assimp::Importer imp;
	const auto pScene = imp.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_MakeLeftHanded | aiProcess_GenUVCoords);
	std::stringstream ss;
	ss << "Error while loading model " << path << ": ";
	if (!pScene)
	{
		ss << imp.GetErrorString();
		throw std::runtime_error(ss.str());
	}
	
	std::vector<Model> models;
	for (size_t i = 0; i < pScene->mNumMeshes; i++)
	{
		aiMesh* mesh = pScene->mMeshes[i];
		aiMaterial* material = pScene->mMaterials[mesh->mMaterialIndex];
		aiString texturePath;
		material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
		int textureIndex = textureManager.getTextureIndexByPath("scenes/Sponza/" + std::string(texturePath.C_Str()));

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
				t.tv[k].spaceCoords = aiToBob(aiVertice);
				t.tv[k].worldCoords = aiToBob(aiVertice);
				t.tv[k].textureCoords = aiToBob(mesh->mTextureCoords[0][vertIndex]);
			}
		}

		models.push_back(Model(tris, textureIndex));
	}
	return models;

	/*
	// parse materials
	std::vector<Material> materials;
	materials.reserve( pScene->mNumMaterials );
	for( size_t i = 0; i < pScene->mNumMaterials; i++ )
	{
		materials.emplace_back( gfx,*pScene->mMaterials[i],pathString );
	}

	for( size_t i = 0; i < pScene->mNumMeshes; i++ )
	{
		const auto& mesh = *pScene->mMeshes[i];
		meshPtrs.push_back( std::make_unique<Mesh>( gfx,materials[mesh.mMaterialIndex],mesh,scale ) );
	}

	int nextId = 0;
	pRoot = ParseNode( nextId,*pScene->mRootNode,scale );
	*/
}
