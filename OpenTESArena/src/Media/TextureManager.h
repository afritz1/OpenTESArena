#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <array>
#include <map>
#include <string>

#include "Color.h"

// The behavior of this class might be changing to work with BSA parsing and
// associated files.

// When loading wall and sprite textures from file, use a texture name parser or
// something that produces a texture/index -> filename mapping. Each of those would 
// get a unique integer ID either before or during parsing (perhaps depending on 
// the order they were parsed). Perhaps the ID could be their offset in GLOBAL.BSA.

class Surface;

struct SDL_PixelFormat;
struct SDL_Renderer;
struct SDL_Surface;
struct SDL_Texture;

class TextureManager
{
private:
	typedef std::array<Color, 256> Palette;

	static const std::string PATH;

	Palette palette;
	// Maybe the surfaces should take a pair of strings: filename and palette name.
	// If no palette name is given in a load method, then assume PAL.COL.
	std::map<std::string, Surface> surfaces;
	std::map<std::string, SDL_Texture*> textures;
	const SDL_Renderer *renderer;
	const SDL_PixelFormat *format;

    SDL_Surface *loadPNG(const std::string &fullPath);
	SDL_Surface *loadIMG(const std::string &fullPath);
	// Perhaps methods like "loadDFA" and "loadCIF" would return a vector of surfaces.
public:
	TextureManager(const SDL_Renderer *renderer, const SDL_PixelFormat *format);
	~TextureManager();

	const SDL_PixelFormat *getFormat() const;
		
	// Gets a surface from the texture manager. It will be loaded from file if not
	// already stored. A valid filename might be something like "TAMRIEL.IMG".
	const Surface &getSurface(const std::string &filename);

	// Similar to getSurface(), only for hardware-accelerated textures.
	const SDL_Texture *getTexture(const std::string &filename);

	// Set the currently used palette (default is PAL.COL).
	void setPalette(const std::string &paletteName);

	// Since cinematics are now loaded image by image instead of all at the same time,
	// there may be some stuttering that occurs. This method loads all of the sequences
	// into memory at the same time to compensate.
	void preloadSequences();

	// This method might be necessary when resizing the window, if SDL causes all of
	// the textures to become black.
	void reloadTextures(SDL_Renderer *renderer);
};

#endif
