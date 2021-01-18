#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <string_view>
#include <unordered_set>

#include "INFFile.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"
#include "components/vfs/manager.hpp"

namespace
{
	// Each '@' section may or may not have some state it currently possesses. They 
	// also have a mode they can be in, via a tag like *BOXCAP or *TEXT.
	struct FloorState
	{
		enum class Mode { None, BoxCap, Ceiling };

		std::optional<INFFile::CeilingData> ceilingData;
		std::string_view textureName;
		FloorState::Mode mode;
		std::optional<int> boxCapID;

		FloorState()
		{
			this->mode = FloorState::Mode::None;
		}
	};

	struct WallState
	{
		enum class Mode
		{
			None, BoxCap, BoxSide, DryChasm, LavaChasm,
			LevelDown, LevelUp, Menu, WetChasm
		};

		std::vector<int> boxCapIDs, boxSideIDs;
		std::string_view textureName;
		WallState::Mode mode;
		std::optional<int> menuID;
		bool dryChasm, lavaChasm, wetChasm;
		// *TRANS, *TRANSWALKTHRU, and *WALKTHRU are unused (set by voxel data instead).

		WallState()
		{
			this->mode = WallState::Mode::None;
			this->dryChasm = false;
			this->lavaChasm = false;
			this->wetChasm = false;
		}
	};

	struct FlatState
	{
		enum class Mode { None, Item };

		FlatState::Mode mode;
		std::optional<int> itemID;

		FlatState()
		{
			this->mode = FlatState::Mode::None;
		}
	};

	struct TextState
	{
		enum class Mode { None, Key, Riddle, Text };

		struct RiddleState
		{
			enum class Mode { Riddle, Correct, Wrong };

			INFFile::RiddleData data;
			RiddleState::Mode mode;

			RiddleState(int firstNumber, int secondNumber)
				: data(firstNumber, secondNumber)
			{
				this->mode = RiddleState::Mode::Riddle;
			}
		};

		std::optional<INFFile::KeyData> keyData;
		std::optional<RiddleState> riddleState;
		std::optional<INFFile::TextData> textData;
		TextState::Mode mode; // Determines which data is in use.
		int id; // *TEXT ID.

		TextState(int id)
		{
			this->mode = TextState::Mode::None;
			this->id = id;
		}
	};
}

INFFile::VoxelTextureData::VoxelTextureData(const char *filename, const std::optional<int> &setIndex)
	: filename(filename), setIndex(setIndex) { }

INFFile::VoxelTextureData::VoxelTextureData(const char *filename)
	: VoxelTextureData(filename, std::nullopt) { }

INFFile::FlatTextureData::FlatTextureData(const char *filename)
	: filename(filename) { }

INFFile::CeilingData::CeilingData()
{
	this->height = CeilingData::DEFAULT_HEIGHT;
	this->outdoorDungeon = false;
}

INFFile::FlatData::FlatData()
{
	this->yOffset = 0;
	this->health = 0;
	this->collider = false;
	this->puddle = false;
	this->largeScale = false;
	this->dark = false;
	this->transparent = false;
	this->ceiling = false;
	this->mediumScale = false;
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

bool INFFile::init(const char *filename)
{
	bool inGlobalBSA; // Set by VFS open() function.

	// Some filenames (i.e., Crystal3.inf) have different casing between the floppy version and
	// CD version, so this needs to use the case-insensitive open() method for correct behavior
	// on Unix-based systems.
	Buffer<std::byte> src;
	if (!VFS::Manager::get().readCaseInsensitive(filename, &src, &inGlobalBSA))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	uint8_t *srcPtr = reinterpret_cast<uint8_t*>(src.get());
	uint8_t *srcEnd = reinterpret_cast<uint8_t*>(src.end());

	// Check if the .INF is encrypted.
	const bool isEncrypted = inGlobalBSA;

	if (isEncrypted)
	{
		// Adapted from BSATool.
		constexpr std::array<uint8_t, 8> encryptionKeys =
		{
			0xEA, 0x7B, 0x4E, 0xBD, 0x19, 0xC9, 0x38, 0x99
		};

		// Iterate through the encoded data, XORing with some encryption keys.
		// The count repeats every 256 bytes, and the key repeats every 8 bytes.
		uint8_t keyIndex = 0;
		uint8_t count = 0;
		for (auto it = srcPtr; it != srcEnd; ++it)
		{
			uint8_t &encryptedByte = *it;
			encryptedByte ^= count + encryptionKeys.at(keyIndex);
			keyIndex = (keyIndex + 1) % encryptionKeys.size();
			count++;
		}
	}

	this->name = filename;

	// Assign the data (now decoded if it was encoded) to the text member exposed
	// to the rest of the program.
	std::string text(reinterpret_cast<char*>(srcPtr), src.getCount());

	// Remove carriage returns (newlines are nicer to work with).
	text = String::replace(text, "\r", "");

	// The parse mode indicates which '@' section is currently being parsed.
	enum class ParseMode
	{
		Floors, Walls, Flats, Sound, Text
	};

	// Initialize loop states to empty (they are non-empty when in use by the loop).
	// I tried re-organizing them into virtual classes, but this way seems to be the
	// most practical for now.
	std::optional<FloorState> floorState;
	std::optional<WallState> wallState;
	std::optional<FlatState> flatState;
	std::optional<TextState> textState;

	// Lambda for flushing state to the INFFile. This is useful during the parse loop,
	// but it's also sometimes necessary at the end of the file because the last element 
	// of certain sections (i.e., @TEXT) might get missed if there is no data after them.
	auto flushTextState = [this, &textState]()
	{
		if (textState->mode == TextState::Mode::Key)
		{
			// Save key data.
			this->keys.insert(std::make_pair(textState->id, textState->keyData.value()));
		}
		else if (textState->mode == TextState::Mode::Riddle)
		{
			// Save riddle data.
			this->riddles.insert(std::make_pair(textState->id, textState->riddleState->data));
		}
		else if (textState->mode == TextState::Mode::Text)
		{
			// Save text data.
			this->texts.insert(std::make_pair(textState->id, textState->textData.value()));
		}
	};

	// Lambda for flushing all states. Most states don't need an explicit flush because 
	// they have no risk of leaving data behind.
	auto flushAllStates = [&floorState, &wallState, &flatState, 
		&textState, &flushTextState]()
	{
		if (floorState.has_value())
		{
			floorState.reset();
		}

		if (wallState.has_value())
		{
			wallState.reset();
		}

		if (flatState.has_value())
		{
			flatState.reset();
		}

		if (textState.has_value())
		{
			flushTextState();
			textState.reset();
		}
	};

	// Lambdas for parsing a line of text.
	auto parseFloorLine = [this, &floorState](const std::string &line)
	{
		const char TYPE_CHAR = '*';

		// Decide what to do based on the first character. Otherwise, read the line
		// as a texture filename.
		if (line.front() == TYPE_CHAR)
		{
			// Initialize floor state if it is null.
			if (!floorState.has_value())
			{
				floorState = FloorState();
			}

			const std::string BOXCAP_STR = "BOXCAP";
			const std::string CEILING_STR = "CEILING";
			const std::string TOP_STR = "TOP"; // Only occurs in LABRNTH{1,2}.INF.

			// See what the type in the line is.
			const std::vector<std::string_view> tokens = StringView::split(line);
			const std::string_view firstToken = tokens.at(0);
			const std::string_view firstTokenType = firstToken.substr(1, firstToken.size() - 1);

			if (firstTokenType == BOXCAP_STR)
			{
				// Write the *BOXCAP's ID to the floor state.
				floorState->boxCapID = std::stoi(std::string(tokens.at(1)));
				floorState->mode = FloorState::Mode::BoxCap;
			}
			else if (firstTokenType == CEILING_STR)
			{
				// Initialize ceiling data.
				floorState->ceilingData = CeilingData();
				floorState->mode = FloorState::Mode::Ceiling;

				// Check up to three numbers on the right: ceiling height, box scale,
				// and indoor/outdoor dungeon boolean. Sometimes there are no numbers.
				if (tokens.size() >= 2)
				{
					floorState->ceilingData->height = std::stoi(std::string(tokens.at(1)));
				}

				if (tokens.size() >= 3)
				{
					floorState->ceilingData->boxScale = std::stoi(std::string(tokens.at(2)));
				}

				if (tokens.size() == 4)
				{
					floorState->ceilingData->outdoorDungeon = tokens.at(3) == "1";
				}
			}
			else if (firstTokenType == TOP_STR)
			{
				// Not sure what *TOP is.
				static_cast<void>(firstTokenType);
			}
			else
			{
				DebugCrash("Unrecognized @FLOOR section \"" + std::string(tokens.at(0)) + "\".");
			}
		}
		else if (!floorState.has_value())
		{
			// No current floor state, so the current line is a loose texture filename
			// (found in some city .INFs).
			const std::vector<std::string_view> tokens = StringView::split(line, '#');

			if (tokens.size() == 1)
			{
				// A regular filename (like an .IMG).
				this->voxelTextures.push_back(VoxelTextureData(line.c_str()));
			}
			else
			{
				// A .SET filename. Expand it for each of the .SET indices.
				const std::string_view textureName = StringView::trimBack(tokens.at(0));
				const int setSize = std::stoi(std::string(tokens.at(1)));

				for (int i = 0; i < setSize; i++)
				{
					this->voxelTextures.push_back(VoxelTextureData(std::string(textureName).c_str(), i));
				}
			}
		}
		else
		{
			// There is existing floor state (or it is in the default state with box cap 
			// ID unset), so this line is expected to be a filename.
			const int currentIndex = [this, &floorState, &line]()
			{
				// If the line contains a '#', it's a .SET file.
				const std::vector<std::string_view> tokens = StringView::split(line, '#');

				// Assign texture data depending on whether the line is for a .SET file.
				if (tokens.size() == 1)
				{
					// Just a regular texture (like an .IMG).
					floorState->textureName = line;

					this->voxelTextures.push_back(
						VoxelTextureData(std::string(floorState->textureName).c_str()));
					return static_cast<int>(this->voxelTextures.size()) - 1;
				}
				else
				{
					// Left side is the filename, right side is the .SET size.
					floorState->textureName = StringView::trimBack(tokens.at(0));
					const int setSize = std::stoi(std::string(tokens.at(1)));

					for (int i = 0; i < setSize; i++)
					{
						this->voxelTextures.push_back(
							VoxelTextureData(std::string(floorState->textureName).c_str(), i));
					}

					return static_cast<int>(this->voxelTextures.size()) - setSize;
				}
			}();

			// Write the boxcap data if a *BOXCAP line is currently stored in the floor state.
			// The floor state ID will be unset for loose filenames that don't have an 
			// associated *BOXCAP line, but might have an associated *CEILING line.
			if (floorState->boxCapID.has_value())
			{
				this->boxCaps.at(floorState->boxCapID.value()) = currentIndex;
			}

			// Write to the ceiling data if it is being defined for the current group.
			if (floorState->ceilingData.has_value())
			{
				this->ceiling.textureIndex = currentIndex;
				this->ceiling.height = floorState->ceilingData->height;
				this->ceiling.boxScale = std::move(floorState->ceilingData->boxScale);
				this->ceiling.outdoorDungeon = floorState->ceilingData->outdoorDungeon;
			}

			// Reset the floor state for any future floor data.
			floorState = FloorState();
		}
	};

	auto parseWallLine = [this, &wallState](const std::string &line)
	{
		const char TYPE_CHAR = '*';

		// Decide what to do based on the first character. Otherwise, read the line
		// as a texture filename.
		if (line.front() == TYPE_CHAR)
		{
			// Initialize wall state if it is null.
			if (!wallState.has_value())
			{
				wallState = WallState();
			}

			// All the different possible '*' sections for walls.
			const std::string BOXCAP_STR = "BOXCAP";
			const std::string BOXSIDE_STR = "BOXSIDE";
			const std::string DOOR_STR = "DOOR"; // *DOOR is ignored.
			const std::string DRYCHASM_STR = "DRYCHASM";
			const std::string LAVACHASM_STR = "LAVACHASM";
			const std::string LEVELDOWN_STR = "LEVELDOWN";
			const std::string LEVELUP_STR = "LEVELUP";
			const std::string MENU_STR = "MENU"; // Exterior <-> interior transitions.
			const std::string TRANS_STR = "TRANS"; // *TRANS is ignored.
			const std::string TRANSWALKTHRU_STR = "TRANSWALKTHRU"; // *TRANSWALKTHRU is ignored.
			const std::string WALKTHRU_STR = "WALKTHRU"; // *WALKTHRU is ignored.
			const std::string WETCHASM_STR = "WETCHASM";

			// See what the type in the line is.
			const std::vector<std::string_view> tokens = StringView::split(line);
			const std::string_view firstToken = tokens.at(0);
			const std::string_view firstTokenType = firstToken.substr(1, firstToken.size() - 1);

			if (firstTokenType == BOXCAP_STR)
			{
				wallState->mode = WallState::Mode::BoxCap;
				wallState->boxCapIDs.push_back(std::stoi(std::string(tokens.at(1))));
			}
			else if (firstTokenType == BOXSIDE_STR)
			{
				wallState->mode = WallState::Mode::BoxSide;
				wallState->boxSideIDs.push_back(std::stoi(std::string(tokens.at(1))));
			}
			else if (firstTokenType == DOOR_STR)
			{
				// Ignore *DOOR lines explicitly so they aren't "unrecognized".
				static_cast<void>(firstTokenType);
			}
			else if (firstTokenType == DRYCHASM_STR)
			{
				wallState->mode = WallState::Mode::DryChasm;
				wallState->dryChasm = true;
			}
			else if (firstTokenType == LAVACHASM_STR)
			{
				wallState->mode = WallState::Mode::LavaChasm;
				wallState->lavaChasm = true;
			}
			else if (firstTokenType == LEVELDOWN_STR)
			{
				wallState->mode = WallState::Mode::LevelDown;
			}
			else if (firstTokenType == LEVELUP_STR)
			{
				wallState->mode = WallState::Mode::LevelUp;
			}
			else if (firstTokenType == MENU_STR)
			{
				wallState->mode = WallState::Mode::Menu;
				wallState->menuID = std::stoi(std::string(tokens.at(1)));
			}
			else if (firstTokenType == TRANS_STR)
			{
				// Ignore *TRANS lines (unused).
				static_cast<void>(firstTokenType);
			}
			else if (firstTokenType == TRANSWALKTHRU_STR)
			{
				// Ignore *TRANSWALKTHRU lines (unused).
				static_cast<void>(firstTokenType);
			}
			else if (firstTokenType == WALKTHRU_STR)
			{
				// Ignore *WALKTHRU lines (unused).
				static_cast<void>(firstTokenType);
			}
			else if (firstTokenType == WETCHASM_STR)
			{
				wallState->mode = WallState::Mode::WetChasm;
				wallState->wetChasm = true;
			}
			else
			{
				DebugCrash("Unrecognized @WALLS section \"" + std::string(firstTokenType) + "\".");
			}
		}
		else if (!wallState.has_value())
		{
			// No existing wall state, so this line contains a "loose" texture name.
			const std::vector<std::string_view> tokens = StringView::split(line, '#');

			if (tokens.size() == 1)
			{
				// A regular filename (like an .IMG).
				this->voxelTextures.push_back(VoxelTextureData(line.c_str()));
			}
			else
			{
				// A .SET filename. Expand it for each of the .SET indices.
				const std::string_view textureName = StringView::trimBack(tokens.at(0));
				const int setSize = std::stoi(std::string(tokens.at(1)));

				for (int i = 0; i < setSize; i++)
				{
					this->voxelTextures.push_back(VoxelTextureData(
						std::string(textureName).c_str(), i));
				}
			}
		}
		else
		{
			// There is existing wall state, so this line contains a texture name associated 
			// with some '*' section(s).
			const int currentIndex = [this, &wallState, &line]()
			{
				// If the line contains a '#', it's a .SET file.
				const std::vector<std::string_view> tokens = StringView::split(line, '#');

				// Assign texture data depending on whether the line is for a .SET file.
				if (tokens.size() == 1)
				{
					// Just a regular texture (like an .IMG).
					wallState->textureName = line;

					this->voxelTextures.push_back(
						VoxelTextureData(std::string(wallState->textureName).c_str()));
					return static_cast<int>(this->voxelTextures.size()) - 1;
				}
				else
				{
					// Left side is the filename, right side is the .SET size.
					wallState->textureName = StringView::trimBack(tokens.at(0));
					const int setSize = std::stoi(std::string(tokens.at(1)));

					for (int i = 0; i < setSize; i++)
					{
						this->voxelTextures.push_back(
							VoxelTextureData(std::string(wallState->textureName).c_str(), i));
					}

					return static_cast<int>(this->voxelTextures.size()) - setSize;
				}
			}();

			// Write ID-related data for each tag (*BOXCAP, *BOXSIDE, etc.) found in the 
			// current wall state.
			for (int boxCapID : wallState->boxCapIDs)
			{
				this->boxCaps.at(boxCapID) = currentIndex;
			}

			for (int boxSideID : wallState->boxSideIDs)
			{
				this->boxSides.at(boxSideID) = currentIndex;
			}

			// Write *MENU ID (if any).
			if (wallState->menuID.has_value())
			{
				this->menus.at(wallState->menuID.value()) = currentIndex;
			}

			// Write texture index for any chasms.
			if (wallState->dryChasm)
			{
				this->dryChasmIndex = currentIndex;
			}
			else if (wallState->lavaChasm)
			{
				this->lavaChasmIndex = currentIndex;
			}
			else if (wallState->wetChasm)
			{
				this->wetChasmIndex = currentIndex;
			}

			// Write the texture index based on remaining wall modes.
			if (wallState->mode == WallState::Mode::LevelDown)
			{
				this->levelDownIndex = currentIndex;
			}
			else if (wallState->mode == WallState::Mode::LevelUp)
			{
				this->levelUpIndex = currentIndex;
			}

			wallState = WallState();
		}
	};

	auto parseFlatLine = [this, &flatState](const std::string &line)
	{
		const char TYPE_CHAR = '*';

		// Check if the first character is a '*' for an *ITEM line. Otherwise, read the line 
		// as a texture filename, and check for extra tokens on the right (F:, Y:, etc.).
		if (line.front() == TYPE_CHAR)
		{
			// Initialize flat state if it is null.
			if (!flatState.has_value())
			{
				flatState = FlatState();
			}

			const std::string ITEM_STR = "ITEM";

			// See what the type in the line is.
			const std::vector<std::string_view> tokens = StringView::split(line);
			const std::string_view firstToken = tokens.at(0);
			const std::string_view firstTokenType = firstToken.substr(1, firstToken.size() - 1);

			if (firstTokenType == ITEM_STR)
			{
				flatState->mode = FlatState::Mode::Item;
				flatState->itemID = std::stoi(std::string(tokens.at(1)));
			}
			else
			{
				DebugCrash("Unrecognized @FLATS section \"" + std::string(firstTokenType) + "\".");
			}
		}
		else
		{
			// Separator for each modifier value to the right of the flat name.
			const char MODIFIER_SEPARATOR = ':';

			// A texture name potentially after an *ITEM line, and potentially with some 
			// modifiers on the right. Each token might be split by tabs or spaces, so always 
			// check for both cases. The texture name always has a tab on the right though 
			// (if there's any whitespace).
			const std::vector<std::string> tokens = [&line, MODIFIER_SEPARATOR]()
			{
				// Trim any extra whitespace (so there are no adjacent duplicates).
				const std::string trimmedStr = String::trimExtra(line);

				// Replace tabs with spaces.
				const std::string replacedStr = String::replace(trimmedStr, '\t', ' ');

				// Special case at *ITEM 55 in CRYSTAL3.INF: do not split on whitespace,
				// because there are no modifiers.
				if (replacedStr.find(MODIFIER_SEPARATOR) == std::string::npos)
				{
					return std::vector<std::string>{ replacedStr };
				}
				else
				{
					// @todo: refine String::split() to account for whitespace in general so
					// we can avoid doing the extra steps above.
					return String::split(replacedStr);
				}
			}();

			// Get the texture name. Creature flats are between *ITEM 32 and *ITEM 54. These do
			// not need their texture line parsed because their animation filename is fetched
			// later as a .CFA (supposedly the placeholder .DFAs are for the level editor).
			const std::string textureName = [&tokens]()
			{
				const std::string &firstToken = tokens.at(0);
				const bool hasDash = firstToken.at(0) == '-'; // @todo: not sure what this is.
				return String::toUppercase(hasDash ?
					firstToken.substr(1, firstToken.size() - 1) : firstToken);
			}();

			// Add the flat's texture name to the textures vector.
			this->flatTextures.push_back(FlatTextureData(textureName.c_str()));

			// Add a new flat data record.
			this->flats.push_back(INFFile::FlatData());

			// Assign the current line's values and modifiers to the new flat.
			INFFile::FlatData &flat = this->flats.back();
			flat.textureIndex = static_cast<int>(this->flatTextures.size() - 1);
			flat.itemIndex = flatState.has_value() ? flatState->itemID : std::nullopt;

			// If the flat has modifiers, then check each modifier and mutate the flat accordingly.
			// If it is a creature then it will ignore these modifiers and use ones from the creature
			// arrays in the .exe data.
			if (tokens.size() >= 2)
			{
				for (size_t i = 1; i < tokens.size(); i++)
				{
					const char FLAT_PROPERTIES_MODIFIER = 'F';
					const char LIGHT_MODIFIER = 'S';
					const char Y_OFFSET_MODIFIER = 'Y';

					const std::string_view modifierStr = tokens[i];
					const char modifierType = std::toupper(modifierStr.at(0));

					// The modifier value comes after the modifier separator.
					const std::vector<std::string_view> modifierTokens =
						StringView::split(modifierStr, MODIFIER_SEPARATOR);
					const int modifierValue = std::stoi(std::string(modifierTokens.at(1)));

					if (modifierType == FLAT_PROPERTIES_MODIFIER)
					{
						// Flat properties (collider, puddle, triple scale, transparent, etc.).
						flat.collider = (modifierValue & (1 << 0)) != 0;
						flat.puddle = (modifierValue & (1 << 1)) != 0;
						flat.largeScale = (modifierValue & (1 << 2)) != 0;
						flat.dark = (modifierValue & (1 << 3)) != 0;
						flat.transparent = (modifierValue & (1 << 4)) != 0;
						flat.ceiling = (modifierValue & (1 << 5)) != 0;
						flat.mediumScale = (modifierValue & (1 << 6)) != 0;
					}
					else if (modifierType == LIGHT_MODIFIER)
					{
						// Light range (in units of voxels).
						flat.lightIntensity = modifierValue;
					}
					else if (modifierType == Y_OFFSET_MODIFIER)
					{
						// Y offset in world (for flying entities, hanging chains, etc.).
						flat.yOffset = modifierValue;
					}
					else
					{
						DebugCrash("Unrecognized modifier \""
							+ std::to_string(modifierType) + "\".");
					}
				}
			}

			// Reset flat state for the next loop.
			flatState = FlatState();
		}
	};

	auto parseSoundLine = [this](const std::string &line)
	{
		// Split into the filename and ID. Make sure the filename is all caps.
		const std::vector<std::string_view> tokens = StringView::split(line);
		std::string vocFilename = String::toUppercase(std::string(tokens.front()));
		const int vocID = std::stoi(std::string(tokens.at(1)));
		this->sounds.emplace(vocID, std::move(vocFilename));
	};

	auto parseTextLine = [this, &textState, &flushTextState](const std::string &line)
	{
		// Start a new text state after each *TEXT tag.
		constexpr char TEXT_CHAR = '*';
		constexpr char KEY_INDEX_CHAR = '+';
		constexpr char RIDDLE_CHAR = '^';
		constexpr char DISPLAYED_ONCE_CHAR = '~';

		// Check the first character in the line to determine any changes in text mode.
		// Otherwise, parse the line based on the current mode.
		if (line.front() == TEXT_CHAR)
		{
			const std::vector<std::string_view> tokens = StringView::split(line);

			// Get the ID after *TEXT.
			const int textID = std::stoi(std::string(tokens.at(1)));

			// If there is existing text state present, save it.
			if (textState.has_value())
			{
				flushTextState();
			}

			// Reset the text state to default with the new *TEXT ID.
			textState = TextState(textID);
		}
		else if (line.front() == KEY_INDEX_CHAR)
		{
			// Get key number. No need for a key section here since it's only one line.
			const std::string_view keyStr = StringView::substr(line, 1, line.size() - 1);
			const int keyNumber = std::stoi(std::string(keyStr));

			textState->mode = TextState::Mode::Key;
			textState->keyData = KeyData(keyNumber);
		}
		else if (line.front() == RIDDLE_CHAR)
		{
			// Get riddle numbers.
			const std::string_view numbers = StringView::substr(line, 1, line.size() - 1);
			const std::vector<std::string_view> tokens = StringView::split(numbers);
			const int firstNumber = std::stoi(std::string(tokens.at(0)));
			const int secondNumber = std::stoi(std::string(tokens.at(1)));

			textState->mode = TextState::Mode::Riddle;
			textState->riddleState = TextState::RiddleState(firstNumber, secondNumber);
		}
		else if (line.front() == DISPLAYED_ONCE_CHAR)
		{
			textState->mode = TextState::Mode::Text;

			const bool displayedOnce = true;
			textState->textData = TextData(displayedOnce);

			// Append the rest of the line to the text data.
			textState->textData->text += line.substr(1, line.size() - 1) + '\n';
		}
		else if (textState->mode == TextState::Mode::Riddle)
		{
			const char ANSWER_CHAR = ':'; // An accepted answer.
			const char RESPONSE_SECTION_CHAR = '`'; // CORRECT/WRONG.

			if (line.front() == ANSWER_CHAR)
			{
				// Add the answer to the answers data.
				const std::string_view answer = StringView::substr(line, 1, line.size() - 1);
				textState->riddleState->data.answers.push_back(std::string(answer));
			}
			else if (line.front() == RESPONSE_SECTION_CHAR)
			{
				// Change riddle mode based on the response section.
				const std::string CORRECT_STR = "CORRECT";
				const std::string WRONG_STR = "WRONG";
				const std::string_view responseSection = StringView::substr(line, 1, line.size() - 1);

				if (responseSection == CORRECT_STR)
				{
					textState->riddleState->mode = TextState::RiddleState::Mode::Correct;
				}
				else if (responseSection == WRONG_STR)
				{
					textState->riddleState->mode = TextState::RiddleState::Mode::Wrong;
				}
			}
			else if (textState->riddleState->mode == TextState::RiddleState::Mode::Riddle)
			{
				// Read the line into the riddle text.
				textState->riddleState->data.riddle += line + '\n';
			}
			else if (textState->riddleState->mode == TextState::RiddleState::Mode::Correct)
			{
				// Read the line into the correct text.
				textState->riddleState->data.correct += line + '\n';
			}
			else if (textState->riddleState->mode == TextState::RiddleState::Mode::Wrong)
			{
				// Read the line into the wrong text.
				textState->riddleState->data.wrong += line + '\n';
			}
		}
		else if (textState->mode == TextState::Mode::Text)
		{
			// Read the line into the text data.
			textState->textData->text += line + '\n';
		}
		else
		{
			// Plain old text after a *TEXT line, and on rare occasions it's after a key 
			// line (+123, like in AGTEMPL.INF).
			if ((textState->mode == TextState::Mode::None) ||
				(textState->mode == TextState::Mode::Key))
			{
				if (textState->mode == TextState::Mode::Key)
				{
					// Save key data and empty the key data state.
					this->keys.insert(std::make_pair(textState->id, textState->keyData.value()));
					textState->keyData.reset();
				}

				textState->mode = TextState::Mode::Text;

				const bool displayedOnce = false;
				textState->textData = TextData(displayedOnce);
			}

			// Read the line into the text data.
			textState->textData->text += line + '\n';
		}
	};

	// Default to "@FLOORS" since the final staff piece dungeon doesn't have that
	// tag even when it's needed.
	ParseMode parseMode = ParseMode::Floors;

	std::stringstream ss(text);
	std::string line;

	while (std::getline(ss, line))
	{
		const char SECTION_SEPARATOR = '@';

		// First check if the line is empty. Then check the first character for any changes 
		// in the current section. Otherwise, parse the line depending on the current mode.
		if (line.size() == 0)
		{
			// Usually, empty lines indicate a separation from two sections, but there are 
			// some riddles with newlines (like *TEXT 0 in LABRNTH2.INF), so don't skip those.
			if (textState.has_value() && textState->riddleState.has_value() &&
				(textState->riddleState->mode == TextState::RiddleState::Mode::Riddle))
			{
				textState->riddleState->data.riddle += '\n';
			}
			else
			{
				// Save any current state into INFFile members.
				flushAllStates();
			}
		}
		else if (line.front() == SECTION_SEPARATOR)
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
			const std::vector<std::string_view> tokens = StringView::split(line);
			line = std::string(tokens.front());

			// See which token the section is.
			const auto sectionIter = Sections.find(line);
			DebugAssertMsg(sectionIter != Sections.end(),
				"Unrecognized .INF section \"" + line + "\".");

			// Flush any existing state.
			flushAllStates();

			// Assign the new parse mode.
			parseMode = sectionIter->second;
		}
		else if (parseMode == ParseMode::Floors)
		{
			parseFloorLine(line);
		}
		else if (parseMode == ParseMode::Walls)
		{
			parseWallLine(line);
		}
		else if (parseMode == ParseMode::Flats)
		{
			parseFlatLine(line);
		}
		else if (parseMode == ParseMode::Sound)
		{
			parseSoundLine(line);
		}
		else if (parseMode == ParseMode::Text)
		{
			parseTextLine(line);
		}
	}

	// Flush any remaining data. Most of these won't ever need flushing -- it's 
	// primarily for @TEXT since it's frequently the last section in the file
	// and has the possibility of an off-by-one error with its *TEXT saving.
	flushAllStates();

	return true;
}

const std::vector<INFFile::VoxelTextureData> &INFFile::getVoxelTextures() const
{
	return this->voxelTextures;
}

const std::vector<INFFile::FlatTextureData> &INFFile::getFlatTextures() const
{
	return this->flatTextures;
}

const std::optional<int> &INFFile::getBoxCap(int index) const
{
	DebugAssertIndex(this->boxCaps, index);
	return this->boxCaps[index];
}

const std::optional<int> &INFFile::getBoxSide(int index) const
{
	DebugAssertIndex(this->boxSides, index);
	const std::optional<int> &opt = this->boxSides[index];

	// This needs to handle errors in the Arena data (i.e., the initial level in some noble houses
	// asks for wall texture #14, which doesn't exist in NOBLE1.INF).
	if (opt.has_value())
	{
		return opt;
	}
	else
	{
		DebugLogWarning("Invalid *BOXSIDE index \"" + std::to_string(index) + "\".");
		DebugAssert(this->boxSides.size() > 0);
		return this->boxSides[0];
	}
}

const std::optional<int> &INFFile::getMenu(int index) const
{
	DebugAssertIndex(this->menus, index);
	return this->menus[index];
}

std::optional<int> INFFile::getMenuIndex(int textureID) const
{
	const auto iter = std::find(this->menus.begin(), this->menus.end(), textureID);
	if (iter != this->menus.end())
	{
		return static_cast<int>(std::distance(this->menus.begin(), iter));
	}
	else
	{
		return std::nullopt;
	}
}

const INFFile::FlatData &INFFile::getFlat(int index) const
{
	DebugAssertIndex(this->flats, index);
	return this->flats[index];
}

const INFFile::FlatData *INFFile::getFlatWithItemIndex(ArenaTypes::ItemIndex itemIndex) const
{
	const auto iter = std::find_if(this->flats.begin(), this->flats.end(),
		[itemIndex](const FlatData &flat)
	{
		return flat.itemIndex.has_value() && (*flat.itemIndex == itemIndex);
	});

	return (iter != this->flats.end()) ? &(*iter) : nullptr;
}

const char *INFFile::getSound(int index) const
{
	const auto soundIter = this->sounds.find(index);

	// The sound indices are sometimes out-of-bounds, which means that the program
	// needs to modify them in some way. For now, just print a warning and return
	// some default sound.
	if (soundIter != this->sounds.end())
	{
		return soundIter->second.c_str();
	}
	else
	{
		DebugLogWarning("Invalid sound index \"" + std::to_string(index) + "\".");
		return this->sounds.at(0).c_str();
	}
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

const char *INFFile::getName() const
{
	return this->name.c_str();
}

const std::optional<int> &INFFile::getDryChasmIndex() const
{
	return this->dryChasmIndex;
}

const std::optional<int> &INFFile::getLavaChasmIndex() const
{
	return this->lavaChasmIndex;
}

const std::optional<int> &INFFile::getLevelDownIndex() const
{
	return this->levelDownIndex;
}

const std::optional<int> &INFFile::getLevelUpIndex() const
{
	return this->levelUpIndex;
}

const std::optional<int> &INFFile::getWetChasmIndex() const
{
	return this->wetChasmIndex;
}

const INFFile::CeilingData &INFFile::getCeiling() const
{
	return this->ceiling;
}
