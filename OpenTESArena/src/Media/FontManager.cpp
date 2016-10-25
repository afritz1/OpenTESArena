#include "FontManager.h"

#include "Font.h"
#include "FontName.h"

FontManager::FontManager()
	: fonts()
{
	
}

FontManager::~FontManager()
{

}

const Font &FontManager::getFont(FontName fontName)
{
	auto fontIter = this->fonts.find(fontName);

	if (fontIter != this->fonts.end())
	{
		return fontIter->second;
	}
	else
	{
		Font font(fontName);

		// Load the font object and insert it into the font manager map.
		fontIter = this->fonts.emplace(std::make_pair(fontName, std::move(font))).first;

		return fontIter->second;
	}
}
