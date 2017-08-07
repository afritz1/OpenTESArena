#include "RichTextString.h"

RichTextString::RichTextString(const std::string &text, FontName fontName,
	const Color &color, TextAlignment alignment, int lineSpacing)
	: text(text), fontName(fontName), color(color), alignment(alignment),
	lineSpacing(lineSpacing) { }

RichTextString::RichTextString(const std::string &text, FontName fontName,
	const Color &color, TextAlignment alignment)
	: RichTextString(text, fontName, color, alignment, 0) { }

RichTextString::~RichTextString()
{

}

const std::string &RichTextString::getText() const
{
	return this->text;
}

FontName RichTextString::getFontName() const
{
	return this->fontName;
}

const Color &RichTextString::getColor() const
{
	return this->color;
}

TextAlignment RichTextString::getAlignment() const
{
	return this->alignment;
}

int RichTextString::getLineSpacing() const
{
	return this->lineSpacing;
}
