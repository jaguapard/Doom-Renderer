#include "VertexTransformerShader.h"

std::vector<TexVertex> VertexTransformerShader::run(const VertexTransformerInput& inp)
{
	std::vector<TexVertex> ret;
	ret.reserve(inp.verts->size());

	for (auto vertex : *inp.verts)
	{
		Vec4 spaceCopy = vertex.spaceCoords;
		spaceCopy.w = 1;

		Vec4 worldCoords = vertex.spaceCoords;
		Vec4 rotatedTranslatedSpace = inp.ctr.rotateAndTranslate(spaceCopy);
		
		real zInv = inp.nearPlaneZ / rotatedTranslatedSpace.z;
		vertex.spaceCoords = inp.ctr.screenSpaceToPixels(rotatedTranslatedSpace * zInv);
		vertex.textureCoords *= zInv;
		vertex.textureCoords.z = zInv;
		vertex.worldCoords = worldCoords * zInv;
		ret.push_back(vertex);
	}

	return ret;
}
