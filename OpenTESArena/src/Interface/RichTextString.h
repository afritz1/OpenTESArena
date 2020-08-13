#ifndef RICH_TEXT_STRING_H
#define RICH_TEXT_STRING_H

#include <string>
#include <vector>

#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontDefinition.h"

// A formatted string for use with Arena's text boxes.

class FontLibrary;

enum class FontName;
enum class TextAlignment;

struct SDL_Surface;

class RichTextString
{
private:
	std::vector<std::vector<FontDefinition::CharID>> characterLists; // Font indices for each line of text.
	std::vector<int> lineWidths; // Width in pixels for each line of surfaces.
	std::string text;
	FontName fontName;
	Color color;
	Int2 dimensions;
	TextAlignment alignment;
	int lineSpacing; // Pixel padding between lines.
	int characterHeight;
public:
	RichTextString(const std::string &text, FontName fontName, const Color &color,
		TextAlignment alignment, int lineSpacing, const FontLibrary &fontLibrary);
	RichTextString(const std::string &text, FontName fontName, const Color &color,
		TextAlignment alignment, const FontLibrary &fontLibrary);

	const std::vector<std::vector<FontDefinition::CharID>> &getCharacterLists() const;
	const std::vector<int> &getLineWidths() const;
	const std::string &getText() const;
	FontName getFontName() const;
	const Color &getColor() const;
	const Int2 &getDimensions() const;
	TextAlignment getAlignment() const;
	int getLineSpacing() const;
	int getCharacterHeight() const;
};

#endif
