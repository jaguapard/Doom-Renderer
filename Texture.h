#pragma once
#include <string>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

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

enum DebugColors
{
	WHITE = 0xFFFFFFFF,
	GREY = 0x7F7F7FFF,
	RED = 0xFF0000FF,
	GREEN = 0x00FF00FF,
	BLUE = 0x0000FFFF,
	YELLOW = 0xFFFF00FF,
};

struct Texture
{
	SDL_Surface* surf;
	std::string name;

	Texture(std::string name);
	SDL_Color getPixel(int x, int y, double lightMult) const;
	~Texture();

	static constexpr TextureDebugMode TEXTURE_DEBUG_MODE = TextureDebugMode::NONE;
};