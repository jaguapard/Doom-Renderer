#include <cassert>
#include "Texture.h"
#include <iostream>
#include "Vec.h"
#include "Statsman.h"
#include "smart.h"

Texture::Texture(std::string name, bool useNameAsPath)
{
	this->name = name;
	if (TEXTURE_DEBUG_MODE != TextureDebugMode::NONE)
	{
		this->constructDebugTexture();
	}
	else
	{
		std::string path;
		if (useNameAsPath) path = name; //TODO: a dirty hack to tell absolute paths from relative 
		else path = "data/graphics/" + name + ".png"; //TODO: doom uses TEXTURES lumps for some dark magic with them, this code does not work for unprepared textures.

		Smart_Surface surf = Smart_Surface(IMG_Load(path.c_str()));

		if (surf)
		{
			Smart_Surface nSurf = Smart_Surface(SDL_ConvertSurfaceFormat(surf.get(), SDL_PIXELFORMAT_RGBA32, 0)); //force convert into this format, since that's the only one we support
			Uint32* surfPixels = reinterpret_cast<Uint32*>(nSurf->pixels);
			int w = nSurf->w;
			int h = nSurf->h;

			int downSamplingMult = 1;
			this->pixels = FloatColorBuffer(w, h);

			for (int y = 0; y < h; ++y)
			{
				for (int x = 0; x < w; ++x)
				{
					this->pixels.setPixel(x, y, surfPixels[y * (w / downSamplingMult) * downSamplingMult + (x / downSamplingMult) * downSamplingMult]);
				}
			}
		}
		else
		{
			std::cout << "Failed to load texture " << name << " from " << path << ": " << IMG_GetError() << "\n" <<
				"Falling back to purple-black checkerboard.\n\n";
			int w = 64, h = 64;

			this->pixels = FloatColorBuffer(w, h);
			for (int y = 0; y < h; ++y)
			{
				for (int x = 0; x < w; ++x)
				{
					Color c = (x % 16 < 8) ^ (y % 16 < 8) ? Color(0, 0, 0) : Color(255, 0, 255);
					this->pixels.setPixel(x, y, c);
				}
			}
		}
	}

	this->checkForTransparentPixels();
}

VectorPack16 Texture::gatherPixels512(const FloatPack16& u, const FloatPack16& v, const Mask16& mask) const
{
	StatCount(statsman.textures.pixelFetches += 16; statsman.textures.gathers++);
	FloatPack16 uFloor = _mm512_floor_ps(u);
	FloatPack16 vFloor = _mm512_floor_ps(v);

	FloatPack16 uFrac = u - uFloor;
	FloatPack16 vFrac = v - vFloor;

	FloatPack16 xPixelPos = uFrac * pixels.getSize().fw;
	FloatPack16 yPixelPos = vFrac * pixels.getSize().fh;

	__m512i xCoords = _mm512_cvttps_epi32(xPixelPos);
	__m512i yCoords = _mm512_cvttps_epi32(yPixelPos);
	return pixels.gatherPixels16(xCoords, yCoords, mask & (uFrac < 1) & (vFrac < 1));
}


int Texture::getW() const
{
	return pixels.getW();
}

int Texture::getH() const
{
	return pixels.getH();
}

bool Texture::hasOnlyOpaquePixels() const
{
	return _hasOnlyOpaquePixels;
}

void Texture::checkForTransparentPixels()
{
	int w = pixels.getW();
	int h = pixels.getH();
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			Vec4 color = pixels.getPixelAsVec4(x, y);
			if (color.w < 1)
			{
				_hasOnlyOpaquePixels = false;
				return;
			}
		}
	}

	_hasOnlyOpaquePixels = true;
}

void Texture::constructDebugTexture()
{
	int tw = 1024, th = 1024;
	this->pixels = FloatColorBuffer(tw, th);

	//WHITE, GREY, RED, GREEN, BLUE, YELLOW
	Color dbg_colors[] = { {255,255,255}, {127,127,127}, {255,0,0}, {0,255,0}, {0,0,255}, {255,255,0} };
	static int textureNumber = 0;
	for (int y = 0; y < tw; ++y)
	{
		for (int x = 0; x < th; ++x)
		{
			switch (TEXTURE_DEBUG_MODE)
			{
			case TextureDebugMode::NONE:
				assert((false, "Debug texture build attempted without debug texture mode set"));
				return;
			case TextureDebugMode::X_GRADIENT:
				pixels.setPixel(x, y, Color(x, x, x, SDL_ALPHA_OPAQUE));
				break;
			case TextureDebugMode::Y_GRADIENT:
				pixels.setPixel(x, y, Color(y, y, y, SDL_ALPHA_OPAQUE));
				break;
			case TextureDebugMode::XY_GRADIENT:
				pixels.setPixel(x, y, Color(x, y, 0, SDL_ALPHA_OPAQUE));
				break;
			case TextureDebugMode::SOLID_WHITE:
				pixels.setPixel(x, y, Color(255, 255, 255, SDL_ALPHA_OPAQUE));
				break;
			case TextureDebugMode::COLORS:
				pixels.setPixel(x, y, dbg_colors[textureNumber % std::size(dbg_colors)]);
				break;
			case TextureDebugMode::CHECKERBOARD:
				Color c = (x % 16 < 8) ^ (y % 16 < 8) ? Color(0, 0, 0) : Color(255, 255, 255);
				pixels.setPixel(x, y, c);
				break;
				//default:
				//	break;
			}
		}
	}
	++textureNumber;
}