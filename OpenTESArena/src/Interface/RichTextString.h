#ifndef RICH_TEXT_STRING_H
#define RICH_TEXT_STRING_H

#include <string>

#include "../Media/Color.h"

// A formatted string for use with Arena's text boxes.

enum class FontName;
enum class TextAlignment;

class RichTextString
{
private:
	std::string text;
	FontName fontName;
	Color color;
	TextAlignment alignment;
	int lineSpacing; // Pixel padding between lines.
public:
	RichTextString(const std::string &text, FontName fontName, const Color &color,
		TextAlignment alignment, int lineSpacing);
	RichTextString(const std::string &text, FontName fontName, const Color &color,
		TextAlignment alignment);
	~RichTextString();

	const std::string &getText() const;
	FontName getFontName() const;
	const Color &getColor() const;
	TextAlignment getAlignment() const;
	int getLineSpacing() const;
};

#endif
