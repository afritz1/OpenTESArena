#include <algorithm>
#include <cassert>

#include "SDL.h"

#include "ListBox.h"

#include "TextBox.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/Font.h"
#include "../Media/FontName.h"
#include "../Media/TextureManager.h"
#include "../Utilities/String.h"

ListBox::ListBox(int x, int y, FontName fontName, const Color &textColor, int maxDisplayed,
	const std::vector<std::string> &elements, TextureManager &textureManager)
	: Surface(x, y, 1, 1), textureManagerRef(textureManager)
{
	assert(maxDisplayed > 0);

	this->elements = elements;
	this->fontName = fontName;
	this->textColor = std::unique_ptr<Color>(new Color(textColor));
	this->maxDisplayed = maxDisplayed;
	this->scrollIndex = 0;

	// Temporary text boxes for getting list box dimensions.
	std::vector<std::unique_ptr<TextBox>> textBoxes;
	for (const auto &element : elements)
	{
		// Remove any new lines.
		auto trimmedElement = String::trimLines(element);
		auto textBox = std::unique_ptr<TextBox>(new TextBox(
			0, 0, textColor, trimmedElement, fontName, textureManager));
		textBoxes.push_back(std::move(textBox));
	}

	// Calculate the proper dimensions of the list box surface.
	int width = this->getMaxWidth(textBoxes);
	int height = this->getTotalHeight();

	// Replace the old SDL surface. It was just a placeholder until now.
	SDL_FreeSurface(this->surface);
	this->surface = SDL_CreateRGBSurface(0, width, height, Surface::DEFAULT_BPP, 0, 0, 0, 0);
	this->optimize(textureManager.getFormat());

	// It's okay for there to be zero elements. Just be blank, then!

	// Draw the text boxes to the parent surface.
	this->updateDisplayText();
}

ListBox::~ListBox()
{

}

int ListBox::getScrollIndex() const
{
	return this->scrollIndex;
}

int ListBox::getElementCount() const
{
	return static_cast<int>(this->elements.size());
}

int ListBox::maxDisplayedElements() const
{
	return this->maxDisplayed;
}

int ListBox::getClickedIndex(const Int2 &point) const
{
	// The point is given in original dimensions relative to the letterbox.
	// The caller should not call this method if the point is outside the list box.
	// Therefore, "Surface::containsPoint()" should be called before to make sure.

	assert(point.getX() >= this->getX());
	assert(point.getX() < (this->getX() + this->getWidth()));
	assert(point.getY() >= this->getY());
	assert(point.getY() < (this->getY() + this->getHeight()));

	const int lineHeight = Font(this->fontName).getCellDimensions().getY();

	// Only the Y component really matters here.
	int index = this->scrollIndex + ((point.getY() - this->getY()) / lineHeight);

	// The caller should always check the returned index before using it (just in
	// case it's out of bounds).
	return index;
}

int ListBox::getMaxWidth(const std::vector<std::unique_ptr<TextBox>> &textBoxes) const
{
	int maxWidth = 0;

	for (const auto &textBox : textBoxes)
	{
		if (textBox->getWidth() > maxWidth)
		{
			maxWidth = textBox->getWidth();
		}
	}

	return maxWidth;
}

int ListBox::getTotalHeight() const
{
	int totalHeight = Font(this->fontName).getCellDimensions().getY() * this->maxDisplayed;
	return totalHeight;
}

void ListBox::updateDisplayText()
{
	// Erase the parent surface's pixels. Don't resize it.
	this->fill(Color::Magenta);
	this->setTransparentColor(Color::Magenta);

	// Draw the relevant text boxes according to scroll index.
	const int totalElements = static_cast<int>(this->elements.size());
	const int indexEnd = std::min(this->scrollIndex + this->maxDisplayed, totalElements);
	for (int i = this->scrollIndex; i < indexEnd; ++i)
	{
		const auto &element = this->elements.at(i);
		auto textBox = std::unique_ptr<TextBox>(new TextBox(0, 0, *this->textColor.get(),
			element, this->fontName, this->textureManagerRef));

		// Blit the text box onto the parent surface at the correct height offset.
		textBox->blit(*this, Int2(0, (i - this->scrollIndex) * textBox->getHeight()));
	}
}

void ListBox::scrollUp()
{
	this->scrollIndex -= 1;
	this->updateDisplayText();
}

void ListBox::scrollDown()
{
	this->scrollIndex += 1;
	this->updateDisplayText();
}
