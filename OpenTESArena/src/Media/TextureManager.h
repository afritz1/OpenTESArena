#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <array>
#include <map>
#include <string>
#include <unordered_map>

#include "Color.h"
#include "../Interface/Surface.h"

// The behavior of this class might be changing to work with BSA parsing and
// associated files.

// When loading wall and sprite textures from file, use a texture name parser or
// something that produces a texture/index -> filename mapping. Each of those would 
// get a unique integer ID either before or during parsing (perhaps depending on 
// the order they were parsed). Perhaps the ID could be their offset in GLOBAL.BSA.

class Renderer;

enum class PaletteName;

struct SDL_PixelFormat;
struct SDL_Surface;
struct SDL_Texture;

class TextureManager
{
private:
	typedef std::array<Color, 256> Palette;

	static const std::string PATH;

	std::map<PaletteName, Palette> palettes;
	std::unordered_map<std::string, std::map<PaletteName, Surface>> surfaces;
	std::unordered_map<std::string, std::map<PaletteName, SDL_Texture*>> textures;
	std::unordered_map<std::string, std::map<PaletteName, std::vector<Surface>>> surfaceSets;
	std::unordered_map<std::string, std::map<PaletteName, std::vector<SDL_Texture*>>> textureSets;
	Renderer &renderer;
	PaletteName activePalette;

	std::vector<SDL_Surface*> loadCFA(const std::string &filename, PaletteName paletteName);
	std::vector<SDL_Surface*> loadCIF(const std::string &filename, PaletteName paletteName);
	std::vector<SDL_Surface*> loadDFA(const std::string &filename, PaletteName paletteName);
	std::vector<SDL_Surface*> loadFLC(const std::string &filename);
	SDL_Surface *loadIMG(const std::string &filename, PaletteName paletteName);
	SDL_Surface *loadPNG(const std::string &fullPath);
	std::vector<SDL_Surface*> loadSET(const std::string &filename, PaletteName paletteName);

	// Initialize the given palette with a certain palette from file.
	void initPalette(Palette &palette, PaletteName paletteName);
public:
	TextureManager(Renderer &renderer);
	~TextureManager();

	TextureManager &operator =(TextureManager &&textureManager);

	// Gets a surface from file. It will be loaded if not already stored with the 
	// requested palette. A valid filename might be something like "TAMRIEL.IMG".
	const Surface &getSurface(const std::string &filename, PaletteName paletteName);
	const Surface &getSurface(const std::string &filename);

	// Similar to getSurface(), only now for hardware-accelerated textures.
	SDL_Texture *getTexture(const std::string &filename, PaletteName paletteName);
	SDL_Texture *getTexture(const std::string &filename);

	// Gets a set of surfaces from a file. This is intended for animations and movies,
	// where the filename essentially points to several images.
	const std::vector<Surface> &getSurfaces(const std::string &filename,
		PaletteName paletteName);
	const std::vector<Surface> &getSurfaces(const std::string &filename);

	// Similar to getSurfaces(), only now for a series of hardware-accelerated textures.
	const std::vector<SDL_Texture*> &getTextures(const std::string &filename,
		PaletteName paletteName);
	const std::vector<SDL_Texture*> &getTextures(const std::string &filename);

	// Sets the palette for subsequent surfaces and textures. If a requested image 
	// is not currently loaded for the active palette, it is loaded from file.
	void setPalette(PaletteName paletteName);

	// To do: remove this method once FLC movies can be loaded through "getTextures()".
	// Since cinematics are now loaded image by image instead of all at the same time,
	// there may be some stuttering that occurs. This method loads all of the sequences
	// into memory at the same time to compensate.
	void preloadSequences();
};

#endif
