#ifndef ARENA_ANIM_UTILS_H
#define ARENA_ANIM_UTILS_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "INFFile.h"
#include "../Entities/EntityAnimationDefinition.h"
#include "../Entities/EntityAnimationInstance.h"
#include "../Media/Palette.h"

class ArenaRandom;
class CFAFile;
class CharacterClassLibrary;
class ExeData;
class MiscAssets;
class TextureManager;

enum class ClimateType;
enum class EntityType;

// Helper values for working with the original animations. These may or may not be directly
// referencing original values and may only exist for convenience in the new engine.

namespace ArenaAnimUtils
{
	// Number of directions a .CFA entity can face.
	constexpr int Directions = 8;

	// First flipped animation ID that requires a mapping to a non-flipped ID for use
	// with a creature .CFA file.
	constexpr int FirstFlippedAnimID = 6;

	// Various special case conditions for generating static animations.
	enum class StaticAnimCondition
	{
		None,
		IsCity,
		IsPalace
	};

	// Animation values for static .DFA files.
	constexpr double StaticIdleSecondsPerFrame = 1.0 / 12.0;
	constexpr double StaticActivatedSecondsPerFrame = StaticIdleSecondsPerFrame;
	constexpr bool StaticIdleLoop = true;
	constexpr bool StaticActivatedLoop = StaticIdleLoop;

	// Animation values for creatures with .CFA files.
	constexpr double CreatureIdleSecondsPerFrame = 1.0 / 12.0;
	constexpr double CreatureLookSecondsPerFrame = 1.0 / 8.0;
	constexpr double CreatureWalkSecondsPerFrame = 1.0 / 12.0;
	constexpr double CreatureAttackSecondsPerFrame = 1.0 / 12.0;
	constexpr double CreatureDeathSecondsPerFrame = 1.0 / 12.0;
	constexpr int CreatureAttackFrameIndex = 10;
	constexpr bool CreatureIdleLoop = true;
	constexpr bool CreatureLookLoop = false;
	constexpr bool CreatureWalkLoop = true;
	constexpr bool CreatureAttackLoop = false;
	constexpr bool CreatureDeathLoop = false;
	const std::vector<int> CreatureIdleIndices = { 0 };
	const std::vector<int> CreatureLookIndices = { 6, 0, 7, 0 };
	const std::vector<int> CreatureWalkIndices = { 0, 1, 2, 3, 4, 5 };
	const std::vector<int> CreatureAttackIndices = { 8, 9, 10, 11 };

	// Animation values for human enemies with .CFA files.
	constexpr double HumanIdleSecondsPerFrame = CreatureIdleSecondsPerFrame;
	constexpr double HumanWalkSecondsPerFrame = CreatureWalkSecondsPerFrame;
	constexpr double HumanAttackSecondsPerFrame = CreatureAttackSecondsPerFrame;
	constexpr double HumanDeathSecondsPerFrame = CreatureDeathSecondsPerFrame;
	constexpr bool HumanIdleLoop = CreatureIdleLoop;
	constexpr bool HumanWalkLoop = CreatureWalkLoop;
	constexpr bool HumanAttackLoop = CreatureAttackLoop;
	constexpr bool HumanDeathLoop = CreatureDeathLoop;
	const std::vector<int> HumanIdleIndices = CreatureIdleIndices;
	const std::vector<int> HumanWalkIndices = CreatureWalkIndices;

	// Animation values for citizens with .CFA files.
	constexpr double CitizenIdleSecondsPerFrame = 1.0 / 4.0;
	constexpr double CitizenWalkSecondsPerFrame = 1.0 / 16.0;
	const bool CitizenIdleLoop = HumanIdleLoop;
	const bool CitizenWalkLoop = HumanWalkLoop;
	const std::vector<int> CitizenIdleIndices = { 6, 7, 8 };
	const std::vector<int> CitizenWalkIndices = HumanWalkIndices;

	// The final boss is sort of a special case. Their *ITEM index is at the very end of 
	// human enemies, but they are treated like a creature.
	bool isFinalBossIndex(int itemIndex);

	// *ITEM 32 to 54 are creatures (rat, goblin, etc.). The final boss is a special case.
	bool isCreatureIndex(int itemIndex, bool *outIsFinalBoss);

	// *ITEM 55 to 72 are human enemies (guard, wizard, etc.).
	bool isHumanEnemyIndex(int itemIndex);

	// Returns whether the given flat index is for a static or dynamic entity.
	EntityType getEntityTypeFromFlat(int flatIndex, const INFFile &inf);

	// Creature IDs are 1-based (rat=1, goblin=2, etc.).
	int getCreatureIDFromItemIndex(int itemIndex);

	// The final boss is a special case, essentially hardcoded at the end of the creatures.
	int getFinalBossCreatureID();

	// Character classes (mage, warrior, etc.) used by human enemies.
	int getCharacterClassIndexFromItemIndex(int itemIndex);

	// Streetlights are hardcoded in the original game to flat index 29. This lets the
	// game give them a light source and toggle them between on and off states.
	int getStreetLightActiveIndex();
	int getStreetLightInactiveIndex();
	bool isStreetLightFlatIndex(int flatIndex, bool isCity);

	// Ruler flats are either a king or queen.
	int getRulerKingIndex();
	int getRulerQueenIndex();
	bool isRulerFlatIndex(int flatIndex, bool isPalace);

	// Original sprite scaling function. Takes sprite texture dimensions and scaling
	// value and outputs dimensions for the final displayed entity.
	void getBaseFlatDimensions(int width, int height, uint16_t scale, int *baseWidth, int *baseHeight);

	// Returns whether the given original animation state ID would be for a flipped animation.
	// Animation state IDs are 1-based, 1 being the entity looking at the player.
	bool isAnimDirectionFlipped(int animDirectionID);

	// Given a creature direction anim ID like 7, will return the index of the non-flipped anim.
	int getDynamicEntityCorrectedAnimDirID(int animDirectionID, bool *outIsFlipped);

	// Works for both creature and human enemy filenames.
	bool trySetDynamicEntityFilenameDirection(std::string &filename, int animDirectionID);

	// Writes the value of the animation direction to the filename if possible.
	bool trySetCitizenFilenameDirection(std::string &filename, int animDirectionID);

	// Writes out values for human enemy animations.
	void getHumanEnemyProperties(int itemIndex, const CharacterClassLibrary &charClassLibrary,
		const ExeData &exeData, int *outTypeIndex, bool *outIsMale);

	// Writes the gender data into the given filename if possible.
	bool trySetHumanFilenameGender(std::string &filename, bool isMale);

	// Writes the human type data into the given filename if possible.
	bool trySetHumanFilenameType(std::string &filename, const std::string_view &type);

	// Writes out static entity animation data to animation states.
	bool tryMakeStaticEntityAnims(int flatIndex, StaticAnimCondition condition,
		const std::optional<bool> &rulerIsMale, const INFFile &inf, TextureManager &textureManager,
		EntityAnimationDefinition *outAnimDef, EntityAnimationInstance *outAnimInst);

	// Writes out dynamic entity animation data to animation states.
	bool tryMakeDynamicEntityAnims(int flatIndex, const INFFile &inf,
		const CharacterClassLibrary &charClassLibrary, const MiscAssets &miscAssets,
		TextureManager &textureManager, EntityAnimationDefinition *outAnimDef,
		EntityAnimationInstance *outAnimInst);

	// Writes out citizen animation data to animation states.
	bool tryMakeCitizenAnims(bool isMale, ClimateType climateType, const INFFile &inf,
		const MiscAssets &miscAssets, TextureManager &textureManager,
		EntityAnimationDefinition *outAnimDef, EntityAnimationInstance *outAnimInst);

	// Transforms the palette used for a citizen's clothes and skin. The given seed value is
	// "pure random" and can essentially be anything.
	Palette transformCitizenColors(int raceIndex, uint16_t seed, const Palette &palette,
		const ExeData &exeData);
}

#endif
