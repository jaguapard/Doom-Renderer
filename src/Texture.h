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
	Color getPixel(Vec3 coords) const; //z and w values are ignored
	Color getPixel(int x, int y) const;
	Color getPixelAtUV(real u, real v) const;

	int getW() const;
	int getH() const;
	bool hasOnlyOpaquePixels() const;

	static constexpr TextureDebugMode TEXTURE_DEBUG_MODE = TextureDebugMode::NONE;
private:
	PixelBuffer<Color> pixels;
	std::string name;
	bool _hasOnlyOpaquePixels = true;
	int64_t wInverse, hInverse; //inverse values for removing idiv
	int bigW, bigH;

	Vec3 dimensionsFloat;
	__m128i dimensionsInt;

	void checkForTransparentPixels();
	//static constexpr int FRACBITS = 16;
	void constructDebugTexture();
	void populateInverses(int w, int h);
};