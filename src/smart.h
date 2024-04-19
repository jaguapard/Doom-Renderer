#pragma once
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <memory>

struct FontDeleter
{
	void operator()(TTF_Font* f) 
	{
		TTF_CloseFont(f);
	}
};

struct SurfaceDeleter
{
	void operator()(SDL_Surface* s) 
	{
		SDL_FreeSurface(s);
	}
};

typedef std::unique_ptr<TTF_Font, FontDeleter> Smart_Font;
typedef std::unique_ptr<SDL_Surface, SurfaceDeleter> Smart_Surface;