#include "TextureDefinition.h"

TextureDefinition::TextureDefinition(std::string &&filename, int index)
	: filename(std::move(filename)), index(index) { }

TextureDefinition::TextureDefinition(std::string &&filename)
	: filename(std::move(filename)), index(std::nullopt) { }

TextureDefinition::TextureDefinition() { }

const std::string &TextureDefinition::getFilename() const
{
	return this->filename;
}

const std::optional<int> &TextureDefinition::getIndex() const
{
	return this->index;
}

void TextureDefinition::setFilename(std::string &&filename)
{
	this->filename = std::move(filename);
}

void TextureDefinition::setIndex(int index)
{
	this->index = index;
}

void TextureDefinition::resetIndex()
{
	this->index = std::nullopt;
}
