#ifndef TEXTURE_H
#define TEXTURE_H

// Wrapper class for SDL_Texture. Unrelated to TextureReferences.

// Not meant for anything clever; it's just a simple container.

struct SDL_Texture;

class Texture
{
private:
	SDL_Texture *texture;
public:
	Texture(SDL_Texture *texture);
	Texture(const Texture&) = delete;
	Texture(Texture &&texture);
	~Texture();

	Texture &operator=(const Texture&) = delete;
	Texture &operator=(Texture&&) = delete;

	int getWidth() const;
	int getHeight() const;
	SDL_Texture *get() const;
};

#endif
