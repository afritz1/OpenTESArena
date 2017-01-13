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
		// Load the font object and insert it into the font manager map.
		fontIter = this->fonts.emplace(std::make_pair(fontName, 
			std::move(Font(fontName)))).first;

		return fontIter->second;
	}
}
