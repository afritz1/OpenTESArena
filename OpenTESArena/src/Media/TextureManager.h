#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "Palette.h"
#include "../Rendering/Texture.h"

// Find a way to map original wall and sprite filenames to unique integer IDs 
// (probably depending on the order they were parsed). Or perhaps the ID could 
// be their offset in GLOBAL.BSA.

class Renderer;

struct SDL_Surface;

class TextureManager
{
private:
	std::unordered_map<std::string, Palette> palettes;

	// The filename and palette name are concatenated when mapping to avoid using two 
	// maps. I.e., "EQUIPMEN.IMG" and "PAL.COL" become "EQUIPMEN.IMGPAL.COL".
	std::unordered_map<std::string, SDL_Surface*> surfaces;
	std::unordered_map<std::string, Texture> textures;
	std::unordered_map<std::string, std::vector<SDL_Surface*>> surfaceSets;
	std::unordered_map<std::string, std::vector<Texture>> textureSets;
	Renderer &renderer;
	std::string activePalette;

	// Specialty method for loading a COL file into the palettes map.
	void loadCOLPalette(const std::string &colName);

	// Specialty method for loading the palette from an IMG file into the palettes map.
	void loadIMGPalette(const std::string &imgName);

	// Helper method for loading a palette file into the palettes map.
	void loadPalette(const std::string &paletteName);
public:
	TextureManager(Renderer &renderer);
	~TextureManager();

	TextureManager &operator=(TextureManager &&textureManager) = delete;

	// Gets a surface from file. It will be loaded if not already stored with the 
	// requested palette. A valid filename might be something like "TAMRIEL.IMG".
	SDL_Surface *getSurface(const std::string &filename, const std::string &paletteName);
	SDL_Surface *getSurface(const std::string &filename);

	// Similar to getSurface(), only now for hardware-accelerated textures.
	const Texture &getTexture(const std::string &filename, const std::string &paletteName);
	const Texture &getTexture(const std::string &filename);
	
	// Gets a set of surfaces from a file. Intended only for obtaining pixel data for use 
	// with renderer buffers. TextureManager::getTextures() should be used instead for 
	// any 2D interface objects.
	const std::vector<SDL_Surface*> &getSurfaces(const std::string &filename,
		const std::string &paletteName);
	const std::vector<SDL_Surface*> &getSurfaces(const std::string &filename);

	// Gets a set of textures from a file. This is intended for animations and movies, 
	// where the filename essentially points to several images. When no palette name 
	// is given, the active one is used.
	const std::vector<Texture> &getTextures(const std::string &filename,
		const std::string &paletteName);
	const std::vector<Texture> &getTextures(const std::string &filename);

	// Sets the palette to use for subsequent images. The source of the palette can be
	// from a loose .COL file, or can be built into an IMG. If the IMG does not have a 
	// built-in palette, an error occurs.
	void setPalette(const std::string &filename);
};

#endif
