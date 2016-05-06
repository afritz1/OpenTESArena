#include <cassert>
#include <map>

#include "TextureFile.h"

#include "TextureName.h"
#include "TextureSequenceName.h"

// The filename of each TextureName (with sub-folders).
const auto TextureFilenames = std::map<TextureName, std::string>
{
	// Interface.
	{ TextureName::CharacterCreation, "interface/character_creation" },
	{ TextureName::CompassFrame, "interface/compass_frame" },
	{ TextureName::Icon, "interface/icon" },
	{ TextureName::IntroTitle, "interface/intro_title" },
	{ TextureName::IntroQuote, "interface/intro_quote" },
	{ TextureName::MainMenu, "interface/main_menu" },
	{ TextureName::ParchmentPopup, "interface/parchment/parchment_popup" },
	{ TextureName::QuillCursor, "interface/pointer" },
	{ TextureName::SwordCursor, "interface/arenarw" },
	{ TextureName::WorldMap, "interface/world_map" },

	// Fonts.
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
	// Interface.
	{ TextureSequenceName::IntroBook, "interface/intro/intro" },
	{ TextureSequenceName::IntroStory, "interface/intro_story/intro_story" },
	{ TextureSequenceName::OpeningScroll, "interface/scroll/scroll" },
	{ TextureSequenceName::NewGameStory, "interface/new_game_story/new_game_story" },
	{ TextureSequenceName::Silmane, "interface/silmane/silmane" },
};

// The number of images in each texture sequence. I think these numbers are necessary
// without doing some directory counting, and that sounds like a bad idea, with all
// the possible mix-ups with text files lying around and duplicates and such.
const auto TextureSequenceCounts = std::map<TextureSequenceName, int>
{
	// Interface.
	{ TextureSequenceName::IntroBook, 75 },
	{ TextureSequenceName::IntroStory, 3 },
	{ TextureSequenceName::OpeningScroll, 49 },
	{ TextureSequenceName::NewGameStory, 9 },
	{ TextureSequenceName::Silmane, 19 },
};

std::vector<TextureSequenceName> TextureFile::getSequenceNames()
{
	auto names = std::vector<TextureSequenceName>();

	for (const auto &pair : TextureSequenceFilenames)
	{
		names.push_back(pair.first);
	}

	return names;
}

std::string TextureFile::fromName(TextureName textureName)
{
	auto filename = TextureFilenames.at(textureName);
	return filename;
}

std::vector<std::string> TextureFile::fromName(TextureSequenceName sequenceName)
{
	auto filename = TextureSequenceFilenames.at(sequenceName);
	int count = TextureSequenceCounts.at(sequenceName);

	auto filenames = std::vector<std::string>();

	// Generate the list of filenames to load from file later.
	for (int i = 0; i < count; ++i)
	{
		auto countString = "-" + std::to_string(i + 1);
		filenames.push_back(filename + countString);
	}

	return filenames;
}
