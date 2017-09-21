#include <sstream>
#include <unordered_set>

#include "INFFile.h"

#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

namespace
{
	// These are all the .INF files in the Arena directory. They are not encrypted,
	// unlike the .INF files inside GLOBAL.BSA.
	const std::unordered_set<std::string> UnencryptedINFs =
	{
		"CRYSTAL3.INF",
		"IMPPAL1.INF",
		"IMPPAL2.INF",
		"IMPPAL3.INF",
		"IMPPAL4.INF"
	};
}

INFFile::CeilingData::CeilingData()
{
	const int defaultHeight = 100;
	const double defaultScale = 1.0;

	this->height = defaultHeight;
	this->boxScale = defaultScale;
	this->outdoorDungeon = false;
}

INFFile::FlatData::FlatData()
{
	this->yOffset = 0;
	this->health = 0;
	this->type = 0;
}

INFFile::INFFile(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Check if the .INF is encrypted.
	const bool isEncrypted = UnencryptedINFs.find(filename) == UnencryptedINFs.end();

	if (isEncrypted)
	{
		// Adapted from BSATool.
		const std::array<uint8_t, 8> encryptionKeys =
		{
			0xEA, 0x7B, 0x4E, 0xBD, 0x19, 0xC9, 0x38, 0x99
		};

		// Iterate through the encoded data, XORing with some encryption keys.
		// The count repeats every 256 bytes, and the key repeats every 8 bytes.
		uint8_t keyIndex = 0;
		uint8_t count = 0;
		for (uint8_t &encryptedByte : srcData)
		{
			encryptedByte ^= count + encryptionKeys.at(keyIndex);
			keyIndex = (keyIndex + 1) % encryptionKeys.size();
			count++;
		}
	}

	// Assign the data (now decoded if it was encoded) to the text member exposed
	// to the rest of the program.
	std::string text(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Remove carriage returns (newlines are nicer to work with).
	text = String::replace(text, "\r", "");

	// To do: maybe make a virtual method for each section, so parsing is nicer?
	// - Most sections have to store some state, so having a class would make sense.

	// The parse mode indicates which '@' section is currently being parsed.
	enum class ParseMode
	{
		Floors, Walls, Flats, Sound, Text
	};

	// Some sections have a mode they can be in, via a tag like *BOXCAP or *TEXT.
	enum class FloorMode { BoxCap, Ceiling };

	enum class WallMode
	{
		BoxCap, BoxSide, Door, DryChasm, LavaChasm, LevelDown,
		LevelUp, Menu, Transition, TransWalkThru, WalkThru, WetChasm,
	};

	enum class FlatMode { Item };

	enum class TextMode { Key, Riddle, Text };

	// Store memory for each encountered type (i.e., *BOXCAP, *DOOR, etc.) in each section.
	// I think it goes like this: for each set of consecutive types, assign them to each
	// consecutive element until the next set of types.
	struct ReferencedFloorTypes
	{
		std::vector<int> boxcap;
		bool ceiling;
	};

	struct ReferencedWallTypes
	{
		std::vector<int> boxcap, boxside;
	};

	struct ReferencedFlatTypes
	{
		std::vector<int> item;
	};

	struct ReferencedTextTypes
	{
		std::vector<int> text;
	};

	std::stringstream ss(text);
	std::string line;
	ParseMode mode = ParseMode::Floors;

	while (std::getline(ss, line))
	{
		// Skip empty lines.
		if (line.size() == 0)
		{
			continue;
		}

		const char SECTION_SEPARATOR = '@';

		// Check for a change of mode.
		if (line.front() == SECTION_SEPARATOR)
		{
			const std::unordered_map<std::string, ParseMode> Sections =
			{
				{ "@FLOORS", ParseMode::Floors },
				{ "@WALLS", ParseMode::Walls },
				{ "@FLATS", ParseMode::Flats },
				{ "@SOUND", ParseMode::Sound },
				{ "@TEXT", ParseMode::Text }
			};

			// Separate the '@' token from other things in the line (like @FLATS NOSHOW).
			line = String::split(line).front();

			// See which token the section is.
			const auto sectionIter = Sections.find(line);
			if (sectionIter != Sections.end())
			{
				mode = sectionIter->second;
				DebugMention("Changed to " + line + " mode.");
			}
			else
			{
				DebugCrash("Unrecognized .INF section \"" + line + "\".");
			}

			continue;
		}

		// Parse the line depending on the current mode (each line of text is guaranteed 
		// to not be empty at this point).
		// - For .SET filenames, the number (#...) is preceded by a tab (for tokenization).
		if (mode == ParseMode::Floors)
		{
			const std::unordered_map<std::string, FloorMode> FloorSections =
			{
				{ "*BOXCAP", FloorMode::BoxCap },
				{ "*CEILING", FloorMode::Ceiling }
			};
		}
		else if (mode == ParseMode::Walls)
		{
			const std::unordered_map<std::string, WallMode> WallSections =
			{
				{ "*BOXCAP", WallMode::BoxCap },
				{ "*BOXSIDE", WallMode::BoxSide },
				{ "*DOOR", WallMode::Door }, // *DOOR is ignored.
				{ "*DRYCHASM", WallMode::DryChasm },
				{ "*LAVACHASM", WallMode::LavaChasm },
				{ "*LEVELDOWN", WallMode::LevelDown },
				{ "*LEVELUP", WallMode::LevelUp },
				{ "*MENU", WallMode::Menu }, // Doors leading to interiors.
				{ "*TRANS", WallMode::Transition },
				{ "*TRANSWALKTHRU", WallMode::TransWalkThru },
				{ "*WALKTHRU", WallMode::WalkThru }, // Probably for hedge archways.
				{ "*WETCHASM", WallMode::WetChasm },
			};
		}
		else if (mode == ParseMode::Flats)
		{
			// Check if line says *TEXT, and if so, get the number. Otherwise, get the
			// filename and any associated modifiers ("F:", "Y:", etc.).

			// I think each *ITEM tag only points to the filename right below it. It
			// doesn't stack up any other filenames.
		}
		else if (mode == ParseMode::Sound)
		{
			// Split into the filename and ID. Make sure the filename is all caps.
			std::vector<std::string> tokens = String::split(line);
			const std::string vocFilename = String::toUppercase(tokens.front());
			const int vocID = std::stoi(tokens.at(1));

			this->sounds.insert(std::make_pair(vocID, vocFilename));
		}
		else if (mode == ParseMode::Text)
		{
			// Reset current text group after each *TEXT tag. Don't worry about empty lines.
			const char TEXT_CHAR = '*';

			// Decide what mode it is if the mode hasn't been decided.
			// if (textMode is null) then check first char on the first line
			// - if (char == '+') then key index
			// - else if (char == '~') then one-shot text
			// - else if (char == '^') then riddle
			// - else regular text
			const char KEY_INDEX_CHAR = '+';
			const char ONE_SHOT_CHAR = '~';
			const char RIDDLE_CHAR = '^';
		}
	}
}

INFFile::~INFFile()
{

}

const INFFile::TextureData &INFFile::getTexture(int index) const
{
	return this->textures.at(index);
}

const std::vector<INFFile::FlatData> &INFFile::getItemList(int index) const
{
	return this->itemLists.at(index);
}

const std::string &INFFile::getBoxcap(int index) const
{
	return this->boxcaps.at(index);
}

const std::string &INFFile::getBoxside(int index) const
{
	return this->boxsides.at(index);
}

const std::string &INFFile::getSound(int index) const
{
	return this->sounds.at(index);
}

const INFFile::TextData &INFFile::getText(int index) const
{
	return this->texts.at(index);
}

const std::string &INFFile::getLavaChasmTexture() const
{
	return this->lavaChasmTexture;
}

const std::string &INFFile::getWetChasmTexture() const
{
	return this->wetChasmTexture;
}

const std::string &INFFile::getDryChasmTexture() const
{
	return this->dryChasmTexture;
}

const std::string &INFFile::getLevelDownTexture() const
{
	return this->levelDownTexture;
}

const std::string &INFFile::getLevelUpTexture() const
{
	return this->levelUpTexture;
}

const std::string &INFFile::getTransitionTexture() const
{
	return this->transitionTexture;
}

const std::string &INFFile::getTransWalkThruTexture() const
{
	return this->transWalkThruTexture;
}

const INFFile::CeilingData &INFFile::getCeiling() const
{
	return this->ceiling;
}
