#ifndef TEXTURE_H
#define TEXTURE_H

// A thin SDL_Texture wrapper.

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

	Texture();
	Texture(const Texture&) = delete;
	Texture(Texture &&texture);
	~Texture();

	Texture &operator=(const Texture&) = delete;
	Texture &operator=(Texture &&texture);

	// Generates a new texture from a pattern.
	static Texture generate(Texture::PatternType type, int width, int height,
		TextureManager &textureManager, Renderer &renderer);

	int getWidth() const;
	int getHeight() const;
	SDL_Texture *get() const;

	// Alternative to constructor to avoid accidentally copying pointers and double-freeing, etc..
	// Most code shouldn't touch a native texture directly.
	void init(SDL_Texture *texture);

	void clear();
};

#endif
