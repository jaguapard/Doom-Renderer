#pragma once
#include "ShaderBase.h"

#include <vector>
#include "../Triangle.h"

struct VertexTransformerInput
{
	std::vector<TexVertex>* verts;
	const CoordinateTransformer ctr;
	real nearPlaneZ;
};

class VertexTransformerShader : public ShaderBase<VertexTransformerInput, std::vector<TexVertex>>
{
public:
	virtual std::vector<TexVertex> run(const VertexTransformerInput& inp);
private:
};