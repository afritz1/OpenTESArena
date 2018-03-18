#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include <string>
#include <vector>

#include "RichTextString.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Rendering/Surface.h"
#include "../Rendering/Texture.h"

// Redesigned for use with the font system using Arena assets.

class Rect;
class Renderer;

struct SDL_Surface;
struct SDL_Texture;

class TextBox
{
public:
	// Data for the text box's shadow (if any).
	struct ShadowData
	{
		Color color;
		Int2 offset;

		ShadowData(const Color &color, const Int2 &offset)
			: color(color), offset(offset) { }
	};
private:
	RichTextString richText;
	Surface surface; // For ListBox compatibility. Identical to "texture".
	Texture texture;
	int x, y;
public:
	TextBox(int x, int y, const RichTextString &richText, const ShadowData *shadow,
		Renderer &renderer);
	TextBox(const Int2 &center, const RichTextString &richText, const ShadowData *shadow,
		Renderer &renderer);
	TextBox(int x, int y, const RichTextString &richText, Renderer &renderer);
	TextBox(const Int2 &center, const RichTextString &richText, Renderer &renderer);

	int getX() const;
	int getY() const;
	const RichTextString &getRichText() const;

	// Gets the bounding box around the text box's content. Useful for tooltips when hovering
	// over it with the mouse.
	Rect getRect() const;

	SDL_Surface *getSurface() const;
	SDL_Texture *getTexture() const;
};

#endif
