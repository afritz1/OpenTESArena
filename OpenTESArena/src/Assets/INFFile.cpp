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

INFFile::KeyData::KeyData(int id)
{
	this->id = id;
}

INFFile::RiddleData::RiddleData(int firstNumber, int secondNumber)
{
	this->firstNumber = firstNumber;
	this->secondNumber = secondNumber;
}

INFFile::TextData::TextData(bool displayedOnce)
{
	this->displayedOnce = displayedOnce;
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
		None, Floors, Walls, Flats, Sound, Text
	};

	// Some sections have a mode they can be in, via a tag like *BOXCAP or *TEXT.
	enum class FloorMode { None, BoxCap, Ceiling };

	enum class WallMode
	{
		None, BoxCap, BoxSide, Door, DryChasm, LavaChasm, LevelDown,
		LevelUp, Menu, Transition, TransWalkThru, WalkThru, WetChasm,
	};

	enum class FlatMode { None, Item };

	enum class TextMode { None, Key, Riddle, Text };

	// Store memory for each encountered type (i.e., *BOXCAP, *DOOR, etc.) in each section.
	// I think it goes like this: for each set of consecutive types, assign them to each
	// consecutive element until the next set of types.
	/*struct ReferencedFloorTypes
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
	};*/

	struct TextState
	{
		enum RiddleMode { Riddle, Correct, Wrong };

		struct RiddleState
		{
			RiddleData data;
			RiddleMode mode;

			RiddleState(int firstNumber, int secondNumber)
				: data(firstNumber, secondNumber)
			{
				this->mode = RiddleMode::Riddle;
			}
		};

		std::unique_ptr<KeyData> keyData;
		std::unique_ptr<RiddleState> riddleState;
		std::unique_ptr<TextData> textData;
		TextMode mode; // Determines which data is in use.
		int id; // *TEXT ID.

		TextState(int id)
		{
			this->mode = TextMode::None;
			this->id = id;
		}
	};

	// Initialize loop states to null (they are non-null when in use by the loop).
	// - To do: Maybe this could be a virtual class that's instantiated as needed for
	//   each category encountered in the .INF file. It would move much of the parsing
	//   code into loosely coupled components (via virtual methods that take INFFile&).
	std::unique_ptr<TextState> textState;

	std::stringstream ss(text);
	std::string line;
	ParseMode parseMode = ParseMode::None;

	while (std::getline(ss, line))
	{
		// Skip empty lines.
		if (line.size() == 0)
		{
			continue;
		}

		const char SECTION_SEPARATOR = '@';
		
		// Check the first character for any changes in section. Otherwise, parse the line 
		// depending on the current mode (each line of text is guaranteed to not be empty 
		// at this point).
		// - For .SET filenames, the number (#...) is preceded by a tab (for tokenization).
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
				// To do: change this to a std::unique_ptr assignment from a factory parser class.
				parseMode = sectionIter->second;
			}
			else
			{
				DebugCrash("Unrecognized .INF section \"" + line + "\".");
			}
		}
		else if (parseMode == ParseMode::Floors)
		{
			const std::unordered_map<std::string, FloorMode> FloorSections =
			{
				{ "*BOXCAP", FloorMode::BoxCap },
				{ "*CEILING", FloorMode::Ceiling }
			};
		}
		else if (parseMode == ParseMode::Walls)
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
		else if (parseMode == ParseMode::Flats)
		{
			// Check if line says *TEXT, and if so, get the number. Otherwise, get the
			// filename and any associated modifiers ("F:", "Y:", etc.).

			// I think each *ITEM tag only points to the filename right below it. It
			// doesn't stack up any other filenames.
		}
		else if (parseMode == ParseMode::Sound)
		{
			// Split into the filename and ID. Make sure the filename is all caps.
			const std::vector<std::string> tokens = String::split(line);
			const std::string vocFilename = String::toUppercase(tokens.front());
			const int vocID = std::stoi(tokens.at(1));

			this->sounds.insert(std::make_pair(vocID, vocFilename));
		}
		else if (parseMode == ParseMode::Text)
		{
			// Start a new text state after each *TEXT tag.
			const char TEXT_CHAR = '*';
			const char KEY_INDEX_CHAR = '+';
			const char RIDDLE_CHAR = '^';
			const char DISPLAYED_ONCE_CHAR = '~';

			// Check the first character in the line to determine any changes in text mode.
			// Otherwise, parse the line based on the current mode.
			if (line.front() == TEXT_CHAR)
			{
				const std::vector<std::string> tokens = String::split(line);

				// Get the ID after *TEXT.
				const int textID = std::stoi(tokens.at(1));

				// If there is existing text state present, save it.
				if (textState.get() != nullptr)
				{
					if (textState->mode == TextMode::Key)
					{
						// Save key data.
						this->keys.insert(std::make_pair(
							textState->id, *textState->keyData.get()));
					}
					else if (textState->mode == TextMode::Riddle)
					{
						// Save riddle data.
						this->riddles.insert(std::make_pair(
							textState->id, textState->riddleState->data));
					}
					else if (textState->mode == TextMode::Text)
					{
						// Save text data.
						this->texts.insert(std::make_pair(
							textState->id, *textState->textData.get()));
					}
				}

				// Reset the text state to default with the new *TEXT ID.
				textState = std::unique_ptr<TextState>(new TextState(textID));
			}
			else if (line.front() == KEY_INDEX_CHAR)
			{
				// Get key number. No need for a key section here since it's only one line.
				const std::string keyStr = line.substr(1, line.size() - 1);
				const int keyNumber = std::stoi(keyStr);

				textState->mode = TextMode::Key;
				textState->keyData = std::unique_ptr<KeyData>(new KeyData(keyNumber));
			}
			else if (line.front() == RIDDLE_CHAR)
			{
				// Get riddle numbers.
				const std::string numbers = line.substr(1, line.size() - 1);
				const std::vector<std::string> tokens = String::split(numbers);
				const int firstNumber = std::stoi(tokens.at(0));
				const int secondNumber = std::stoi(tokens.at(1));

				textState->mode = TextMode::Riddle;
				textState->riddleState = std::unique_ptr<TextState::RiddleState>(
					new TextState::RiddleState(firstNumber, secondNumber));
			}
			else if (line.front() == DISPLAYED_ONCE_CHAR)
			{
				textState->mode = TextMode::Text;

				const bool displayedOnce = true;
				textState->textData = std::unique_ptr<TextData>(new TextData(displayedOnce));

				// Append the rest of the line to the text data.
				textState->textData->text += line.substr(1, line.size() - 1) + '\n';
			}
			else if (textState->mode == TextMode::Riddle)
			{
				const char ANSWER_CHAR = ':'; // An accepted answer.
				const char RESPONSE_SECTION_CHAR = '`'; // CORRECT/WRONG.

				if (line.front() == ANSWER_CHAR)
				{
					// Add the answer to the answers data.
					const std::string answer = line.substr(1, line.size() - 1);
					textState->riddleState->data.answers.push_back(answer);
				}
				else if (line.front() == RESPONSE_SECTION_CHAR)
				{
					// Change riddle mode based on the response section.
					const std::string CORRECT_STR = "CORRECT";
					const std::string WRONG_STR = "WRONG";
					const std::string responseSection = line.substr(1, line.size() - 1);

					if (responseSection == CORRECT_STR)
					{
						textState->riddleState->mode = TextState::RiddleMode::Correct;
					}
					else if (responseSection == WRONG_STR)
					{
						textState->riddleState->mode = TextState::RiddleMode::Wrong;
					}
				}
				else if (textState->riddleState->mode == TextState::RiddleMode::Riddle)
				{
					// Read the line into the riddle text.
					textState->riddleState->data.riddle += line + '\n';
				}
				else if (textState->riddleState->mode == TextState::RiddleMode::Correct)
				{
					// Read the line into the correct text.
					textState->riddleState->data.correct += line + '\n';
				}
				else if (textState->riddleState->mode == TextState::RiddleMode::Wrong)
				{
					// Read the line into the wrong text.
					textState->riddleState->data.wrong += line + '\n';
				}
			}
			else if (textState->mode == TextMode::Text)
			{
				// Read the line into the text data.
				textState->textData->text += line + '\n';
			}
			else
			{
				// Plain old text after a *TEXT line.
				if (textState->mode == TextMode::None)
				{
					textState->mode = TextMode::Text;

					const bool displayedOnce = false;
					textState->textData = std::unique_ptr<TextData>(new TextData(displayedOnce));
				}

				// Read the line into the text data.
				textState->textData->text += line + '\n';
			}
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

bool INFFile::hasKeyIndex(int index) const
{
	return this->keys.find(index) != this->keys.end();
}

bool INFFile::hasRiddleIndex(int index) const
{
	return this->riddles.find(index) != this->riddles.end();
}

bool INFFile::hasTextIndex(int index) const
{
	return this->texts.find(index) != this->texts.end();
}

const INFFile::KeyData &INFFile::getKey(int index) const
{
	return this->keys.at(index);
}

const INFFile::RiddleData &INFFile::getRiddle(int index) const
{
	return this->riddles.at(index);
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
