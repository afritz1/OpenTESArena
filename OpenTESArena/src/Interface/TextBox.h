#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include <string>
#include <vector>

#include "../Math/Vector2.h"
#include "../Media/Color.h"

// Redesigned for use with the font system using Arena assets.

class Font;
class Rect;
class Renderer;

enum class FontName;
enum class TextAlignment;

struct SDL_Surface;
struct SDL_Texture;

class TextBox
{
private:
	SDL_Surface *surface; // For ListBox compatibility. Identical to "texture".
	SDL_Texture *texture, *shadowTexture;
	Color textColor;
	FontName fontName;
	TextAlignment alignment;
	int x, y;
public:
	TextBox(int x, int y, const Color &textColor, const Color &shadowColor,
		const std::string &text, const Font &font, TextAlignment alignment, Renderer &renderer);
	TextBox(const Int2 &center, const Color &textColor, const Color &shadowColor, 
		const std::string &text, const Font &font, TextAlignment alignment, Renderer &renderer);
	TextBox(int x, int y, const Color &textColor, const std::string &text,
		const Font &font, TextAlignment alignment, Renderer &renderer);
	TextBox(const Int2 &center, const Color &textColor, const std::string &text,
		const Font &font, TextAlignment alignment, Renderer &renderer);
	virtual ~TextBox();

	int getX() const;
	int getY() const;
	TextAlignment getAlignment() const;
	FontName getFontName() const;
	const Color &getTextColor() const;

	// Gets the bounding box around the text box's content. Useful for tooltips when hovering
	// over it with the mouse.
	Rect getRect() const;

	SDL_Surface *getSurface() const;
	SDL_Texture *getTexture() const;

	// Gets the copy of the text box texture that uses the shadow color for text.
	SDL_Texture *getShadowTexture() const;
};

#endif
