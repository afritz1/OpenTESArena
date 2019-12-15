#ifndef ARENA_ANIM_UTILS_H
#define ARENA_ANIM_UTILS_H

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "../Entities/EntityAnimationData.h"
#include "../Media/Palette.h"

class ArenaRandom;
class CFAFile;
class ExeData;
class INFFile;
class MiscAssets;

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

	// Animation values for static .DFA files.
	constexpr double StaticIdleSecondsPerFrame = 1.0 / 12.0;
	const bool StaticIdleLoop = true;

	// Animation values for creatures with .CFA files.
	constexpr double CreatureIdleSecondsPerFrame = 1.0 / 12.0;
	constexpr double CreatureLookSecondsPerFrame = 1.0 / 8.0;
	constexpr double CreatureWalkSecondsPerFrame = 1.0 / 12.0;
	constexpr double CreatureAttackSecondsPerFrame = 1.0 / 12.0;
	constexpr double CreatureDeathSecondsPerFrame = 1.0 / 12.0;
	constexpr int CreatureAttackFrameIndex = 10;
	const bool CreatureIdleLoop = true;
	const bool CreatureLookLoop = false;
	const bool CreatureWalkLoop = true;
	const bool CreatureAttackLoop = false;
	const bool CreatureDeathLoop = false;
	const std::vector<int> CreatureIdleIndices = { 0 };
	const std::vector<int> CreatureLookIndices = { 6, 0, 7, 0 };
	const std::vector<int> CreatureWalkIndices = { 0, 1, 2, 3, 4, 5 };
	const std::vector<int> CreatureAttackIndices = { 8, 9, 10, 11 };

	// Animation values for human enemies with .CFA files.
	constexpr double HumanIdleSecondsPerFrame = CreatureIdleSecondsPerFrame;
	constexpr double HumanWalkSecondsPerFrame = CreatureWalkSecondsPerFrame;
	constexpr double HumanAttackSecondsPerFrame = CreatureAttackSecondsPerFrame;
	constexpr double HumanDeathSecondsPerFrame = CreatureDeathSecondsPerFrame;
	const bool HumanIdleLoop = CreatureIdleLoop;
	const bool HumanWalkLoop = CreatureWalkLoop;
	const bool HumanAttackLoop = CreatureAttackLoop;
	const bool HumanDeathLoop = CreatureDeathLoop;
	const std::vector<int> HumanIdleIndices = CreatureIdleIndices;
	const std::vector<int> HumanWalkIndices = CreatureWalkIndices;

	// Animation values for citizens with .CFA files.
	constexpr double CitizenIdleSecondsPerFrame = 1.0 / 4.0;
	constexpr double CitizenWalkSecondsPerFrame = HumanWalkSecondsPerFrame;
	const bool CitizenIdleLoop = HumanIdleLoop;
	const bool CitizenWalkLoop = HumanWalkLoop;
	const std::vector<int> CitizenIdleIndices = { 6, 7, 8 };
	const std::vector<int> CitizenWalkIndices = HumanWalkIndices;

	// Cache for .CFA/.DFA/.IMG files referenced multiple times during entity loading.
	template <typename T>
	class AnimFileCache
	{
	private:
		std::unordered_map<std::string, T> files;
	public:
		bool tryGet(const std::string &filename, const T **outFile)
		{
			auto iter = this->files.find(filename);
			if (iter == this->files.end())
			{
				T file;
				if (!file.init(filename.c_str()))
				{
					DebugLogError("Couldn't init cached anim file \"" + filename + "\".");
					return false;
				}

				iter = this->files.emplace(std::make_pair(filename, std::move(file))).first;
			}

			*outFile = &iter->second;
			return true;
		}
	};

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
	bool isStreetLightFlatIndex(int flatIndex);

	// Original sprite scaling function. Takes sprite texture dimensions and scaling
	// value and outputs dimensions for the final displayed entity.
	void getBaseFlatDimensions(int width, int height, uint16_t scale, int *baseWidth, int *baseHeight);

	// Returns whether the given original animation state ID would be for a flipped animation.
	// Animation state IDs are 1-based, 1 being the entity looking at the player.
	bool isAnimDirectionFlipped(int animDirectionID);

	// Given a creature direction anim ID like 7, will return the index of the non-flipped anim.
	int getDynamicEntityCorrectedAnimID(int animDirectionID, bool *outIsFlipped);

	// Helper function for generating a default entity animation state for later modification.
	EntityAnimationData::State makeAnimState(EntityAnimationData::StateType stateType,
		double secondsPerFrame, bool loop, bool flipped = false);

	// Works for both creature and human enemy filenames.
	bool trySetDynamicEntityFilenameDirection(std::string &filename, int animDirectionID);

	// Writes the value of the animation direction to the filename if possible.
	bool trySetCitizenFilenameDirection(std::string &filename, int animDirectionID);

	// Writes out values for human enemy animations.
	void getHumanEnemyProperties(int itemIndex, const MiscAssets &miscAssets,
		int *outTypeIndex, bool *outIsMale);

	// Writes the gender data into the given filename if possible.
	bool trySetHumanFilenameGender(std::string &filename, bool isMale);

	// Writes the human type data into the given filename if possible.
	bool trySetHumanFilenameType(std::string &filename, const std::string_view &type);

	// Static entity animation state for idle.
	EntityAnimationData::State makeStaticEntityIdleAnimState(int flatIndex,
		const INFFile &inf, const ExeData &exeData);

	// Write out to lists of dynamic entity animation states for each animation direction.
	// For any of the dynamic entity anim states, if the returned state list is empty,
	// it is assumed that the entity has no information for that state.
	void makeDynamicEntityAnimStates(int flatIndex, const INFFile &inf,
		const MiscAssets &miscAssets, AnimFileCache<CFAFile> &cfaCache,
		std::vector<EntityAnimationData::State> *outIdleStates,
		std::vector<EntityAnimationData::State> *outLookStates,
		std::vector<EntityAnimationData::State> *outWalkStates,
		std::vector<EntityAnimationData::State> *outAttackStates,
		std::vector<EntityAnimationData::State> *outDeathStates);

	// Write out to lists of citizen animation states for each animation direction.
	void makeCitizenAnimStates(bool isMale, ClimateType climateType, const INFFile &inf,
		const MiscAssets &miscAssets, AnimFileCache<CFAFile> &cfaCache,
		std::vector<EntityAnimationData::State> *outIdleStates,
		std::vector<EntityAnimationData::State> *outWalkStates);

	// Transforms the palette used for a citizen's clothes. The given seed value is "pure random"
	// and can essentially be anything.
	Palette transformCitizenClothing(uint16_t seed, const Palette &palette, const ExeData &exeData);

	// Transforms the palette used for a citizen's skin.
	Palette transformCitizenSkin(int raceIndex, const Palette &palette, const ExeData &exeData);
}

#endif
