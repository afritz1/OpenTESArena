#ifndef LIST_BOX_H
#define LIST_BOX_H

#include <memory>
#include <string>
#include <vector>

#include "Surface.h"
#include "Texture.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"

// This class defines a list of displayed text boxes. The index of a clicked text 
// box can be obtained, and the list can be scrolled up and down. A list box is
// intended to only be left-aligned.

// Though the index of a selected item can be obtained, this class is not intended
// for holding data about those selected items. It is simply a view for the text.

class FontLibrary;
class Renderer;
class TextBox;

enum class FontName;

class ListBox
{
private:
	std::vector<std::unique_ptr<TextBox>> textBoxes;
	Int2 point;
	FontName fontName;
	Surface clearSurface; // For clearing the texture upon updating.
	Texture texture;
	int scrollIndex;
	int characterHeight;
	int maxDisplayed;
	int rowSpacing;

	// Helper functions for pairing each line of a list box with a text color.
	static std::vector<std::pair<std::string, Color>> makeStringColorPairs(
		const std::vector<std::string> &strings, const std::vector<Color> &colors);
	static std::vector<std::pair<std::string, Color>> makeStringColorPairs(
		const std::vector<std::string> &strings, const Color &color);

	// Updates the texture to show the currently visible text boxes.
	void updateDisplay();
public:
	// Per-element color customization and customizable distance between elements.
	ListBox(int x, int y, const std::vector<std::pair<std::string, Color>> &elements,
		FontName fontName, int maxDisplayed, int rowSpacing, const FontLibrary &fontLibrary,
		Renderer &renderer);

	// Customizable distance between elements.
	ListBox(int x, int y, const Color &textColor, const std::vector<std::string> &elements,
		FontName fontName, int maxDisplayed, int rowSpacing, const FontLibrary &fontLibrary,
		Renderer &renderer);

	// Per-element color customization.
	ListBox(int x, int y, const std::vector<std::pair<std::string, Color>> &elements,
		FontName fontName, int maxDisplayed, const FontLibrary &fontLibrary, Renderer &renderer);

	// No color or distance customization.
	ListBox(int x, int y, const Color &textColor, const std::vector<std::string> &elements,
		FontName fontName, int maxDisplayed, const FontLibrary &fontLibrary, Renderer &renderer);

	// Gets the index of the top-most displayed element.
	int getScrollIndex() const;

	// Gets the total number of text boxes (elements) in the list box.
	int getElementCount() const;

	// Gets the max number of displayed text boxes.
	int getMaxDisplayedCount() const;

	// Gets the top left corner of the list box.
	const Int2 &getPoint() const;

	// Gets the texture for drawing to the screen.
	const Texture &getTexture() const;

	// Gets the width and height of the list box.
	Int2 getDimensions() const;

	// Returns whether the given point is within the bounds of the list box.
	bool contains(const Int2 &point);

	// Gets the index of a clicked element. ListBox::contains() should be called 
	// beforehand to make sure the given point is within the list box's bounds.
	int getClickedIndex(const Int2 &point) const;

	// Decrement the scroll index by one. Without bounds checking on the caller's behalf,
	// it will go out-of-bounds when the scroll index is -1.
	void scrollUp();

	// Increment the scroll index by one. Without bounds checking on the caller's behalf, 
	// it can keep scrolling down for a really long time.
	void scrollDown();

	// Instead of a remove() method, just recreate the list box.
};

#endif
