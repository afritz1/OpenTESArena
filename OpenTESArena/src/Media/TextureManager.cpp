#include <cassert>
#include <iostream>

#include "SDL2\SDL.h"
#include "SDL2\SDL_image.h"

#include "TextureManager.h"

#include "TextureName.h"
#include "TextureSequenceName.h"
#include "../Interface/Surface.h"
#include "../Utilities/Debug.h"

// The format extension for all textures will be PNG.
const std::string TextureExtension = ".png";

// The filename of each TextureName (with sub-folders). This should be known only 
// to the TextureManager.
const auto TextureFilenames = std::map<TextureName, std::string>
{
	// Interface
	{ TextureName::CharacterCreation, "interface/character_creation" },
	{ TextureName::Icon, "interface/icon" },
	{ TextureName::IntroTitle, "interface/intro_title" },
	{ TextureName::IntroQuote, "interface/intro_quote" },
	{ TextureName::MainMenu, "interface/main_menu" },
	{ TextureName::ParchmentPopup, "interface/parchment/parchment_popup" },
	{ TextureName::QuillCursor, "interface/pointer" },
	{ TextureName::SwordCursor, "interface/arenarw" },
	{ TextureName::WorldMap, "interface/world_map" },

	// Fonts
	{ TextureName::FontA, "interface/fonts/font_a" },
	{ TextureName::FontArena, "interface/fonts/arena_font" },
	{ TextureName::FontB, "interface/fonts/font_b" },
	{ TextureName::FontC, "interface/fonts/font_c" },
	{ TextureName::FontChar, "interface/fonts/char_font" },
	{ TextureName::FontD, "interface/fonts/font_d" },
	{ TextureName::FontFour, "interface/fonts/font_4" },
	{ TextureName::FontS, "interface/fonts/font_s" },
	{ TextureName::FontTeeny, "interface/fonts/teeny_font" },
};

// The filename prefix of each TextureSequenceName (with sub-folders). When looking
// for files in the sequence, "-#" will be appended to the name for the number of
// the file, with more numbers added as needed.
const auto TextureSequenceFilenames = std::map<TextureSequenceName, std::string>
{
	// Interface
	{ TextureSequenceName::IntroBook, "interface/intro/intro" },
	{ TextureSequenceName::IntroStory, "interface/intro_story/intro_story" },
	{ TextureSequenceName::OpeningScroll, "interface/scroll/scroll" },
	{ TextureSequenceName::NewGameStory, "interface/new_game_story/new_game_story" },
	{ TextureSequenceName::Silmane, "interface/silmane/silmane" },
};

// The number of images in each texture sequence. I think these numbers are necessary
// without doing some directory counting, and that sounds like a bad idea, with all
// the possible mix-ups with note files and duplicates and such.
const auto TextureSequenceCounts = std::map<TextureSequenceName, int>
{
	// Interface
	{ TextureSequenceName::IntroBook, 75 },
	{ TextureSequenceName::IntroStory, 3 },
	{ TextureSequenceName::OpeningScroll, 49 },
	{ TextureSequenceName::NewGameStory, 9 },
	{ TextureSequenceName::Silmane, 19 },
};

const std::string TextureManager::PATH = "data/textures/";

TextureManager::TextureManager(const SDL_PixelFormat *format)
{
	assert(format != nullptr);

	// Intialize PNG file loading.
	int imgFlags = IMG_INIT_PNG;
	Debug::check((IMG_Init(imgFlags) & imgFlags) != SDL_FALSE, "Texture Manager", 
		"Couldn't initialize texture loader, " + std::string(IMG_GetError()));
	
	this->surfaces = std::map<TextureName, Surface>();
	this->sequences = std::map<TextureSequenceName, std::vector<Surface>>();
	this->format = format;

	assert(this->surfaces.size() == 0);
	assert(this->sequences.size() == 0);
	assert(this->format == format);
}

TextureManager::~TextureManager()
{
	IMG_Quit();
}

SDL_Surface *TextureManager::loadFromFile(const std::string &filename)
{
	// Path to the file.
	auto fullPath = TextureManager::PATH + filename;

	// Load the SDL_Surface from file.
	auto unOptSurface = IMG_Load(fullPath.c_str());
	Debug::check(unOptSurface != nullptr, "Texture Manager",
		"Could not open texture \"" + fullPath + "\".");

	// Try to optimize the SDL_Surface.
	auto optSurface = SDL_ConvertSurface(unOptSurface, this->format, 0);
	SDL_FreeSurface(unOptSurface);
	Debug::check(optSurface != nullptr, "Texture Manager",
		"Could not optimize texture \"" + fullPath + "\".");

	return optSurface;
}

const SDL_PixelFormat *TextureManager::getFormat() const
{
	return this->format;
}

const Surface &TextureManager::getSurface(TextureName name)
{
	if (this->surfaces.find(name) != this->surfaces.end())
	{
		// Get the existing surface.
		auto &surface = this->surfaces.at(name);
		return surface;
	}
	else
	{
		// Load optimized SDL_Surface from file.
		const auto &filename = TextureFilenames.at(name) + TextureExtension;
		auto optSurface = this->loadFromFile(filename);

		// Create surface from SDL_Surface. No need to optimize it again.
		auto surface = Surface(optSurface);

		// Add the new texture.
		this->surfaces.insert(std::pair<TextureName, Surface>(name, surface));

		// Try this method again.
		assert(this->surfaces.find(name) != this->surfaces.end());
		return this->getSurface(name);
	}
}

const std::vector<Surface> &TextureManager::getSequence(TextureSequenceName name)
{
	if (this->sequences.find(name) != this->sequences.end())
	{
		// Get the existing sequence.
		auto &sequence = this->sequences.at(name);
		return sequence;
	}
	else
	{
		// Add the empty sequence first.
		this->sequences.insert(std::pair<TextureSequenceName, std::vector<Surface>>(
			name, std::vector<Surface>()));

		const auto &prefix = TextureSequenceFilenames.at(name);
		const int count = TextureSequenceCounts.at(name);

		for (int i = 0; i < count; ++i)
		{
			// Load optimized SDL_Surface from file.		
			auto countString = "-" + std::to_string(i + 1);
			auto filename = prefix + countString + TextureExtension;
			auto optSurface = this->loadFromFile(filename);

			// Create surface from SDL_Surface. No need to optimize it again.
			auto surface = Surface(optSurface);

			// Push back the new texture.
			this->sequences.at(name).push_back(surface);
		}

		// Try this method again.
		assert(this->sequences.find(name) != this->sequences.end());
		assert(this->sequences.at(name).size() == TextureSequenceCounts.at(name));
		return this->getSequence(name);
	}
}
