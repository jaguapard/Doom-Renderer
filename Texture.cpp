#include "Texture.h"
#include <iostream>
#include "Vec.h"
#include "Statsman.h"

Texture::Texture(std::string name)
{
	this->name = name;
	SDL_Surface* surf;

	if (TEXTURE_DEBUG_MODE == TextureDebugMode::NONE)
	{
		std::string path = "D:/Games/GZDoom/Doom2_unpacked/graphics/" + name + ".png"; //TODO: doom uses TEXTURES lumps for some dark magic with them, this code does not work for unprepared textures.
		surf = IMG_Load(path.c_str());

		if (surf)
		{
			SDL_Surface* old = surf;
			surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
			SDL_FreeSurface(old);
		}
		else
		{
			std::cout << "Failed to load texture " << name << " from " << path << ": " << IMG_GetError() << "\n" <<
				"Falling back to purple-grey checkerboard.\n\n";
			int w = 64, h = 64;

			surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
			for (int y = 0; y < h; ++y)
			{
				for (int x = 0; x < w; ++x)
				{
					Uint32* ptr = (Uint32*)(surf->pixels);
					ptr[y * w + x] = (x % 16 < 8) ^ (y % 16 < 8) ? 0x7F7F7FFF : 0xFF00FFFF;
				}
			}
		}
	}
	else
	{
		surf = SDL_CreateRGBSurfaceWithFormat(0, 1024, 1024, 32, SDL_PIXELFORMAT_RGBA32);
		int tw = 1024, th = 1024;
		Uint32 dbg_colors[] = { WHITE, GREY, RED, GREEN, BLUE, YELLOW };
		static int textureNumber = 0;
		for (int y = 0; y < tw; ++y)
		{
			for (int x = 0; x < th; ++x)
			{
				Uint32* px = (Uint32*)(surf->pixels);
				Uint8 bx = x, by = y;
				switch (TEXTURE_DEBUG_MODE) //TODO: this colors are now broken after switching to SDL_PIXELFORMAT_RGBA32 everywhere. fix
				{
				case TextureDebugMode::NONE:
					break;
				case TextureDebugMode::X_GRADIENT:
					px[by * tw + bx] = (bx << 24) + (bx << 16) + (bx << 8) + 255;
					break;
				case TextureDebugMode::Y_GRADIENT:
					px[by * tw + bx] = (by << 24) + (by << 16) + (by << 8) + 255;
					break;
				case TextureDebugMode::XY_GRADIENT:
					px[by * tw + bx] = (x << 24) + (y << 16) + 255;
					break;
				case TextureDebugMode::SOLID_WHITE:
					px[y * tw + x] = 0xFFFFFFFF;
					break;
				case TextureDebugMode::COLORS:
					px[y * tw + x] = dbg_colors[textureNumber % std::size(dbg_colors)];
					break;
				case TextureDebugMode::CHECKERBOARD:
					px[y * tw + x] = (x % 16 < 8) ^ (y % 16 < 8) ? 0xFF : 0xFFFFFFFF;
					break;
				default:
					break;
				}
			}
		}
		++textureNumber;
	}

	int w = surf->w;
	int h = surf->h;
	Uint32* surfPixels = (Uint32*)(surf->pixels);
	this->pixels = PixelBuffer<Color>(w, h);
	for (int y = 0; y < h; ++y) //convert SDL surface to buffer of floats and free the surface
	{
		for (int x = 0; x < w; ++x)
			this->pixels.setPixelUnsafe(x, y, Color(surfPixels[y * w + x]));
	}
	SDL_FreeSurface(surf);
}

Color Texture::getPixel(int x, int y, double lightMult) const
{
	StatCount(statsman.textures.pixelFetches++);
	int w = pixels.getW();
	int h = pixels.getH();
	x %= w;
	y %= h;
	if (x < 0) x += w; //this adds prevent reflection of wrapped coordinates around 0 
	if (y < 0) y += h;

	Color c = pixels.getPixelUnsafe(x, y); //due to manipulations with input x and y, it should never go out of bounds
	c.r *= lightMult;
	c.g *= lightMult;
	c.b *= lightMult;
	return c;
}
