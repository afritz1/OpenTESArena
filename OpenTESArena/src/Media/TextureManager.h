#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <map>
#include <string>

#include "TextureName.h"

class Surface;

struct SDL_PixelFormat;
struct SDL_Surface;

class TextureManager
{
private:
	std::map<TextureName, Surface> surfaces;
	const SDL_PixelFormat *format;

	static const std::string PATH;

	SDL_Surface *loadFromFile(TextureName name);
public:
	TextureManager(const SDL_PixelFormat *format);
	~TextureManager();

	const SDL_PixelFormat *getFormat() const;
	const Surface &getSurface(TextureName name);
};

#endif