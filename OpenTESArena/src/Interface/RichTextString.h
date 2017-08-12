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

	const std::string &getText() const;
	FontName getFontName() const;
	const Color &getColor() const;
	const Int2 &getDimensions() const;
	TextAlignment getAlignment() const;
	int getLineSpacing() const;

	// Splits the text into separate lines based on each newline character. The vector
	// is guaranteed to have at least one non-empty string.
	static std::vector<std::string> splitLines(const std::string &text);

	// Gets the surfaces used for each character of the given text. Intended for a
	// single line of text (that is, text without newlines).
	static std::vector<const SDL_Surface*> getTextSurfaces(
		const std::string &line, const Font &font);
};

#endif
