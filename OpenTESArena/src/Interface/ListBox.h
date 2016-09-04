#ifndef LIST_BOX_H
#define LIST_BOX_H

#include <string>
#include <vector>

#include "Surface.h"

// The text color and texture manager need to be accessible so they can be remembered 
// for the update display text method.

class Color;
class Font;
class Int2;
class Renderer;
class TextBox;

enum class FontName;

class ListBox : public Surface
{
private:
	std::vector<std::string> elements;
	std::unique_ptr<Color> textColor;
	Renderer &rendererRef;
	const Font &fontRef;
	int scrollIndex, maxDisplayed;

	int getMaxWidth(const std::vector<std::unique_ptr<TextBox>> &textBoxes) const;
	int getTotalHeight() const;
	void updateDisplayText();
public:
	// No "center" constructor, because a list box is intended to be left-aligned.
	ListBox(int x, int y, const Font &font, const Color &textColor, int maxDisplayed,
		const std::vector<std::string> &elements, Renderer &renderer);
	virtual ~ListBox();

	// Index of the top-most displayed element.
	int getScrollIndex() const;

	// Number of elements in the list box (side note: can't return an int cast by reference).
	int getElementCount() const;

	// Maximum number of visible lines (this determines the box size).
	int maxDisplayedElements() const;

	// Get the index of a clicked element.
	int getClickedIndex(const Int2 &point) const;

	// Decrement the scroll index by one. Without bounds checking on the caller's behalf,
	// it will go out-of-bounds when the scroll index is -1.
	void scrollUp();

	// Increment the scroll index by one. Without bounds checking on the caller's behalf, 
	// it can keep scrolling down for a really long time.
	void scrollDown();

	// Maybe add a "remove(int index)" method sometime, for containers and things...
	// or just remake the list box each time.
};

#endif
