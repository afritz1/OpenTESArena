#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <array>
#include <map>
#include <string>
#include <unordered_map>

#include "Color.h"

// The behavior of this class might be changing to work with BSA parsing and
// associated files.

// When loading wall and sprite textures from file, use a texture name parser or
// something that produces a texture/index -> filename mapping. Each of those would 
// get a unique integer ID either before or during parsing (perhaps depending on 
// the order they were parsed). Perhaps the ID could be their offset in GLOBAL.BSA.

class Surface;

enum class PaletteName;

struct SDL_PixelFormat;
struct SDL_Renderer;
struct SDL_Surface;
struct SDL_Texture;

// Hash function for the unordered map.
// From http://stackoverflow.com/questions/18098178/how-do-i-use-unordered-set.
namespace std
{
	template <>
	struct hash<std::pair<std::string, PaletteName>>
	{
		size_t operator()(const std::pair<std::string, PaletteName> &pair) const
		{
			size_t k1 = std::hash<std::string>()(pair.first);
			size_t k2 = 51 + std::hash<PaletteName>()(pair.second);
			return k1 * k2;
		}
	};
}

class TextureManager
{
private:
	typedef std::array<Color, 256> Palette;

	static const std::string PATH;

	std::map<PaletteName, Palette> palettes;
	std::unordered_map<std::pair<std::string, PaletteName>, Surface> surfaces;
	std::unordered_map<std::pair<std::string, PaletteName>, SDL_Texture*> textures;
	const SDL_Renderer *renderer;
	const SDL_PixelFormat *format;
	PaletteName activePalette;

	SDL_Surface *loadPNG(const std::string &fullPath);
	SDL_Surface *loadIMG(const std::string &filename, PaletteName paletteName);
	// Perhaps methods like "loadDFA" and "loadCIF" would return a vector of surfaces.

	// Initialize the given palette with a certain palette from file.
	void initPalette(Palette &palette, PaletteName paletteName);
public:
	TextureManager(const SDL_Renderer *renderer, const SDL_PixelFormat *format);
	~TextureManager();

	const SDL_PixelFormat *getFormat() const;

	// Gets a surface from the texture manager. It will be loaded from file if not
	// already stored. A valid filename might be something like "TAMRIEL.IMG".
	const Surface &getSurface(const std::string &filename, PaletteName paletteName);
	const Surface &getSurface(const std::string &filename);

	// Similar to getSurface(), only now for hardware-accelerated textures.
	const SDL_Texture *getTexture(const std::string &filename, PaletteName paletteName);
	const SDL_Texture *getTexture(const std::string &filename);

	// Sets the palette for subsequent surfaces and textures. If a requested image 
	// is not currently loaded for the active palette, it is loaded from file.
	void setPalette(PaletteName paletteName);

	// Since cinematics are now loaded image by image instead of all at the same time,
	// there may be some stuttering that occurs. This method loads all of the sequences
	// into memory at the same time to compensate.
	void preloadSequences();

	// This method might be necessary when resizing the window, if SDL causes all of
	// the textures to become black.
	void reloadTextures(SDL_Renderer *renderer);
};

#endif
