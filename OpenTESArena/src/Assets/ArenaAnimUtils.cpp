#include <array>

#include "ArenaAnimUtils.h"
#include "CFAFile.h"
#include "DFAFile.h"
#include "IMGFile.h"
#include "INFFile.h"
#include "MiscAssets.h"
#include "../Entities/EntityType.h"
#include "../Items/ArmorMaterialType.h"
#include "../World/ClimateType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

namespace ArenaAnimUtils
{
	bool isFinalBossIndex(int itemIndex)
	{
		return itemIndex == 73;
	}

	bool isCreatureIndex(int itemIndex, bool *outIsFinalBoss)
	{
		const bool isFinalBoss = isFinalBossIndex(itemIndex);
		*outIsFinalBoss = isFinalBoss;
		return (itemIndex >= 32 && itemIndex <= 54) || isFinalBoss;
	}

	bool isHumanEnemyIndex(int itemIndex)
	{
		return itemIndex >= 55 && itemIndex <= 72;
	}

	EntityType getEntityTypeFromFlat(int flatIndex, const INFFile &inf)
	{
		const auto &flatData = inf.getFlat(flatIndex);
		if (flatData.itemIndex.has_value())
		{
			const int itemIndex = flatData.itemIndex.value();

			// Creature *ITEM values are between 32 and 54. Other dynamic entities (like humans)
			// are higher.
			bool dummy;
			return (isCreatureIndex(itemIndex, &dummy) || isHumanEnemyIndex(itemIndex)) ?
				EntityType::Dynamic : EntityType::Static;
		}
		else
		{
			return EntityType::Static;
		}
	}

	int getCreatureIDFromItemIndex(int itemIndex)
	{
		return itemIndex - 31;
	}

	int getFinalBossCreatureID()
	{
		return 24;
	}

	int getCharacterClassIndexFromItemIndex(int itemIndex)
	{
		return itemIndex - 55;
	}

	bool isStreetLightFlatIndex(int flatIndex)
	{
		return flatIndex == 29;
	}

	void getBaseFlatDimensions(int width, int height, uint16_t scale, int *baseWidth, int *baseHeight)
	{
		*baseWidth = (width * scale) / 256;
		*baseHeight = (((height * scale) / 256) * 200) / 256;
	}

	bool isAnimDirectionFlipped(int animDirectionID)
	{
		DebugAssert(animDirectionID >= 1);
		DebugAssert(animDirectionID <= Directions);
		return animDirectionID >= FirstFlippedAnimID;
	}

	int getDynamicEntityCorrectedAnimID(int animDirectionID, bool *outIsFlipped)
	{
		// If the animation direction points to a flipped animation, the ID needs to be
		// corrected to point to the non-flipped version.
		if (isAnimDirectionFlipped(animDirectionID))
		{
			*outIsFlipped = true;
			return ((FirstFlippedAnimID - 1) * 2) - animDirectionID;
		}
		else
		{
			*outIsFlipped = false;
			return animDirectionID;
		}
	}

	EntityAnimationData::State makeAnimState(EntityAnimationData::StateType stateType,
		double secondsPerFrame, bool loop, bool flipped)
	{
		return EntityAnimationData::State(stateType, secondsPerFrame, loop, flipped);
	}

	bool trySetDynamicEntityFilenameDirection(std::string &filename, int animDirectionID)
	{
		DebugAssert(filename.size() > 0);
		DebugAssert(animDirectionID >= 1);
		DebugAssert(animDirectionID <= Directions);

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

	bool trySetCitizenFilenameDirection(std::string &filename, int animDirectionID)
	{
		// Same as dynamic entities (creatures and human enemies).
		return trySetDynamicEntityFilenameDirection(filename, animDirectionID);
	}

	void getHumanEnemyProperties(int itemIndex, const MiscAssets &miscAssets,
		int *outTypeIndex, bool *outIsMale)
	{
		const auto &exeData = miscAssets.getExeData();

		const int charClassIndex = getCharacterClassIndexFromItemIndex(itemIndex);
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

	bool trySetHumanFilenameGender(std::string &filename, bool isMale)
	{
		if (filename.size() == 0)
		{
			DebugLogError("Need human anim filename.");
			return false;
		}

		filename[0] = isMale ? '0' : '1';
		return true;
	}

	bool trySetHumanFilenameType(std::string &filename, const std::string_view &type)
	{
		if (filename.size() == 0)
		{
			DebugLogError("Need human anim filename.");
			return false;
		}

		if (type.size() != 3)
		{
			DebugLogError("Invalid human type string \"" + std::string(type) + "\".");
			return false;
		}

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

	EntityAnimationData::State makeStaticEntityIdleAnimState(int flatIndex,
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

		EntityAnimationData::State animState = makeAnimState(
			EntityAnimationData::StateType::Idle,
			StaticIdleSecondsPerFrame,
			StaticIdleLoop);

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

	void makeDynamicEntityAnimStates(int flatIndex, const INFFile &inf,
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
		const bool isCreature = isCreatureIndex(itemIndex, &isFinalBoss);
		const bool isHuman = isHumanEnemyIndex(itemIndex);

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
			getBaseFlatDimensions(width, height, creatureScale, &baseWidth, &baseHeight);
			*outWidth = static_cast<double>(baseWidth) / MIFFile::ARENA_UNITS;
			*outHeight = static_cast<double>(baseHeight) / MIFFile::ARENA_UNITS;
		};

		// Lambda for converting human dimensions to the in-engine values.
		auto makeHumanKeyframeDimensions = [](int width, int height, double *outWidth, double *outHeight)
		{
			const uint16_t humanScale = 256;
			int baseWidth, baseHeight;
			getBaseFlatDimensions(width, height, humanScale, &baseWidth, &baseHeight);
			*outWidth = static_cast<double>(baseWidth) / MIFFile::ARENA_UNITS;
			*outHeight = static_cast<double>(baseHeight) / MIFFile::ARENA_UNITS;
		};

		// Write animation states for idle, look, and walk for the given anim direction.
		auto tryWriteAnimStates = [&miscAssets, &cfaCache, outIdleStates, outLookStates, outWalkStates,
			&exeData, itemIndex, isFinalBoss, isCreature, isHuman, &makeCreatureKeyframeDimensions,
			&makeHumanKeyframeDimensions](int animDirectionID)
		{
			DebugAssert(animDirectionID >= 1);
			DebugAssert(animDirectionID <= Directions);

			bool animIsFlipped;
			const int correctedAnimDirID = getDynamicEntityCorrectedAnimID(animDirectionID, &animIsFlipped);

			// Determine which dynamic entity animation to load.
			if (isCreature)
			{
				const auto &creatureAnimFilenames = exeData.entities.creatureAnimationFilenames;
				const int creatureID = isFinalBoss ?
					getFinalBossCreatureID() : getCreatureIDFromItemIndex(itemIndex);
				const int creatureIndex = creatureID - 1;

				DebugAssertIndex(creatureAnimFilenames, creatureIndex);
				std::string creatureFilename = String::toUppercase(creatureAnimFilenames[creatureIndex]);
				if (!trySetDynamicEntityFilenameDirection(creatureFilename, correctedAnimDirID))
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
				EntityAnimationData::State idleState = makeAnimState(
					EntityAnimationData::StateType::Idle,
					CreatureIdleSecondsPerFrame,
					CreatureIdleLoop,
					animIsFlipped);
				EntityAnimationData::State lookState = makeAnimState(
					EntityAnimationData::StateType::Look,
					CreatureLookSecondsPerFrame,
					CreatureLookLoop,
					animIsFlipped);
				EntityAnimationData::State walkState = makeAnimState(
					EntityAnimationData::StateType::Walk,
					CreatureWalkSecondsPerFrame,
					CreatureWalkLoop,
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

				writeStateKeyframes(&idleState, CreatureIdleIndices);
				writeStateKeyframes(&lookState, CreatureLookIndices);
				writeStateKeyframes(&walkState, CreatureWalkIndices);

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
				getHumanEnemyProperties(itemIndex, miscAssets, &humanFilenameTypeIndex, &isMale);

				const int templateIndex = 0; // Idle/walk template index.
				const auto &humanFilenameTemplates = exeData.entities.humanFilenameTemplates;
				DebugAssertIndex(humanFilenameTemplates, templateIndex);
				std::string animName = humanFilenameTemplates[templateIndex];
				if (!trySetDynamicEntityFilenameDirection(animName, correctedAnimDirID))
				{
					DebugLogError("Couldn't set human filename direction \"" +
						animName + "\" (" + std::to_string(correctedAnimDirID) + ").");
					return false;
				}

				const auto &humanFilenameTypes = exeData.entities.humanFilenameTypes;
				DebugAssertIndex(humanFilenameTypes, humanFilenameTypeIndex);
				const std::string_view humanFilenameType = humanFilenameTypes[humanFilenameTypeIndex];
				if (!trySetHumanFilenameType(animName, humanFilenameType))
				{
					DebugLogError("Couldn't set human filename type \"" +
						animName + "\" (" + std::to_string(correctedAnimDirID) + ").");
					return false;
				}

				// Special case for plate sprites: female is replaced with male, since they would
				// apparently look the same in armor.
				const bool isPlate = humanFilenameTypeIndex == 0;

				if (!trySetHumanFilenameGender(animName, isMale || isPlate))
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
				EntityAnimationData::State idleState = makeAnimState(
					EntityAnimationData::StateType::Idle,
					HumanIdleSecondsPerFrame,
					HumanIdleLoop,
					animIsFlipped);
				EntityAnimationData::State walkState = makeAnimState(
					EntityAnimationData::StateType::Walk,
					HumanWalkSecondsPerFrame,
					HumanWalkLoop,
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

				writeStateKeyframes(&idleState, HumanIdleIndices);
				writeStateKeyframes(&walkState, HumanWalkIndices);

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
					getFinalBossCreatureID() : getCreatureIDFromItemIndex(itemIndex);
				const int creatureIndex = creatureID - 1;

				DebugAssertIndex(creatureAnimFilenames, creatureIndex);
				std::string creatureFilename = String::toUppercase(creatureAnimFilenames[creatureIndex]);
				if (!trySetDynamicEntityFilenameDirection(creatureFilename, animDirectionID))
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

				EntityAnimationData::State attackState = makeAnimState(
					EntityAnimationData::StateType::Attack,
					CreatureAttackSecondsPerFrame,
					CreatureAttackLoop,
					animIsFlipped);

				for (size_t i = 0; i < CreatureAttackIndices.size(); i++)
				{
					const int frameIndex = CreatureAttackIndices[i];

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
				getHumanEnemyProperties(itemIndex, miscAssets, &humanFilenameTypeIndex, &isMale);

				const int attackTemplateIndex = 1;
				const auto &humanFilenameTemplates = exeData.entities.humanFilenameTemplates;
				DebugAssertIndex(humanFilenameTemplates, attackTemplateIndex);
				std::string animName = humanFilenameTemplates[attackTemplateIndex];
				if (!trySetDynamicEntityFilenameDirection(animName, animDirectionID))
				{
					DebugLogError("Couldn't set human attack filename direction \"" +
						animName + "\" (" + std::to_string(animDirectionID) + ").");
					return false;
				}

				const auto &humanFilenameTypes = exeData.entities.humanFilenameTypes;
				DebugAssertIndex(humanFilenameTypes, humanFilenameTypeIndex);
				const std::string_view humanFilenameType = humanFilenameTypes[humanFilenameTypeIndex];
				if (!trySetHumanFilenameType(animName, humanFilenameType))
				{
					DebugLogError("Couldn't set human attack filename type \"" +
						animName + "\" (" + std::to_string(animDirectionID) + ").");
					return false;
				}

				// Special case for plate sprites: female is replaced with male, since they would
				// apparently look the same in armor.
				const bool isPlate = humanFilenameTypeIndex == 0;

				if (!trySetHumanFilenameGender(animName, isMale || isPlate))
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

				EntityAnimationData::State attackState = makeAnimState(
					EntityAnimationData::StateType::Attack,
					HumanAttackSecondsPerFrame,
					HumanAttackLoop,
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
					getFinalBossCreatureID() : getCreatureIDFromItemIndex(itemIndex);
				const int creatureIndex = creatureID - 1;

				DebugAssertIndex(creatureAnimFilenames, creatureIndex);
				std::string creatureFilename = String::toUppercase(creatureAnimFilenames[creatureIndex]);
				if (!trySetDynamicEntityFilenameDirection(creatureFilename, animDirectionID))
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

				EntityAnimationData::State deathState = makeAnimState(
					EntityAnimationData::StateType::Death,
					CreatureDeathSecondsPerFrame,
					CreatureDeathLoop,
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

				EntityAnimationData::State deathState = makeAnimState(
					EntityAnimationData::StateType::Death,
					HumanDeathSecondsPerFrame,
					HumanDeathLoop,
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

		for (int i = 1; i <= Directions; i++)
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

	void makeCitizenAnimStates(bool isMale, ClimateType climateType, const INFFile &inf,
		const MiscAssets &miscAssets, AnimFileCache<CFAFile> &cfaCache,
		std::vector<EntityAnimationData::State> *outIdleStates,
		std::vector<EntityAnimationData::State> *outWalkStates)
	{
		auto makeKeyframeDimension = [](int value)
		{
			return static_cast<double>(value) / MIFFile::ARENA_UNITS;
		};

		// Index into citizen animation filenames, depends on the climate and gender.
		const int citizenIndex = [isMale, climateType]()
		{
			if (isMale)
			{
				switch (climateType)
				{
				case ClimateType::Temperate:
					return 2;
				case ClimateType::Desert:
					return 0;
				case ClimateType::Mountain:
					return 1;
				default:
					DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(climateType)));
				}
			}
			else
			{
				switch (climateType)
				{
				case ClimateType::Temperate:
					return 0;
				case ClimateType::Desert:
					return 1;
				case ClimateType::Mountain:
					return 2;
				default:
					DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(climateType)));
				}
			}
		}();

		// Citizen animation filename list, depends on the gender.
		const auto &exeData = miscAssets.getExeData();
		const auto &citizenAnimFilenames = isMale ?
			exeData.entities.maleCitizenAnimationFilenames :
			exeData.entities.femaleCitizenAnimationFilenames;

		auto tryWriteAnimStates = [isMale, climateType, &inf, &miscAssets, &cfaCache,
			outIdleStates, outWalkStates, &makeKeyframeDimension, citizenIndex,
			&citizenAnimFilenames](int animDirectionID)
		{
			DebugAssert(animDirectionID >= 1);
			DebugAssert(animDirectionID <= Directions);

			bool animIsFlipped;
			const int correctedAnimDirID = getDynamicEntityCorrectedAnimID(animDirectionID, &animIsFlipped);

			DebugAssertIndex(citizenAnimFilenames, citizenIndex);
			std::string citizenFilename = String::toUppercase(citizenAnimFilenames[citizenIndex]);
			if (!trySetCitizenFilenameDirection(citizenFilename, correctedAnimDirID))
			{
				DebugLogError("Couldn't set citizen filename direction \"" +
					citizenFilename + "\" (" + std::to_string(correctedAnimDirID) + ").");
				return false;
			}

			// Load the .CFA of the citizen at the given direction.
			const CFAFile *cfa;
			if (!cfaCache.tryGet(citizenFilename.c_str(), &cfa))
			{
				DebugLogError("Couldn't get cached .CFA file \"" + citizenFilename + "\".");
				return false;
			}

			// Prepare the states to write out.
			EntityAnimationData::State idleState = makeAnimState(
				EntityAnimationData::StateType::Idle,
				CitizenIdleSecondsPerFrame,
				CitizenIdleLoop,
				animIsFlipped);
			EntityAnimationData::State walkState = makeAnimState(
				EntityAnimationData::StateType::Walk,
				CitizenWalkSecondsPerFrame,
				CitizenWalkLoop,
				animIsFlipped);

			// Lambda for writing keyframes to an anim state.
			auto writeStateKeyframes = [&makeKeyframeDimension, &cfa](
				EntityAnimationData::State *outState, const std::vector<int> &indices)
			{
				for (size_t i = 0; i < indices.size(); i++)
				{
					const int frameIndex = indices[i];

					const double width = makeKeyframeDimension(cfa->getWidth());
					const double height = makeKeyframeDimension(cfa->getHeight());
					const int textureID = frameIndex;

					EntityAnimationData::Keyframe keyframe(width, height, textureID);
					outState->addKeyframe(std::move(keyframe));
				}
			};

			writeStateKeyframes(&idleState, CitizenIdleIndices);
			writeStateKeyframes(&walkState, CitizenWalkIndices);

			// Write animation filename to each.
			idleState.setTextureName(std::string(citizenFilename));
			walkState.setTextureName(std::string(citizenFilename));

			// Write out the states to their respective state lists.
			outIdleStates->push_back(std::move(idleState));
			outWalkStates->push_back(std::move(walkState));
			return true;
		};

		for (int i = 1; i <= Directions; i++)
		{
			if (!tryWriteAnimStates(i))
			{
				DebugLogError("Couldn't make anim states for direction \"" + std::to_string(i) + "\".");
			}
		}
	}

	Palette transformCitizenClothing(uint16_t seed, const Palette &palette, const ExeData &exeData)
	{
		const std::array<uint8_t, 16> &colorBase = exeData.entities.citizenColorBase;

		uint16_t val = seed & 0x7FFF;
		Palette newPalette = palette;
		for (const uint8_t color : colorBase)
		{
			const bool flag = (val & 0x8000) != 0;
			val = Bytes::rol(val, 1);
			if (flag)
			{
				const uint8_t block = val & 0xF;
				const uint8_t dest = color;
				if ((dest == 128) && (block == 11))
				{
					// No green hair.
					continue;
				}

				DebugAssertIndex(colorBase, block);
				const uint8_t src = colorBase[block];

				for (int j = 0; j < 10; j++)
				{
					const int oldIndex = dest + j;
					const int newIndex = src + j;
					DebugAssertIndex(palette.get(), oldIndex);
					DebugAssertIndex(newPalette.get(), newIndex);
					newPalette.get()[newIndex] = palette.get()[oldIndex];
				}
			}
		}

		return newPalette;
	}

	Palette transformCitizenSkin(int raceIndex, const Palette &palette, const ExeData &exeData)
	{
		const std::array<uint8_t, 10> &skinColors = exeData.entities.citizenSkinColors;
		Palette newPalette = palette;

		// Run the palette transformation if the given race should have its colors transformed.
		const std::array<int, 9> RaceOffsets = { -1, 148, -1, 52, 192, -1, -1, 116, 148 };
		DebugAssertIndex(RaceOffsets, raceIndex);
		const int raceOffset = RaceOffsets[raceIndex];
		const bool hasTransformation = raceOffset != -1;
		if (hasTransformation)
		{
			for (int i = 0; i < 10; i++)
			{
				const int oldIndex = raceOffset + i;
				const int newIndex = skinColors[i];
				DebugAssertIndex(palette.get(), oldIndex);
				DebugAssertIndex(newPalette.get(), newIndex);
				newPalette.get()[newIndex] = palette.get()[oldIndex];
			}
		}

		return newPalette;
	}
}