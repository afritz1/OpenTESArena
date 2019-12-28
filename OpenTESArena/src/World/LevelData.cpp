#include <algorithm>
#include <functional>

#include "LevelData.h"
#include "VoxelData.h"
#include "VoxelDataType.h"
#include "../Assets/CFAFile.h"
#include "../Assets/COLFile.h"
#include "../Assets/DFAFile.h"
#include "../Assets/ExeData.h"
#include "../Assets/IMGFile.h"
#include "../Assets/INFFile.h"
#include "../Assets/MiscAssets.h"
#include "../Assets/SETFile.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/EntityType.h"
#include "../Entities/StaticEntity.h"
#include "../Items/ArmorMaterialType.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../World/WorldType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

namespace
{
	// Number of directions a dynamic entity can face.
	constexpr int MAX_ANIM_DIRECTIONS = 8;

	// First flipped animation ID that requires a mapping to a non-flipped ID for use
	// with a creature .CFA file.
	constexpr int FIRST_FLIPPED_ANIM_ID = 6;

	// Animation values for creatures with .CFA files.
	constexpr double CREATURE_ANIM_IDLE_SECONDS_PER_FRAME = 1.0 / 4.0;
	constexpr double CREATURE_ANIM_LOOK_SECONDS_PER_FRAME = 1.0 / 4.0;
	constexpr double CREATURE_ANIM_WALK_SECONDS_PER_FRAME = 1.0 / 8.0;
	constexpr double CREATURE_ANIM_ATTACK_SECONDS_PER_FRAME = 1.0 / 8.0;
	constexpr double CREATURE_ANIM_DEATH_SECONDS_PER_FRAME = 1.0 / 4.0;
	constexpr int CREATURE_ANIM_ATTACK_FRAME_INDEX = 10;
	const bool CREATURE_ANIM_IDLE_LOOP = true;
	const bool CREATURE_ANIM_LOOK_LOOP = false;
	const bool CREATURE_ANIM_WALK_LOOP = true;
	const bool CREATURE_ANIM_ATTACK_LOOP = false;
	const bool CREATURE_ANIM_DEATH_LOOP = false;
	const std::vector<int> CreatureAnimIndicesIdle = { 0 };
	const std::vector<int> CreatureAnimIndicesLook = { 6, 0, 7, 0 };
	const std::vector<int> CreatureAnimIndicesWalk = { 0, 1, 2, 3, 4, 5 };
	const std::vector<int> CreatureAnimIndicesAttack = { 8, 9, 10, 11 };

	// Animation values for human enemies with .CFA files.
	constexpr double HUMAN_ANIM_IDLE_SECONDS_PER_FRAME = CREATURE_ANIM_IDLE_SECONDS_PER_FRAME;
	constexpr double HUMAN_ANIM_WALK_SECONDS_PER_FRAME = CREATURE_ANIM_WALK_SECONDS_PER_FRAME;
	constexpr double HUMAN_ANIM_ATTACK_SECONDS_PER_FRAME = CREATURE_ANIM_ATTACK_SECONDS_PER_FRAME;
	constexpr double HUMAN_ANIM_DEATH_SECONDS_PER_FRAME = CREATURE_ANIM_DEATH_SECONDS_PER_FRAME;
	const bool HUMAN_ANIM_IDLE_LOOP = true;
	const bool HUMAN_ANIM_WALK_LOOP = true;
	const bool HUMAN_ANIM_ATTACK_LOOP = false;
	const bool HUMAN_ANIM_DEATH_LOOP = false;
	const std::vector<int> HumanAnimIndicesIdle = { 0 };
	const std::vector<int> HumanAnimIndicesWalk = { 0, 1, 2, 3, 4, 5 };

	// Cache for .CFA/.DFA files referenced multiple times during entity loading.
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
	bool IsFinalBossIndex(int itemIndex)
	{
		return itemIndex == 73;
	}

	// *ITEM 32 to 54 are creatures (rat, goblin, etc.). The final boss is a special case.
	bool IsCreatureIndex(int itemIndex, bool *outIsFinalBoss)
	{
		const bool isFinalBoss = IsFinalBossIndex(itemIndex);
		*outIsFinalBoss = isFinalBoss;
		return (itemIndex >= 32 && itemIndex <= 54) || isFinalBoss;
	}

	// *ITEM 55 to 72 are human enemies (guard, wizard, etc.).
	bool IsHumanEnemyIndex(int itemIndex)
	{
		return itemIndex >= 55 && itemIndex <= 72;
	}

	// Returns whether the given flat index is for a static or dynamic entity.
	EntityType GetEntityTypeFromFlat(int flatIndex, const INFFile &inf)
	{
		const auto &flatData = inf.getFlat(flatIndex);
		if (flatData.itemIndex.has_value())
		{
			const int itemIndex = flatData.itemIndex.value();

			// Creature *ITEM values are between 32 and 54. Other dynamic entities (like humans)
			// are higher.
			bool dummy;
			return (IsCreatureIndex(itemIndex, &dummy) || IsHumanEnemyIndex(itemIndex)) ?
				EntityType::Dynamic : EntityType::Static;
		}
		else
		{
			return EntityType::Static;
		}
	}

	// Creature IDs are 1-based (rat=1, goblin=2, etc.).
	int GetCreatureIDFromItemIndex(int itemIndex)
	{
		return itemIndex - 31;
	}

	// The final boss is a special case, essentially hardcoded at the end of the creatures.
	int GetFinalBossCreatureID()
	{
		return 24;
	}

	// Character classes (mage, warrior, etc.) used by human enemies.
	int GetCharacterClassIndexFromItemIndex(int itemIndex)
	{
		return itemIndex - 55;
	}

	// Streetlights are hardcoded in the original game to flat index 29. This lets the
	// game give them a light source and toggle them between on and off states.
	bool IsStreetLightFlatIndex(int flatIndex)
	{
		return flatIndex == 29;
	}

	// Original sprite scaling function. Takes sprite texture dimensions and scaling
	// value and outputs dimensions for the final displayed entity.
	void GetBaseFlatDimensions(int width, int height, uint16_t scale,
		int *baseWidth, int *baseHeight)
	{
		*baseWidth = (width * scale) / 256;
		*baseHeight = (((height * scale) / 256) * 200) / 256;
	}

	// Returns whether the given original animation state ID would be for a flipped animation.
	// Animation state IDs are 1-based, 1 being the entity looking at the player.
	bool IsAnimDirectionFlipped(int animDirectionID)
	{
		DebugAssert(animDirectionID >= 1);
		DebugAssert(animDirectionID <= MAX_ANIM_DIRECTIONS);
		return animDirectionID >= FIRST_FLIPPED_ANIM_ID;
	}

	// Given a creature direction anim ID like 7, will return the index of the non-flipped anim.
	int GetDynamicEntityCorrectedAnimID(int animDirectionID, bool *outIsFlipped)
	{
		// If the animation direction points to a flipped animation, the ID needs to be
		// corrected to point to the non-flipped version.
		if (IsAnimDirectionFlipped(animDirectionID))
		{
			*outIsFlipped = true;
			return ((FIRST_FLIPPED_ANIM_ID - 1) * 2) - animDirectionID;
		}
		else
		{
			*outIsFlipped = false;
			return animDirectionID;
		}
	}

	// Helper function for generating a default entity animation state for later modification.
	EntityAnimationData::State MakeAnimState(EntityAnimationData::StateType stateType,
		double secondsPerFrame, bool loop, bool flipped = false)
	{
		return EntityAnimationData::State(stateType, secondsPerFrame, loop, flipped);
	}

	// Works for both creature and human enemy filenames.
	bool TrySetDynamicEntityFilenameDirection(std::string &filename, int animDirectionID)
	{
		DebugAssert(filename.size() > 0);
		DebugAssert(animDirectionID >= 1);
		DebugAssert(animDirectionID <= MAX_ANIM_DIRECTIONS);

		const size_t index = filename.find('@');
		if (index != std::string::npos)
		{
			const char animDirectionChar = '0' + animDirectionID;
			filename[index] = animDirectionChar;
			return true;
		}
		else
		{
			DebugLogError("Couldn't replace direction in \"" + filename + "\".");
			return false;
		}
	}

	void GetHumanEnemyProperties(int itemIndex, const MiscAssets &miscAssets,
		int *outTypeIndex, bool *outIsMale)
	{
		const auto &exeData = miscAssets.getExeData();

		const int charClassIndex = GetCharacterClassIndexFromItemIndex(itemIndex);
		const auto &charClasses = miscAssets.getClassDefinitions();
		DebugAssertIndex(charClasses, charClassIndex);
		const CharacterClass &charClass = charClasses[charClassIndex];

		// Properties about the character class.
		*outTypeIndex = [&exeData, &charClass]()
		{
			// Find which armors the class can wear.
			bool hasPlate = false;
			bool hasChain = false;
			bool hasLeather = false;

			const auto &allowedArmors = charClass.getAllowedArmors();
			for (const ArmorMaterialType armorType : allowedArmors)
			{
				hasPlate |= armorType == ArmorMaterialType::Plate;
				hasChain |= armorType == ArmorMaterialType::Chain;
				hasLeather |= armorType == ArmorMaterialType::Leather;
			}

			if (hasPlate)
			{
				return 0;
			}
			else if (hasChain)
			{
				return 1;
			}
			else if (hasLeather)
			{
				return 2;
			}
			else if (charClass.canCastMagic())
			{
				// Spellcaster.
				return 4;
			}
			else if (charClass.getClassIndex() == 12)
			{
				// Monk.
				return 5;
			}
			else if (charClass.getClassIndex() == 15)
			{
				// Barbarian.
				return 6;
			}
			else
			{
				// Unarmored.
				return 3;
			}
		}();

		// Assume all non-randomly generated enemies are male.
		*outIsMale = true;
	}

	bool TrySetHumanFilenameGender(std::string &filename, bool isMale)
	{
		DebugAssert(filename.size() > 0);
		filename[0] = isMale ? '0' : '1';
		return true;
	}

	bool TrySetHumanFilenameType(std::string &filename, const std::string_view &type)
	{
		DebugAssert(filename.size() > 0);
		DebugAssert(type.size() == 3);

		const size_t index = filename.find("XXX");
		if (index != std::string::npos)
		{
			filename.replace(index, type.size(), type.data());
			return true;
		}
		else
		{
			DebugLogError("Couldn't replace type in \"" + filename + "\".");
			return false;
		}
	}

	// Static entity animation state for idle.
	EntityAnimationData::State MakeStaticEntityIdleAnimState(int flatIndex,
		const INFFile &inf, const ExeData &exeData)
	{
		const INFFile::FlatData &flatData = inf.getFlat(flatIndex);
		const std::vector<INFFile::FlatTextureData> &flatTextures = inf.getFlatTextures();

		DebugAssertIndex(flatTextures, flatData.textureIndex);
		const INFFile::FlatTextureData &flatTextureData = flatTextures[flatData.textureIndex];
		const std::string &flatTextureName = flatTextureData.filename;
		const std::string_view extension = StringView::getExtension(flatTextureName);
		const bool isDFA = extension == "DFA";
		const bool isIMG = extension == "IMG";
		const bool noExtension = extension.size() == 0;

		// A flat's appearance may be modified by some .INF properties.
		constexpr double mediumScaleValue = INFFile::FlatData::MEDIUM_SCALE / 100.0;
		constexpr double largeScaleValue = INFFile::FlatData::LARGE_SCALE / 100.0;
		const double dimensionModifier = flatData.largeScale ? largeScaleValue :
			(flatData.mediumScale ? mediumScaleValue : 1.0);

		auto makeKeyframeDimension = [dimensionModifier](int value)
		{
			return (static_cast<double>(value) * dimensionModifier) / MIFFile::ARENA_UNITS;
		};

		EntityAnimationData::State animState = MakeAnimState(
			EntityAnimationData::StateType::Idle, 1.0 / 12.0, true);

		// Determine how to populate the animation state with keyframes.
		if (isDFA)
		{
			DFAFile dfa;
			if (!dfa.init(flatTextureName.c_str()))
			{
				DebugCrash("Couldn't init .DFA file \"" + flatTextureName + "\".");
			}

			animState.setTextureName(std::string(flatTextureName));

			for (int i = 0; i < dfa.getImageCount(); i++)
			{
				const double width = makeKeyframeDimension(dfa.getWidth());
				const double height = makeKeyframeDimension(dfa.getHeight());
				const int textureID = i;

				EntityAnimationData::Keyframe keyframe(width, height, textureID);
				animState.addKeyframe(std::move(keyframe));
			}

			return animState;
		}
		else if (isIMG)
		{
			IMGFile img;
			if (!img.init(flatTextureName.c_str()))
			{
				DebugCrash("Couldn't init .IMG file \"" + flatTextureName + "\".");
			}

			animState.setTextureName(std::string(flatTextureName));

			const double width = makeKeyframeDimension(img.getWidth());
			const double height = makeKeyframeDimension(img.getHeight());
			const int textureID = 0;

			EntityAnimationData::Keyframe keyframe(width, height, textureID);
			animState.addKeyframe(std::move(keyframe));
			return animState;
		}
		else if (noExtension)
		{
			// Ignore texture names with no extension. They appear to be lore-related names
			// that were used at one point in Arena's development.
			return animState;
		}
		else
		{
			DebugLogError("Unrecognized flat texture name \"" + flatTextureName + "\".");
			return animState;
		}
	}

	// For any of the dynamic entity anim states, if the returned state list is empty,
	// it is assumed that the entity has no information for that state.

	// Write out to lists of dynamic entity animation states for each animation direction.
	void MakeDynamicEntityAnimStates(int flatIndex, const INFFile &inf,
		const MiscAssets &miscAssets, AnimFileCache<CFAFile> &cfaCache,
		std::vector<EntityAnimationData::State> *outIdleStates,
		std::vector<EntityAnimationData::State> *outLookStates,
		std::vector<EntityAnimationData::State> *outWalkStates,
		std::vector<EntityAnimationData::State> *outAttackStates,
		std::vector<EntityAnimationData::State> *outDeathStates)
	{
		DebugAssert(outIdleStates != nullptr);
		DebugAssert(outLookStates != nullptr);
		DebugAssert(outWalkStates != nullptr);
		DebugAssert(outAttackStates != nullptr);
		DebugAssert(outDeathStates != nullptr);

		const auto &exeData = miscAssets.getExeData();
		const INFFile::FlatData &flatData = inf.getFlat(flatIndex);
		const std::optional<int> &optItemIndex = flatData.itemIndex;
		DebugAssert(optItemIndex.has_value());
		const int itemIndex = *optItemIndex;

		bool isFinalBoss;
		const bool isCreature = IsCreatureIndex(itemIndex, &isFinalBoss);
		const bool isHuman = IsHumanEnemyIndex(itemIndex);

		// Lambda for converting creature dimensions to the in-engine values.
		auto makeCreatureKeyframeDimensions = [&exeData](int creatureIndex, int width, int height,
			double *outWidth, double *outHeight)
		{
			// Get the scale value of the creature.
			const uint16_t creatureScale = [&exeData, creatureIndex]()
			{
				const auto &creatureScales = exeData.entities.creatureScales;

				DebugAssertIndex(creatureScales, creatureIndex);
				const uint16_t scaleValue = creatureScales[creatureIndex];

				// Special case: 0 == 256.
				return (scaleValue == 0) ? 256 : scaleValue;
			}();

			int baseWidth, baseHeight;
			GetBaseFlatDimensions(width, height, creatureScale, &baseWidth, &baseHeight);
			*outWidth = static_cast<double>(baseWidth) / MIFFile::ARENA_UNITS;
			*outHeight = static_cast<double>(baseHeight) / MIFFile::ARENA_UNITS;
		};

		// Lambda for converting human dimensions to the in-engine values.
		auto makeHumanKeyframeDimensions = [](int width, int height, double *outWidth, double *outHeight)
		{
			const uint16_t humanScale = 256;
			int baseWidth, baseHeight;
			GetBaseFlatDimensions(width, height, humanScale, &baseWidth, &baseHeight);
			*outWidth = static_cast<double>(baseWidth) / MIFFile::ARENA_UNITS;
			*outHeight = static_cast<double>(baseHeight) / MIFFile::ARENA_UNITS;
		};

		// Write animation states for idle, look, and walk for the given anim direction.
		auto tryWriteAnimStates = [&miscAssets, &cfaCache, outIdleStates, outLookStates, outWalkStates,
			&exeData, itemIndex, isFinalBoss, isCreature, isHuman, &makeCreatureKeyframeDimensions,
			&makeHumanKeyframeDimensions](int animDirectionID)
		{
			DebugAssert(animDirectionID >= 1);
			DebugAssert(animDirectionID <= MAX_ANIM_DIRECTIONS);

			bool animIsFlipped;
			const int correctedAnimDirID = GetDynamicEntityCorrectedAnimID(animDirectionID, &animIsFlipped);

			// Determine which dynamic entity animation to load.
			if (isCreature)
			{
				const auto &creatureAnimFilenames = exeData.entities.creatureAnimationFilenames;
				const int creatureID = isFinalBoss ?
					GetFinalBossCreatureID() : GetCreatureIDFromItemIndex(itemIndex);
				const int creatureIndex = creatureID - 1;

				DebugAssertIndex(creatureAnimFilenames, creatureIndex);
				std::string creatureFilename = String::toUppercase(creatureAnimFilenames[creatureIndex]);
				if (!TrySetDynamicEntityFilenameDirection(creatureFilename, correctedAnimDirID))
				{
					DebugLogError("Couldn't set creature filename direction \"" +
						creatureFilename + "\" (" + std::to_string(correctedAnimDirID) + ").");
					return false;
				}

				// Load the .CFA of the creature at the given direction.
				const CFAFile *cfa;
				if (!cfaCache.tryGet(creatureFilename.c_str(), &cfa))
				{
					DebugLogError("Couldn't get cached .CFA file \"" + creatureFilename + "\".");
					return false;
				}

				// Prepare the states to write out.
				EntityAnimationData::State idleState = MakeAnimState(
					EntityAnimationData::StateType::Idle,
					CREATURE_ANIM_IDLE_SECONDS_PER_FRAME,
					CREATURE_ANIM_IDLE_LOOP,
					animIsFlipped);
				EntityAnimationData::State lookState = MakeAnimState(
					EntityAnimationData::StateType::Look,
					CREATURE_ANIM_LOOK_SECONDS_PER_FRAME,
					CREATURE_ANIM_LOOK_LOOP,
					animIsFlipped);
				EntityAnimationData::State walkState = MakeAnimState(
					EntityAnimationData::StateType::Walk,
					CREATURE_ANIM_WALK_SECONDS_PER_FRAME,
					CREATURE_ANIM_WALK_LOOP,
					animIsFlipped);

				// Lambda for writing keyframes to an anim state.
				auto writeStateKeyframes = [&makeCreatureKeyframeDimensions, creatureIndex, &cfa](
					EntityAnimationData::State *outState, const std::vector<int> &indices)
				{
					for (size_t i = 0; i < indices.size(); i++)
					{
						const int frameIndex = indices[i];

						double width, height;
						makeCreatureKeyframeDimensions(
							creatureIndex, cfa->getWidth(), cfa->getHeight(), &width, &height);
						const int textureID = frameIndex;

						EntityAnimationData::Keyframe keyframe(width, height, textureID);
						outState->addKeyframe(std::move(keyframe));
					}
				};

				writeStateKeyframes(&idleState, CreatureAnimIndicesIdle);
				writeStateKeyframes(&lookState, CreatureAnimIndicesLook);
				writeStateKeyframes(&walkState, CreatureAnimIndicesWalk);

				// Write animation filename to each.
				idleState.setTextureName(std::string(creatureFilename));
				lookState.setTextureName(std::string(creatureFilename));
				walkState.setTextureName(std::string(creatureFilename));

				// Write out the states to their respective state lists.
				outIdleStates->push_back(std::move(idleState));
				outLookStates->push_back(std::move(lookState));
				outWalkStates->push_back(std::move(walkState));
				return true;
			}
			else if (isHuman)
			{
				int humanFilenameTypeIndex;
				bool isMale;
				GetHumanEnemyProperties(itemIndex, miscAssets, &humanFilenameTypeIndex, &isMale);

				const int templateIndex = 0; // Idle/walk template index.
				const auto &humanFilenameTemplates = exeData.entities.humanFilenameTemplates;
				DebugAssertIndex(humanFilenameTemplates, templateIndex);
				std::string animName = humanFilenameTemplates[templateIndex];
				if (!TrySetDynamicEntityFilenameDirection(animName, correctedAnimDirID))
				{
					DebugLogError("Couldn't set human filename direction \"" +
						animName + "\" (" + std::to_string(correctedAnimDirID) + ").");
					return false;
				}

				const auto &humanFilenameTypes = exeData.entities.humanFilenameTypes;
				DebugAssertIndex(humanFilenameTypes, humanFilenameTypeIndex);
				const std::string_view humanFilenameType = humanFilenameTypes[humanFilenameTypeIndex];
				if (!TrySetHumanFilenameType(animName, humanFilenameType))
				{
					DebugLogError("Couldn't set human filename type \"" +
						animName + "\" (" + std::to_string(correctedAnimDirID) + ").");
					return false;
				}

				// Special case for plate sprites: female is replaced with male, since they would
				// apparently look the same in armor.
				const bool isPlate = humanFilenameTypeIndex == 0;

				if (!TrySetHumanFilenameGender(animName, isMale || isPlate))
				{
					DebugLogError("Couldn't set human filename gender \"" +
						animName + "\" (" + std::to_string(correctedAnimDirID) + ").");
					return false;
				}

				animName = String::toUppercase(animName);

				// Not all permutations of human filenames exist. If a series is missing,
				// then probably need to have special behavior.
				const CFAFile *cfa;
				if (!cfaCache.tryGet(animName.c_str(), &cfa))
				{
					DebugLogError("Couldn't get cached .CFA file \"" + animName + "\".");
					return false;
				}

				// Prepare the states to write out. Human enemies don't have look animations.
				EntityAnimationData::State idleState = MakeAnimState(
					EntityAnimationData::StateType::Idle,
					HUMAN_ANIM_IDLE_SECONDS_PER_FRAME,
					HUMAN_ANIM_IDLE_LOOP,
					animIsFlipped);
				EntityAnimationData::State walkState = MakeAnimState(
					EntityAnimationData::StateType::Walk,
					HUMAN_ANIM_WALK_SECONDS_PER_FRAME,
					HUMAN_ANIM_WALK_LOOP,
					animIsFlipped);

				// Lambda for writing keyframes to an anim state.
				auto writeStateKeyframes = [&makeHumanKeyframeDimensions, &cfa](
					EntityAnimationData::State *outState, const std::vector<int> &indices)
				{
					for (size_t i = 0; i < indices.size(); i++)
					{
						const int frameIndex = indices[i];

						double width, height;
						makeHumanKeyframeDimensions(cfa->getWidth(), cfa->getHeight(), &width, &height);
						const int textureID = frameIndex;

						EntityAnimationData::Keyframe keyframe(width, height, textureID);
						outState->addKeyframe(std::move(keyframe));
					}
				};

				writeStateKeyframes(&idleState, HumanAnimIndicesIdle);
				writeStateKeyframes(&walkState, HumanAnimIndicesWalk);

				// Write animation filename to each.
				idleState.setTextureName(std::string(animName));
				walkState.setTextureName(std::string(animName));

				// Write out the states to their respective state lists.
				outIdleStates->push_back(std::move(idleState));
				outWalkStates->push_back(std::move(walkState));
				return true;
			}
			else
			{
				DebugLogError("Not implemented.");
				return false;
			}
		};

		auto tryWriteAttackAnimStates = [&miscAssets, &cfaCache, outAttackStates, &exeData, itemIndex,
			isFinalBoss, isCreature, isHuman, &makeCreatureKeyframeDimensions, &makeHumanKeyframeDimensions]()
		{
			// Attack state is only in the first .CFA file.
			const int animDirectionID = 1;
			const bool animIsFlipped = false;

			if (isCreature)
			{
				const auto &creatureAnimFilenames = exeData.entities.creatureAnimationFilenames;

				const int creatureID = isFinalBoss ?
					GetFinalBossCreatureID() : GetCreatureIDFromItemIndex(itemIndex);
				const int creatureIndex = creatureID - 1;

				DebugAssertIndex(creatureAnimFilenames, creatureIndex);
				std::string creatureFilename = String::toUppercase(creatureAnimFilenames[creatureIndex]);
				if (!TrySetDynamicEntityFilenameDirection(creatureFilename, animDirectionID))
				{
					DebugLogError("Couldn't set creature filename direction \"" +
						creatureFilename + "\" (" + std::to_string(animDirectionID) + ").");
					return false;
				}

				// Load the .CFA of the creature at the given direction.
				const CFAFile *cfa;
				if (!cfaCache.tryGet(creatureFilename.c_str(), &cfa))
				{
					DebugLogError("Couldn't get cached .CFA file \"" + creatureFilename + "\".");
					return false;
				}

				EntityAnimationData::State attackState = MakeAnimState(
					EntityAnimationData::StateType::Attack,
					CREATURE_ANIM_ATTACK_SECONDS_PER_FRAME,
					CREATURE_ANIM_ATTACK_LOOP,
					animIsFlipped);

				for (size_t i = 0; i < CreatureAnimIndicesAttack.size(); i++)
				{
					const int frameIndex = CreatureAnimIndicesAttack[i];

					double width, height;
					makeCreatureKeyframeDimensions(
						creatureIndex, cfa->getWidth(), cfa->getHeight(), &width, &height);
					const int textureID = frameIndex;

					EntityAnimationData::Keyframe keyframe(width, height, textureID);
					attackState.addKeyframe(std::move(keyframe));
				}

				// Write animation filename.
				attackState.setTextureName(std::move(creatureFilename));

				outAttackStates->push_back(std::move(attackState));
				return true;
			}
			else if (isHuman)
			{
				int humanFilenameTypeIndex;
				bool isMale;
				GetHumanEnemyProperties(itemIndex, miscAssets, &humanFilenameTypeIndex, &isMale);

				const int attackTemplateIndex = 1;
				const auto &humanFilenameTemplates = exeData.entities.humanFilenameTemplates;
				DebugAssertIndex(humanFilenameTemplates, attackTemplateIndex);
				std::string animName = humanFilenameTemplates[attackTemplateIndex];
				if (!TrySetDynamicEntityFilenameDirection(animName, animDirectionID))
				{
					DebugLogError("Couldn't set human attack filename direction \"" +
						animName + "\" (" + std::to_string(animDirectionID) + ").");
					return false;
				}

				const auto &humanFilenameTypes = exeData.entities.humanFilenameTypes;
				DebugAssertIndex(humanFilenameTypes, humanFilenameTypeIndex);
				const std::string_view humanFilenameType = humanFilenameTypes[humanFilenameTypeIndex];
				if (!TrySetHumanFilenameType(animName, humanFilenameType))
				{
					DebugLogError("Couldn't set human attack filename type \"" +
						animName + "\" (" + std::to_string(animDirectionID) + ").");
					return false;
				}

				// Special case for plate sprites: female is replaced with male, since they would
				// apparently look the same in armor.
				const bool isPlate = humanFilenameTypeIndex == 0;

				if (!TrySetHumanFilenameGender(animName, isMale || isPlate))
				{
					DebugLogError("Couldn't set human attack filename gender \"" +
						animName + "\" (" + std::to_string(animDirectionID) + ").");
					return false;
				}

				animName = String::toUppercase(animName);

				const CFAFile *cfa;
				if (!cfaCache.tryGet(animName.c_str(), &cfa))
				{
					DebugLogError("Couldn't get cached .CFA file \"" + animName + "\".");
					return false;
				}

				EntityAnimationData::State attackState = MakeAnimState(
					EntityAnimationData::StateType::Attack,
					HUMAN_ANIM_ATTACK_SECONDS_PER_FRAME,
					HUMAN_ANIM_ATTACK_LOOP,
					animIsFlipped);

				for (int i = 0; i < cfa->getImageCount(); i++)
				{
					const int frameIndex = i;

					double width, height;
					makeHumanKeyframeDimensions(cfa->getWidth(), cfa->getHeight(), &width, &height);
					const int textureID = frameIndex;

					EntityAnimationData::Keyframe keyframe(width, height, textureID);
					attackState.addKeyframe(std::move(keyframe));
				}

				// Write animation filename.
				attackState.setTextureName(std::move(animName));

				outAttackStates->push_back(std::move(attackState));
				return true;
			}
			else
			{
				DebugLogError("Not implemented.");
				return false;
			}
		};

		auto tryWriteDeathAnimStates = [&inf, &cfaCache, outDeathStates, &exeData, itemIndex, isFinalBoss,
			isCreature, isHuman, &makeCreatureKeyframeDimensions]()
		{
			const bool animIsFlipped = false;

			if (isCreature)
			{
				// Death state is only in the last .CFA file.
				const int animDirectionID = 6;

				const auto &creatureAnimFilenames = exeData.entities.creatureAnimationFilenames;
				const int creatureID = isFinalBoss ?
					GetFinalBossCreatureID() : GetCreatureIDFromItemIndex(itemIndex);
				const int creatureIndex = creatureID - 1;

				DebugAssertIndex(creatureAnimFilenames, creatureIndex);
				std::string creatureFilename = String::toUppercase(creatureAnimFilenames[creatureIndex]);
				if (!TrySetDynamicEntityFilenameDirection(creatureFilename, animDirectionID))
				{
					DebugLogError("Couldn't set creature filename direction \"" +
						creatureFilename + "\" (" + std::to_string(animDirectionID) + ").");
					return false;
				}

				// Load the .CFA of the creature at the given direction.
				const CFAFile *cfa;
				if (!cfaCache.tryGet(creatureFilename.c_str(), &cfa))
				{
					DebugLogError("Couldn't get cached .CFA file \"" + creatureFilename + "\".");
					return false;
				}

				EntityAnimationData::State deathState = MakeAnimState(
					EntityAnimationData::StateType::Death,
					CREATURE_ANIM_DEATH_SECONDS_PER_FRAME,
					CREATURE_ANIM_DEATH_LOOP,
					animIsFlipped);

				for (int i = 0; i < cfa->getImageCount(); i++)
				{
					double width, height;
					makeCreatureKeyframeDimensions(
						creatureIndex, cfa->getWidth(), cfa->getHeight(), &width, &height);
					const int textureID = i;

					EntityAnimationData::Keyframe keyframe(width, height, textureID);
					deathState.addKeyframe(std::move(keyframe));
				}

				// Write animation filename.
				deathState.setTextureName(std::move(creatureFilename));

				outDeathStates->push_back(std::move(deathState));
				return true;
			}
			else if (isHuman)
			{
				// Humans use a single dead body image.
				const int corpseItemIndex = 2;
				const INFFile::FlatData *corpseFlat = inf.getFlatWithItemIndex(corpseItemIndex);
				DebugAssertMsg(corpseFlat != nullptr, "Missing human corpse flat.");
				const int corpseFlatTextureIndex = corpseFlat->textureIndex;
				const auto &flatTextures = inf.getFlatTextures();
				DebugAssertIndex(flatTextures, corpseFlatTextureIndex);
				std::string animName = String::toUppercase(flatTextures[corpseFlatTextureIndex].filename);

				IMGFile img;
				if (!img.init(animName.c_str()))
				{
					DebugLogError("Couldn't init .IMG file \"" + animName + "\".");
					return false;
				}

				EntityAnimationData::State deathState = MakeAnimState(
					EntityAnimationData::StateType::Death,
					HUMAN_ANIM_ATTACK_SECONDS_PER_FRAME,
					HUMAN_ANIM_ATTACK_LOOP,
					animIsFlipped);

				deathState.setTextureName(std::string(animName));

				// Human corpse is not affected by human scaling values.
				const double width = static_cast<double>(img.getWidth()) / MIFFile::ARENA_UNITS;
				const double height = static_cast<double>(img.getHeight()) / MIFFile::ARENA_UNITS;
				const int textureID = 0;

				EntityAnimationData::Keyframe keyframe(width, height, textureID);
				deathState.addKeyframe(std::move(keyframe));
				outDeathStates->push_back(std::move(deathState));
				return true;
			}
			else
			{
				DebugLogError("Not implemented.");
				return false;
			}
		};

		for (int i = 1; i <= MAX_ANIM_DIRECTIONS; i++)
		{
			if (!tryWriteAnimStates(i))
			{
				DebugLogError("Couldn't make anim states for direction \"" + std::to_string(i) + "\".");
			}
		}

		if (!tryWriteAttackAnimStates())
		{
			DebugLogError("Couldn't make attack anim states.");
		}

		if (!tryWriteDeathAnimStates())
		{
			DebugLogError("Couldn't make death anim states.");
		}
	}
}

LevelData::FlatDef::FlatDef(int flatIndex)
{
	this->flatIndex = flatIndex;
}

int LevelData::FlatDef::getFlatIndex() const
{
	return this->flatIndex;
}

const std::vector<Int2> &LevelData::FlatDef::getPositions() const
{
	return this->positions;
}

void LevelData::FlatDef::addPosition(const Int2 &position)
{
	this->positions.push_back(position);
}

LevelData::Lock::Lock(const Int2 &position, int lockLevel)
	: position(position)
{
	this->lockLevel = lockLevel;
}

const Int2 &LevelData::Lock::getPosition() const
{
	return this->position;
}

int LevelData::Lock::getLockLevel() const
{
	return this->lockLevel;
}

LevelData::TextTrigger::TextTrigger(const std::string &text, bool displayedOnce)
	: text(text)
{
	this->displayedOnce = displayedOnce;
	this->previouslyDisplayed = false;
}

const std::string &LevelData::TextTrigger::getText() const
{
	return this->text;
}

bool LevelData::TextTrigger::isSingleDisplay() const
{
	return this->displayedOnce;
}

bool LevelData::TextTrigger::hasBeenDisplayed() const
{
	return this->previouslyDisplayed;
}

void LevelData::TextTrigger::setPreviouslyDisplayed(bool previouslyDisplayed)
{
	this->previouslyDisplayed = previouslyDisplayed;
}

LevelData::DoorState::DoorState(const Int2 &voxel, double percentOpen,
	DoorState::Direction direction)
	: voxel(voxel)
{
	this->percentOpen = percentOpen;
	this->direction = direction;
}

LevelData::DoorState::DoorState(const Int2 &voxel)
	: DoorState(voxel, 0.0, DoorState::Direction::Opening) { }

const Int2 &LevelData::DoorState::getVoxel() const
{
	return this->voxel;
}

double LevelData::DoorState::getPercentOpen() const
{
	return this->percentOpen;
}

bool LevelData::DoorState::isClosing() const
{
	return this->direction == Direction::Closing;
}

bool LevelData::DoorState::isClosed() const
{
	return this->percentOpen == 0.0;
}

void LevelData::DoorState::setDirection(DoorState::Direction direction)
{
	this->direction = direction;
}

void LevelData::DoorState::update(double dt)
{
	const double delta = DoorState::DEFAULT_SPEED * dt;

	// Decide how to change the door state depending on its current direction.
	if (this->direction == DoorState::Direction::Opening)
	{
		this->percentOpen = std::min(this->percentOpen + delta, 1.0);
		const bool isOpen = this->percentOpen == 1.0;

		if (isOpen)
		{
			this->direction = DoorState::Direction::None;
		}
	}
	else if (this->direction == DoorState::Direction::Closing)
	{
		this->percentOpen = std::max(this->percentOpen - delta, 0.0);

		if (this->isClosed())
		{
			this->direction = DoorState::Direction::None;
		}
	}
}

LevelData::FadeState::FadeState(const Int3 &voxel, double targetSeconds)
	: voxel(voxel)
{
	this->currentSeconds = 0.0;
	this->targetSeconds = targetSeconds;
}

LevelData::FadeState::FadeState(const Int3 &voxel)
	: FadeState(voxel, FadeState::DEFAULT_SECONDS) { }

const Int3 &LevelData::FadeState::getVoxel() const
{
	return this->voxel;
}

double LevelData::FadeState::getPercentDone() const
{
	return std::clamp(this->currentSeconds / this->targetSeconds, 0.0, 1.0);
}

bool LevelData::FadeState::isDoneFading() const
{
	return this->getPercentDone() == 1.0;
}

void LevelData::FadeState::update(double dt)
{
	this->currentSeconds = std::min(this->currentSeconds + dt, this->targetSeconds);
}

LevelData::LevelData(int gridWidth, int gridHeight, int gridDepth, const std::string &infName,
	const std::string &name)
	: voxelGrid(gridWidth, gridHeight, gridDepth), name(name)
{
	if (!this->inf.init(infName.c_str()))
	{
		DebugCrash("Could not init .INF file \"" + infName + "\".");
	}
}

LevelData::~LevelData()
{

}

const std::string &LevelData::getName() const
{
	return this->name;
}

double LevelData::getCeilingHeight() const
{
	return static_cast<double>(this->inf.getCeiling().height) / MIFFile::ARENA_UNITS;
}

std::vector<LevelData::FlatDef> &LevelData::getFlats()
{
	return this->flatsLists;
}

const std::vector<LevelData::FlatDef> &LevelData::getFlats() const
{
	return this->flatsLists;
}

std::vector<LevelData::DoorState> &LevelData::getOpenDoors()
{
	return this->openDoors;
}

const std::vector<LevelData::DoorState> &LevelData::getOpenDoors() const
{
	return this->openDoors;
}

std::vector<LevelData::FadeState> &LevelData::getFadingVoxels()
{
	return this->fadingVoxels;
}

const std::vector<LevelData::FadeState> &LevelData::getFadingVoxels() const
{
	return this->fadingVoxels;
}

const INFFile &LevelData::getInfFile() const
{
	return this->inf;
}

EntityManager &LevelData::getEntityManager()
{
	return this->entityManager;
}

const EntityManager &LevelData::getEntityManager() const
{
	return this->entityManager;
}

VoxelGrid &LevelData::getVoxelGrid()
{
	return this->voxelGrid;
}

const VoxelGrid &LevelData::getVoxelGrid() const
{
	return this->voxelGrid;
}

const LevelData::Lock *LevelData::getLock(const Int2 &voxel) const
{
	const auto lockIter = this->locks.find(voxel);
	return (lockIter != this->locks.end()) ? &lockIter->second : nullptr;
}

void LevelData::addFlatInstance(int flatIndex, const Int2 &flatPosition)
{
	// Add position to instance list if the flat def has already been created.
	const auto iter = std::find_if(this->flatsLists.begin(), this->flatsLists.end(),
		[flatIndex](const FlatDef &flatDef)
	{
		return flatDef.getFlatIndex() == flatIndex;
	});

	if (iter != this->flatsLists.end())
	{
		iter->addPosition(flatPosition);
	}
	else
	{
		// Create new def.
		FlatDef flatDef(flatIndex);
		flatDef.addPosition(flatPosition);
		this->flatsLists.push_back(std::move(flatDef));
	}
}

void LevelData::setVoxel(int x, int y, int z, uint16_t id)
{
	this->voxelGrid.setVoxel(x, y, z, id);
}

void LevelData::readFLOR(const uint16_t *flor, const INFFile &inf, int gridWidth, int gridDepth)
{
	// Lambda for obtaining a two-byte FLOR voxel.
	auto getFlorVoxel = [flor, gridWidth, gridDepth](int x, int z)
	{
		// Read voxel data in reverse order.
		const int index = (((gridDepth - 1) - z) * 2) + ((((gridWidth - 1) - x) * 2) * gridDepth);
		const uint16_t voxel = Bytes::getLE16(reinterpret_cast<const uint8_t*>(flor) + index);
		return voxel;
	};

	// Lambda for obtaining the voxel data index of a typical (non-chasm) FLOR voxel.
	auto getFlorDataIndex = [this](uint16_t florVoxel, int floorTextureID)
	{
		// See if the voxel already has a mapping.
		const auto floorIter = std::find_if(
			this->floorDataMappings.begin(), this->floorDataMappings.end(),
			[florVoxel](const std::pair<uint16_t, int> &pair)
		{
			return pair.first == florVoxel;
		});

		if (floorIter != this->floorDataMappings.end())
		{
			return floorIter->second;
		}
		else
		{
			// Insert new mapping.
			const int index = this->voxelGrid.addVoxelData(
				VoxelData::makeFloor(floorTextureID));
			this->floorDataMappings.push_back(std::make_pair(florVoxel, index));
			return index;
		}
	};

	using ChasmDataFunc = VoxelData(*)(const INFFile &inf, const std::array<bool, 4>&);

	// Lambda for obtaining the voxel data index of a chasm voxel. The given function argument
	// returns the created voxel data if there was no previous mapping.
	auto getChasmDataIndex = [this, &inf](uint16_t florVoxel, ChasmDataFunc chasmFunc,
		const std::array<bool, 4> &adjacentFaces)
	{
		const auto chasmIter = std::find_if(
			this->chasmDataMappings.begin(), this->chasmDataMappings.end(),
			[florVoxel, &adjacentFaces](const auto &tuple)
		{
			return (std::get<0>(tuple) == florVoxel) && (std::get<1>(tuple) == adjacentFaces);
		});

		if (chasmIter != this->chasmDataMappings.end())
		{
			return std::get<2>(*chasmIter);
		}
		else
		{
			const int index = this->voxelGrid.addVoxelData(chasmFunc(inf, adjacentFaces));
			this->chasmDataMappings.push_back(std::make_tuple(florVoxel, adjacentFaces, index));
			return index;
		}
	};

	// Helper lambdas for creating each type of chasm voxel data.
	auto makeDryChasmVoxelData = [](const INFFile &inf, const std::array<bool, 4> &adjacentFaces)
	{
		const int dryChasmID = [&inf]()
		{
			const int *ptr = inf.getDryChasmIndex();
			if (ptr != nullptr)
			{
				return *ptr;
			}
			else
			{
				DebugLogWarning("Missing *DRYCHASM ID.");
				return 0;
			}
		}();

		DebugAssert(adjacentFaces.size() == 4);
		return VoxelData::makeChasm(dryChasmID,
			adjacentFaces[0], adjacentFaces[1], adjacentFaces[2], adjacentFaces[3],
			VoxelData::ChasmData::Type::Dry);
	};

	auto makeLavaChasmVoxelData = [](const INFFile &inf, const std::array<bool, 4> &adjacentFaces)
	{
		const int lavaChasmID = [&inf]()
		{
			const int *ptr = inf.getLavaChasmIndex();
			if (ptr != nullptr)
			{
				return *ptr;
			}
			else
			{
				DebugLogWarning("Missing *LAVACHASM ID.");
				return 0;
			}
		}();

		DebugAssert(adjacentFaces.size() == 4);
		return VoxelData::makeChasm(lavaChasmID,
			adjacentFaces[0], adjacentFaces[1], adjacentFaces[2], adjacentFaces[3],
			VoxelData::ChasmData::Type::Lava);
	};

	auto makeWetChasmVoxelData = [](const INFFile &inf, const std::array<bool, 4> &adjacentFaces)
	{
		const int wetChasmID = [&inf]()
		{
			const int *ptr = inf.getWetChasmIndex();
			if (ptr != nullptr)
			{
				return *ptr;
			}
			else
			{
				DebugLogWarning("Missing *WETCHASM ID.");
				return 0;
			}
		}();

		DebugAssert(adjacentFaces.size() == 4);
		return VoxelData::makeChasm(wetChasmID,
			adjacentFaces[0], adjacentFaces[1], adjacentFaces[2], adjacentFaces[3],
			VoxelData::ChasmData::Type::Wet);
	};

	// Write the voxel IDs into the voxel grid.
	for (int x = 0; x < gridWidth; x++)
	{
		for (int z = 0; z < gridDepth; z++)
		{
			auto getFloorTextureID = [](uint16_t voxel)
			{
				return (voxel & 0xFF00) >> 8;
			};

			auto getFlatIndex = [](uint16_t voxel)
			{
				return voxel & 0x00FF;
			};

			auto isChasm = [](int id)
			{
				return (id == MIFFile::DRY_CHASM) ||
					(id == MIFFile::LAVA_CHASM) ||
					(id == MIFFile::WET_CHASM);
			};

			const uint16_t florVoxel = getFlorVoxel(x, z);
			const int floorTextureID = getFloorTextureID(florVoxel);

			// See if the floor voxel is either solid or a chasm.
			if (!isChasm(floorTextureID))
			{
				// Get the voxel data index associated with the floor value, or add it
				// if it doesn't exist yet.
				const int dataIndex = getFlorDataIndex(florVoxel, floorTextureID);
				this->setVoxel(x, 0, z, dataIndex);
			}
			else
			{
				// The voxel is a chasm. See which of its four faces are adjacent to
				// a solid floor voxel.
				const uint16_t northVoxel = getFlorVoxel(std::min(x + 1, gridWidth - 1), z);
				const uint16_t eastVoxel = getFlorVoxel(x, std::min(z + 1, gridDepth - 1));
				const uint16_t southVoxel = getFlorVoxel(std::max(x - 1, 0), z);
				const uint16_t westVoxel = getFlorVoxel(x, std::max(z - 1, 0));

				const std::array<bool, 4> adjacentFaces
				{
					!isChasm(getFloorTextureID(northVoxel)), // North.
					!isChasm(getFloorTextureID(eastVoxel)), // East.
					!isChasm(getFloorTextureID(southVoxel)), // South.
					!isChasm(getFloorTextureID(westVoxel)) // West.
				};

				if (floorTextureID == MIFFile::DRY_CHASM)
				{
					const int dataIndex = getChasmDataIndex(
						florVoxel, makeDryChasmVoxelData, adjacentFaces);
					this->setVoxel(x, 0, z, dataIndex);
				}
				else if (floorTextureID == MIFFile::LAVA_CHASM)
				{
					const int dataIndex = getChasmDataIndex(
						florVoxel, makeLavaChasmVoxelData, adjacentFaces);
					this->setVoxel(x, 0, z, dataIndex);
				}
				else if (floorTextureID == MIFFile::WET_CHASM)
				{
					const int dataIndex = getChasmDataIndex(
						florVoxel, makeWetChasmVoxelData, adjacentFaces);
					this->setVoxel(x, 0, z, dataIndex);
				}
			}

			// See if the FLOR voxel contains a FLAT index (for raised platform flats).
			const int flatIndex = getFlatIndex(florVoxel);
			if (flatIndex > 0)
			{
				this->addFlatInstance(flatIndex - 1, Int2(x, z));
			}
		}
	}
}

void LevelData::readMAP1(const uint16_t *map1, const INFFile &inf, WorldType worldType,
	int gridWidth, int gridDepth, const ExeData &exeData)
{
	// Lambda for obtaining a two-byte MAP1 voxel.
	auto getMap1Voxel = [map1, gridWidth, gridDepth](int x, int z)
	{
		// Read voxel data in reverse order.
		const int index = (((gridDepth - 1) - z) * 2) + ((((gridWidth - 1) - x) * 2) * gridDepth);
		const uint16_t voxel = Bytes::getLE16(reinterpret_cast<const uint8_t*>(map1) + index);
		return voxel;
	};

	// Lambda for finding if there's an existing mapping for a MAP1 voxel.
	auto findWallMapping = [this](uint16_t map1Voxel)
	{
		return std::find_if(this->wallDataMappings.begin(), this->wallDataMappings.end(),
			[map1Voxel](const std::pair<uint16_t, int> &pair)
		{
			return pair.first == map1Voxel;
		});
	};

	// Lambda for obtaining the voxel data index of a general-case MAP1 object. The function
	// parameter returns the created voxel data if no previous mapping exists, and is intended
	// for general cases where the voxel data type does not need extra parameters.
	auto getDataIndex = [this, &findWallMapping](uint16_t map1Voxel, VoxelData(*func)(uint16_t))
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			const int index = this->voxelGrid.addVoxelData(func(map1Voxel));
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for obtaining the voxel data index of a solid wall.
	auto getWallDataIndex = [this, &inf, &findWallMapping](uint16_t map1Voxel, uint8_t mostSigByte)
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			// Lambda for creating a basic solid wall voxel data.
			auto makeWallVoxelData = [map1Voxel, &inf, mostSigByte]()
			{
				const int textureIndex = mostSigByte - 1;

				// Menu index if the voxel has the *MENU tag, or -1 if it is
				// not a *MENU voxel.
				const int menuIndex = inf.getMenuIndex(textureIndex);
				const bool isMenu = menuIndex != -1;

				// Determine what the type of the wall is (level up/down, menu, 
				// or just plain solid).
				const VoxelData::WallData::Type type = [&inf, textureIndex, isMenu]()
				{
					// Returns whether the given index pointer is non-null and
					// matches the current texture index.
					auto matchesIndex = [textureIndex](const int *index)
					{
						return (index != nullptr) && (*index == textureIndex);
					};

					if (matchesIndex(inf.getLevelUpIndex()))
					{
						return VoxelData::WallData::Type::LevelUp;
					}
					else if (matchesIndex(inf.getLevelDownIndex()))
					{
						return VoxelData::WallData::Type::LevelDown;
					}
					else if (isMenu)
					{
						return VoxelData::WallData::Type::Menu;
					}
					else
					{
						return VoxelData::WallData::Type::Solid;
					}
				}();

				VoxelData voxelData = VoxelData::makeWall(textureIndex, textureIndex, textureIndex,
					(isMenu ? &menuIndex : nullptr), type);

				// Set the *MENU index if it's a menu voxel.
				if (isMenu)
				{
					VoxelData::WallData &wallData = voxelData.wall;
					wallData.menuID = menuIndex;
				}

				return voxelData;
			};

			const int index = this->voxelGrid.addVoxelData(makeWallVoxelData());
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for obtaining the voxel data index of a raised platform.
	auto getRaisedDataIndex = [this, &inf, worldType, &exeData, &findWallMapping](
		uint16_t map1Voxel, uint8_t mostSigByte, int x, int z)
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			// Lambda for creating a raised voxel data.
			auto makeRaisedVoxelData = [worldType, &exeData, map1Voxel, &inf, mostSigByte, x, z]()
			{
				const uint8_t wallTextureID = map1Voxel & 0x000F;
				const uint8_t capTextureID = (map1Voxel & 0x00F0) >> 4;

				const int sideID = [&inf, wallTextureID]()
				{
					const int *ptr = inf.getBoxSide(wallTextureID);
					if (ptr != nullptr)
					{
						return *ptr;
					}
					else
					{
						DebugLogWarning("Missing *BOXSIDE ID \"" +
							std::to_string(wallTextureID) + "\".");
						return 0;
					}
				}();

				const int floorID = [&inf, x, z]()
				{
					const auto &id = inf.getCeiling().textureIndex;

					if (id.has_value())
					{
						return id.value();
					}
					else
					{
						DebugLogWarning("Missing platform floor ID (" +
							std::to_string(x) + ", " + std::to_string(z) + ").");
						return 0;
					}
				}();

				const int ceilingID = [&inf, capTextureID]()
				{
					const int *ptr = inf.getBoxCap(capTextureID);
					if (ptr != nullptr)
					{
						return *ptr;
					}
					else
					{
						DebugLogWarning("Missing *BOXCAP ID \"" +
							std::to_string(capTextureID) + "\".");
						return 0;
					}
				}();

				const auto &wallHeightTables = exeData.wallHeightTables;
				const int heightIndex = mostSigByte & 0x07;
				const int thicknessIndex = (mostSigByte & 0x78) >> 3;
				int baseOffset, baseSize;

				if (worldType == WorldType::City)
				{
					baseOffset = wallHeightTables.box1b.at(heightIndex);
					baseSize = wallHeightTables.box2b.at(thicknessIndex);
				}
				else if (worldType == WorldType::Interior)
				{
					baseOffset = wallHeightTables.box1a.at(heightIndex);

					const int boxSize = wallHeightTables.box2a.at(thicknessIndex);
					const auto &boxScale = inf.getCeiling().boxScale;
					baseSize = boxScale.has_value() ?
						((boxSize * (*boxScale)) / 256) : boxSize;
				}
				else if (worldType == WorldType::Wilderness)
				{
					baseOffset = wallHeightTables.box1c.at(heightIndex);

					const int boxSize = 32;
					const auto &boxScale = inf.getCeiling().boxScale;
					baseSize = (boxSize *
						(boxScale.has_value() ? boxScale.value() : 192)) / 256;
				}
				else
				{
					throw DebugException("Invalid world type \"" +
						std::to_string(static_cast<int>(worldType)) + "\".");
				}

				const double yOffset =
					static_cast<double>(baseOffset) / MIFFile::ARENA_UNITS;
				const double ySize =
					static_cast<double>(baseSize) / MIFFile::ARENA_UNITS;

				const double normalizedScale =
					static_cast<double>(inf.getCeiling().height) /
					MIFFile::ARENA_UNITS;
				const double yOffsetNormalized = yOffset / normalizedScale;
				const double ySizeNormalized = ySize / normalizedScale;

				// @todo: might need some tweaking with box3/box4 values.
				const double vTop = std::max(
					0.0, 1.0 - yOffsetNormalized - ySizeNormalized);
				const double vBottom = std::min(vTop + ySizeNormalized, 1.0);

				return VoxelData::makeRaised(sideID, floorID, ceilingID,
					yOffsetNormalized, ySizeNormalized, vTop, vBottom);
			};

			const int index = this->voxelGrid.addVoxelData(makeRaisedVoxelData());
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for creating type 0x9 voxel data.
	auto makeType9VoxelData = [](uint16_t map1Voxel)
	{
		const int textureIndex = (map1Voxel & 0x00FF) - 1;
		const bool collider = (map1Voxel & 0x0100) == 0;
		return VoxelData::makeTransparentWall(textureIndex, collider);
	};

	// Lambda for obtaining the voxel data index of a type 0xA voxel.
	auto getTypeADataIndex = [this, worldType, &findWallMapping](
		uint16_t map1Voxel, int textureIndex)
	{
		const auto wallIter = findWallMapping(map1Voxel);
		if (wallIter != this->wallDataMappings.end())
		{
			return wallIter->second;
		}
		else
		{
			// Lambda for creating type 0xA voxel data.
			auto makeTypeAVoxelData = [worldType, map1Voxel, textureIndex]()
			{
				const double yOffset = [worldType, map1Voxel]()
				{
					const int baseOffset = (map1Voxel & 0x0E00) >> 9;
					const int fullOffset = (worldType == WorldType::Interior) ?
						(baseOffset * 8) : ((baseOffset * 32) - 8);

					return static_cast<double>(fullOffset) / MIFFile::ARENA_UNITS;
				}();

				const bool collider = (map1Voxel & 0x0100) != 0;

				// "Flipped" is not present in the original game, but has been added
				// here so that all edge voxel texture coordinates (i.e., palace
				// graphics, store signs) can be correct. Currently only palace
				// graphics and gates are type 0xA colliders, I believe.
				const bool flipped = collider;

				const VoxelData::Facing facing = [map1Voxel]()
				{
					// Orientation is a multiple of 4 (0, 4, 8, C), where 0 is north
					// and C is east. It is stored in two bits above the texture index.
					const int orientation = (map1Voxel & 0x00C0) >> 4;
					if (orientation == 0x0)
					{
						return VoxelData::Facing::PositiveX;
					}
					else if (orientation == 0x4)
					{
						return VoxelData::Facing::NegativeZ;
					}
					else if (orientation == 0x8)
					{
						return VoxelData::Facing::NegativeX;
					}
					else
					{
						return VoxelData::Facing::PositiveZ;
					}
				}();

				return VoxelData::makeEdge(
					textureIndex, yOffset, collider, flipped, facing);
			};

			const int index = this->voxelGrid.addVoxelData(makeTypeAVoxelData());
			this->wallDataMappings.push_back(std::make_pair(map1Voxel, index));
			return index;
		}
	};

	// Lambda for creating type 0xB voxel data.
	auto makeTypeBVoxelData = [](uint16_t map1Voxel)
	{
		const int textureIndex = (map1Voxel & 0x003F) - 1;
		const VoxelData::DoorData::Type doorType = [map1Voxel]()
		{
			const int type = (map1Voxel & 0x00C0) >> 4;
			if (type == 0x0)
			{
				return VoxelData::DoorData::Type::Swinging;
			}
			else if (type == 0x4)
			{
				return VoxelData::DoorData::Type::Sliding;
			}
			else if (type == 0x8)
			{
				return VoxelData::DoorData::Type::Raising;
			}
			else
			{
				// I don't believe any doors in Arena split (but they are
				// supported by the engine).
				DebugUnhandledReturnMsg(
					VoxelData::DoorData::Type, std::to_string(type));
			}
		}();

		return VoxelData::makeDoor(textureIndex, doorType);
	};

	// Lambda for creating type 0xD voxel data.
	auto makeTypeDVoxelData = [](uint16_t map1Voxel)
	{
		const int textureIndex = (map1Voxel & 0x00FF) - 1;
		const bool isRightDiag = (map1Voxel & 0x0100) == 0;
		return VoxelData::makeDiagonal(textureIndex, isRightDiag);
	};

	// Write the voxel IDs into the voxel grid.
	for (int x = 0; x < gridWidth; x++)
	{
		for (int z = 0; z < gridDepth; z++)
		{
			const uint16_t map1Voxel = getMap1Voxel(x, z);

			if ((map1Voxel & 0x8000) == 0)
			{
				// A voxel of some kind.
				const bool voxelIsEmpty = map1Voxel == 0;

				if (!voxelIsEmpty)
				{
					const uint8_t mostSigByte = (map1Voxel & 0x7F00) >> 8;
					const uint8_t leastSigByte = map1Voxel & 0x007F;
					const bool voxelIsSolid = mostSigByte == leastSigByte;

					if (voxelIsSolid)
					{
						// Regular solid wall.
						const int dataIndex = getWallDataIndex(map1Voxel, mostSigByte);
						this->setVoxel(x, 1, z, dataIndex);
					}
					else
					{
						// Raised platform.
						const int dataIndex = getRaisedDataIndex(map1Voxel, mostSigByte, x, z);
						this->setVoxel(x, 1, z, dataIndex);
					}
				}
			}
			else
			{
				// A special voxel, or an object of some kind.
				const uint8_t mostSigNibble = (map1Voxel & 0xF000) >> 12;

				if (mostSigNibble == 0x8)
				{
					// The lower byte determines the index of a FLAT for an object.
					const uint8_t flatIndex = map1Voxel & 0x00FF;
					if (flatIndex > 0)
					{
						this->addFlatInstance(flatIndex, Int2(x, z));
					}
				}
				else if (mostSigNibble == 0x9)
				{
					// Transparent block with 1-sided texture on all sides, such as wooden 
					// arches in dungeons. These do not have back-faces (especially when 
					// standing in the voxel itself).
					const int dataIndex = getDataIndex(map1Voxel, makeType9VoxelData);
					this->setVoxel(x, 1, z, dataIndex);
				}
				else if (mostSigNibble == 0xA)
				{
					// Transparent block with 2-sided texture on one side (i.e., fence).
					const int textureIndex = (map1Voxel & 0x003F) - 1;

					// It is clamped non-negative due to a case in the center province's city
					// where one temple voxel has all zeroes for its texture index, and it
					// appears solid gray in the original game (presumably a silent bug).
					if (textureIndex >= 0)
					{
						const int dataIndex = getTypeADataIndex(map1Voxel, textureIndex);
						this->setVoxel(x, 1, z, dataIndex);
					}
				}
				else if (mostSigNibble == 0xB)
				{
					// Door voxel.
					const int dataIndex = getDataIndex(map1Voxel, makeTypeBVoxelData);
					this->setVoxel(x, 1, z, dataIndex);
				}
				else if (mostSigNibble == 0xC)
				{
					// Unknown.
					DebugLogWarning("Voxel type 0xC not implemented.");
				}
				else if (mostSigNibble == 0xD)
				{
					// Diagonal wall. Its type is determined by the nineth bit.
					const int dataIndex = getDataIndex(map1Voxel, makeTypeDVoxelData);
					this->setVoxel(x, 1, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readMAP2(const uint16_t *map2, const INFFile &inf, int gridWidth, int gridDepth)
{
	// Lambda for obtaining a two-byte MAP2 voxel.
	auto getMap2Voxel = [map2, gridWidth, gridDepth](int x, int z)
	{
		// Read voxel data in reverse order.
		const int index = (((gridDepth - 1) - z) * 2) + ((((gridWidth - 1) - x) * 2) * gridDepth);
		const uint16_t voxel = Bytes::getLE16(reinterpret_cast<const uint8_t*>(map2) + index);
		return voxel;
	};

	// Lambda for getting the number of stories a MAP2 voxel takes up.
	auto getMap2VoxelHeight = [](uint16_t map2Voxel)
	{
		if ((map2Voxel & 0x80) == 0x80)
		{
			return 2;
		}
		else if ((map2Voxel & 0x8000) == 0x8000)
		{
			return 3;
		}
		else if ((map2Voxel & 0x8080) == 0x8080)
		{
			return 4;
		}
		else
		{
			return 1;
		}
	};

	// Lambda for obtaining the voxel data index for a MAP2 voxel.
	auto getMap2DataIndex = [this, &inf](uint16_t map2Voxel)
	{
		const auto map2Iter = std::find_if(
			this->map2DataMappings.begin(), this->map2DataMappings.end(),
			[map2Voxel](const std::pair<uint16_t, int> &pair)
		{
			return pair.first == map2Voxel;
		});

		if (map2Iter != this->map2DataMappings.end())
		{
			return map2Iter->second;
		}
		else
		{
			const int textureIndex = (map2Voxel & 0x007F) - 1;
			const int *menuID = nullptr;
			const int index = this->voxelGrid.addVoxelData(VoxelData::makeWall(
				textureIndex, textureIndex, textureIndex, menuID,
				VoxelData::WallData::Type::Solid));
			this->map2DataMappings.push_back(std::make_pair(map2Voxel, index));
			return index;
		}
	};

	// Write the voxel IDs into the voxel grid.
	for (int x = 0; x < gridWidth; x++)
	{
		for (int z = 0; z < gridDepth; z++)
		{
			const uint16_t map2Voxel = getMap2Voxel(x, z);

			if (map2Voxel != 0)
			{
				// Number of stories the MAP2 voxel occupies.
				const int height = getMap2VoxelHeight(map2Voxel);

				const int dataIndex = getMap2DataIndex(map2Voxel);

				for (int y = 2; y < (height + 2); y++)
				{
					this->setVoxel(x, y, z, dataIndex);
				}
			}
		}
	}
}

void LevelData::readCeiling(const INFFile &inf, int width, int depth)
{
	const INFFile::CeilingData &ceiling = inf.getCeiling();

	// Get the index of the ceiling texture name in the textures array.
	const int ceilingIndex = [&ceiling]()
	{
		// @todo: get ceiling from .INFs without *CEILING (like START.INF). Maybe
		// hardcoding index 1 is enough?
		return ceiling.textureIndex.value_or(1);
	}();

	// Define the ceiling voxel data.
	const int index = this->voxelGrid.addVoxelData(
		VoxelData::makeCeiling(ceilingIndex));

	// Set all the ceiling voxels.
	for (int x = 0; x < width; x++)
	{
		for (int z = 0; z < depth; z++)
		{
			this->setVoxel(x, 2, z, index);
		}
	}
}

void LevelData::readLocks(const std::vector<ArenaTypes::MIFLock> &locks, int width, int depth)
{
	for (const auto &lock : locks)
	{
		const Int2 lockPosition = VoxelGrid::getTransformedCoordinate(
			Int2(lock.x, lock.y), width, depth);
		this->locks.insert(std::make_pair(
			lockPosition, LevelData::Lock(lockPosition, lock.lockLevel)));
	}
}

void LevelData::setActive(const MiscAssets &miscAssets, TextureManager &textureManager,
	Renderer &renderer)
{
	// Clear renderer textures, distant sky, and entities.
	renderer.clearTextures();
	renderer.clearDistantSky();
	this->entityManager.clear();

	// Palette for voxels and flats, required in the renderer so it can conditionally transform
	// certain palette indices for transparency.
	COLFile col;
	col.init(PaletteFile::fromName(PaletteName::Default).c_str());
	const Palette &palette = col.getPalette();

	// Load .INF voxel textures into the renderer.
	const auto &voxelTextures = this->inf.getVoxelTextures();
	const int voxelTextureCount = static_cast<int>(voxelTextures.size());
	for (int i = 0; i < voxelTextureCount; i++)
	{
		DebugAssertIndex(voxelTextures, i);
		const auto &textureData = voxelTextures[i];

		const std::string textureName = String::toUppercase(textureData.filename);
		const std::string_view extension = StringView::getExtension(textureName);
		const bool isIMG = extension == "IMG";
		const bool isSET = extension == "SET";
		const bool noExtension = extension.size() == 0;

		if (isIMG)
		{
			IMGFile img;
			if (!img.init(textureName.c_str()))
			{
				DebugCrash("Couldn't init .IMG file \"" + textureName + "\".");
			}

			renderer.setVoxelTexture(i, img.getPixels(), palette);
		}
		else if (isSET)
		{
			SETFile set;
			if (!set.init(textureName.c_str()))
			{
				DebugCrash("Couldn't init .SET file \"" + textureName + "\".");
			}

			// Use the texture data's .SET index to obtain the correct surface.
			DebugAssert(textureData.setIndex.has_value());
			const uint8_t *srcPixels = set.getPixels(*textureData.setIndex);
			renderer.setVoxelTexture(i, srcPixels, palette);
		}
		else if (noExtension)
		{
			// Ignore texture names with no extension. They appear to be lore-related names
			// that were used at one point in Arena's development.
			static_cast<void>(textureData);
		}
		else
		{
			DebugCrash("Unrecognized voxel texture extension \"" + textureName + "\".");
		}
	}

	// Initialize entities from the flat defs list and write their textures to the renderer.
	const auto &exeData = miscAssets.getExeData();
	for (const auto &flatDef : this->flatsLists)
	{
		const int flatIndex = flatDef.getFlatIndex();
		const INFFile::FlatData &flatData = this->inf.getFlat(flatIndex);
		const EntityType entityType = GetEntityTypeFromFlat(flatIndex, this->inf);
		const std::optional<int> &optItemIndex = flatData.itemIndex;

		bool isFinalBoss;
		const bool isCreature = optItemIndex.has_value() &&
			IsCreatureIndex(*optItemIndex, &isFinalBoss);
		const bool isHumanEnemy = optItemIndex.has_value() && IsHumanEnemyIndex(*optItemIndex);

		// Must be at least one instance of the entity for the loop to try and
		// instantiate it and write textures to the renderer.
		DebugAssert(flatDef.getPositions().size() > 0);

		// Entity data index is currently the flat index (depends on .INF file).
		const int dataIndex = flatIndex;

		// Add a new entity data instance.
		// @todo: assign creature data here from .exe data if the flat is a creature.
		DebugAssert(this->entityManager.getEntityData(dataIndex) == nullptr);
		EntityData newEntityData;
		if (isCreature)
		{
			// Read from .exe data instead for creatures.
			const int itemIndex = *optItemIndex;
			const int creatureID = isFinalBoss ?
				GetFinalBossCreatureID() : GetCreatureIDFromItemIndex(itemIndex);
			const int creatureIndex = creatureID - 1;

			std::string displayName = [&exeData, isFinalBoss, creatureIndex]()
			{
				if (!isFinalBoss)
				{
					const auto &creatureNames = exeData.entities.creatureNames;
					DebugAssertIndex(creatureNames, creatureIndex);
					return creatureNames[creatureIndex];
				}
				else
				{
					// @todo: return final boss class name?
					return std::string("TODO");
				}
			}();

			const auto &creatureYOffsets = exeData.entities.creatureYOffsets;
			DebugAssertIndex(creatureYOffsets, creatureIndex);
			const int yOffset = creatureYOffsets[creatureIndex];

			const bool collider = true;
			const bool puddle = false;
			const bool largeScale = false;
			const bool dark = false;
			const bool transparent = false; // Apparently ghost properties aren't in .INF files.
			const bool ceiling = false;
			const bool mediumScale = false;
			newEntityData.init(std::move(displayName), flatIndex, yOffset, collider, puddle,
				largeScale, dark, transparent, ceiling, mediumScale);
		}
		else if (isHumanEnemy)
		{
			// Use character class name as the display name.
			const auto &charClassNames = exeData.charClasses.classNames;
			const int charClassIndex = GetCharacterClassIndexFromItemIndex(*optItemIndex);
			DebugAssertIndex(charClassNames, charClassIndex);
			const std::string_view charClassName = charClassNames[charClassIndex];

			newEntityData.init(std::string(charClassName), flatIndex, flatData.yOffset,
				flatData.collider, flatData.puddle, flatData.largeScale, flatData.dark,
				flatData.transparent, flatData.ceiling, flatData.mediumScale);
		}
		else
		{
			// No display name.
			std::string displayName;
			newEntityData.init(std::move(displayName), flatIndex, flatData.yOffset,
				flatData.collider, flatData.puddle, flatData.largeScale, flatData.dark,
				flatData.transparent, flatData.ceiling, flatData.mediumScale);
		}

		// Add entity animation data. Static entities have only idle animations (and maybe on/off
		// state for lampposts). Dynamic entities have several animation states and directions.
		auto &entityAnimData = newEntityData.getAnimationData();
		std::vector<EntityAnimationData::State> idleStates, lookStates, walkStates,
			attackStates, deathStates;

		// Cache for .CFA files referenced multiple times.
		AnimFileCache<CFAFile> cfaCache;

		if (entityType == EntityType::Static)
		{
			EntityAnimationData::State animState =
				MakeStaticEntityIdleAnimState(flatIndex, this->inf, exeData);

			// The entity can only be instantiated if there is at least one animation frame.
			const bool success = animState.getKeyframes().getCount() > 0;
			if (success)
			{
				idleStates.push_back(std::move(animState));
				entityAnimData.addStateList(std::vector<EntityAnimationData::State>(idleStates));
			}
			else
			{
				continue;
			}
		}
		else if (entityType == EntityType::Dynamic)
		{
			MakeDynamicEntityAnimStates(flatIndex, this->inf, miscAssets, cfaCache,
				&idleStates, &lookStates, &walkStates, &attackStates, &deathStates);

			// Must at least have an idle state.
			DebugAssert(idleStates.size() > 0);
			entityAnimData.addStateList(std::vector<EntityAnimationData::State>(idleStates));

			if (lookStates.size() > 0)
			{
				entityAnimData.addStateList(std::vector<EntityAnimationData::State>(lookStates));
			}

			if (walkStates.size() > 0)
			{
				entityAnimData.addStateList(std::vector<EntityAnimationData::State>(walkStates));
			}

			if (attackStates.size() > 0)
			{
				entityAnimData.addStateList(std::vector<EntityAnimationData::State>(attackStates));
			}

			if (deathStates.size() > 0)
			{
				entityAnimData.addStateList(std::vector<EntityAnimationData::State>(deathStates));
			}
		}
		else
		{
			DebugCrash("Unrecognized entity type \"" +
				std::to_string(static_cast<int>(entityType)) + "\".");
		}

		this->entityManager.addEntityData(std::move(newEntityData));

		// Initialize each instance of the flat def.
		for (const Int2 &position : flatDef.getPositions())
		{
			Entity *entity = [this, entityType]() -> Entity*
			{
				if (entityType == EntityType::Static)
				{
					StaticEntity *staticEntity = this->entityManager.makeStaticEntity();
					staticEntity->setDerivedType(StaticEntityType::Doodad);
					return staticEntity;
				}
				else if (entityType == EntityType::Dynamic)
				{
					DynamicEntity *dynamicEntity = this->entityManager.makeDynamicEntity();
					dynamicEntity->setDerivedType(DynamicEntityType::NPC);
					dynamicEntity->setDirection(Double2::UnitX);
					return dynamicEntity;
				}
				else
				{
					DebugCrash("Unrecognized entity type \"" +
						std::to_string(static_cast<int>(entityType)) + "\".");
					return nullptr;
				}
			}();

			entity->init(dataIndex);

			const Double2 positionXZ(
				static_cast<double>(position.x) + 0.50,
				static_cast<double>(position.y) + 0.50);
			entity->setPosition(positionXZ);
		}

		auto addTexturesFromState = [&renderer, &palette, flatIndex, &cfaCache](
			const EntityAnimationData::State &animState, int angleID)
		{
			// Check whether the animation direction ID is for a flipped animation.
			const bool isFlipped = IsAnimDirectionFlipped(angleID);

			// Write the flat def's textures to the renderer.
			const std::string &entityAnimName = animState.getTextureName();
			const std::string_view extension = StringView::getExtension(entityAnimName);
			const bool isCFA = extension == "CFA";
			const bool isDFA = extension == "DFA";
			const bool isIMG = extension == "IMG";
			const bool noExtension = extension.size() == 0;

			// Entities can be partially transparent. Some palette indices determine whether
			// there should be any "alpha blending" (in the original game, it implements alpha
			// using light level diminishing with 13 different levels in an .LGT file).
			auto addFlatTexture = [&renderer, &palette, isFlipped](const uint8_t *texels, int width,
				int height, int flatIndex, EntityAnimationData::StateType stateType, int angleID)
			{
				renderer.addFlatTexture(flatIndex, stateType, angleID, isFlipped, texels,
					width, height, palette);
			};

			if (isCFA)
			{
				const CFAFile *cfa;
				if (!cfaCache.tryGet(entityAnimName.c_str(), &cfa))
				{
					DebugCrash("Couldn't get cached .CFA file \"" + entityAnimName + "\".");
				}

				for (int i = 0; i < cfa->getImageCount(); i++)
				{
					addFlatTexture(cfa->getPixels(i), cfa->getWidth(), cfa->getHeight(),
						flatIndex, animState.getType(), angleID);
				}
			}
			else if (isDFA)
			{
				DFAFile dfa;
				if (!dfa.init(entityAnimName.c_str()))
				{
					DebugCrash("Couldn't init .DFA file \"" + entityAnimName + "\".");
				}

				for (int i = 0; i < dfa.getImageCount(); i++)
				{
					addFlatTexture(dfa.getPixels(i), dfa.getWidth(), dfa.getHeight(),
						flatIndex, animState.getType(), angleID);
				}
			}
			else if (isIMG)
			{
				IMGFile img;
				if (!img.init(entityAnimName.c_str()))
				{
					DebugCrash("Could not init .IMG file \"" + entityAnimName + "\".");
				}

				addFlatTexture(img.getPixels(), img.getWidth(), img.getHeight(),
					flatIndex, animState.getType(), angleID);
			}
			else if (noExtension)
			{
				// Ignore texture names with no extension. They appear to be lore-related names
				// that were used at one point in Arena's development.
				static_cast<void>(entityAnimName);
			}
			else
			{
				DebugCrash("Unrecognized flat texture name \"" + entityAnimName + "\".");
			}
		};

		auto addTexturesFromStateList = [&renderer, &palette, &addTexturesFromState](
			const std::vector<EntityAnimationData::State> &animStateList)
		{
			for (size_t i = 0; i < animStateList.size(); i++)
			{
				const auto &animState = animStateList[i];
				const int angleID = static_cast<int>(i + 1);
				addTexturesFromState(animState, angleID);
			}
		};

		// Add textures to the renderer for each of the entity's animation states.
		// @todo: don't add duplicate textures to the renderer (needs to be handled both here and
		// in the renderer implementation, because it seems to group textures by flat index only,
		// which could be wasteful).
		// - probably do it by having a hash set of <flatIndex, stateType> pairs and checking
		//   in the addTextureFromState lambda.
		addTexturesFromStateList(idleStates);
		addTexturesFromStateList(lookStates);
		addTexturesFromStateList(walkStates);
		addTexturesFromStateList(attackStates);
		addTexturesFromStateList(deathStates);
	}
}

void LevelData::tick(Game &game, double dt)
{
	this->entityManager.tick(game, dt);
}
