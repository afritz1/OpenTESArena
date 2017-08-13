#ifndef RICH_TEXT_STRING_H
#define RICH_TEXT_STRING_H

#include <string>
#include <vector>

#include "../Math/Vector2.h"
#include "../Media/Color.h"

// A formatted string for use with Arena's text boxes.

class Font;
class FontManager;

enum class FontName;
enum class TextAlignment;

struct SDL_Surface;

class RichTextString
{
private:
	std::vector<std::vector<const SDL_Surface*>> surfaceLists; // Surfaces for each line of text.
	std::string text;
	FontName fontName;
	Color color;
	Int2 dimensions;
	TextAlignment alignment;
	int lineSpacing; // Pixel padding between lines.
public:
	RichTextString(const std::string &text, FontName fontName, const Color &color,
		TextAlignment alignment, int lineSpacing, FontManager &fontManager);
	RichTextString(const std::string &text, FontName fontName, const Color &color,
		TextAlignment alignment, FontManager &fontManager);
	~RichTextString();

	const std::vector<std::vector<const SDL_Surface*>> &getSurfaceLists() const;
	const std::string &getText() const;
	FontName getFontName() const;
	const Color &getColor() const;
	const Int2 &getDimensions() const;
	TextAlignment getAlignment() const;
	int getLineSpacing() const;
};

#endif
