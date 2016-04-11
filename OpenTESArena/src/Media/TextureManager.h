#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <map>
#include <string>
#include <vector>

class Surface;

enum class TextureName;
enum class TextureSequenceName;

struct SDL_PixelFormat;
struct SDL_Surface;

class TextureManager
{
private:
	std::map<TextureName, Surface> surfaces;
	std::map<TextureSequenceName, std::vector<Surface>> sequences;
	const SDL_PixelFormat *format;

	static const std::string PATH;

	SDL_Surface *loadFromFile(const std::string &filename);
public:
	TextureManager(const SDL_PixelFormat *format);
	~TextureManager();

	const SDL_PixelFormat *getFormat() const;
	const Surface &getSurface(TextureName name);
	const std::vector<Surface> &getSequence(TextureSequenceName name);
};

#endif
