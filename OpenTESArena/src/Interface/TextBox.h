#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include <string>
#include <vector>

// Redesigned for use with the font system using Arena assets.

class Color;
class Font;
class Int2;
class Renderer;

enum class TextAlignment;

struct SDL_Surface;
struct SDL_Texture;

class TextBox
{
private:
	SDL_Surface *surface; // For ListBox compatibility. Identical to "texture".
	SDL_Texture *texture;
	int x, y;
public:
	TextBox(int x, int y, const Color &textColor, const std::string &text,
		const Font &font, TextAlignment alignment, Renderer &renderer);
	TextBox(const Int2 &center, const Color &textColor, const std::string &text,
		const Font &font, TextAlignment alignment, Renderer &renderer);
	virtual ~TextBox();

	int getX() const;
	int getY() const;
	SDL_Surface *getSurface() const;
	SDL_Texture *getTexture() const;
};

#endif
