#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include <memory>
#include <string>
#include <vector>

#include "../Media/FontName.h"
#include "Surface.h"

// A scrollable text box could just have a text box surface, and then use a Rectangle
// to use only the visible part of it. The scroll bar can be thought of as a kind of
// "sliding window"; the size of the clickable scroll bar is the percentage of the lines 
// shown, and the position would follow a similar pattern.

class Color;
class TextureManager;

class TextBox : public Surface
{
private:
	FontName fontName;

	// Gets the distance a new line jumps down in pixels.
	int getNewLineHeight(TextureManager &textureManager) const;

	// Gets the argmax width from a vector of surfaces.
	int getMaxWidth(const std::vector<std::unique_ptr<Surface>> &surfaces);

	// Converts a string of text to lines of text based on new line characters.
	std::vector<std::string> textToLines(const std::string &text) const;

	// Converts a line of letter surfaces into one big surface.
	std::unique_ptr<Surface> combineSurfaces(
		const std::vector<std::unique_ptr<Surface>> &letterSurfaces);

	// Converts a line of text to surfaces.
	std::vector<std::unique_ptr<Surface>> lineToSurfaces(const std::string &line,
		TextureManager &textureManager) const;

	// Gets a trimmed letter surface from the font texture.
	std::unique_ptr<Surface> getTrimmedLetter(unsigned char c, 
		TextureManager &textureManager) const;
public:
	TextBox(int x, int y, const Color &textColor, const std::string &text,
		FontName fontName, TextureManager &textureManager);
	TextBox(const Int2 &center, const Color &textColor, const std::string &text,
		FontName fontName, TextureManager &textureManager);
	virtual ~TextBox();

	FontName getFontName() const;
};

#endif
