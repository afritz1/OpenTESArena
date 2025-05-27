#ifndef INF_FILE_H
#define INF_FILE_H

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ArenaTypes.h"

#include "components/dos/DOSUtils.h"
#include "components/utilities/BufferView.h"

struct INFVoxelTexture
{
	std::string filename;
	std::optional<int> setIndex; // Index into .SET file texture (if any).

	INFVoxelTexture(const char *filename, const std::optional<int> &setIndex);
	INFVoxelTexture(const char *filename);
};

struct INFFlatTexture
{
	std::string filename;

	INFFlatTexture(const char *filename);
};

struct INFCeiling
{
	static constexpr int DEFAULT_HEIGHT = 100;

	std::optional<int> textureIndex; // Index into textures vector (if any).

	// Size of ceiling (first *CEILING number). Determines wall and dry chasm height.
	int height;

	// Main floor box scale (second *CEILING number). Formula: (Y * boxScale) / 256.
	// If missing and in wilderness, then use 192. Else if missing and not in wilderness,
	// then box values are unchanged.
	std::optional<int> boxScale;

	bool outdoorDungeon; // True when third *CEILING number is 1 (for main quest dungeons?).

	INFCeiling();
};

struct INFFlat
{
	// Scale values for stretching flats.
	static constexpr double MEDIUM_SCALE = 1.5 * 128.0; // 150% larger.
	static constexpr double LARGE_SCALE = 3.0 * 128.0; // 300% larger.

	int textureIndex; // Index into textures vector.

	// *ITEM value, if any. *ITEM 32 should be associated with rats, the first
	// creature type. The highest *ITEM number is 95, although some of them past
	// 63 might not be used (character class names, lore names, etc.).
	std::optional<ArenaTypes::ItemIndex> itemIndex;

	int yOffset; // Offsets the flat some number of pixels. Negative goes up.
	int health; // Number of hit points.

	// Flat modifiers.
	// 0x1: Collider. 0x2: Reflect (puddle). 0x4: Triple scale (trees). 0x8: Dark.
	// 0x10: Transparent (ghosts). 0x20: Ceiling (attached to ceiling?), 
	// 0x40: 150% scale (some furniture?)
	bool collider;
	bool puddle;
	bool largeScale;
	bool dark;
	bool transparent;
	bool ceiling;
	bool mediumScale;

	// Used with N:#, where '#' is the death effect. The "next flat" is probably 
	// used for displaying corpses.
	std::string nextFlat;
	std::optional<int> deathEffect;

	// Used with S:#, where '#' is light intensity (for candles, etc.).
	std::optional<int> lightIntensity;

	INFFlat();
};

struct INFKey
{
	int id; // Key ID (starts with '+').
	int revisedID; // ID to use with texture lookup

	INFKey(int id);
};

struct INFRiddle
{
	std::vector<std::string> answers; // Accepted answers from the player.
	std::string riddle, correct, wrong;
	int firstNumber, secondNumber; // Not sure what these are.

	INFRiddle(int firstNumber, int secondNumber);
};

struct INFLoreText
{
	std::string text; // Stores display text for a text trigger.
	bool isDisplayedOnce; // Whether the text is only displayed once (starts with '~').

	INFLoreText(bool isDisplayedOnce);
};

// An .INF file contains definitions of what the IDs in a .MIF file point to. These 
// are mostly texture IDs, but also text IDs and sound IDs telling which voxels have 
// which kinds of triggers, etc..
class INFFile
{
private:
	// Texture filenames in the order they are discovered. .SET files are expanded;
	// that is, a four-element .SET will occupy four consecutive indices, and each integer
	// pointer will contain the index (otherwise it is null).
	std::vector<INFVoxelTexture> voxelTextures;
	std::vector<INFFlatTexture> flatTextures;

	// References to texture names in the textures vector (if any).
	std::array<std::optional<int>, 16> boxCaps, boxSides, menus;

	// Flat data in the order they are discovered. Each record holds various data for a flat 
	// (i.e., texture index, etc.).
	std::vector<INFFlat> flats;

	// .VOC files for each sound ID.
	std::unordered_map<int, std::string> sounds;

	// A *TEXT field can be one of 1) lore text, 2) riddle, or 3) door key ID, accessed by its *TEXT #.
	std::unordered_map<int, INFKey> keys;
	std::unordered_map<int, INFRiddle> riddles;
	std::unordered_map<int, INFLoreText> loreTexts;

	std::string name;

	// References into the textures vector (if any).
	std::optional<int> dryChasmIndex, lavaChasmIndex, levelDownIndex, levelUpIndex, wetChasmIndex;

	// Ceiling data (height, box scale(?), etc.).
	INFCeiling ceiling;
public:
	bool init(const char *filename);

	BufferView<const INFVoxelTexture> getVoxelTextures() const;
	BufferView<const INFFlatTexture> getFlatTextures() const;
	const std::optional<int> &getBoxCap(int index) const;
	const std::optional<int> &getBoxSide(int index) const;
	const std::optional<int> &getMenu(int index) const;
	std::optional<int> getMenuIndex(int textureID) const; // Temporary hack?
	const INFFlat &getFlat(ArenaTypes::FlatIndex flatIndex) const;
	ArenaTypes::FlatIndex findFlatIndexWithItemIndex(ArenaTypes::ItemIndex itemIndex) const;
	const char *getSound(int index) const;
	bool hasKeyIndex(int index) const;
	bool hasRiddleIndex(int index) const;
	bool hasLoreTextIndex(int index) const;
	const INFKey &getKey(int index) const;
	const INFRiddle &getRiddle(int index) const;
	const INFLoreText &getLoreText(int index) const;
	const char *getName() const;
	const std::optional<int> &getDryChasmIndex() const;
	const std::optional<int> &getLavaChasmIndex() const;
	const std::optional<int> &getLevelDownIndex() const;
	const std::optional<int> &getLevelUpIndex() const;
	const std::optional<int> &getWetChasmIndex() const;
	const INFCeiling &getCeiling() const;
};

#endif
