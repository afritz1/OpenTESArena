#ifndef TEXTURE_H
#define TEXTURE_H

struct SDL_Texture;

// A thin SDL_Texture wrapper.
class Texture
{
private:
	SDL_Texture *texture;
public:
	Texture();
	Texture(const Texture&) = delete;
	Texture(Texture &&texture);
	~Texture();

	Texture &operator=(const Texture&) = delete;
	Texture &operator=(Texture &&texture);

	int getWidth() const;
	int getHeight() const;
	SDL_Texture *get() const;

	// Alternative to constructor to avoid accidentally copying pointers and double-freeing, etc..
	// Most code shouldn't touch a native texture directly.
	void init(SDL_Texture *texture);

	void destroy();
};

#endif
