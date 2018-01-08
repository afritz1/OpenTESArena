#include <algorithm>
#include <cctype>
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

	// Each '@' section may or may not have some state it currently possesses. They 
	// also have a mode they can be in, via a tag like *BOXCAP or *TEXT.
	struct FloorState
	{
		enum class Mode { None, BoxCap, Ceiling };

		std::unique_ptr<INFFile::CeilingData> ceilingData; // Non-null when present.
		std::string textureName;
		FloorState::Mode mode;
		int boxCapID;

		FloorState()
		{
			this->mode = FloorState::Mode::None;
			this->boxCapID = INFFile::NO_INDEX;
		}
	};

	struct WallState
	{
		enum class Mode
		{
			None, BoxCap, BoxSide, DryChasm, LavaChasm, LevelDown, LevelUp,
			Menu, Transition, TransWalkThru, WalkThru, WetChasm
		};

		std::vector<int> boxCapIDs, boxSideIDs;
		std::string textureName;
		WallState::Mode mode;
		int menuID;
		bool dryChasm, lavaChasm, wetChasm;

		WallState()
		{
			this->mode = WallState::Mode::None;
			this->menuID = INFFile::NO_INDEX;
			this->dryChasm = false;
			this->lavaChasm = false;
			this->wetChasm = false;
		}
	};

	struct FlatState
	{
		enum class Mode { None, Item };

		FlatState::Mode mode;
		int itemID;

		FlatState()
		{
			this->mode = FlatState::Mode::None;
			this->itemID = INFFile::NO_INDEX;
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

		std::unique_ptr<INFFile::KeyData> keyData;
		std::unique_ptr<RiddleState> riddleState;
		std::unique_ptr<INFFile::TextData> textData;
		TextState::Mode mode; // Determines which data is in use.
		int id; // *TEXT ID.

		TextState(int id)
		{
			this->mode = TextState::Mode::None;
			this->id = id;
		}
	};
}

INFFile::VoxelTextureData::VoxelTextureData(const std::string &filename, int setIndex)
	: filename(filename), setIndex(setIndex) { }

INFFile::VoxelTextureData::VoxelTextureData(const std::string &filename)
	: filename(filename), setIndex(INFFile::NO_INDEX) { }

INFFile::FlatTextureData::FlatTextureData(const std::string &filename)
	: filename(filename) { }

INFFile::CeilingData::CeilingData()
{
	const int defaultHeight = 128;
	const double defaultScale = 1.0;

	this->textureIndex = INFFile::NO_INDEX;
	this->height = defaultHeight;
	this->boxScale = defaultScale;
	this->outdoorDungeon = false;
}

INFFile::FlatData::FlatData()
{
	this->textureIndex = INFFile::NO_INDEX;
	this->yOffset = 0;
	this->health = 0;
	this->collider = false;
	this->puddle = false;
	this->doubleScale = false;
	this->dark = false;
	this->transparent = false;
	this->ceiling = false;
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

const int INFFile::NO_INDEX = -1;

INFFile::INFFile(const std::string &filename)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	DebugAssert(stream != nullptr, "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	std::vector<uint8_t> srcData(stream->tellg());
	stream->seekg(0, std::ios::beg);
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

	// Initialize texture indices to "unset".
	this->boxCaps.fill(INFFile::NO_INDEX);
	this->boxSides.fill(INFFile::NO_INDEX);
	this->menus.fill(INFFile::NO_INDEX);
	this->items.fill(INFFile::NO_INDEX);
	this->dryChasmIndex = INFFile::NO_INDEX;
	this->lavaChasmIndex = INFFile::NO_INDEX;
	this->levelDownIndex = INFFile::NO_INDEX;
	this->levelUpIndex = INFFile::NO_INDEX;
	this->transitionIndex = INFFile::NO_INDEX;
	this->transWalkThruIndex = INFFile::NO_INDEX;
	this->walkThruIndex = INFFile::NO_INDEX;
	this->wetChasmIndex = INFFile::NO_INDEX;

	// Assign the data (now decoded if it was encoded) to the text member exposed
	// to the rest of the program.
	std::string text(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// Remove carriage returns (newlines are nicer to work with).
	text = String::replace(text, "\r", "");

	// The parse mode indicates which '@' section is currently being parsed.
	enum class ParseMode
	{
		Floors, Walls, Flats, Sound, Text
	};

	// Initialize loop states to null (they are non-null when in use by the loop).
	// I tried re-organizing them into virtual classes, but this way seems to be the
	// most practical for now.
	std::unique_ptr<FloorState> floorState;
	std::unique_ptr<WallState> wallState;
	std::unique_ptr<FlatState> flatState;
	std::unique_ptr<TextState> textState;

	// Lambda for flushing state to the INFFile. This is useful during the parse loop,
	// but it's also sometimes necessary at the end of the file because the last element 
	// of certain sections (i.e., @TEXT) might get missed if there is no data after them.
	auto flushTextState = [this, &textState]()
	{
		if (textState->mode == TextState::Mode::Key)
		{
			// Save key data.
			this->keys.insert(std::make_pair(textState->id, *textState->keyData.get()));
		}
		else if (textState->mode == TextState::Mode::Riddle)
		{
			// Save riddle data.
			this->riddles.insert(std::make_pair(textState->id, textState->riddleState->data));
		}
		else if (textState->mode == TextState::Mode::Text)
		{
			// Save text data.
			this->texts.insert(std::make_pair(textState->id, *textState->textData.get()));
		}
	};

	// Lambda for flushing all states. Most states don't need an explicit flush because 
	// they have no risk of leaving data behind.
	auto flushAllStates = [&floorState, &wallState, &flatState, 
		&textState, &flushTextState]()
	{
		if (floorState.get() != nullptr)
		{
			floorState = nullptr;
		}

		if (wallState.get() != nullptr)
		{
			wallState = nullptr;
		}

		if (flatState.get() != nullptr)
		{
			flatState = nullptr;
		}

		if (textState.get() != nullptr)
		{
			flushTextState();
			textState = nullptr;
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
			if (floorState.get() == nullptr)
			{
				floorState = std::unique_ptr<FloorState>(new FloorState());
			}

			const std::string BOXCAP_STR = "BOXCAP";
			const std::string CEILING_STR = "CEILING";
			const std::string TOP_STR = "TOP"; // Only occurs in LABRNTH{1,2}.INF.

			// See what the type in the line is.
			const std::vector<std::string> tokens = String::split(line);
			const std::string firstToken = tokens.at(0);
			const std::string firstTokenType = firstToken.substr(1, firstToken.size() - 1);

			if (firstTokenType == BOXCAP_STR)
			{
				// Write the *BOXCAP's ID to the floor state.
				floorState->boxCapID = std::stoi(tokens.at(1));
				floorState->mode = FloorState::Mode::BoxCap;
			}
			else if (firstTokenType == CEILING_STR)
			{
				// Initialize ceiling data.
				floorState->ceilingData = std::unique_ptr<CeilingData>(new CeilingData());
				floorState->mode = FloorState::Mode::Ceiling;

				// Check up to three numbers on the right: ceiling height, box scale,
				// and indoor/outdoor dungeon boolean. Sometimes there are no numbers.
				if (tokens.size() >= 2)
				{
					floorState->ceilingData->height = std::stoi(tokens.at(1));
				}

				if (tokens.size() >= 3)
				{
					// To do: This might need some more math. (Y * boxScale) / 256?
					floorState->ceilingData->boxScale = std::stoi(tokens.at(2));
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
				DebugCrash("Unrecognized @FLOOR section \"" + tokens.at(0) + "\".");
			}
		}
		else if (floorState.get() == nullptr)
		{
			// No current floor state, so the current line is a loose texture filename
			// (found in some city .INFs).
			const std::vector<std::string> tokens = String::split(line, '#');

			if (tokens.size() == 1)
			{
				// A regular filename (like an .IMG).
				this->voxelTextures.push_back(VoxelTextureData(line));
			}
			else
			{
				// A .SET filename. Expand it for each of the .SET indices.
				const std::string textureName = String::trimBack(tokens.at(0));
				const int setSize = std::stoi(tokens.at(1));

				for (int i = 0; i < setSize; i++)
				{
					this->voxelTextures.push_back(VoxelTextureData(textureName, i));
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
				const std::vector<std::string> tokens = String::split(line, '#');

				// Assign texture data depending on whether the line is for a .SET file.
				if (tokens.size() == 1)
				{
					// Just a regular texture (like an .IMG).
					floorState->textureName = line;

					this->voxelTextures.push_back(VoxelTextureData(floorState->textureName));
					return static_cast<int>(this->voxelTextures.size()) - 1;
				}
				else
				{
					// Left side is the filename, right side is the .SET size.
					floorState->textureName = String::trimBack(tokens.at(0));
					const int setSize = std::stoi(tokens.at(1));

					for (int i = 0; i < setSize; i++)
					{
						this->voxelTextures.push_back(
							VoxelTextureData(floorState->textureName, i));
					}

					return static_cast<int>(this->voxelTextures.size()) - setSize;
				}
			}();

			// Write the boxcap data if a *BOXCAP line is currently stored in the floor state.
			// The floor state ID will be unset for loose filenames that don't have an 
			// associated *BOXCAP line, but might have an associated *CEILING line.
			if (floorState->boxCapID != INFFile::NO_INDEX)
			{
				this->boxCaps.at(floorState->boxCapID) = currentIndex;
			}

			// Write to the ceiling data if it is being defined for the current group.
			if (floorState->ceilingData.get() != nullptr)
			{
				this->ceiling.textureIndex = currentIndex;
				this->ceiling.height = floorState->ceilingData->height;
				this->ceiling.boxScale = floorState->ceilingData->boxScale;
				this->ceiling.outdoorDungeon = floorState->ceilingData->outdoorDungeon;
			}

			// Reset the floor state for any future floor data.
			floorState = std::unique_ptr<FloorState>(new FloorState());
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
			if (wallState.get() == nullptr)
			{
				wallState = std::unique_ptr<WallState>(new WallState());
			}

			// All the different possible '*' sections for walls.
			const std::string BOXCAP_STR = "BOXCAP";
			const std::string BOXSIDE_STR = "BOXSIDE";
			const std::string DOOR_STR = "DOOR"; // *DOOR is ignored.
			const std::string DRYCHASM_STR = "DRYCHASM";
			const std::string LAVACHASM_STR = "LAVACHASM";
			const std::string LEVELDOWN_STR = "LEVELDOWN";
			const std::string LEVELUP_STR = "LEVELUP";
			const std::string MENU_STR = "MENU"; // Doors leading to interiors.
			const std::string TRANS_STR = "TRANS";
			const std::string TRANSWALKTHRU_STR = "TRANSWALKTHRU"; // Store signs, hedge archways. Ignored?
			const std::string WALKTHRU_STR = "WALKTHRU"; // Probably for hedge archways.
			const std::string WETCHASM_STR = "WETCHASM";

			// See what the type in the line is.
			const std::vector<std::string> tokens = String::split(line);
			const std::string firstToken = tokens.at(0);
			const std::string firstTokenType = firstToken.substr(1, firstToken.size() - 1);

			if (firstTokenType == BOXCAP_STR)
			{
				wallState->mode = WallState::Mode::BoxCap;
				wallState->boxCapIDs.push_back(std::stoi(tokens.at(1)));
			}
			else if (firstTokenType == BOXSIDE_STR)
			{
				wallState->mode = WallState::Mode::BoxSide;
				wallState->boxSideIDs.push_back(std::stoi(tokens.at(1)));
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
				wallState->menuID = std::stoi(tokens.at(1));
			}
			else if (firstTokenType == TRANS_STR)
			{
				wallState->mode = WallState::Mode::Transition;
			}
			else if (firstTokenType == TRANSWALKTHRU_STR)
			{
				wallState->mode = WallState::Mode::TransWalkThru;
			}
			else if (firstTokenType == WALKTHRU_STR)
			{
				wallState->mode = WallState::Mode::WalkThru;
			}
			else if (firstTokenType == WETCHASM_STR)
			{
				wallState->mode = WallState::Mode::WetChasm;
				wallState->wetChasm = true;
			}
			else
			{
				DebugCrash("Unrecognized @WALLS section \"" + firstTokenType + "\".");
			}
		}
		else if (wallState.get() == nullptr)
		{
			// No existing wall state, so this line contains a "loose" texture name.
			const std::vector<std::string> tokens = String::split(line, '#');

			if (tokens.size() == 1)
			{
				// A regular filename (like an .IMG).
				this->voxelTextures.push_back(VoxelTextureData(line));
			}
			else
			{
				// A .SET filename. Expand it for each of the .SET indices.
				const std::string textureName = String::trimBack(tokens.at(0));
				const int setSize = std::stoi(tokens.at(1));

				for (int i = 0; i < setSize; i++)
				{
					this->voxelTextures.push_back(VoxelTextureData(textureName, i));
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
				const std::vector<std::string> tokens = String::split(line, '#');

				// Assign texture data depending on whether the line is for a .SET file.
				if (tokens.size() == 1)
				{
					// Just a regular texture (like an .IMG).
					wallState->textureName = line;

					this->voxelTextures.push_back(VoxelTextureData(wallState->textureName));
					return static_cast<int>(this->voxelTextures.size()) - 1;
				}
				else
				{
					// Left side is the filename, right side is the .SET size.
					wallState->textureName = String::trimBack(tokens.at(0));
					const int setSize = std::stoi(tokens.at(1));

					for (int i = 0; i < setSize; i++)
					{
						this->voxelTextures.push_back(VoxelTextureData(wallState->textureName, i));
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
			if (wallState->menuID != INFFile::NO_INDEX)
			{
				this->menus.at(wallState->menuID) = currentIndex;
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
			else if (wallState->mode == WallState::Mode::Transition)
			{
				this->transitionIndex = currentIndex;
			}
			else if (wallState->mode == WallState::Mode::TransWalkThru)
			{
				this->transWalkThruIndex = currentIndex;
			}
			else if (wallState->mode == WallState::Mode::WalkThru)
			{
				this->walkThruIndex = currentIndex;
			}

			wallState = std::unique_ptr<WallState>(new WallState());
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
			if (flatState.get() == nullptr)
			{
				flatState = std::unique_ptr<FlatState>(new FlatState());
			}

			const std::string ITEM_STR = "ITEM";

			// See what the type in the line is.
			const std::vector<std::string> tokens = String::split(line);
			const std::string firstToken = tokens.at(0);
			const std::string firstTokenType = firstToken.substr(1, firstToken.size() - 1);

			if (firstTokenType == ITEM_STR)
			{
				flatState->mode = FlatState::Mode::Item;
				flatState->itemID = std::stoi(tokens.at(1));
			}
			else
			{
				DebugCrash("Unrecognized @FLATS section \"" + firstTokenType + "\".");
			}
		}
		else
		{
			// A texture name potentially after an *ITEM line, and potentially with some 
			// modifiers on the right. Each token might be split by tabs or spaces, so always 
			// check for both cases. The texture name always has a tab on the right though 
			// (if there's any whitespace).
			const std::vector<std::string> tokens = [&line]()
			{
				// Trim any extra whitespace (so there are no adjacent duplicates).
				const std::string trimmedStr = String::trimExtra(line);

				// Replace tabs with spaces.
				const std::string replacedStr = String::replace(trimmedStr, '\t', ' ');

				// To do: refine String::split() to account for whitespace in general so
				// we can avoid doing the extra steps above.
				return String::split(replacedStr);
			}();

			// Get the texture name and check if it starts with a dash.
			const std::string &firstToken = tokens.at(0);
			const bool hasDash = firstToken.at(0) == '-'; // To do: not sure what this does.
			const std::string textureName = String::toUppercase(
				hasDash ? firstToken.substr(1, firstToken.size() - 1) : firstToken);

			// Add the flat's texture name to the textures vector.
			this->flatTextures.push_back(FlatTextureData(textureName));

			// Add a new flat data record and keep its index.
			this->flats.push_back(INFFile::FlatData());
			const int flatIndex = static_cast<int>(this->flats.size() - 1);

			// The current line is "loose" if the previous line was not an *ITEM line.
			const bool looseTextureName = (flatState.get() == nullptr) || 
				(flatState->itemID == INFFile::NO_INDEX);

			// If an *ITEM index is currently stored, then pair it with the new flat's index.
			if (!looseTextureName)
			{
				this->items.at(flatState->itemID) = flatIndex;
			}

			// Assign the current line's values and modifiers to the new flat.
			INFFile::FlatData &flat = this->flats.back();
			flat.textureIndex = static_cast<int>(this->flatTextures.size() - 1);

			// If the flat also has modifiers, then check each modifier and mutate the 
			// flat accordingly.
			if (tokens.size() >= 2)
			{
				for (size_t i = 1; i < tokens.size(); i++)
				{
					const char MODIFIER_SEPARATOR = ':';
					const char FLAT_PROPERTIES_MODIFIER = 'F';
					const char LIGHT_MODIFIER = 'S';
					const char Y_OFFSET_MODIFIER = 'Y';

					const std::string &modifierStr = tokens.at(i);
					const char modifierType = std::toupper(modifierStr.at(0));

					// The modifier value comes after the modifier separator.
					const std::vector<std::string> modifierTokens =
						String::split(modifierStr, MODIFIER_SEPARATOR);
					const int modifierValue = std::stoi(modifierTokens.at(1));

					if (modifierType == FLAT_PROPERTIES_MODIFIER)
					{
						// Flat properties (collider, puddle, double scale, transparent, etc.).
						flat.collider = (modifierValue & (1 << 0)) != 0;
						flat.puddle = (modifierValue & (1 << 1)) != 0;
						flat.doubleScale = (modifierValue & (1 << 2)) != 0;
						flat.dark = (modifierValue & (1 << 3)) != 0;
						flat.transparent = (modifierValue & (1 << 4)) != 0;
						flat.ceiling = (modifierValue & (1 << 5)) != 0;
					}
					else if (modifierType == LIGHT_MODIFIER)
					{
						// Light range (in units of voxels).
						flat.lightIntensity = std::unique_ptr<int>(new int(modifierValue));
					}
					else if (modifierType == Y_OFFSET_MODIFIER)
					{
						// Y offset in world (for flying entities, hanging chains, etc.).
						flat.yOffset = modifierValue;
					}
					else
					{
						DebugCrash("Unrecognized modifier '" + std::to_string(modifierType) + "'.");
					}
				}
			}

			// Reset flat state for the next loop.
			flatState = std::unique_ptr<FlatState>(new FlatState());
		}
	};

	auto parseSoundLine = [this](const std::string &line)
	{
		// Split into the filename and ID. Make sure the filename is all caps.
		const std::vector<std::string> tokens = String::split(line);
		const std::string vocFilename = String::toUppercase(tokens.front());
		const int vocID = std::stoi(tokens.at(1));

		this->sounds.insert(std::make_pair(vocID, vocFilename));
	};

	auto parseTextLine = [this, &textState, &flushTextState](const std::string &line)
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
				flushTextState();
			}

			// Reset the text state to default with the new *TEXT ID.
			textState = std::unique_ptr<TextState>(new TextState(textID));
		}
		else if (line.front() == KEY_INDEX_CHAR)
		{
			// Get key number. No need for a key section here since it's only one line.
			const std::string keyStr = line.substr(1, line.size() - 1);
			const int keyNumber = std::stoi(keyStr);

			textState->mode = TextState::Mode::Key;
			textState->keyData = std::unique_ptr<KeyData>(new KeyData(keyNumber));
		}
		else if (line.front() == RIDDLE_CHAR)
		{
			// Get riddle numbers.
			const std::string numbers = line.substr(1, line.size() - 1);
			const std::vector<std::string> tokens = String::split(numbers);
			const int firstNumber = std::stoi(tokens.at(0));
			const int secondNumber = std::stoi(tokens.at(1));

			textState->mode = TextState::Mode::Riddle;
			textState->riddleState = std::unique_ptr<TextState::RiddleState>(
				new TextState::RiddleState(firstNumber, secondNumber));
		}
		else if (line.front() == DISPLAYED_ONCE_CHAR)
		{
			textState->mode = TextState::Mode::Text;

			const bool displayedOnce = true;
			textState->textData = std::unique_ptr<TextData>(new TextData(displayedOnce));

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
					this->keys.insert(std::make_pair(textState->id, *textState->keyData.get()));
					textState->keyData = nullptr;
				}

				textState->mode = TextState::Mode::Text;

				const bool displayedOnce = false;
				textState->textData = std::unique_ptr<TextData>(new TextData(displayedOnce));
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
			if ((textState.get() != nullptr) &&
				(textState->riddleState.get() != nullptr) &&
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
			line = String::split(line).front();

			// See which token the section is.
			const auto sectionIter = Sections.find(line);
			DebugAssert(sectionIter != Sections.end(),
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
}

INFFile::~INFFile()
{

}

const std::vector<INFFile::VoxelTextureData> &INFFile::getVoxelTextures() const
{
	return this->voxelTextures;
}

const std::vector<INFFile::FlatTextureData> &INFFile::getFlatTextures() const
{
	return this->flatTextures;
}

const int *INFFile::getBoxCap(int index) const
{
	const int *ptr = &this->boxCaps.at(index);
	return ((*ptr) != INFFile::NO_INDEX) ? ptr : nullptr;
}

const int *INFFile::getBoxSide(int index) const
{
	const int *ptr = &this->boxSides.at(index);

	// Some null pointers were being returned here, and they appear to be errors within
	// the Arena data (i.e., the initial level in some noble houses asks for wall texture 
	// #14, which doesn't exist in NOBLE1.INF), so maybe it should resort to a default 
	// index instead.
	if ((*ptr) != INFFile::NO_INDEX)
	{
		return ptr;
	}
	else
	{
		DebugWarning("Invalid *BOXSIDE index \"" + std::to_string(index) + "\".");
		return &this->boxSides.at(0);
	}
}

const int *INFFile::getMenu(int index) const
{
	const int *ptr = &this->menus.at(index);
	return ((*ptr) != INFFile::NO_INDEX) ? ptr : nullptr;
}

int INFFile::getMenuIndex(int textureID) const
{
	// Returns the index of the texture ID in the menus array, or -1 if not found.
	const auto iter = std::find(this->menus.begin(), this->menus.end(), textureID);
	return (iter != this->menus.end()) ? 
		static_cast<int>(iter - this->menus.begin()) : -1;
}

const INFFile::FlatData &INFFile::getFlat(int index) const
{
	return this->flats.at(index);
}

const INFFile::FlatData &INFFile::getItem(int index) const
{
	const int flatIndex = this->items.at(index);
	return this->flats.at(flatIndex);
}

const std::string &INFFile::getSound(int index) const
{
	const auto soundIter = this->sounds.find(index);

	// The sound indices are sometimes out-of-bounds, which means that the program
	// needs to modify them in some way. For now, just print a warning and return
	// some default sound.
	if (soundIter != this->sounds.end())
	{
		return soundIter->second;
	}
	else
	{
		DebugWarning("Invalid sound index \"" + std::to_string(index) + "\".");
		return this->sounds.at(0);
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

const int *INFFile::getDryChasmIndex() const
{
	const int *ptr = &this->dryChasmIndex;
	return ((*ptr) != INFFile::NO_INDEX) ? ptr : nullptr;
}

const int *INFFile::getLavaChasmIndex() const
{
	const int *ptr = &this->lavaChasmIndex;
	return ((*ptr) != INFFile::NO_INDEX) ? ptr : nullptr;
}

const int *INFFile::getLevelDownIndex() const
{
	const int *ptr = &this->levelDownIndex;
	return ((*ptr) != INFFile::NO_INDEX) ? ptr : nullptr;
}

const int *INFFile::getLevelUpIndex() const
{
	const int *ptr = &this->levelUpIndex;
	return ((*ptr) != INFFile::NO_INDEX) ? ptr : nullptr;
}

const int *INFFile::getTransitionIndex() const
{
	const int *ptr = &this->transitionIndex;
	return ((*ptr) != INFFile::NO_INDEX) ? ptr : nullptr;
}

const int *INFFile::getTransWalkThruIndex() const
{
	const int *ptr = &this->transWalkThruIndex;
	return ((*ptr) != INFFile::NO_INDEX) ? ptr : nullptr;
}

const int *INFFile::getWalkThruIndex() const
{
	const int *ptr = &this->walkThruIndex;
	return ((*ptr) != INFFile::NO_INDEX) ? ptr : nullptr;
}

const int *INFFile::getWetChasmIndex() const
{
	const int *ptr = &this->wetChasmIndex;
	return ((*ptr) != INFFile::NO_INDEX) ? ptr : nullptr;
}

const INFFile::CeilingData &INFFile::getCeiling() const
{
	return this->ceiling;
}
