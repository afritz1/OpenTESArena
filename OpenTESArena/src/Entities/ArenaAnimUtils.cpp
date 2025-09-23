#include <array>
#include <limits>
#include <numeric>

#include "ArenaAnimUtils.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/MIFUtils.h"
#include "../Assets/TextureManager.h"
#include "../Entities/EntityDefinition.h"
#include "../Stats/CharacterClassDefinition.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

// Private functions.
namespace ArenaAnimUtils
{
	// Animation scale helper values based on the original game.
	constexpr double MediumScale = INFFlat::MEDIUM_SCALE / 100.0;
	constexpr double LargeScale = INFFlat::LARGE_SCALE / 100.0;

	constexpr int HumanFilenameTypeIndexPlate = 0;

	// General-case keyframe dimension conversion from image space to world space which can
	// represent the entity's world space size.
	double MakeDefaultKeyframeDimension(int dim)
	{
		return static_cast<double>(dim) / MIFUtils::ARENA_UNITS;
	}

	// Converts an original static entity's dimension to vector space which can represent the
	// entity's world space size.
	double MakeStaticKeyframeDimension(int dim, double modifier)
	{
		return (static_cast<double>(dim) * modifier) / MIFUtils::ARENA_UNITS;
	}

	// Converts an original creature's dimensions to vector space which can represent the entity's
	// world space size.
	void MakeCreatureKeyframeDimensions(int creatureIndex, int width, int height, const ExeData &exeData,
		double *outWidth, double *outHeight)
	{
		// Get the original scale value of the creature.
		const uint16_t creatureScale = [&exeData, creatureIndex]()
		{
			const auto &creatureScales = exeData.entities.creatureScales;
			DebugAssertIndex(creatureScales, creatureIndex);
			const uint16_t scaleValue = creatureScales[creatureIndex];

			// Special case: 0 == 256.
			return (scaleValue == 0) ? 256 : scaleValue;
		}();

		int baseWidth, baseHeight;
		ArenaAnimUtils::getBaseFlatDimensions(width, height, creatureScale, &baseWidth, &baseHeight);
		*outWidth = static_cast<double>(baseWidth) / MIFUtils::ARENA_UNITS;
		*outHeight = static_cast<double>(baseHeight) / MIFUtils::ARENA_UNITS;
	}

	// Converts an original human's dimensions to vector space which can represent the entity's
	// world space size.
	void MakeHumanKeyframeDimensions(int width, int height, double *outWidth, double *outHeight)
	{
		constexpr uint16_t humanScale = 256;
		int baseWidth, baseHeight;
		ArenaAnimUtils::getBaseFlatDimensions(width, height, humanScale, &baseWidth, &baseHeight);
		*outWidth = static_cast<double>(baseWidth) / MIFUtils::ARENA_UNITS;
		*outHeight = static_cast<double>(baseHeight) / MIFUtils::ARENA_UNITS;
	}

	void MakeCitizenKeyframeDimensions(int width, int height, double *outWidth, double *outHeight)
	{
		MakeHumanKeyframeDimensions(width, height, outWidth, outHeight);
	}

	void MakeVfxKeyframeDimensions(int width, int height, double *outWidth, double *outHeight)
	{
		MakeHumanKeyframeDimensions(width, height, outWidth, outHeight);
	}

	int GetCitizenAnimationFilenameIndex(bool isMale, ArenaClimateType climateType)
	{
		if (isMale)
		{
			switch (climateType)
			{
			case ArenaClimateType::Temperate:
				return 2;
			case ArenaClimateType::Desert:
				return 1;
			case ArenaClimateType::Mountain:
				return 0;
			default:
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(climateType)));
			}
		}
		else
		{
			switch (climateType)
			{
			case ArenaClimateType::Temperate:
				return 0;
			case ArenaClimateType::Desert:
				return 1;
			case ArenaClimateType::Mountain:
				return 2;
			default:
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(climateType)));
			}
		}
	}

	bool tryAddStaticEntityAnimState(ArenaFlatIndex flatIndex, const char *stateName, double secondsPerFrame,
		bool isLooping, const INFFile &inf, TextureManager &textureManager, EntityAnimationDefinition *outAnimDef)
	{
		const INFFlat &flatData = inf.getFlat(flatIndex);
		const char *flatTextureName = [&inf, &flatData]()
		{
			const Span<const INFFlatTexture> flatTextures = inf.getFlatTextures();
			DebugAssertIndex(flatTextures, flatData.textureIndex);
			const INFFlatTexture &flatTextureData = flatTextures[flatData.textureIndex];
			return flatTextureData.filename.c_str();
		}();

		// Avoid files with no extension. They are lore-based names that are not used in-game.
		const std::string_view extension = StringView::getExtension(flatTextureName);
		if (extension.size() == 0)
		{
			return true;
		}

		const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(flatTextureName);
		if (!metadataID.has_value())
		{
			DebugLogWarning("Couldn't get static anim texture file metadata for \"" + std::string(flatTextureName) + "\".");
			return false;
		}

		const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
		const int keyframeCount = textureFileMetadata.getTextureCount();
		const double stateSeconds = static_cast<double>(keyframeCount) * secondsPerFrame;
		const int stateIndex = outAnimDef->addState(stateName, stateSeconds, isLooping);

		constexpr bool isMirrored = false; // Static anims never appear mirrored.
		const int keyframeListIndex = outAnimDef->addKeyframeList(stateIndex, isMirrored);

		const double dimensionModifier = ArenaAnimUtils::getDimensionModifier(flatData);
		for (int i = 0; i < textureFileMetadata.getTextureCount(); i++)
		{
			const double width = MakeStaticKeyframeDimension(textureFileMetadata.getWidth(i), dimensionModifier);
			const double height = MakeStaticKeyframeDimension(textureFileMetadata.getHeight(i), dimensionModifier);
			TextureAsset textureAsset(std::string(textureFileMetadata.getFilename()), i);
			outAnimDef->addKeyframe(keyframeListIndex, std::move(textureAsset), width, height);
		}

		return true;
	}

	// 'Basic' dynamic entity anim state being one of: Idle, Look, Walk.
	bool tryAddDynamicEntityCreatureBasicAnimState(int creatureID, const char *stateName, double secondsPerFrame,
		bool isLooping, Span<const int> animIndices, const ExeData &exeData, TextureManager &textureManager,
		EntityAnimationDefinition *outAnimDef)
	{
		DebugAssert(outAnimDef != nullptr);

		auto tryAddDirectionToState = [creatureID, &animIndices, &exeData, &textureManager, outAnimDef](int direction, int stateIndex)
		{
			DebugAssert(direction >= 1);
			DebugAssert(direction <= ArenaAnimUtils::Directions);

			bool isMirrored;
			const int correctedDirection = ArenaAnimUtils::getDynamicEntityCorrectedAnimDirID(direction, &isMirrored);

			const auto &creatureAnimFilenames = exeData.entities.creatureAnimationFilenames;
			const int creatureIndex = ArenaAnimUtils::getCreatureIndexFromID(creatureID);
			DebugAssertIndex(creatureAnimFilenames, creatureIndex);
			std::string creatureFilename = String::toUppercase(creatureAnimFilenames[creatureIndex]);

			// Revise the filename based on which direction is being initialized.
			if (!ArenaAnimUtils::trySetDynamicEntityFilenameDirection(creatureFilename, correctedDirection))
			{
				DebugLogWarning("Couldn't set creature filename direction \"" + creatureFilename + "\" (" + std::to_string(correctedDirection) + ").");
				return false;
			}

			const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(creatureFilename.c_str());
			if (!metadataID.has_value())
			{
				DebugLogWarning("Couldn't get creature anim texture file metadata for \"" + creatureFilename + "\".");
				return false;
			}

			const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
			const int keyframeListIndex = outAnimDef->addKeyframeList(stateIndex, isMirrored);
			for (const int frameIndex : animIndices)
			{
				// Certain creatures don't have anim frames for a look animation, so just use
				// frame 0 as a fallback.
				const int correctedFrameIndex = frameIndex < textureFileMetadata.getTextureCount() ? frameIndex : 0;

				double width, height;
				MakeCreatureKeyframeDimensions(creatureIndex, textureFileMetadata.getWidth(correctedFrameIndex),
					textureFileMetadata.getHeight(correctedFrameIndex), exeData, &width, &height);

				TextureAsset textureAsset(std::string(textureFileMetadata.getFilename()), correctedFrameIndex);
				outAnimDef->addKeyframe(keyframeListIndex, std::move(textureAsset), width, height);
			}

			return true;
		};

		// Add empty state that will have its duration calculated later.
		const int stateIndex = outAnimDef->addState(stateName, 0.0, isLooping);

		for (int i = 1; i <= Directions; i++)
		{
			if (!tryAddDirectionToState(i, stateIndex))
			{
				DebugLogWarning("Couldn't add creature anim keyframe list for creature ID \"" +
					std::to_string(creatureID) + "\" direction \"" + std::to_string(i) + "\".");
			}
		}

		// Get total seconds using the forward-facing keyframe list if it exists.
		EntityAnimationDefinitionState &animDefState = outAnimDef->states[stateIndex];
		double stateSeconds = 0.0;
		if (animDefState.keyframeListCount > 0)
		{
			const int firstKeyframeListIndex = animDefState.keyframeListsIndex;
			const EntityAnimationDefinitionKeyframeList &firstKeyframeList = outAnimDef->keyframeLists[firstKeyframeListIndex];
			stateSeconds = static_cast<double>(firstKeyframeList.keyframeCount) * secondsPerFrame;
		}
		else
		{
			DebugLogWarning("Missing keyframe list for creature ID \"" + std::to_string(creatureID) + "\" to determine total seconds from.");
		}

		animDefState.seconds = stateSeconds;
		return true;
	}

	// Idle or walk animation state for human enemies.
	bool tryAddDynamicEntityHumanBasicAnimState(int charClassIndex, bool isMale, const char *stateName, double secondsPerFrame,
		bool isLooping, Span<const int> animIndices, const CharacterClassLibrary &charClassLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, EntityAnimationDefinition *outAnimDef)
	{
		DebugAssert(outAnimDef != nullptr);

		const auto &exeData = binaryAssetLibrary.getExeData();
		int humanFilenameTypeIndex;
		ArenaAnimUtils::getHumanEnemyProperties(charClassIndex, charClassLibrary, exeData, &humanFilenameTypeIndex);

		auto tryAddDirectionToState = [isMale, &animIndices, &textureManager, humanFilenameTypeIndex, &exeData, outAnimDef](int direction, int stateIndex)
		{
			DebugAssert(direction >= 1);
			DebugAssert(direction <= ArenaAnimUtils::Directions);

			bool isMirrored;
			const int correctedDirection = ArenaAnimUtils::getDynamicEntityCorrectedAnimDirID(direction, &isMirrored);

			// Revise the filename based on which direction is being initialized.
			constexpr int templateIndex = 0; // Idle/walk template index.
			const auto &humanFilenameTemplates = exeData.entities.humanFilenameTemplates;
			DebugAssertIndex(humanFilenameTemplates, templateIndex);
			std::string animName = humanFilenameTemplates[templateIndex];
			if (!trySetDynamicEntityFilenameDirection(animName, correctedDirection))
			{
				DebugLogWarning("Couldn't set human filename direction \"" + animName + "\" (" + std::to_string(correctedDirection) + ").");
				return false;
			}

			const auto &humanFilenameTypes = exeData.entities.humanFilenameTypes;
			DebugAssertIndex(humanFilenameTypes, humanFilenameTypeIndex);
			const std::string_view humanFilenameType = humanFilenameTypes[humanFilenameTypeIndex];
			if (!trySetHumanFilenameType(animName, humanFilenameType))
			{
				DebugLogWarning("Couldn't set human filename type \"" + animName + "\" (" + std::to_string(correctedDirection) + ").");
				return false;
			}

			// Special case for plate sprites: female is replaced with male, since they apparently look the same in armor.
			const bool isPlate = humanFilenameTypeIndex == ArenaAnimUtils::HumanFilenameTypeIndexPlate;
			const bool appearsAsMale = isMale || isPlate;

			if (!trySetHumanFilenameGender(animName, appearsAsMale))
			{
				DebugLogWarning("Couldn't set human filename gender \"" + animName + "\" (" + std::to_string(correctedDirection) + ").");
				return false;
			}

			animName = String::toUppercase(animName);

			// Not all permutations of human filenames exist. If a series is missing, then probably
			// need to have special behavior.
			const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(animName.c_str());
			if (!metadataID.has_value())
			{
				DebugLogWarning("Couldn't get human anim texture file metadata for \"" + animName + "\".");
				return false;
			}

			const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
			const int keyframeListIndex = outAnimDef->addKeyframeList(stateIndex, isMirrored);
			for (const int frameIndex : animIndices)
			{
				double width, height;
				MakeHumanKeyframeDimensions(textureFileMetadata.getWidth(frameIndex), textureFileMetadata.getHeight(frameIndex), &width, &height);

				TextureAsset textureAsset(std::string(textureFileMetadata.getFilename()), frameIndex);
				outAnimDef->addKeyframe(keyframeListIndex, std::move(textureAsset), width, height);
			}

			return true;
		};

		// Add empty state that will have its duration calculated later.
		const int stateIndex = outAnimDef->addState(stateName, 0.0, isLooping);

		for (int i = 1; i <= Directions; i++)
		{
			if (!tryAddDirectionToState(i, stateIndex))
			{
				DebugLogWarning("Couldn't add human anim keyframe list for character class \"" +
					std::to_string(charClassIndex) + "\" direction \"" + std::to_string(i) + "\".");
			}
		}

		// Get total seconds using the forward-facing keyframe list if it exists.
		EntityAnimationDefinitionState &animDefState = outAnimDef->states[stateIndex];
		double stateSeconds = 0.0;
		if (animDefState.keyframeListCount > 0)
		{
			const int firstKeyframeListIndex = animDefState.keyframeListsIndex;
			const EntityAnimationDefinitionKeyframeList &firstKeyframeList = outAnimDef->keyframeLists[firstKeyframeListIndex];
			stateSeconds = static_cast<double>(firstKeyframeList.keyframeCount) * secondsPerFrame;
		}
		else
		{
			DebugLogWarning("Missing keyframe list for character class \"" + std::to_string(charClassIndex) + "\" to determine total seconds from.");
		}

		animDefState.seconds = stateSeconds;
		return true;
	}

	bool tryAddDynamicEntityCreatureAttackAnimState(int creatureID, const ExeData &exeData, TextureManager &textureManager,
		EntityAnimationDefinition *outAnimDef)
	{
		// Attack state is only in the first .CFA file and is never mirrored because it only faces forward.
		constexpr int direction = 1;
		constexpr bool isMirrored = false;

		const auto &creatureAnimFilenames = exeData.entities.creatureAnimationFilenames;
		const int creatureIndex = ArenaAnimUtils::getCreatureIndexFromID(creatureID);

		DebugAssertIndex(creatureAnimFilenames, creatureIndex);
		std::string creatureFilename = String::toUppercase(creatureAnimFilenames[creatureIndex]);
		if (!trySetDynamicEntityFilenameDirection(creatureFilename, direction))
		{
			DebugLogError("Couldn't set creature filename direction \"" +
				creatureFilename + "\" (" + std::to_string(direction) + ").");
			return false;
		}

		const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(creatureFilename.c_str());
		if (!metadataID.has_value())
		{
			DebugLogWarning("Couldn't get creature attack texture file metadata for \"" + creatureFilename + "\".");
			return false;
		}

		const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);

		// Add empty state that will have its duration calculated later.
		const int stateIndex = outAnimDef->addState(EntityAnimationUtils::STATE_ATTACK.c_str(), 0.0, CreatureAttackLoop);
		const int keyframeListIndex = outAnimDef->addKeyframeList(stateIndex, isMirrored);

		for (const int frameIndex : CreatureAttackIndices)
		{
			double width, height;
			MakeCreatureKeyframeDimensions(creatureIndex, textureFileMetadata.getWidth(frameIndex),
				textureFileMetadata.getHeight(frameIndex), exeData, &width, &height);

			TextureAsset textureAsset(std::string(textureFileMetadata.getFilename()), frameIndex);
			outAnimDef->addKeyframe(keyframeListIndex, std::move(textureAsset), width, height);
		}

		EntityAnimationDefinitionState &animDefState = outAnimDef->states[stateIndex];
		const int firstKeyframeListIndex = animDefState.keyframeListsIndex;
		const EntityAnimationDefinitionKeyframeList &firstKeyframeList = outAnimDef->keyframeLists[firstKeyframeListIndex];
		animDefState.seconds = static_cast<double>(firstKeyframeList.keyframeCount) * CreatureAttackSecondsPerFrame;
		return true;
	}

	bool tryAddDynamicEntityHumanAttackAnimState(int charClassIndex, bool isMale, const CharacterClassLibrary &charClassLibrary,
		const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, EntityAnimationDefinition *outAnimDef)
	{
		// Attack state is only in the first .CFA file and is never mirrored because it only faces forward.
		constexpr int direction = 1;
		constexpr bool isMirrored = false;

		const auto &exeData = binaryAssetLibrary.getExeData();
		int humanFilenameTypeIndex;
		ArenaAnimUtils::getHumanEnemyProperties(charClassIndex, charClassLibrary, exeData, &humanFilenameTypeIndex);

		constexpr int attackTemplateIndex = 1;
		const auto &humanFilenameTemplates = exeData.entities.humanFilenameTemplates;
		DebugAssertIndex(humanFilenameTemplates, attackTemplateIndex);
		std::string animName = humanFilenameTemplates[attackTemplateIndex];
		if (!ArenaAnimUtils::trySetDynamicEntityFilenameDirection(animName, direction))
		{
			DebugLogError("Couldn't set human attack filename direction \"" + animName + "\" (" + std::to_string(direction) + ").");
			return false;
		}

		const auto &humanFilenameTypes = exeData.entities.humanFilenameTypes;
		DebugAssertIndex(humanFilenameTypes, humanFilenameTypeIndex);
		const std::string_view humanFilenameType = humanFilenameTypes[humanFilenameTypeIndex];
		if (!ArenaAnimUtils::trySetHumanFilenameType(animName, humanFilenameType))
		{
			DebugLogError("Couldn't set human attack filename type \"" + animName + "\" (" + std::to_string(direction) + ").");
			return false;
		}

		// Special case for plate sprites: female is replaced with male, since they apparently look the same in armor.
		const bool isPlate = humanFilenameTypeIndex == ArenaAnimUtils::HumanFilenameTypeIndexPlate;
		const bool appearsAsMale = isMale || isPlate;

		if (!trySetHumanFilenameGender(animName, appearsAsMale))
		{
			DebugLogError("Couldn't set human attack filename gender \"" + animName + "\" (" + std::to_string(direction) + ").");
			return false;
		}

		animName = String::toUppercase(animName);
		const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(animName.c_str());
		if (!metadataID.has_value())
		{
			DebugLogWarning("Couldn't get human attack texture file metadata for \"" + animName + "\".");
			return false;
		}

		const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);

		// Add empty state that will have its duration calculated later.
		const int stateIndex = outAnimDef->addState(EntityAnimationUtils::STATE_ATTACK.c_str(), 0.0, HumanAttackLoop);
		const int keyframeListIndex = outAnimDef->addKeyframeList(stateIndex, isMirrored);

		// No need for extra anim indices list, just use sequential image IDs.
		for (int i = 0; i < textureFileMetadata.getTextureCount(); i++)
		{
			double width, height;
			MakeHumanKeyframeDimensions(textureFileMetadata.getWidth(i), textureFileMetadata.getHeight(i), &width, &height);

			TextureAsset textureAsset(std::string(textureFileMetadata.getFilename()), i);
			outAnimDef->addKeyframe(keyframeListIndex, std::move(textureAsset), width, height);
		}

		EntityAnimationDefinitionState &animDefState = outAnimDef->states[stateIndex];
		const int firstKeyframeListIndex = animDefState.keyframeListsIndex;
		const EntityAnimationDefinitionKeyframeList &firstKeyframeList = outAnimDef->keyframeLists[firstKeyframeListIndex];
		animDefState.seconds = static_cast<double>(firstKeyframeList.keyframeCount) * HumanAttackSecondsPerFrame;
		return true;
	}

	bool tryAddDynamicEntityCreatureDeathAnimState(int creatureID, const ExeData &exeData, TextureManager &textureManager,
		EntityAnimationDefinition *outAnimDef)
	{
		// Death state is only in the last .CFA file.
		constexpr int direction = 6;
		constexpr bool isMirrored = false;

		const auto &creatureAnimFilenames = exeData.entities.creatureAnimationFilenames;
		const int creatureIndex = ArenaAnimUtils::getCreatureIndexFromID(creatureID);

		DebugAssertIndex(creatureAnimFilenames, creatureIndex);
		std::string creatureFilename = String::toUppercase(creatureAnimFilenames[creatureIndex]);
		if (!ArenaAnimUtils::trySetDynamicEntityFilenameDirection(creatureFilename, direction))
		{
			DebugLogError("Couldn't set creature filename direction \"" + creatureFilename + "\" (" + std::to_string(direction) + ").");
			return false;
		}

		const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(creatureFilename.c_str());
		if (!metadataID.has_value())
		{
			DebugLogWarning("Couldn't get creature death texture file metadata for \"" + creatureFilename + "\".");
			return false;
		}

		const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);

		// Add empty state that will have its duration calculated later.
		const int stateIndex = outAnimDef->addState(EntityAnimationUtils::STATE_DEATH.c_str(), 0.0, CreatureDeathLoop);
		const int keyframeListIndex = outAnimDef->addKeyframeList(stateIndex, isMirrored);

		// No need for extra anim indices list, just use sequential image IDs.
		for (int i = 0; i < textureFileMetadata.getTextureCount(); i++)
		{
			double width, height;
			ArenaAnimUtils::MakeCreatureKeyframeDimensions(creatureIndex, textureFileMetadata.getWidth(i),
				textureFileMetadata.getHeight(i), exeData, &width, &height);

			TextureAsset textureAsset(std::string(textureFileMetadata.getFilename()), i);
			outAnimDef->addKeyframe(keyframeListIndex, std::move(textureAsset), width, height);
		}

		EntityAnimationDefinitionState &animDefState = outAnimDef->states[stateIndex];
		const int firstKeyframeListIndex = animDefState.keyframeListsIndex;
		const EntityAnimationDefinitionKeyframeList &firstKeyframeList = outAnimDef->keyframeLists[firstKeyframeListIndex];
		animDefState.seconds = static_cast<double>(firstKeyframeList.keyframeCount) * CreatureDeathSecondsPerFrame;
		return true;
	}

	bool tryAddDynamicEntityHumanDeathAnimState(TextureManager &textureManager, EntityAnimationDefinition *outAnimDef)
	{
		constexpr bool isMirrored = false;

		// Humans use a single dead body image.
		const std::string &animName = ArenaAnimUtils::HumanDeathFilename;
		const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(animName.c_str());
		if (!metadataID.has_value())
		{
			DebugLogWarning("Couldn't get human death texture file metadata for \"" + animName + "\".");
			return false;
		}

		const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);

		// Add empty state that will have its duration calculated later.
		const int stateIndex = outAnimDef->addState(EntityAnimationUtils::STATE_DEATH.c_str(), 0.0, HumanDeathLoop);
		const int keyframeListIndex = outAnimDef->addKeyframeList(stateIndex, isMirrored);

		const double width = MakeDefaultKeyframeDimension(textureFileMetadata.getWidth(0));
		const double height = MakeDefaultKeyframeDimension(textureFileMetadata.getHeight(0));
		TextureAsset textureAsset(std::string(textureFileMetadata.getFilename()));
		outAnimDef->addKeyframe(keyframeListIndex, std::move(textureAsset), width, height);

		EntityAnimationDefinitionState &animDefState = outAnimDef->states[stateIndex];
		const int firstKeyframeListIndex = animDefState.keyframeListsIndex;
		const EntityAnimationDefinitionKeyframeList &firstKeyframeList = outAnimDef->keyframeLists[firstKeyframeListIndex];
		animDefState.seconds = static_cast<double>(firstKeyframeList.keyframeCount) * HumanDeathSecondsPerFrame;
		return true;
	}

	// Citizens have idle and walk animation states.
	bool tryAddDynamicEntityCitizenBasicAnimState(const char *stateName, double secondsPerFrame, bool isLooping,
		int citizenIndex, bool isMale, Span<const int> animIndices, const ExeData &exeData,
		TextureManager &textureManager, EntityAnimationDefinition *outAnimDef)
	{
		// Animation filename list depends on the gender.
		const auto &citizenAnimFilenames = isMale ? exeData.entities.maleCitizenAnimationFilenames : exeData.entities.femaleCitizenAnimationFilenames;

		auto tryAddDirectionToState = [citizenIndex, isMale, &animIndices, &exeData, &textureManager,
			outAnimDef, &citizenAnimFilenames](int direction, int stateIndex)
		{
			DebugAssert(direction >= 1);
			DebugAssert(direction <= ArenaAnimUtils::Directions);

			bool isMirrored;
			const int correctedDirection = ArenaAnimUtils::getDynamicEntityCorrectedAnimDirID(direction, &isMirrored);

			DebugAssertIndex(citizenAnimFilenames, citizenIndex);
			std::string citizenFilename = String::toUppercase(citizenAnimFilenames[citizenIndex]);
			if (!trySetCitizenFilenameDirection(citizenFilename, correctedDirection))
			{
				DebugLogError("Couldn't set citizen filename direction \"" + citizenFilename + "\" (" + std::to_string(correctedDirection) + ").");
				return false;
			}

			const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(citizenFilename.c_str());
			if (!metadataID.has_value())
			{
				DebugLogWarning("Couldn't get citizen anim texture file metadata for \"" + citizenFilename + "\".");
				return false;
			}

			const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
			const int keyframeListIndex = outAnimDef->addKeyframeList(stateIndex, isMirrored);
			for (const int frameIndex : animIndices)
			{
				// Citizens only have forward-facing idle animations, so use frame 0 for other facings.
				const int correctedFrameIndex = frameIndex < textureFileMetadata.getTextureCount() ? frameIndex : 0;

				double width, height;
				MakeCitizenKeyframeDimensions(textureFileMetadata.getWidth(correctedFrameIndex),
					textureFileMetadata.getHeight(correctedFrameIndex), &width, &height);

				TextureAsset textureAsset(std::string(textureFileMetadata.getFilename()), correctedFrameIndex);
				outAnimDef->addKeyframe(keyframeListIndex, std::move(textureAsset), width, height);
			}

			return true;
		};

		// Add empty state that will have its duration calculated later.
		const int stateIndex = outAnimDef->addState(stateName, 0.0, isLooping);

		for (int i = 1; i <= ArenaAnimUtils::Directions; i++)
		{
			if (!tryAddDirectionToState(i, stateIndex))
			{
				DebugLogWarning("Couldn't make citizen anim states for direction \"" + std::to_string(i) + "\".");
			}
		}

		// Get total seconds using the forward-facing keyframe list if it exists.
		EntityAnimationDefinitionState &animDefState = outAnimDef->states[stateIndex];
		double stateSeconds = 0.0;
		if (animDefState.keyframeListCount > 0)
		{
			const int firstKeyframeListIndex = animDefState.keyframeListsIndex;
			const EntityAnimationDefinitionKeyframeList &firstKeyframeList = outAnimDef->keyframeLists[firstKeyframeListIndex];
			stateSeconds = static_cast<double>(firstKeyframeList.keyframeCount) * secondsPerFrame;
		}
		else
		{
			DebugLogWarning("Missing keyframe list for citizen ID \"" + std::to_string(citizenIndex) + "\" to determine total seconds from.");
		}

		animDefState.seconds = stateSeconds;
		return true;
	}
}

bool ArenaAnimUtils::isFinalBossIndex(ArenaItemIndex itemIndex)
{
	return itemIndex == 73;
}

bool ArenaAnimUtils::isCreatureIndex(ArenaItemIndex itemIndex, bool *outIsFinalBoss)
{
	const bool isFinalBoss = ArenaAnimUtils::isFinalBossIndex(itemIndex);
	*outIsFinalBoss = isFinalBoss;
	return (itemIndex >= 32 && itemIndex <= 54) || isFinalBoss;
}

bool ArenaAnimUtils::isHumanEnemyIndex(ArenaItemIndex itemIndex)
{
	return itemIndex >= 55 && itemIndex <= 72;
}

bool ArenaAnimUtils::isNpcShopkeeper(ArenaItemIndex itemIndex)
{
	return itemIndex == 15;
}

bool ArenaAnimUtils::isNpcBeggar(ArenaItemIndex itemIndex)
{
	return itemIndex == 16;
}

bool ArenaAnimUtils::isNpcFirebreather(ArenaItemIndex itemIndex)
{
	return itemIndex == 17;
}

bool ArenaAnimUtils::isNpcProstitute(ArenaItemIndex itemIndex)
{
	return (itemIndex >= 18) && (itemIndex <= 20);
}

bool ArenaAnimUtils::isNpcJester(ArenaItemIndex itemIndex)
{
	return itemIndex == 21;
}

bool ArenaAnimUtils::isNpcStreetVendor(ArenaItemIndex itemIndex)
{
	return (itemIndex == 22) || (itemIndex == 23);
}

bool ArenaAnimUtils::isNpcMusician(ArenaItemIndex itemIndex)
{
	return (itemIndex == 24) || (itemIndex == 25);
}

bool ArenaAnimUtils::isNpcPriest(ArenaItemIndex itemIndex)
{
	return (itemIndex == 26) || (itemIndex == 27);
}

bool ArenaAnimUtils::isNpcThief(ArenaItemIndex itemIndex)
{
	return itemIndex == 28;
}

bool ArenaAnimUtils::isNpcSnakeCharmer(ArenaItemIndex itemIndex)
{
	return itemIndex == 29;
}

bool ArenaAnimUtils::isNpcStreetVendorAlchemist(ArenaItemIndex itemIndex)
{
	return itemIndex == 30;
}

bool ArenaAnimUtils::isNpcWizard(ArenaItemIndex itemIndex)
{
	return itemIndex == 31;
}

bool ArenaAnimUtils::isNpcTavernPatron(ArenaItemIndex itemIndex)
{
	const bool isTavernPatronMan = (itemIndex >= 83) && (itemIndex <= 86);
	const bool isTavernPatronWoman = (itemIndex >= 87) && (itemIndex <= 90);
	return isTavernPatronMan || isTavernPatronWoman;
}

std::optional<StaticNpcPersonalityType> ArenaAnimUtils::tryGetStaticNpcPersonalityType(ArenaItemIndex itemIndex)
{
	if (ArenaAnimUtils::isNpcShopkeeper(itemIndex))
	{
		return StaticNpcPersonalityType::Shopkeeper;
	}
	else if (ArenaAnimUtils::isNpcBeggar(itemIndex))
	{
		return StaticNpcPersonalityType::Beggar;
	}
	else if (ArenaAnimUtils::isNpcFirebreather(itemIndex))
	{
		return StaticNpcPersonalityType::Firebreather;
	}
	else if (ArenaAnimUtils::isNpcProstitute(itemIndex))
	{
		return StaticNpcPersonalityType::Prostitute;
	}
	else if (ArenaAnimUtils::isNpcJester(itemIndex))
	{
		return StaticNpcPersonalityType::Jester;
	}
	else if (ArenaAnimUtils::isNpcStreetVendor(itemIndex))
	{
		return StaticNpcPersonalityType::StreetVendor;
	}
	else if (ArenaAnimUtils::isNpcMusician(itemIndex))
	{
		return StaticNpcPersonalityType::Musician;
	}
	else if (ArenaAnimUtils::isNpcPriest(itemIndex))
	{
		return StaticNpcPersonalityType::Priest;
	}
	else if (ArenaAnimUtils::isNpcThief(itemIndex))
	{
		return StaticNpcPersonalityType::Thief;
	}
	else if (ArenaAnimUtils::isNpcSnakeCharmer(itemIndex))
	{
		return StaticNpcPersonalityType::SnakeCharmer;
	}
	else if (ArenaAnimUtils::isNpcStreetVendorAlchemist(itemIndex))
	{
		return StaticNpcPersonalityType::StreetVendorAlchemist;
	}
	else if (ArenaAnimUtils::isNpcWizard(itemIndex))
	{
		return StaticNpcPersonalityType::Wizard;
	}
	else if (ArenaAnimUtils::isNpcTavernPatron(itemIndex))
	{
		return StaticNpcPersonalityType::TavernPatron;
	}
	else
	{
		return std::nullopt;
	}
}

bool ArenaAnimUtils::isLockedHolderContainerIndex(ArenaItemIndex itemIndex)
{
	return itemIndex == ArenaAnimUtils::LockedChestItemIndex;
}

bool ArenaAnimUtils::isUnlockedHolderContainerIndex(ArenaItemIndex itemIndex)
{
	return itemIndex == ArenaAnimUtils::UnlockedChestItemIndex;
}

bool ArenaAnimUtils::isLockableContainerFlatIndex(ArenaFlatIndex flatIndex, const INFFile &inf)
{
	const INFFlat &flat = inf.getFlat(flatIndex);
	const std::optional<int> &itemIndex = flat.itemIndex;
	if (!itemIndex.has_value())
	{
		return false;
	}

	return ArenaAnimUtils::isLockedHolderContainerIndex(*itemIndex) || ArenaAnimUtils::isUnlockedHolderContainerIndex(*itemIndex);
}

bool ArenaAnimUtils::isTreasurePileContainerIndex(ArenaItemIndex itemIndex)
{
	return itemIndex >= 2 && itemIndex <= 6;
}

bool ArenaAnimUtils::isContainerIndex(ArenaItemIndex itemIndex)
{
	const bool isHolder = ArenaAnimUtils::isLockedHolderContainerIndex(itemIndex) || ArenaAnimUtils::isUnlockedHolderContainerIndex(itemIndex);
	return isHolder || ArenaAnimUtils::isTreasurePileContainerIndex(itemIndex);
}

bool ArenaAnimUtils::isDynamicEntity(ArenaFlatIndex flatIndex, const INFFile &inf)
{
	const auto &flatData = inf.getFlat(flatIndex);
	const std::optional<ArenaItemIndex> &optItemIndex = flatData.itemIndex;
	if (!optItemIndex.has_value())
	{
		return false;
	}

	const ArenaItemIndex itemIndex = optItemIndex.value();

	// Creature *ITEM values are between 32 and 54. Other dynamic entities (like humans)
	// are higher.
	bool dummy;
	return isCreatureIndex(itemIndex, &dummy) || isHumanEnemyIndex(itemIndex);
}

bool ArenaAnimUtils::isGhost(int creatureIndex)
{
	return (creatureIndex == 11) || (creatureIndex == 14);
}

int ArenaAnimUtils::getCreatureIDFromItemIndex(ArenaItemIndex itemIndex)
{
	return ArenaAnimUtils::isFinalBossIndex(itemIndex) ? ArenaAnimUtils::FinalBossCreatureID : (itemIndex - 31);
}

int ArenaAnimUtils::getCreatureIndexFromID(int creatureID)
{
	return creatureID - 1;
}

int ArenaAnimUtils::getCharacterClassIndexFromItemIndex(ArenaItemIndex itemIndex)
{
	return itemIndex - 55;
}

bool ArenaAnimUtils::isStreetLightFlatIndex(ArenaFlatIndex flatIndex, MapType mapType)
{
	// Wilderness and interiors do not manage streetlights. There are animating streetlights
	// in the wilderness (sharing the exact same texture as city ones) but their activated state
	// never changes in the original game.
	if (mapType != MapType::City)
	{
		return false;
	}

	return (flatIndex == ArenaAnimUtils::StreetLightActiveIndex) || (flatIndex == ArenaAnimUtils::StreetLightInactiveIndex);
}

bool ArenaAnimUtils::isRulerFlatIndex(ArenaFlatIndex flatIndex, ArenaInteriorType interiorType)
{
	if (interiorType != ArenaInteriorType::Palace)
	{
		return false;
	}

	return (flatIndex == ArenaAnimUtils::RulerKingIndex) || (flatIndex == ArenaAnimUtils::RulerQueenIndex);
}

void ArenaAnimUtils::getBaseFlatDimensions(int width, int height, uint16_t scale, int *baseWidth, int *baseHeight)
{
	*baseWidth = (width * scale) / 256;
	*baseHeight = (((height * scale) / 256) * 200) / 256;
}

double ArenaAnimUtils::getDimensionModifier(const INFFlat &flatData)
{
	if (flatData.largeScale)
	{
		return ArenaAnimUtils::LargeScale;
	}
	else if (flatData.mediumScale)
	{
		return ArenaAnimUtils::MediumScale;
	}
	else
	{
		return 1.0;
	}
}

bool ArenaAnimUtils::isAnimDirectionMirrored(int animDirectionID)
{
	DebugAssert(animDirectionID >= 1);
	DebugAssert(animDirectionID <= Directions);
	return animDirectionID >= FirstMirroredAnimID;
}

int ArenaAnimUtils::getDynamicEntityCorrectedAnimDirID(int animDirectionID, bool *outIsMirrored)
{
	// If the animation direction points to a mirrored animation, the ID needs to be
	// corrected to point to the non-mirrored version.
	if (isAnimDirectionMirrored(animDirectionID))
	{
		*outIsMirrored = true;
		return ((FirstMirroredAnimID - 1) * 2) - animDirectionID;
	}
	else
	{
		*outIsMirrored = false;
		return animDirectionID;
	}
}

bool ArenaAnimUtils::trySetDynamicEntityFilenameDirection(std::string &filename, int animDirectionID)
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

bool ArenaAnimUtils::trySetCitizenFilenameDirection(std::string &filename, int animDirectionID)
{
	// Same as dynamic entities (creatures and human enemies).
	return trySetDynamicEntityFilenameDirection(filename, animDirectionID);
}

void ArenaAnimUtils::getHumanEnemyProperties(int charClassIndex, const CharacterClassLibrary &charClassLibrary, const ExeData &exeData, int *outTypeIndex)
{
	int charClassDefIndex;
	const bool success = charClassLibrary.findDefinitionIndexIf(
		[charClassIndex](const CharacterClassDefinition &def)
	{
		return def.originalClassIndex == charClassIndex;
	}, &charClassDefIndex);

	if (!success)
	{
		DebugLogWarningFormat("Couldn't get character class definition for index \"%d\".", charClassIndex);
		return;
	}

	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefIndex);

	// Properties about the character class.
	*outTypeIndex = [&exeData, &charClassDef]()
	{
		// Find which armors the class can wear.
		bool hasPlate = false;
		bool hasChain = false;
		bool hasLeather = false;

		for (int i = 0; i < charClassDef.getAllowedArmorCount(); i++)
		{
			const int allowedArmor = charClassDef.getAllowedArmor(i);
			hasPlate |= allowedArmor == 2;
			hasChain |= allowedArmor == 1;
			hasLeather |= allowedArmor == 0;
		}

		const int originalClassIndex = charClassDef.originalClassIndex;

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
		else if (charClassDef.castsMagic)
		{
			// Spellcaster.
			return 4;
		}
		else if (originalClassIndex == 12)
		{
			// Monk.
			return 5;
		}
		else if (originalClassIndex == 15)
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
}

bool ArenaAnimUtils::trySetHumanFilenameGender(std::string &filename, bool isMale)
{
	if (filename.size() == 0)
	{
		DebugLogError("Need human anim filename.");
		return false;
	}

	filename[0] = isMale ? '0' : '1';
	return true;
}

bool ArenaAnimUtils::trySetHumanFilenameType(std::string &filename, const std::string_view type)
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

bool ArenaAnimUtils::tryMakeStaticEntityAnims(ArenaFlatIndex flatIndex, MapType mapType,
	const std::optional<ArenaInteriorType> &interiorType, const std::optional<bool> &rulerIsMale,
	const INFFile &inf, TextureManager &textureManager, EntityAnimationDefinition *outAnimDef)
{
	DebugAssert(outAnimDef != nullptr);

	// Generate animation states based on what the entity needs. The animations to load depend on
	// the flat index. The wilderness does not have any streetlights (there is no ID for them).
	const bool isRuler = interiorType.has_value() && ArenaAnimUtils::isRulerFlatIndex(flatIndex, *interiorType);
	const bool isStreetlight = ArenaAnimUtils::isStreetLightFlatIndex(flatIndex, mapType);
	const bool isLockableChest = ArenaAnimUtils::isLockableContainerFlatIndex(flatIndex, inf);
	if (isRuler)
	{
		DebugAssert(rulerIsMale.has_value());
		outAnimDef->init(EntityAnimationUtils::STATE_IDLE.c_str());

		const ArenaFlatIndex rulerFlatIndex = *rulerIsMale ? ArenaAnimUtils::RulerKingIndex : ArenaAnimUtils::RulerQueenIndex;
		if (!ArenaAnimUtils::tryAddStaticEntityAnimState(rulerFlatIndex, EntityAnimationUtils::STATE_IDLE.c_str(),
			StaticIdleSecondsPerFrame, StaticIdleLoop, inf, textureManager, outAnimDef))
		{
			DebugLogWarningFormat("Couldn't add ruler idle anim state for ID \"%d\".", flatIndex);
			return false;
		}
	}
	else if (isStreetlight)
	{
		outAnimDef->init(EntityAnimationUtils::STATE_IDLE.c_str());

		const ArenaFlatIndex idleFlatIndex = ArenaAnimUtils::StreetLightInactiveIndex;
		if (!ArenaAnimUtils::tryAddStaticEntityAnimState(idleFlatIndex, EntityAnimationUtils::STATE_IDLE.c_str(),
			StaticIdleSecondsPerFrame, StaticIdleLoop, inf, textureManager, outAnimDef))
		{
			DebugLogWarningFormat("Couldn't add streetlight idle anim state for ID \"%d\".", idleFlatIndex);
			return false;
		}

		const ArenaFlatIndex activeFlatIndex = ArenaAnimUtils::StreetLightActiveIndex;
		if (!ArenaAnimUtils::tryAddStaticEntityAnimState(activeFlatIndex, EntityAnimationUtils::STATE_ACTIVATED.c_str(),
			StaticActivatedSecondsPerFrame, StaticActivatedLoop, inf, textureManager, outAnimDef))
		{
			DebugLogWarningFormat("Couldn't add streetlight active anim state for ID \"%d\".", activeFlatIndex);
			return false;
		}
	}
	else if (isLockableChest)
	{
		outAnimDef->init(EntityAnimationUtils::STATE_UNLOCKED.c_str());

		const ArenaFlatIndex lockedFlatIndex = inf.findFlatIndexWithItemIndex(ArenaAnimUtils::LockedChestItemIndex);
		const ArenaFlatIndex unlockedFlatIndex = inf.findFlatIndexWithItemIndex(ArenaAnimUtils::UnlockedChestItemIndex);

		if (!ArenaAnimUtils::tryAddStaticEntityAnimState(lockedFlatIndex, EntityAnimationUtils::STATE_LOCKED.c_str(),
			StaticIdleSecondsPerFrame, StaticIdleLoop, inf, textureManager, outAnimDef))
		{
			DebugLogWarningFormat("Couldn't add locked anim state for ID \"%d\".", lockedFlatIndex);
			return false;
		}

		if (!ArenaAnimUtils::tryAddStaticEntityAnimState(unlockedFlatIndex, EntityAnimationUtils::STATE_UNLOCKED.c_str(),
			StaticIdleSecondsPerFrame, StaticIdleLoop, inf, textureManager, outAnimDef))
		{
			DebugLogWarningFormat("Couldn't add unlocked anim state for ID \"%d\".", unlockedFlatIndex);
			return false;
		}
	}
	else
	{
		// General static entity animation.
		outAnimDef->init(EntityAnimationUtils::STATE_IDLE.c_str());

		if (!ArenaAnimUtils::tryAddStaticEntityAnimState(flatIndex, EntityAnimationUtils::STATE_IDLE.c_str(),
			StaticIdleSecondsPerFrame, StaticIdleLoop, inf, textureManager, outAnimDef))
		{
			DebugLogWarningFormat("Couldn't add idle anim state for ID \"%d\".", flatIndex);
			return false;
		}
	}

	outAnimDef->populateLinearizedIndices();
	return true;
}

bool ArenaAnimUtils::tryMakeDynamicEntityCreatureAnims(int creatureID, const ExeData &exeData,
	TextureManager &textureManager, EntityAnimationDefinition *outAnimDef)
{
	outAnimDef->init(EntityAnimationUtils::STATE_IDLE.c_str());

	// Basic states are idle/look/walk.
	if (!ArenaAnimUtils::tryAddDynamicEntityCreatureBasicAnimState(creatureID, EntityAnimationUtils::STATE_IDLE.c_str(),
		CreatureIdleSecondsPerFrame, CreatureIdleLoop, CreatureIdleIndices, exeData, textureManager, outAnimDef))
	{
		DebugLogWarning("Couldn't add idle anim state for creature ID \"" + std::to_string(creatureID) + "\".");
		return false;
	}

	if (!ArenaAnimUtils::tryAddDynamicEntityCreatureBasicAnimState(creatureID, EntityAnimationUtils::STATE_LOOK.c_str(),
		CreatureLookSecondsPerFrame, CreatureLookLoop, CreatureLookIndices, exeData, textureManager, outAnimDef))
	{
		DebugLogWarning("Couldn't add look anim state for creature ID \"" + std::to_string(creatureID) + "\".");
		return false;
	}

	if (!ArenaAnimUtils::tryAddDynamicEntityCreatureBasicAnimState(creatureID, EntityAnimationUtils::STATE_WALK.c_str(),
		CreatureWalkSecondsPerFrame, CreatureWalkLoop, CreatureWalkIndices, exeData, textureManager, outAnimDef))
	{
		DebugLogWarning("Couldn't add walk anim state for creature ID \"" + std::to_string(creatureID) + "\".");
		return false;
	}

	// Attack state.
	if (!ArenaAnimUtils::tryAddDynamicEntityCreatureAttackAnimState(creatureID, exeData, textureManager, outAnimDef))
	{
		DebugLogWarning("Couldn't add attack anim for creature ID \"" + std::to_string(creatureID) + "\".");
		return false;
	}

	// Death state.
	if (!ArenaAnimUtils::tryAddDynamicEntityCreatureDeathAnimState(creatureID, exeData, textureManager, outAnimDef))
	{
		DebugLogWarning("Couldn't add death anim for creature ID \"" + std::to_string(creatureID) + "\".");
		return false;
	}

	outAnimDef->populateLinearizedIndices();
	return true;
}

bool ArenaAnimUtils::tryMakeDynamicEntityHumanAnims(int charClassIndex, bool isMale, const CharacterClassLibrary &charClassLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager, EntityAnimationDefinition *outAnimDef)
{
	outAnimDef->init(EntityAnimationUtils::STATE_IDLE.c_str());

	// Basic states are idle and walk. Human enemies don't have look animations.
	if (!ArenaAnimUtils::tryAddDynamicEntityHumanBasicAnimState(charClassIndex, isMale, EntityAnimationUtils::STATE_IDLE.c_str(),
		HumanIdleSecondsPerFrame, HumanIdleLoop, HumanIdleIndices, charClassLibrary, binaryAssetLibrary, textureManager, outAnimDef))
	{
		DebugLogWarning("Couldn't add idle anim state for character class \"" + std::to_string(charClassIndex) + "\".");
		return false;
	}

	if (!ArenaAnimUtils::tryAddDynamicEntityHumanBasicAnimState(charClassIndex, isMale, EntityAnimationUtils::STATE_WALK.c_str(),
		HumanWalkSecondsPerFrame, HumanWalkLoop, HumanWalkIndices, charClassLibrary, binaryAssetLibrary, textureManager, outAnimDef))
	{
		DebugLogWarning("Couldn't add walk anim state for character class \"" + std::to_string(charClassIndex) + "\".");
		return false;
	}

	// Attack state.
	if (!ArenaAnimUtils::tryAddDynamicEntityHumanAttackAnimState(charClassIndex, isMale, charClassLibrary, binaryAssetLibrary,
		textureManager, outAnimDef))
	{
		DebugLogWarning("Couldn't add attack anim for character class \"" + std::to_string(charClassIndex) + "\".");
		return false;
	}

	// Death state.
	if (!ArenaAnimUtils::tryAddDynamicEntityHumanDeathAnimState(textureManager, outAnimDef))
	{
		DebugLogWarning("Couldn't add death anim for character class \"" + std::to_string(charClassIndex) + "\".");
		return false;
	}

	outAnimDef->populateLinearizedIndices();
	return true;
}

bool ArenaAnimUtils::tryMakeDynamicEntityAnims(ArenaFlatIndex flatIndex, const std::optional<bool> &isMale,
	const INFFile &inf, const CharacterClassLibrary &charClassLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, EntityAnimationDefinition *outAnimDef)
{
	DebugAssert(outAnimDef != nullptr);

	const auto &exeData = binaryAssetLibrary.getExeData();
	const INFFlat &flatData = inf.getFlat(flatIndex);
	const std::optional<ArenaItemIndex> &optItemIndex = flatData.itemIndex;
	if (!optItemIndex.has_value())
	{
		DebugLogWarning("Can't make dynamic entity anim states for flat \"" + std::to_string(flatIndex) + "\" without *ITEM index.");
		return false;
	}

	const ArenaItemIndex itemIndex = *optItemIndex;

	bool isFinalBoss;
	const bool isCreature = ArenaAnimUtils::isCreatureIndex(itemIndex, &isFinalBoss);
	const bool isHuman = ArenaAnimUtils::isHumanEnemyIndex(itemIndex);

	bool success = false;
	if (isCreature)
	{
		const int creatureID = ArenaAnimUtils::getCreatureIDFromItemIndex(itemIndex);
		success = ArenaAnimUtils::tryMakeDynamicEntityCreatureAnims(creatureID, exeData, textureManager, outAnimDef);
	}
	else if (isHuman)
	{
		DebugAssert(isMale.has_value());
		const int charClassIndex = ArenaAnimUtils::getCharacterClassIndexFromItemIndex(itemIndex);
		success = ArenaAnimUtils::tryMakeDynamicEntityHumanAnims(charClassIndex, *isMale, charClassLibrary, binaryAssetLibrary, textureManager, outAnimDef);
	}
	else
	{
		DebugLogError("Unrecognized *ITEM index \"" + std::to_string(itemIndex) + "\".");
		success = false;
	}

	outAnimDef->populateLinearizedIndices();
	return success;
}

bool ArenaAnimUtils::tryMakeCitizenAnims(ArenaClimateType climateType, bool isMale, const ExeData &exeData,
	TextureManager &textureManager, EntityAnimationDefinition *outAnimDef)
{
	outAnimDef->init(EntityAnimationUtils::STATE_IDLE.c_str());
	const int animFilenameIndex = GetCitizenAnimationFilenameIndex(isMale, climateType);

	if (!ArenaAnimUtils::tryAddDynamicEntityCitizenBasicAnimState(EntityAnimationUtils::STATE_IDLE.c_str(),
		CitizenIdleSecondsPerFrame, CitizenIdleLoop, animFilenameIndex, isMale, CitizenIdleIndices, exeData,
		textureManager, outAnimDef))
	{
		DebugLogWarning("Couldn't add idle anim state for citizen ID \"" + std::to_string(animFilenameIndex) + "\".");
		return false;
	}

	if (!ArenaAnimUtils::tryAddDynamicEntityCitizenBasicAnimState(EntityAnimationUtils::STATE_WALK.c_str(),
		CitizenWalkSecondsPerFrame, CitizenWalkLoop, animFilenameIndex, isMale, CitizenWalkIndices, exeData,
		textureManager, outAnimDef))
	{
		DebugLogWarning("Couldn't add walk anim state for citizen ID \"" + std::to_string(animFilenameIndex) + "\".");
	}

	outAnimDef->populateLinearizedIndices();
	return true;
}

bool ArenaAnimUtils::tryMakeVfxAnim(const std::string &animFilename, bool isLooping, TextureManager &textureManager, EntityAnimationDefinition *outAnimDef)
{
	DebugAssert(outAnimDef->stateCount == 0);
	outAnimDef->init(EntityAnimationUtils::STATE_IDLE.c_str());

	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(animFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugLogWarning("Couldn't get VFX anim texture file metadata for \"" + animFilename + "\".");
		return false;
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	const int keyframeCount = textureFileMetadata.getTextureCount();
	const double stateSeconds = static_cast<double>(keyframeCount) * VfxIdleSecondsPerFrame;
	const int stateIndex = outAnimDef->addState(EntityAnimationUtils::STATE_IDLE.c_str(), stateSeconds, isLooping);

	constexpr bool isMirrored = false;
	const int keyframeListIndex = outAnimDef->addKeyframeList(stateIndex, isMirrored);
	for (int i = 0; i < keyframeCount; i++)
	{
		double width, height;
		MakeVfxKeyframeDimensions(textureFileMetadata.getWidth(i), textureFileMetadata.getHeight(i), &width, &height);

		TextureAsset textureAsset(std::string(textureFileMetadata.getFilename()), i);
		outAnimDef->addKeyframe(keyframeListIndex, std::move(textureAsset), width, height);
	}

	outAnimDef->populateLinearizedIndices();
	return true;
}

PaletteIndices ArenaAnimUtils::transformCitizenColors(int raceIndex, uint16_t seed, const ExeData &exeData)
{
	PaletteIndices newPaletteIndices;
	std::iota(newPaletteIndices.begin(), newPaletteIndices.end(), 0);

	// Clothes transformation.
	const Span<const uint8_t> colorBase = exeData.entities.citizenColorBase;
	uint16_t val = seed & 0x7FFF;
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
				DebugAssertIndex(newPaletteIndices, oldIndex);
				newPaletteIndices[oldIndex] = newIndex;
			}
		}
	}

	const Span<const uint8_t> skinColors = exeData.entities.citizenSkinColors;

	// Skin transformation, only if the given race should have its colors transformed.
	constexpr int RaceOffsets[] = { -1, 148, -1, 52, 192, -1, -1, 116, 148 };
	DebugAssertIndex(RaceOffsets, raceIndex);
	const int raceOffset = RaceOffsets[raceIndex];
	const bool hasTransformation = raceOffset != -1;
	if (hasTransformation)
	{
		for (int i = 0; i < 10; i++)
		{
			const int oldIndex = raceOffset + i;
			const int newIndex = skinColors[i];
			DebugAssertIndex(newPaletteIndices, newIndex);
			newPaletteIndices[newIndex] = oldIndex;
		}
	}

	return newPaletteIndices;
}
