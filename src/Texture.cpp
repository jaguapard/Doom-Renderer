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
			this->pixels = PixelBuffer<Color>(w, h);

			for (int y = 0; y < h; ++y)
			{
				for (int x = 0; x < w; ++x)
				{
					this->pixels.setPixel(x, y, surfPixels[y * w + x]);
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

Color Texture::getPixel(const Vec4& coords) const
{
	StatCount(statsman.textures.pixelFetches++);

	__m128i intCoords = _mm_cvttps_epi32(coords);
	__m128i intCoords64 = _mm_cvtepi32_epi64(intCoords);

	//expanding intCoords basically acts like a shuffle to put dwords in places where mul expects them to be (that intrinsic is a little bit screwed up)
	__m128i divPrefab64 = _mm_mul_epi32(intCoords64, pixels.getSize().dimensionsIntReciprocal64);
	__m128i div64 = _mm_srli_epi64(divPrefab64, 32);

	__m128i dimInt64 = pixels.getSize().dimensionsInt64;
	__m128i mod = _mm_sub_epi64(intCoords64, _mm_mul_epi32(div64, dimInt64));

	__m128i cmp = _mm_cmplt_epi32(mod, _mm_setzero_si128());
	__m128i add = _mm_and_si128(cmp, dimInt64);
	__m128i textureCoords = _mm_add_epi32(mod, add);

	return pixels.getPixel64(textureCoords);
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

Color Texture::getPixelAtUV(real u, real v) const
{
	return this->getPixel(u * pixels.getW(), v * pixels.getH());
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