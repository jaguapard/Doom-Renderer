#include "Texture.h"
#include <iostream>

Texture::Texture(std::string name)
{
	this->name = name;

	if (TEXTURE_DEBUG_MODE == TextureDebugMode::NONE)
	{
		std::string path = "D:/Games/GZDoom/Doom2_unpacked/graphics/" + name + ".png"; //TODO: doom uses TEXTURES lumps for some dark magic with them, this code does not work for unprepared textures.
		surf = IMG_Load(path.c_str());

		if (surf)
		{
			SDL_Surface* old = surf;
			surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_ABGR32, 0);
			SDL_FreeSurface(old);
		}
		else
		{
			std::cout << "Failed to load texture " << name << " from " << path << ": " << IMG_GetError() << "\n" <<
				"Falling back to purple-black checkerboard.\n\n";
			int w = 64, h = 64;

			surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ABGR32);
			for (int y = 0; y < h; ++y)
			{
				for (int x = 0; x < w; ++x)
				{
					Uint32* ptr = (Uint32*)(surf->pixels);
					ptr[y * w + x] = (x % 16 < 8) ^ (y % 16 < 8) ? 0xFF : 0xFF00FFFF;
				}
			}
		}
	}
	else
	{
		surf = SDL_CreateRGBSurfaceWithFormat(0, 1024, 1024, 32, SDL_PIXELFORMAT_ABGR32);
		int tw = 1024, th = 1024;
		Uint32 dbg_colors[] = { WHITE, GREY, RED, GREEN, BLUE, YELLOW };
		static int textureNumber = 0;
		for (int y = 0; y < tw; ++y)
		{
			for (int x = 0; x < th; ++x)
			{
				Uint32* px = (Uint32*)(surf->pixels);
				Uint8 bx = x, by = y;
				switch (TEXTURE_DEBUG_MODE)
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
}

uint32_t Texture::getPixel(int x, int y)
{
	x = abs(x);
	y = abs(y);
	x %= surf->w;
	y %= surf->h;
	uint32_t* px = (uint32_t*)surf->pixels;
	return px[y * surf->w + x];
}

Texture::~Texture()
{
	//SDL_FreeSurface(surf);
}
