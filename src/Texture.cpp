#include <cassert>
#include "Texture.h"
#include <iostream>
#include "Vec.h"
#include "Statsman.h"
#include "smart.h"

Texture::Texture(std::string name)
{
	this->name = name;
	if (TEXTURE_DEBUG_MODE != TextureDebugMode::NONE)
	{
		this->constructDebugTexture();
	}
	else
	{
		std::string path;
		if (name.size() > 1 && name[1] == ':') path = name; //TODO: a dirty hack to tell absolute paths from relative 
		else path = "data/graphics/" + name + ".png"; //TODO: doom uses TEXTURES lumps for some dark magic with them, this code does not work for unprepared textures.

		Smart_Surface surf = Smart_Surface(IMG_Load(path.c_str()));

		if (surf)
		{
			Smart_Surface nSurf = Smart_Surface(SDL_ConvertSurfaceFormat(surf.get(), SDL_PIXELFORMAT_RGBA32, 0)); //force convert into this format, since that's the only one we support
			Uint32* surfPixels = reinterpret_cast<Uint32*>(nSurf->pixels);
			int w = nSurf->w;
			int h = nSurf->h;

			int downSamplingMult = 4;
			this->pixels = PixelBuffer<Color>(w/downSamplingMult, h/downSamplingMult);

			for (int y = 0; y < h/downSamplingMult; ++y)
			{
				for (int x = 0; x < w/downSamplingMult; ++x)
				{
					this->pixels.setPixel(x, y, surfPixels[y * w*downSamplingMult + x*downSamplingMult]);
				}
			}
		}
		else
		{
			std::cout << "Failed to load texture " << name << " from " << path << ": " << IMG_GetError() << "\n" <<
				"Falling back to purple-black checkerboard.\n\n";
			int w = 64, h = 64;

			this->pixels = PixelBuffer<Color>(w, h);
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

Color Texture::getPixelAtUV(const Vec4& uv) const
{
	StatCount(statsman.textures.pixelFetches++);
#if SSE_VER >= 41
	//rounding to negative infinity forces the frac calculation to always result in positive values in range (0..1),
	//allowing us to skip the sign check and adjusting coordinates passed to pixels.getPixel. Truncating doesn't allow this
	Vec4 floor = _mm_round_ps(uv, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
	Vec4 frac = uv - floor;
	return pixels.getPixel(frac * pixels.getSize().dimensionsFloat);
#else
	return getPixel(uv.x * getW(), uv.y * getH());
#endif
}

Color Texture::getPixel(int x, int y) const
{
	StatCount(statsman.textures.pixelFetches++);

	int w = pixels.getW();
	int h = pixels.getH();
	x %= w; //TODO: this is very slow. Our textures do not change size at runtime, so it can be optimized by fast integer modulo techiques
	y %= h;
	if (x < 0) x += w; //can't just flip the sign of modulo - that will make textures reflect around 0, i.e. x=-1 will map to 1 instead of (w-1)
	if (y < 0) y += h;
	return pixels.getPixel(x, y); //due to previous manipulations with input x and y, it should never go out of bounds
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
	for (const auto it : pixels)
	{
		if (it.a < SDL_ALPHA_OPAQUE)
		{
			_hasOnlyOpaquePixels = false;
			break;
		}
	}
	_hasOnlyOpaquePixels = true;
}

void Texture::constructDebugTexture()
{
	int tw = 1024, th = 1024;
	this->pixels = PixelBuffer<Color>(tw, th);

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