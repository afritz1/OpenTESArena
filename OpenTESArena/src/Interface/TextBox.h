#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include <string>
#include <vector>

#include "RichTextString.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"

// Redesigned for use with the font system using Arena assets.

class Rect;
class Renderer;

struct SDL_Surface;
struct SDL_Texture;

class TextBox
{
private:
	RichTextString richText;
	SDL_Surface *surface; // For ListBox compatibility. Identical to "texture".
	SDL_Texture *texture, *shadowTexture;
	int x, y;
public:
	TextBox(int x, int y, const RichTextString &richText, const Color &shadowColor,
		Renderer &renderer);
	TextBox(const Int2 &center, const RichTextString &richText, const Color &shadowColor,
		Renderer &renderer);
	TextBox(int x, int y, const RichTextString &richText, Renderer &renderer);
	TextBox(const Int2 &center, const RichTextString &richText, Renderer &renderer);
	virtual ~TextBox();

	int getX() const;
	int getY() const;
	const RichTextString &getRichText() const;

	// Gets the bounding box around the text box's content. Useful for tooltips when hovering
	// over it with the mouse.
	Rect getRect() const;

	SDL_Surface *getSurface() const;
	SDL_Texture *getTexture() const;

	// Gets the copy of the text box texture that uses the shadow color for text.
	SDL_Texture *getShadowTexture() const;
};

#endif
