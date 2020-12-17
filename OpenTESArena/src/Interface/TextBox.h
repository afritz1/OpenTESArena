#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include <string>
#include <vector>

#include "RichTextString.h"
#include "Surface.h"
#include "Texture.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"

// Redesigned for use with the font system using Arena assets.

class FontLibrary;
class Rect;
class Renderer;

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
	// Default number of characters per line before a newline occurs.
	static constexpr int DEFAULT_TEXT_WRAP = 60;

	TextBox(int x, int y, const RichTextString &richText, const ShadowData *shadow,
		const FontLibrary &fontLibrary, Renderer &renderer);
	TextBox(const Int2 &center, const RichTextString &richText, const ShadowData *shadow,
		const FontLibrary &fontLibrary, Renderer &renderer);
	TextBox(int x, int y, const RichTextString &richText, const FontLibrary &fontLibrary,
		Renderer &renderer);
	TextBox(const Int2 &center, const RichTextString &richText, const FontLibrary &fontLibrary,
		Renderer &renderer);

	int getX() const;
	int getY() const;
	const RichTextString &getRichText() const;

	// Gets the bounding box around the text box's content. Useful for tooltips when hovering
	// over it with the mouse.
	Rect getRect() const;

	const Surface &getSurface() const;
	const Texture &getTexture() const;
};

#endif
