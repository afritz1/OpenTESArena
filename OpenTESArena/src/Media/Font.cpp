#include <cassert>
#include <map>

#include "SDL.h"

#include "Font.h"

#include "FontName.h"
#include "../Assets/FontFile.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Utilities/Debug.h"

namespace
{
	const std::map<FontName, std::string> FontFilenames =
	{
		{ FontName::A, "FONT_A.DAT" },
		{ FontName::Arena, "ARENAFNT.DAT" },
		{ FontName::B, "FONT_B.DAT" },
		{ FontName::C, "FONT_C.DAT" },
		{ FontName::Char, "CHARFNT.DAT" },
		{ FontName::D, "FONT_D.DAT" },
		{ FontName::Four, "FONT4.DAT" },
		{ FontName::S, "FONT_S.DAT" },
		{ FontName::Teeny, "TEENYFNT.DAT" }
	};
}

Font::Font(FontName fontName)
	: characters()
{
	// Load the font file for this font name.
	const std::string &filename = FontFilenames.at(fontName);
	FontFile fontFile(filename);

	const int elementHeight = fontFile.getHeight();
	this->characterHeight = elementHeight;

	// There are 95 characters, plus space.
	this->characters.resize(96);

	// Create an SDL surface for each character image. Start with space (ASCII 32), 
	// and end with delete (ASCII 127).
	for (int i = 0; i < 96; ++i)
	{
		const char c = i + 32;
		const int elementWidth = fontFile.getWidth(c);
		const uint32_t *elementPixels = fontFile.getPixels(c);

		SDL_Surface *surface = Surface::createSurfaceWithFormat(elementWidth,
			elementHeight, Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

		uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
		const int pixelCount = surface->w * surface->h;

		for (int index = 0; index < pixelCount; ++index)
		{
			pixels[index] = elementPixels[index];
		}

		this->characters.at(i) = surface;
	}
}

Font::Font(Font &&font)
{
	this->characters = std::move(font.characters);
	this->characterHeight = font.characterHeight;
}

Font::~Font()
{
	for (auto *surface : this->characters)
	{
		SDL_FreeSurface(surface);
	}
}

Font &Font::operator=(Font &&font)
{
	this->characters = std::move(font.characters);
	this->characterHeight = font.characterHeight;

	return *this;
}

const std::string &Font::fromName(FontName fontName)
{
	const std::string &filename = FontFilenames.at(fontName);
	return filename;
}

int Font::getCharacterHeight() const
{
	return this->characterHeight;
}

SDL_Surface *Font::getSurface(char c) const
{
	Debug::check((c >= 32) && (c <= 127), "Font", "Character value \"" +
		std::to_string(c) + "\" out of range (must be ASCII 32-127).");

	// Space (ASCII 32) is at index 0.
	SDL_Surface *surface = this->characters.at(c - 32);
	return surface;
}
