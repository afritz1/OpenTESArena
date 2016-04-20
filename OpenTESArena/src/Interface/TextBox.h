#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include <memory>
#include <string>
#include <vector>

#include "../Media/FontName.h"
#include "Surface.h"

// 4/19/2016 - This class's constructor needs to be redone to account for variable 
// spacing between characters. I think letters should be drawn with respect to the
// bottom left corner of each text line, not the top left corner.

// A scrollable text box could just have a text box surface, and then use a Rectangle
// to use only the visible part of it. The scroll bar can be thought of as a kind of
// "sliding window"; the size of the clickable scroll bar is the percentage of the lines 
// shown, and the position would follow a similar pattern.

class Color;
class TextureManager;

class TextBox : public Surface
{
private:
	std::vector<std::string> textLines;
	FontName fontName;
public:
	TextBox(int x, int y, const Color &textColor, const std::string &text,
		FontName fontName, TextureManager &textureManager);

	// The centered text box constructor is now a desired feature. Its Surface::point
	// member will be based on the length and width of the final text box and the
	// given "center" argument.
	//TextBox(const Int2 &center, ...);

	~TextBox();

	// Alignment...? Maybe it should be a constructor argument, not a void function. 
	// Things like game world on-screen text might be centered. Everything else is 
	// left-aligned.
};

#endif
