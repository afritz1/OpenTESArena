#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <map>
#include <string>
#include <unordered_map>

#include "Palette.h"

// Find a way to map original wall and sprite filenames to unique integer IDs 
// (probably depending on the order they were parsed). Or perhaps the ID could 
// be their offset in GLOBAL.BSA.

class Renderer;
class Surface;

enum class PaletteName;

struct SDL_PixelFormat;
struct SDL_Surface;
struct SDL_Texture;

class TextureManager
{
private:
	static const std::string PATH;

	std::map<PaletteName, Palette> palettes;
	std::unordered_map<std::string, std::map<PaletteName, Surface>> surfaces;
	std::unordered_map<std::string, std::map<PaletteName, SDL_Texture*>> textures;
	std::unordered_map<std::string, std::map<PaletteName, std::vector<Surface>>> surfaceSets;
	std::unordered_map<std::string, std::map<PaletteName, std::vector<SDL_Texture*>>> textureSets;
	Renderer &renderer;
	PaletteName activePalette;
		
	SDL_Surface *loadPNG(const std::string &fullPath);

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
