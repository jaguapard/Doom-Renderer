#pragma once
#include <array>
#include <immintrin.h>

#include <SDL/SDL.h>

#include "helpers.h"
#include "Vec.h"
#include "CoordinateTransformer.h"
#include "ZBuffer.h"
#include "Texture.h"
#include "TextureManager.h"
#include "TexVertex.h"
#include "PointLight.h"
#include "misc/GameSettings.h"

class Model;

struct Triangle
{
	//std::array<TexVertex, 3> tv;
	TexVertex tv[3];

	void sortByAscendingSpaceX();
	void sortByAscendingSpaceY();
	void sortByAscendingSpaceZ();
	void sortByAscendingTextureX();
	void sortByAscendingTextureY();

	static std::pair<Triangle, Triangle> pairFromRect(std::array<TexVertex, 4> rectPoints);

	Vec4 getNormalVector() const;
private:

};