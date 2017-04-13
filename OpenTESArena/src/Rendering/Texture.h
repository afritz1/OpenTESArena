#ifndef TEXTURE_H
#define TEXTURE_H

// Wrapper class for SDL_Texture. Unrelated to TextureReferences.

// Not meant for anything clever; it's just a simple container.

class Renderer;
class TextureManager;

struct SDL_Texture;

class Texture
{
private:
	SDL_Texture *texture;
public:
	// Generated texture types. These refer to patterns used with pop-ups and buttons.
	enum class PatternType
	{
		Parchment,
		Dark,
		Custom1 // Light gray with borders.
	};

	Texture(SDL_Texture *texture);
	Texture(const Texture&) = delete;
	Texture(Texture &&texture);
	~Texture();

	Texture &operator=(const Texture&) = delete;
	Texture &operator=(Texture&&) = delete;

	// Generates a new texture from a pattern.
	static SDL_Texture *generate(Texture::PatternType type, int width, int height,
		TextureManager &textureManager, Renderer &renderer);

	int getWidth() const;
	int getHeight() const;
	SDL_Texture *get() const;
};

#endif
