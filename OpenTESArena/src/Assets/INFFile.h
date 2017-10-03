#ifndef INF_FILE_H
#define INF_FILE_H

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// An .INF file contains definitions of what the IDs in a .MIF file point to. These 
// are mostly texture IDs, but also text IDs and sound IDs telling which voxels have 
// which kinds of triggers, etc..

class INFFile
{
public:
	// To do: this could be defined somewhere else in the code.
	enum class DoorType { Swing = 0, Slide = 1, Up = 2, Split = 3 };

	struct TextureData
	{
		std::string filename;
		std::unique_ptr<int> setIndex; // Non-null when texture is a .SET file.

		TextureData(const std::string &filename, int setIndex);
		TextureData(const std::string &filename);
		TextureData(TextureData&&) = default;
		TextureData();
		~TextureData();
	};

	struct CeilingData
	{
		TextureData texture;
		int height; // Size of walls on main floor. Default is 100.
		double boxScale; // Main floor box scale. Formula: (Y * boxScale) / 256.
		bool outdoorDungeon; // True when third *CEILING number is 1 (for main quest dungeons?).

		CeilingData();
	};

	struct FlatData
	{
		std::string filename;

		int yOffset; // Offsets the flat some number of pixels. Negative goes up.
		int health; // Number of hit points.

		// Bit array of flat modifiers?
		// 0x1: Collider. 0x2: Reflect (puddle). 0x4: Double (scale?). 0x8: Dark.
		// 0x10: Transparent (ghosts). 0x20: Ceiling (attached to ceiling?).
		uint16_t type;

		// Used with N:#, where '#' is the death effect. The "next flat" is probably 
		// used for displaying corpses.
		std::string nextFlat;
		std::unique_ptr<int> deathEffect;

		// Used with S:#, where '#' is light intensity (for candles, etc.).
		std::unique_ptr<int> lightIntensity;

		FlatData();
	};

	struct KeyData
	{
		int id; // Key ID (starts with '+').

		KeyData(int id);
	};

	struct RiddleData
	{
		std::vector<std::string> answers; // Accepted answers from the player.
		std::string riddle, correct, wrong;
		int firstNumber, secondNumber; // Not sure what these are.

		RiddleData(int firstNumber, int secondNumber);
	};

	struct TextData
	{
		std::string text; // Stores display text for a text trigger.
		bool displayedOnce; // Whether the text is only displayed once (starts with '~').

		TextData(bool displayedOnce);
	};
private:
	// Texture filenames in the order they are discovered. .SET files are expanded;
	// that is, a four-element .SET will occupy four consecutive indices, and each integer
	// pointer will contain the index (otherwise it is null).
	std::vector<TextureData> textures;

	// Each item list is a set of texture filenames. The highest *ITEM number is 95,
	// although those past 63 might not be used (character class names, lore names, etc.).
	std::array<std::vector<FlatData>, 96> itemLists;

	std::array<std::string, 16> boxcaps, boxsides;

	// Some .INFs have an outlier (*DOOR 90), so this shouldn't be a sequential data structure.
	std::unordered_map<int, std::string> doorTextures;

	// .VOC files for each sound ID.
	std::unordered_map<int, std::string> sounds;

	// Key info for *TEXT IDs.
	std::unordered_map<int, KeyData> keys;

	// Riddle info for *TEXT IDs.
	std::unordered_map<int, RiddleData> riddles;

	// Text pop-ups for *TEXT IDs. Some places have several dozen *TEXT definitions.
	std::unordered_map<int, TextData> texts;

	// Specific wall textures.
	std::string lavaChasmTexture, wetChasmTexture, dryChasmTexture, levelDownTexture,
		levelUpTexture, transitionTexture, transWalkThruTexture;

	// Height of voxels on the main floor. Default is 100. Not sure what the second number 
	// of *CEILING is.
	CeilingData ceiling;
public:
	INFFile(const std::string &filename);
	~INFFile();

	const TextureData &getTexture(int index) const;
	const std::vector<FlatData> &getItemList(int index) const;
	const std::string &getBoxcap(int index) const;
	const std::string &getBoxside(int index) const;
	const std::string &getSound(int index) const;
	bool hasKeyIndex(int index) const;
	bool hasRiddleIndex(int index) const;
	bool hasTextIndex(int index) const;
	const KeyData &getKey(int index) const;
	const RiddleData &getRiddle(int index) const;
	const TextData &getText(int index) const;
	const std::string &getLavaChasmTexture() const;
	const std::string &getWetChasmTexture() const;
	const std::string &getDryChasmTexture() const;
	const std::string &getLevelDownTexture() const;
	const std::string &getLevelUpTexture() const;
	const std::string &getTransitionTexture() const;
	const std::string &getTransWalkThruTexture() const;
	const CeilingData &getCeiling() const;
};

#endif
