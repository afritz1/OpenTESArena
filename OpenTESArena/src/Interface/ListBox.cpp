#include <algorithm>
#include <cassert>

#include "SDL.h"

#include "ListBox.h"

#include "TextAlignment.h"
#include "TextBox.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/Font.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/String.h"

ListBox::ListBox(int x, int y, const Font &font, const Color &textColor, int maxDisplayed,
	const std::vector<std::string> &elements, Renderer &renderer)
	: Surface(x, y, 1, 1), fontRef(font), rendererRef(renderer)
{
	assert(maxDisplayed > 0);

	this->elements = elements;
	this->textColor = std::unique_ptr<Color>(new Color(textColor));
	this->maxDisplayed = maxDisplayed;
	this->scrollIndex = 0;

	// Temporary text boxes for getting list box dimensions.
	std::vector<std::unique_ptr<TextBox>> textBoxes;
	for (const auto &element : elements)
	{
		// Remove any new lines.
		auto trimmedElement = String::trimLines(element);
		std::unique_ptr<TextBox> textBox(new TextBox(
			0, 0, textColor, trimmedElement, font, TextAlignment::Left, renderer));
		textBoxes.push_back(std::move(textBox));
	}

	// Calculate the proper dimensions of the list box surface.
	int width = this->getMaxWidth(textBoxes);
	int height = this->getTotalHeight();

	// Replace the old SDL surface. It was just a placeholder until now.
	// Surface::optimize() can be avoided by just giving the ARGB masks instead.
	SDL_FreeSurface(this->surface);
	this->surface = Surface::createSurfaceWithFormat(width, height,
		Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

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

	const int lineHeight = this->fontRef.getCharacterHeight();

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
		int textBoxWidth;
		SDL_QueryTexture(textBox->getTexture(), nullptr, nullptr, &textBoxWidth, nullptr);

		if (textBoxWidth > maxWidth)
		{
			maxWidth = textBoxWidth;
		}
	}

	return maxWidth;
}

int ListBox::getTotalHeight() const
{
	int totalHeight = this->fontRef.getCharacterHeight() * this->maxDisplayed;
	return totalHeight;
}

void ListBox::updateDisplayText()
{
	// Erase the parent surface's pixels. Don't resize it.
	this->fill(Color::Transparent);
	this->setTransparentColor(Color::Transparent);

	// Draw the relevant text boxes according to scroll index.
	const int totalElements = static_cast<int>(this->elements.size());
	const int indexEnd = std::min(this->scrollIndex + this->maxDisplayed, totalElements);
	for (int i = this->scrollIndex; i < indexEnd; ++i)
	{
		const auto &element = this->elements.at(i);
		std::unique_ptr<TextBox> textBox(new TextBox(0, 0, *this->textColor.get(),
			element, this->fontRef, TextAlignment::Left, this->rendererRef));

		// Blit the text box onto the parent surface at the correct height offset.
		SDL_Surface *textBoxSurface = textBox->getSurface();

		SDL_Rect rect;
		rect.x = 0;
		rect.y = (i - this->scrollIndex) * textBoxSurface->h;
		rect.w = textBoxSurface->w;
		rect.h = textBoxSurface->h;

		SDL_BlitSurface(textBoxSurface, nullptr, this->getSurface(), &rect);
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
