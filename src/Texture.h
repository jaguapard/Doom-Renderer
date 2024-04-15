#pragma once
#include <string>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <vector>

#include "Color.h"
#include "PixelBuffer.h"

enum class TextureDebugMode
{
	NONE, //use textures defined in the map file
	X_GRADIENT, //fill all textures with texture's x % 256 on all pixels
	Y_GRADIENT, //fill all textures with texture's y % 256 on all pixels
	XY_GRADIENT,
	SOLID_WHITE, //fill with solid white color
	COLORS, //fill with differing colors, based on texture's index
	CHECKERBOARD, //fill with checkerboard pattern
};

class Texture
{
public:
	Texture(std::string name);
	Color getPixel(int x, int y) const;

	static constexpr TextureDebugMode TEXTURE_DEBUG_MODE = TextureDebugMode::NONE;
private:
	PixelBuffer<Color> pixels;
	std::string name;
	int wMask = -1, hMask = -1;

	void constructDebugTexture();
};