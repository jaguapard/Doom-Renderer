#include "Sky.h"

std::vector<Triangle> generateSphereMesh(int horizontalDivisions, int verticalDivisions, real radius, Vec4 sizeMult = { 1,1,1 }, Vec4 shift = { 0,0,0 })
{
	assert(horizontalDivisions * (verticalDivisions - 1) % 3 == 0);
	std::vector<Triangle> ret;
	std::vector<Vec4> world, texture;
	for (int m = 0; m < horizontalDivisions; m++)
	{
		for (int n = 0; n < verticalDivisions; n++)
		{
			real x, y, z;
			real mp, np;
			mp = real(m) / horizontalDivisions;
			np = real(n) / verticalDivisions;
			x = sin(M_PI * mp) * cos(2 * M_PI * np);
			y = sin(M_PI * mp) * sin(2 * M_PI * np);
			z = cos(M_PI * mp);
			world.push_back(Vec4(x, y, z));
			texture.push_back(Vec4(mp, np));

			mp = real(m + 1) / horizontalDivisions;
			np = real(n) / verticalDivisions;
			x = sin(M_PI * mp) * cos(2 * M_PI * np);
			y = sin(M_PI * mp) * sin(2 * M_PI * np);
			z = cos(M_PI * mp);
			world.push_back(Vec4(x, y, z));
			texture.push_back(Vec4(mp, np));

			mp = real(m + 1) / horizontalDivisions;
			np = real(n + 1) / verticalDivisions;
			x = sin(M_PI * mp) * cos(2 * M_PI * np);
			y = sin(M_PI * mp) * sin(2 * M_PI * np);
			z = cos(M_PI * mp);
			world.push_back(Vec4(x, y, z));
			texture.push_back(Vec4(mp, np));

			mp = real(m + 1) / horizontalDivisions;
			np = real(n + 1) / verticalDivisions;
			x = sin(M_PI * mp) * cos(2 * M_PI * np);
			y = sin(M_PI * mp) * sin(2 * M_PI * np);
			z = cos(M_PI * mp);
			world.push_back(Vec4(x, y, z));
			texture.push_back(Vec4(mp, np));

			mp = real(m) / horizontalDivisions;
			np = real(n + 1) / verticalDivisions;
			x = sin(M_PI * mp) * cos(2 * M_PI * np);
			y = sin(M_PI * mp) * sin(2 * M_PI * np);
			z = cos(M_PI * mp);
			world.push_back(Vec4(x, y, z));
			texture.push_back(Vec4(mp, np));

			mp = real(m) / horizontalDivisions;
			np = real(n) / verticalDivisions;
			x = sin(M_PI * mp) * cos(2 * M_PI * np);
			y = sin(M_PI * mp) * sin(2 * M_PI * np);
			z = cos(M_PI * mp);
			world.push_back(Vec4(x, y, z));
			texture.push_back(Vec4(mp, np));
		}
	}

	assert(world.size() % 3 == 0);
	for (int i = 0; i < world.size(); i += 3)
	{
		Triangle t;
		for (int j = 0; j < 3; ++j)
		{
			Vec4 textureCoords = texture[i + j];
			std::swap(textureCoords.x, textureCoords.y);
			std::swap(world[i + j].z, world[i + j].y);
			Vec4 adjWorld = (world[i + j] * sizeMult + shift) * radius;
			t.tv[j] = { adjWorld, textureCoords };
		}

		ret.push_back(t);
	}
	return ret;
}

Sky::Sky(std::string textureName, TextureManager& textureManager)
{
	const Texture& texture = textureManager.getTextureByName(textureName);
	double aspectRatio = texture.getW() / texture.getH();
	Vec4 uvMult = Vec4(texture.getH(), texture.getH(), 1);
	Vec4 aspectRatioDistortion = Vec4(2 / aspectRatio, 1, 1); //stretching the sphere helps with textures being too stretched. 2, since X wraps around 2 times
	auto skyTriangles = generateSphereMesh(60, 30, 65536, aspectRatioDistortion, {0, -0.4, 0});

	int textureIndex = textureManager.getTextureIndexByName(textureName);
	for (auto& it : skyTriangles)
	{
		for (auto& tv : it.tv)
		{
			Vec4 preremapUv = tv.textureCoords; //to stop texture abruptly jumping back to the beginning at sphere's end, we must wrap it properly
			if (preremapUv.x <= 0.5) preremapUv.x = 2 * preremapUv.x;
			else if (preremapUv.x > 0.5) preremapUv.x = (1 - preremapUv.x) * 2; //this remaps range (0,1) to (0,1),(1,0) flipping direction when x == 0.5
			tv.textureCoords = preremapUv;
		}
	}
	this->skyModel = Model(skyTriangles, textureIndex, textureManager);
}

void Sky::addToRenderQueue(TriangleRenderContext ctx)
{
	ctx.lightMult = 1;
	ctx.backfaceCullingEnabled = false; //force sky to have no face culling
	this->skyModel.addToRenderQueue(ctx);
}
