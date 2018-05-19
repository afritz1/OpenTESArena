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
	struct VoxelTextureData
	{
		std::string filename;
		int setIndex; // -1 when texture is not a .SET file; index into the .SET otherwise.

		VoxelTextureData(const std::string &filename, int setIndex);
		VoxelTextureData(const std::string &filename);
		VoxelTextureData(VoxelTextureData&&) = default;
		VoxelTextureData() = default;
	};

	struct FlatTextureData
	{
		std::string filename;

		FlatTextureData(const std::string &filename);
		FlatTextureData(FlatTextureData&&) = default;
		FlatTextureData() = default;
	};

	struct CeilingData
	{
		int textureIndex; // Index into textures vector.

		// Size of walls on main floor (first *CEILING number). Default is 128.
		int height;

		// Main floor box scale (second *CEILING number). Formula: (Y * boxScale) / 256.
		// If missing and in wilderness, then use 192. Else if missing and not in wilderness,
		// then box values are unchanged.
		std::unique_ptr<int> boxScale;

		bool outdoorDungeon; // True when third *CEILING number is 1 (for main quest dungeons?).

		CeilingData();
	};

	struct FlatData
	{
		int textureIndex; // Index into textures vector.

		int yOffset; // Offsets the flat some number of pixels. Negative goes up.
		int health; // Number of hit points.

		// Flat modifiers.
		// 0x1: Collider. 0x2: Reflect (puddle). 0x4: Double (scale?). 0x8: Dark.
		// 0x10: Transparent (ghosts). 0x20: Ceiling (attached to ceiling?).
		bool collider;
		bool puddle;
		bool doubleScale;
		bool dark;
		bool transparent;
		bool ceiling;

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
	std::vector<VoxelTextureData> voxelTextures;
	std::vector<FlatTextureData> flatTextures;

	// References to texture names in the textures vector. -1 if unset.
	std::array<int, 16> boxCaps, boxSides, menus;

	// Flat data in the order they are discovered. Each record holds various data for a flat 
	// (i.e., texture index, etc.).
	std::vector<FlatData> flats;

	// Indices into the flats vector for flats paired with an *ITEM index. The highest *ITEM 
	// number is 95, although those past 63 might not be used (character class names, lore 
	// names, etc.).
	std::array<int, 96> items;

	// .VOC files for each sound ID.
	std::unordered_map<int, std::string> sounds;

	// Key info for *TEXT IDs.
	std::unordered_map<int, KeyData> keys;

	// Riddle info for *TEXT IDs.
	std::unordered_map<int, RiddleData> riddles;

	// Text pop-ups for *TEXT IDs. Some places have several dozen *TEXT definitions.
	std::unordered_map<int, TextData> texts;

	std::string name;

	// References into the textures vector. -1 if unset.
	int dryChasmIndex, lavaChasmIndex, levelDownIndex, levelUpIndex, wetChasmIndex;

	// Ceiling data (height, box scale(?), etc.).
	CeilingData ceiling;
public:
	INFFile(const std::string &filename);

	// Arbitrary index used for unset indices.
	static const int NO_INDEX;

	const std::vector<VoxelTextureData> &getVoxelTextures() const;
	const std::vector<FlatTextureData> &getFlatTextures() const;
	const int *getBoxCap(int index) const;
	const int *getBoxSide(int index) const;
	const int *getMenu(int index) const;
	int getMenuIndex(int textureID) const; // Temporary hack?
	const FlatData &getFlat(int index) const;
	const FlatData &getItem(int index) const;
	const std::string &getSound(int index) const;
	bool hasKeyIndex(int index) const;
	bool hasRiddleIndex(int index) const;
	bool hasTextIndex(int index) const;
	const KeyData &getKey(int index) const;
	const RiddleData &getRiddle(int index) const;
	const TextData &getText(int index) const;
	const std::string &getName() const;
	const int *getDryChasmIndex() const;
	const int *getLavaChasmIndex() const;
	const int *getLevelDownIndex() const;
	const int *getLevelUpIndex() const;
	const int *getWetChasmIndex() const;
	const CeilingData &getCeiling() const;
};

#endif
