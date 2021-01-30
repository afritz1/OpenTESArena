#include <array>
#include <limits>

#include "ArenaAnimUtils.h"
#include "ArenaTypes.h"
#include "BinaryAssetLibrary.h"
#include "CFAFile.h"
#include "DFAFile.h"
#include "IMGFile.h"
#include "MIFUtils.h"
#include "../Entities/CharacterClassDefinition.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Entities/EntityType.h"
#include "../Items/ArmorMaterialType.h"
#include "../Media/TextureManager.h"
#include "../World/ClimateType.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

// Private functions.
namespace ArenaAnimUtils
{
	// Animation scale helper values based on the original game.
	constexpr double MediumScale = INFFile::FlatData::MEDIUM_SCALE / 100.0;
	constexpr double LargeScale = INFFile::FlatData::LARGE_SCALE / 100.0;

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

	bool tryMakeStaticEntityAnimState(ArenaTypes::FlatIndex flatIndex, const char *stateName,
		double secondsPerFrame, bool looping, const INFFile &inf, TextureManager &textureManager,
		EntityAnimationDefinition::State *outDefState,
		EntityAnimationInstance::State *outInstState)
	{
		const INFFile::FlatData &flatData = inf.getFlat(flatIndex);
		const char *flatTextureName = [&inf, &flatData]()
		{
			const std::vector<INFFile::FlatTextureData> &flatTextures = inf.getFlatTextures();
			DebugAssertIndex(flatTextures, flatData.textureIndex);
			const INFFile::FlatTextureData &flatTextureData = flatTextures[flatData.textureIndex];
			return flatTextureData.filename.c_str();
		}();

		// Avoid files with no extension. They are lore-based names that are not used in-game.
		const std::string_view extension = StringView::getExtension(flatTextureName);
		if (extension.size() == 0)
		{
			return true;
		}

		const std::optional<TextureFileMetadata> textureFileMetadata = textureManager.tryGetMetadata(flatTextureName);
		if (!textureFileMetadata.has_value())
		{
			DebugLogWarning("Couldn't get static anim texture file metadata for \"" + std::string(flatTextureName) + "\".");
			return false;
		}

		const double dimensionModifier = ArenaAnimUtils::getDimensionModifier(flatData);

		EntityAnimationDefinition::KeyframeList defKeyframeList;
		EntityAnimationInstance::KeyframeList instKeyframeList;
		constexpr bool flipped = false; // Static anims cannot be flipped.
		defKeyframeList.init(flipped);

		for (int i = 0; i < textureFileMetadata->getTextureCount(); i++)
		{
			const double width = MakeStaticKeyframeDimension(textureFileMetadata->getWidth(i), dimensionModifier);
			const double height = MakeStaticKeyframeDimension(textureFileMetadata->getHeight(i), dimensionModifier);
			TextureAssetReference textureAssetRef(std::string(textureFileMetadata->getFilename()), i);
			defKeyframeList.addKeyframe(EntityAnimationDefinition::Keyframe(std::move(textureAssetRef), width, height));
			instKeyframeList.addKeyframe(EntityAnimationInstance::Keyframe());
		}

		const int keyframeCount = defKeyframeList.getKeyframeCount();
		const double totalSeconds = static_cast<double>(keyframeCount) * secondsPerFrame;
		outDefState->init(stateName, totalSeconds, looping);
		outDefState->addKeyframeList(std::move(defKeyframeList));
		outInstState->addKeyframeList(std::move(instKeyframeList));
		return true;
	}

	// 'Basic' dynamic entity anim state being one of: Idle, Look, Walk.
	bool tryMakeDynamicEntityCreatureBasicAnimState(int creatureID, const char *stateName,
		double secondsPerFrame, bool looping, const std::vector<int> &animIndices,
		const ExeData &exeData, TextureManager &textureManager,
		EntityAnimationDefinition::State *outDefState, EntityAnimationInstance::State *outInstState)
	{
		DebugAssert(outDefState != nullptr);
		DebugAssert(outInstState != nullptr);

		auto tryAddDirectionToState = [creatureID, &animIndices, &exeData, &textureManager,
			outDefState, outInstState](int direction)
		{
			DebugAssert(direction >= 1);
			DebugAssert(direction <= Directions);

			bool animIsFlipped;
			const int correctedDirection =
				ArenaAnimUtils::getDynamicEntityCorrectedAnimDirID(direction, &animIsFlipped);

			const auto &creatureAnimFilenames = exeData.entities.creatureAnimationFilenames;
			const int creatureIndex = ArenaAnimUtils::getCreatureIndexFromID(creatureID);

			DebugAssertIndex(creatureAnimFilenames, creatureIndex);
			std::string creatureFilename = String::toUppercase(creatureAnimFilenames[creatureIndex]);

			// Revise the filename based on which direction is being initialized.
			if (!ArenaAnimUtils::trySetDynamicEntityFilenameDirection(creatureFilename, correctedDirection))
			{
				DebugLogWarning("Couldn't set creature filename direction \"" +
					creatureFilename + "\" (" + std::to_string(correctedDirection) + ").");
				return false;
			}

			const std::optional<TextureFileMetadata> textureFileMetadata =
				textureManager.tryGetMetadata(creatureFilename.c_str());
			if (!textureFileMetadata.has_value())
			{
				DebugLogWarning("Couldn't get creature anim texture file metadata for \"" + creatureFilename + "\".");
				return false;
			}

			EntityAnimationDefinition::KeyframeList defKeyframeList;
			EntityAnimationInstance::KeyframeList instKeyframeList;
			defKeyframeList.init(animIsFlipped);

			for (const int frameIndex : animIndices)
			{
				// Certain creatures don't have anim frames for a look animation, so just use
				// frame 0 as a fallback.
				const int correctedFrameIndex = frameIndex < textureFileMetadata->getTextureCount() ? frameIndex : 0;

				double width, height;
				MakeCreatureKeyframeDimensions(creatureIndex, textureFileMetadata->getWidth(correctedFrameIndex),
					textureFileMetadata->getHeight(correctedFrameIndex), exeData, &width, &height);

				TextureAssetReference textureAssetRef(std::string(textureFileMetadata->getFilename()), correctedFrameIndex);
				defKeyframeList.addKeyframe(EntityAnimationDefinition::Keyframe(std::move(textureAssetRef), width, height));
				instKeyframeList.addKeyframe(EntityAnimationInstance::Keyframe());
			}

			outDefState->addKeyframeList(std::move(defKeyframeList));
			outInstState->addKeyframeList(std::move(instKeyframeList));
			return true;
		};

		for (int i = 1; i <= Directions; i++)
		{
			if (!tryAddDirectionToState(i))
			{
				DebugLogWarning("Couldn't add creature anim keyframe list for creature ID \"" +
					std::to_string(creatureID) + "\" direction \"" + std::to_string(i) + "\".");
			}
		}

		// Get total seconds using the forward-facing keyframe list if it exists, otherwise don't
		// use this state.
		if (outDefState->getKeyframeListCount() == 0)
		{
			DebugLogWarning("Missing keyframe list for creature ID \"" + std::to_string(creatureID) +
				"\" to determine total seconds from.");
			return false;
		}

		const int keyframeCount = outDefState->getKeyframeList(0).getKeyframeCount();
		const double totalSeconds = static_cast<double>(keyframeCount) * secondsPerFrame;
		outDefState->init(stateName, totalSeconds, looping);
		return true;
	}

	// Idle or walk animation state for human enemies.
	bool tryMakeDynamicEntityHumanBasicAnimState(int charClassIndex, bool isMale, const char *stateName,
		double secondsPerFrame, bool looping, const std::vector<int> &animIndices,
		const CharacterClassLibrary &charClassLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, EntityAnimationDefinition::State *outDefState,
		EntityAnimationInstance::State *outInstState)
	{
		const auto &exeData = binaryAssetLibrary.getExeData();
		int humanFilenameTypeIndex;
		ArenaAnimUtils::getHumanEnemyProperties(charClassIndex, charClassLibrary, exeData,
			&humanFilenameTypeIndex);

		auto tryAddDirectionToState = [isMale, &animIndices, &textureManager, humanFilenameTypeIndex,
			&exeData, outDefState, outInstState](int direction)
		{
			DebugAssert(direction >= 1);
			DebugAssert(direction <= Directions);

			bool animIsFlipped;
			const int correctedDirection =
				ArenaAnimUtils::getDynamicEntityCorrectedAnimDirID(direction, &animIsFlipped);

			// Revise the filename based on which direction is being initialized.
			constexpr int templateIndex = 0; // Idle/walk template index.
			const auto &humanFilenameTemplates = exeData.entities.humanFilenameTemplates;
			DebugAssertIndex(humanFilenameTemplates, templateIndex);
			std::string animName = humanFilenameTemplates[templateIndex];
			if (!trySetDynamicEntityFilenameDirection(animName, correctedDirection))
			{
				DebugLogWarning("Couldn't set human filename direction \"" +
					animName + "\" (" + std::to_string(correctedDirection) + ").");
				return false;
			}

			const auto &humanFilenameTypes = exeData.entities.humanFilenameTypes;
			DebugAssertIndex(humanFilenameTypes, humanFilenameTypeIndex);
			const std::string_view humanFilenameType = humanFilenameTypes[humanFilenameTypeIndex];
			if (!trySetHumanFilenameType(animName, humanFilenameType))
			{
				DebugLogWarning("Couldn't set human filename type \"" +
					animName + "\" (" + std::to_string(correctedDirection) + ").");
				return false;
			}

			// Special case for plate sprites: female is replaced with male, since they would
			// apparently look the same in armor.
			const bool isPlate = humanFilenameTypeIndex == ArenaAnimUtils::HumanFilenameTypeIndexPlate;

			if (!trySetHumanFilenameGender(animName, isMale || isPlate))
			{
				DebugLogWarning("Couldn't set human filename gender \"" +
					animName + "\" (" + std::to_string(correctedDirection) + ").");
				return false;
			}

			animName = String::toUppercase(animName);

			// Not all permutations of human filenames exist. If a series is missing, then probably
			// need to have special behavior.
			const std::optional<TextureFileMetadata> textureFileMetadata =
				textureManager.tryGetMetadata(animName.c_str());
			if (!textureFileMetadata.has_value())
			{
				DebugLogWarning("Couldn't get human anim texture file metadata for \"" + animName + "\".");
				return false;
			}

			EntityAnimationDefinition::KeyframeList defKeyframeList;
			EntityAnimationInstance::KeyframeList instKeyframeList;
			defKeyframeList.init(animIsFlipped);

			for (const int frameIndex : animIndices)
			{
				double width, height;
				MakeHumanKeyframeDimensions(textureFileMetadata->getWidth(frameIndex),
					textureFileMetadata->getHeight(frameIndex), &width, &height);

				TextureAssetReference textureAssetRef(std::string(textureFileMetadata->getFilename()), frameIndex);
				defKeyframeList.addKeyframe(EntityAnimationDefinition::Keyframe(std::move(textureAssetRef), width, height));
				instKeyframeList.addKeyframe(EntityAnimationInstance::Keyframe());
			}

			outDefState->addKeyframeList(std::move(defKeyframeList));
			outInstState->addKeyframeList(std::move(instKeyframeList));
			return true;
		};

		for (int i = 1; i <= Directions; i++)
		{
			if (!tryAddDirectionToState(i))
			{
				DebugLogWarning("Couldn't add human anim keyframe list for character class \"" +
					std::to_string(charClassIndex) + "\" direction \"" + std::to_string(i) + "\".");
			}
		}

		// Get total seconds using the forward-facing keyframe list if it exists, otherwise don't
		// use this state.
		if (outDefState->getKeyframeListCount() == 0)
		{
			DebugLogWarning("Missing keyframe list for character class \"" +
				std::to_string(charClassIndex) + "\" to determine total seconds from.");
			return false;
		}

		const int keyframeCount = outDefState->getKeyframeList(0).getKeyframeCount();
		const double totalSeconds = static_cast<double>(keyframeCount) * secondsPerFrame;
		outDefState->init(stateName, totalSeconds, looping);
		return true;
	}

	bool tryMakeDynamicEntityCreatureAttackAnimState(int creatureID, const ExeData &exeData,
		TextureManager &textureManager, EntityAnimationDefinition::State *outDefState,
		EntityAnimationInstance::State *outInstState)
	{
		// Attack state is only in the first .CFA file and is never flipped because it only
		// faces forward.
		constexpr int direction = 1;
		constexpr bool animIsFlipped = false;

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

		const std::optional<TextureFileMetadata> textureFileMetadata =
			textureManager.tryGetMetadata(creatureFilename.c_str());
		if (!textureFileMetadata.has_value())
		{
			DebugLogWarning("Couldn't get creature attack texture file metadata for \"" + creatureFilename + "\".");
			return false;
		}

		EntityAnimationDefinition::KeyframeList defKeyframeList;
		EntityAnimationInstance::KeyframeList instKeyframeList;
		defKeyframeList.init(animIsFlipped);

		for (const int frameIndex : CreatureAttackIndices)
		{
			double width, height;
			MakeCreatureKeyframeDimensions(creatureIndex, textureFileMetadata->getWidth(frameIndex),
				textureFileMetadata->getHeight(frameIndex), exeData, &width, &height);

			TextureAssetReference textureAssetRef(std::string(textureFileMetadata->getFilename()), frameIndex);
			defKeyframeList.addKeyframe(EntityAnimationDefinition::Keyframe(std::move(textureAssetRef), width, height));
			instKeyframeList.addKeyframe(EntityAnimationInstance::Keyframe());
		}

		outDefState->addKeyframeList(std::move(defKeyframeList));
		outInstState->addKeyframeList(std::move(instKeyframeList));

		const int keyframeCount = outDefState->getKeyframeList(0).getKeyframeCount();
		const double totalSeconds = static_cast<double>(keyframeCount) * CreatureAttackSecondsPerFrame;
		outDefState->init(EntityAnimationUtils::STATE_ATTACK.c_str(), totalSeconds, CreatureAttackLoop);
		return true;
	}

	bool tryMakeDynamicEntityHumanAttackAnimState(int charClassIndex, bool isMale,
		const CharacterClassLibrary &charClassLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, EntityAnimationDefinition::State *outDefState,
		EntityAnimationInstance::State *outInstState)
	{
		// Attack state is only in the first .CFA file and is never flipped because it only
		// faces forward.
		constexpr int direction = 1;
		constexpr bool animIsFlipped = false;

		const auto &exeData = binaryAssetLibrary.getExeData();
		int humanFilenameTypeIndex;
		ArenaAnimUtils::getHumanEnemyProperties(charClassIndex, charClassLibrary, exeData,
			&humanFilenameTypeIndex);

		constexpr int attackTemplateIndex = 1;
		const auto &humanFilenameTemplates = exeData.entities.humanFilenameTemplates;
		DebugAssertIndex(humanFilenameTemplates, attackTemplateIndex);
		std::string animName = humanFilenameTemplates[attackTemplateIndex];
		if (!ArenaAnimUtils::trySetDynamicEntityFilenameDirection(animName, direction))
		{
			DebugLogError("Couldn't set human attack filename direction \"" +
				animName + "\" (" + std::to_string(direction) + ").");
			return false;
		}

		const auto &humanFilenameTypes = exeData.entities.humanFilenameTypes;
		DebugAssertIndex(humanFilenameTypes, humanFilenameTypeIndex);
		const std::string_view humanFilenameType = humanFilenameTypes[humanFilenameTypeIndex];
		if (!ArenaAnimUtils::trySetHumanFilenameType(animName, humanFilenameType))
		{
			DebugLogError("Couldn't set human attack filename type \"" +
				animName + "\" (" + std::to_string(direction) + ").");
			return false;
		}

		// Special case for plate sprites: female is replaced with male, since they would
		// apparently look the same in armor.
		const bool isPlate = humanFilenameTypeIndex == ArenaAnimUtils::HumanFilenameTypeIndexPlate;

		if (!trySetHumanFilenameGender(animName, isMale || isPlate))
		{
			DebugLogError("Couldn't set human attack filename gender \"" +
				animName + "\" (" + std::to_string(direction) + ").");
			return false;
		}

		animName = String::toUppercase(animName);
		const std::optional<TextureFileMetadata> textureFileMetadata = textureManager.tryGetMetadata(animName.c_str());
		if (!textureFileMetadata.has_value())
		{
			DebugLogWarning("Couldn't get human attack texture file metadata for \"" + animName + "\".");
			return false;
		}

		EntityAnimationDefinition::KeyframeList defKeyframeList;
		EntityAnimationInstance::KeyframeList instKeyframeList;
		defKeyframeList.init(animIsFlipped);

		// No need for extra anim indices list, just use sequential image IDs.
		for (int i = 0; i < textureFileMetadata->getTextureCount(); i++)
		{
			double width, height;
			MakeHumanKeyframeDimensions(textureFileMetadata->getWidth(i),
				textureFileMetadata->getHeight(i), &width, &height);

			TextureAssetReference textureAssetRef(std::string(textureFileMetadata->getFilename()), i);
			defKeyframeList.addKeyframe(EntityAnimationDefinition::Keyframe(std::move(textureAssetRef), width, height));
			instKeyframeList.addKeyframe(EntityAnimationInstance::Keyframe());
		}

		outDefState->addKeyframeList(std::move(defKeyframeList));
		outInstState->addKeyframeList(std::move(instKeyframeList));

		const int keyframeCount = outDefState->getKeyframeList(0).getKeyframeCount();
		const double totalSeconds = static_cast<double>(keyframeCount) * HumanAttackSecondsPerFrame;
		outDefState->init(EntityAnimationUtils::STATE_ATTACK.c_str(), totalSeconds, HumanAttackLoop);
		return true;
	}

	bool tryMakeDynamicEntityCreatureDeathAnimState(int creatureID, const ExeData &exeData,
		TextureManager &textureManager, EntityAnimationDefinition::State *outDefState,
		EntityAnimationInstance::State *outInstState)
	{
		// Death state is only in the last .CFA file.
		constexpr int direction = 6;
		constexpr bool animIsFlipped = false;

		const auto &creatureAnimFilenames = exeData.entities.creatureAnimationFilenames;
		const int creatureIndex = ArenaAnimUtils::getCreatureIndexFromID(creatureID);

		DebugAssertIndex(creatureAnimFilenames, creatureIndex);
		std::string creatureFilename = String::toUppercase(creatureAnimFilenames[creatureIndex]);
		if (!ArenaAnimUtils::trySetDynamicEntityFilenameDirection(creatureFilename, direction))
		{
			DebugLogError("Couldn't set creature filename direction \"" +
				creatureFilename + "\" (" + std::to_string(direction) + ").");
			return false;
		}

		const std::optional<TextureFileMetadata> textureFileMetadata =
			textureManager.tryGetMetadata(creatureFilename.c_str());
		if (!textureFileMetadata.has_value())
		{
			DebugLogWarning("Couldn't get creature death texture file metadata for \"" + creatureFilename + "\".");
			return false;
		}

		EntityAnimationDefinition::KeyframeList defKeyframeList;
		EntityAnimationInstance::KeyframeList instKeyframeList;
		defKeyframeList.init(animIsFlipped);

		// No need for extra anim indices list, just use sequential image IDs.
		for (int i = 0; i < textureFileMetadata->getTextureCount(); i++)
		{
			double width, height;
			ArenaAnimUtils::MakeCreatureKeyframeDimensions(creatureIndex, textureFileMetadata->getWidth(i),
				textureFileMetadata->getHeight(i), exeData, &width, &height);

			TextureAssetReference textureAssetRef(std::string(textureFileMetadata->getFilename()), i);
			defKeyframeList.addKeyframe(EntityAnimationDefinition::Keyframe(std::move(textureAssetRef), width, height));
			instKeyframeList.addKeyframe(EntityAnimationInstance::Keyframe());
		}

		outDefState->addKeyframeList(std::move(defKeyframeList));
		outInstState->addKeyframeList(std::move(instKeyframeList));

		const int keyframeCount = outDefState->getKeyframeList(0).getKeyframeCount();
		const double totalSeconds = static_cast<double>(keyframeCount) * CreatureDeathSecondsPerFrame;
		outDefState->init(EntityAnimationUtils::STATE_DEATH.c_str(), totalSeconds, CreatureDeathLoop);
		return true;
	}

	bool tryMakeDynamicEntityHumanDeathAnimState(const INFFile &inf, TextureManager &textureManager,
		EntityAnimationDefinition::State *outDefState, EntityAnimationInstance::State *outInstState)
	{
		constexpr bool animIsFlipped = false;

		// Humans use a single dead body image.
		const std::string animName = [&inf]()
		{
			constexpr ArenaTypes::ItemIndex corpseItemIndex = 2;
			const INFFile::FlatData *corpseFlat = inf.getFlatWithItemIndex(corpseItemIndex);
			DebugAssertMsg(corpseFlat != nullptr, "Missing human corpse flat.");
			const int corpseFlatTextureIndex = corpseFlat->textureIndex;
			const auto &flatTextures = inf.getFlatTextures();
			DebugAssertIndex(flatTextures, corpseFlatTextureIndex);
			const INFFile::FlatTextureData &flatTextureData = flatTextures[corpseFlatTextureIndex];
			return String::toUppercase(flatTextureData.filename);
		}();

		const std::optional<TextureFileMetadata> textureFileMetadata = textureManager.tryGetMetadata(animName.c_str());
		if (!textureFileMetadata.has_value())
		{
			DebugLogWarning("Couldn't get human death texture file metadata for \"" + animName + "\".");
			return false;
		}

		EntityAnimationDefinition::KeyframeList defKeyframeList;
		EntityAnimationInstance::KeyframeList instKeyframeList;
		defKeyframeList.init(animIsFlipped);

		const double width = MakeDefaultKeyframeDimension(textureFileMetadata->getWidth(0));
		const double height = MakeDefaultKeyframeDimension(textureFileMetadata->getHeight(0));
		TextureAssetReference textureAssetRef(std::string(textureFileMetadata->getFilename()));
		defKeyframeList.addKeyframe(EntityAnimationDefinition::Keyframe(std::move(textureAssetRef), width, height));
		instKeyframeList.addKeyframe(EntityAnimationInstance::Keyframe());
		outDefState->addKeyframeList(std::move(defKeyframeList));
		outInstState->addKeyframeList(std::move(instKeyframeList));

		const int keyframeCount = outDefState->getKeyframeList(0).getKeyframeCount();
		const double totalSeconds = static_cast<double>(keyframeCount) * HumanDeathSecondsPerFrame;
		outDefState->init(EntityAnimationUtils::STATE_DEATH.c_str(), totalSeconds, HumanDeathLoop);
		return true;
	}

	// Citizens have idle and walk animation states.
	bool tryMakeDynamicEntityCitizenBasicAnimState(const char *stateName, double secondsPerFrame,
		bool looping, int citizenIndex, bool isMale, const std::vector<int> &animIndices,
		const ExeData &exeData, TextureManager &textureManager,
		EntityAnimationDefinition::State *outDefState, EntityAnimationInstance::State *outInstState)
	{
		// Citizen animation filename list, depends on the gender.
		const auto &citizenAnimFilenames = isMale ? exeData.entities.maleCitizenAnimationFilenames :
			exeData.entities.femaleCitizenAnimationFilenames;

		auto tryAddDirectionToState = [citizenIndex, isMale, &animIndices, &exeData,
			&textureManager, outDefState, outInstState, &citizenAnimFilenames](int direction)
		{
			DebugAssert(direction >= 1);
			DebugAssert(direction <= Directions);

			bool animIsFlipped;
			const int correctedDirection =
				ArenaAnimUtils::getDynamicEntityCorrectedAnimDirID(direction, &animIsFlipped);

			DebugAssertIndex(citizenAnimFilenames, citizenIndex);
			std::string citizenFilename = String::toUppercase(citizenAnimFilenames[citizenIndex]);
			if (!trySetCitizenFilenameDirection(citizenFilename, correctedDirection))
			{
				DebugLogError("Couldn't set citizen filename direction \"" +
					citizenFilename + "\" (" + std::to_string(correctedDirection) + ").");
				return false;
			}

			const std::optional<TextureFileMetadata> textureFileMetadata =
				textureManager.tryGetMetadata(citizenFilename.c_str());
			if (!textureFileMetadata.has_value())
			{
				DebugLogWarning("Couldn't get citizen anim texture file metadata for \"" + citizenFilename + "\".");
				return false;
			}

			EntityAnimationDefinition::KeyframeList defKeyframeList;
			EntityAnimationInstance::KeyframeList instKeyframeList;
			defKeyframeList.init(animIsFlipped);

			for (const int frameIndex : animIndices)
			{
				// Citizens only have forward-facing idle animations, so use frame 0 for other facings.
				const int correctedFrameIndex = frameIndex < textureFileMetadata->getTextureCount() ? frameIndex : 0;

				double width, height;
				MakeCitizenKeyframeDimensions(textureFileMetadata->getWidth(correctedFrameIndex),
					textureFileMetadata->getHeight(correctedFrameIndex), &width, &height);

				TextureAssetReference textureAssetRef(std::string(textureFileMetadata->getFilename()), correctedFrameIndex);
				defKeyframeList.addKeyframe(EntityAnimationDefinition::Keyframe(std::move(textureAssetRef), width, height));
				instKeyframeList.addKeyframe(EntityAnimationInstance::Keyframe());
			}

			outDefState->addKeyframeList(std::move(defKeyframeList));
			outInstState->addKeyframeList(std::move(instKeyframeList));
			return true;
		};

		for (int i = 1; i <= Directions; i++)
		{
			if (!tryAddDirectionToState(i))
			{
				DebugLogWarning("Couldn't make citizen anim states for direction \"" +
					std::to_string(i) + "\".");
			}
		}

		// Get total seconds using the forward-facing keyframe list if it exists, otherwise don't
		// use this state.
		if (outDefState->getKeyframeListCount() == 0)
		{
			DebugLogWarning("Missing keyframe list for citizen ID \"" + std::to_string(citizenIndex) +
				"\" to determine total seconds from.");
			return false;
		}

		const int keyframeCount = outDefState->getKeyframeList(0).getKeyframeCount();
		const double totalSeconds = static_cast<double>(keyframeCount) * secondsPerFrame;
		outDefState->init(stateName, totalSeconds, looping);
		return true;
	}
}

bool ArenaAnimUtils::isFinalBossIndex(ArenaTypes::ItemIndex itemIndex)
{
	return itemIndex == 73;
}

bool ArenaAnimUtils::isCreatureIndex(ArenaTypes::ItemIndex itemIndex, bool *outIsFinalBoss)
{
	const bool isFinalBoss = ArenaAnimUtils::isFinalBossIndex(itemIndex);
	*outIsFinalBoss = isFinalBoss;
	return (itemIndex >= 32 && itemIndex <= 54) || isFinalBoss;
}

bool ArenaAnimUtils::isHumanEnemyIndex(ArenaTypes::ItemIndex itemIndex)
{
	return itemIndex >= 55 && itemIndex <= 72;
}

EntityType ArenaAnimUtils::getEntityTypeFromFlat(ArenaTypes::FlatIndex flatIndex, const INFFile &inf)
{
	const auto &flatData = inf.getFlat(flatIndex);
	if (flatData.itemIndex.has_value())
	{
		const ArenaTypes::ItemIndex itemIndex = flatData.itemIndex.value();

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

ArenaTypes::ItemIndex ArenaAnimUtils::getFirstCreatureItemIndex()
{
	return 32;
}

int ArenaAnimUtils::getCreatureIDFromItemIndex(ArenaTypes::ItemIndex itemIndex)
{
	return ArenaAnimUtils::isFinalBossIndex(itemIndex) ?
		ArenaAnimUtils::getFinalBossCreatureID() : (itemIndex - 31);
}

int ArenaAnimUtils::getFinalBossCreatureID()
{
	return 24;
}

int ArenaAnimUtils::getCreatureIndexFromID(int creatureID)
{
	return creatureID - 1;
}

int ArenaAnimUtils::getCharacterClassIndexFromItemIndex(ArenaTypes::ItemIndex itemIndex)
{
	return itemIndex - 55;
}

ArenaTypes::FlatIndex ArenaAnimUtils::getStreetLightActiveIndex()
{
	return 29;
}

ArenaTypes::FlatIndex ArenaAnimUtils::getStreetLightInactiveIndex()
{
	return 30;
}

bool ArenaAnimUtils::isStreetLightFlatIndex(ArenaTypes::FlatIndex flatIndex, MapType mapType)
{
	// Wilderness and interiors do not have streetlights.
	if (mapType != MapType::City)
	{
		return false;
	}

	return (flatIndex == ArenaAnimUtils::getStreetLightActiveIndex()) ||
		(flatIndex == ArenaAnimUtils::getStreetLightInactiveIndex());
}

ArenaTypes::FlatIndex ArenaAnimUtils::getRulerKingIndex()
{
	return 0;
}

ArenaTypes::FlatIndex ArenaAnimUtils::getRulerQueenIndex()
{
	return 1;
}

bool ArenaAnimUtils::isRulerFlatIndex(ArenaTypes::FlatIndex flatIndex, ArenaTypes::InteriorType interiorType)
{
	if (interiorType != ArenaTypes::InteriorType::Palace)
	{
		return false;
	}

	return (flatIndex == ArenaAnimUtils::getRulerKingIndex()) ||
		(flatIndex == ArenaAnimUtils::getRulerQueenIndex());
}

void ArenaAnimUtils::getBaseFlatDimensions(int width, int height, uint16_t scale, int *baseWidth, int *baseHeight)
{
	*baseWidth = (width * scale) / 256;
	*baseHeight = (((height * scale) / 256) * 200) / 256;
}

double ArenaAnimUtils::getDimensionModifier(const INFFile::FlatData &flatData)
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

bool ArenaAnimUtils::isAnimDirectionFlipped(int animDirectionID)
{
	DebugAssert(animDirectionID >= 1);
	DebugAssert(animDirectionID <= Directions);
	return animDirectionID >= FirstFlippedAnimID;
}

int ArenaAnimUtils::getDynamicEntityCorrectedAnimDirID(int animDirectionID, bool *outIsFlipped)
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

void ArenaAnimUtils::getHumanEnemyProperties(int charClassIndex,
	const CharacterClassLibrary &charClassLibrary, const ExeData &exeData, int *outTypeIndex)
{
	int charClassDefIndex;
	const bool success = charClassLibrary.findDefinitionIndexIf(
		[charClassIndex](const CharacterClassDefinition &def)
	{
		const auto &originalClassIndex = def.getOriginalClassIndex();
		return originalClassIndex.has_value() && (*originalClassIndex == charClassIndex);
	}, &charClassDefIndex);

	if (!success)
	{
		DebugLogWarning("Couldn't get character class definition for index \"" +
			std::to_string(charClassIndex) + "\".");
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

		const auto &originalClassIndex = charClassDef.getOriginalClassIndex();

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
		else if (charClassDef.canCastMagic())
		{
			// Spellcaster.
			return 4;
		}
		else if (originalClassIndex.has_value() && (*originalClassIndex == 12))
		{
			// Monk.
			return 5;
		}
		else if (originalClassIndex.has_value() && (*originalClassIndex == 15))
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

bool ArenaAnimUtils::trySetHumanFilenameType(std::string &filename, const std::string_view &type)
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

bool ArenaAnimUtils::tryMakeStaticEntityAnims(ArenaTypes::FlatIndex flatIndex, MapType mapType,
	const std::optional<ArenaTypes::InteriorType> &interiorType, const std::optional<bool> &rulerIsMale,
	const INFFile &inf, TextureManager &textureManager, EntityAnimationDefinition *outAnimDef,
	EntityAnimationInstance *outAnimInst)
{
	DebugAssert(outAnimDef != nullptr);
	DebugAssert(outAnimInst != nullptr);

	// Some of the original game's flats reference lore-based names which have no animation.
	// Filter these out so the caller doesn't get a state with zero frames.
	auto addStateIfNotEmpty = [](auto *outAnim, auto &&state)
	{
		if (state.getKeyframeListCount() > 0)
		{
			const auto &keyframeList = state.getKeyframeList(0);
			if (keyframeList.getKeyframeCount() > 0)
			{
				outAnim->addState(std::move(state));
			}
		}
	};

	// Generate animation states based on what the entity needs. The animations to load depend on
	// the flat index. The wilderness does not have any streetlights (there is no ID for them).
	// @todo: see how treasure chests fit into this. Their flat indices seem to be variable.
	const bool isRuler = interiorType.has_value() && ArenaAnimUtils::isRulerFlatIndex(flatIndex, *interiorType);
	const bool isStreetlight = ArenaAnimUtils::isStreetLightFlatIndex(flatIndex, mapType);
	if (isRuler)
	{
		DebugAssert(rulerIsMale.has_value());
		const ArenaTypes::FlatIndex rulerFlatIndex = *rulerIsMale ?
			ArenaAnimUtils::getRulerKingIndex() : ArenaAnimUtils::getRulerQueenIndex();

		EntityAnimationDefinition::State defIdleState;
		EntityAnimationInstance::State instIdleState;
		if (!ArenaAnimUtils::tryMakeStaticEntityAnimState(rulerFlatIndex,
			EntityAnimationUtils::STATE_IDLE.c_str(), StaticIdleSecondsPerFrame, StaticIdleLoop,
			inf, textureManager, &defIdleState, &instIdleState))
		{
			DebugLogWarning("Couldn't make ruler idle anim state for ID \"" +
				std::to_string(flatIndex) + "\".");
			return false;
		}

		addStateIfNotEmpty(outAnimDef, defIdleState);
		addStateIfNotEmpty(outAnimInst, instIdleState);
	}
	else if (isStreetlight)
	{
		const ArenaTypes::FlatIndex idleFlatIndex = ArenaAnimUtils::getStreetLightInactiveIndex();
		EntityAnimationDefinition::State defIdleState;
		EntityAnimationInstance::State instIdleState;
		if (!ArenaAnimUtils::tryMakeStaticEntityAnimState(idleFlatIndex,
			EntityAnimationUtils::STATE_IDLE.c_str(), StaticIdleSecondsPerFrame, StaticIdleLoop,
			inf, textureManager, &defIdleState, &instIdleState))
		{
			DebugLogWarning("Couldn't make streetlight idle anim state for ID \"" +
				std::to_string(idleFlatIndex) + "\".");
			return false;
		}

		const ArenaTypes::FlatIndex activeFlatIndex = ArenaAnimUtils::getStreetLightActiveIndex();
		EntityAnimationDefinition::State defActivatedState;
		EntityAnimationInstance::State instActivatedState;
		if (!ArenaAnimUtils::tryMakeStaticEntityAnimState(activeFlatIndex,
			EntityAnimationUtils::STATE_ACTIVATED.c_str(), StaticActivatedSecondsPerFrame,
			StaticActivatedLoop, inf, textureManager, &defActivatedState, &instActivatedState))
		{
			DebugLogWarning("Couldn't make streetlight active anim state for ID \"" +
				std::to_string(activeFlatIndex) + "\".");
			return false;
		}

		addStateIfNotEmpty(outAnimDef, defIdleState);
		addStateIfNotEmpty(outAnimDef, defActivatedState);
		addStateIfNotEmpty(outAnimInst, instIdleState);
		addStateIfNotEmpty(outAnimInst, instActivatedState);
	}
	else
	{
		// General static entity animation.
		EntityAnimationDefinition::State defIdleState;
		EntityAnimationInstance::State instIdleState;
		if (!ArenaAnimUtils::tryMakeStaticEntityAnimState(flatIndex,
			EntityAnimationUtils::STATE_IDLE.c_str(), StaticIdleSecondsPerFrame, StaticIdleLoop,
			inf, textureManager, &defIdleState, &instIdleState))
		{
			DebugLogWarning("Couldn't make idle anim state for ID \"" +
				std::to_string(flatIndex) + "\".");
			return false;
		}

		addStateIfNotEmpty(outAnimDef, defIdleState);
		addStateIfNotEmpty(outAnimInst, instIdleState);
	}

	return true;
}

bool ArenaAnimUtils::tryMakeDynamicEntityCreatureAnims(int creatureID, const ExeData &exeData,
	TextureManager &textureManager, EntityAnimationDefinition *outAnimDef,
	EntityAnimationInstance *outAnimInst)
{
	// Basic states are idle/look/walk.
	EntityAnimationDefinition::State idleDefState;
	EntityAnimationInstance::State idleInstState;
	if (!ArenaAnimUtils::tryMakeDynamicEntityCreatureBasicAnimState(creatureID,
		EntityAnimationUtils::STATE_IDLE.c_str(), CreatureIdleSecondsPerFrame, CreatureIdleLoop,
		CreatureIdleIndices, exeData, textureManager, &idleDefState, &idleInstState))
	{
		DebugLogWarning("Couldn't make creature idle anim state for creature ID \"" +
			std::to_string(creatureID) + "\".");
	}

	EntityAnimationDefinition::State lookDefState;
	EntityAnimationInstance::State lookInstState;
	if (!ArenaAnimUtils::tryMakeDynamicEntityCreatureBasicAnimState(creatureID,
		EntityAnimationUtils::STATE_LOOK.c_str(), CreatureLookSecondsPerFrame, CreatureLookLoop,
		CreatureLookIndices, exeData, textureManager, &lookDefState, &lookInstState))
	{
		DebugLogWarning("Couldn't make creature look anim state for creature ID \"" +
			std::to_string(creatureID) + "\".");
	}

	EntityAnimationDefinition::State walkDefState;
	EntityAnimationInstance::State walkInstState;
	if (!ArenaAnimUtils::tryMakeDynamicEntityCreatureBasicAnimState(creatureID,
		EntityAnimationUtils::STATE_WALK.c_str(), CreatureWalkSecondsPerFrame, CreatureWalkLoop,
		CreatureWalkIndices, exeData, textureManager, &walkDefState, &walkInstState))
	{
		DebugLogWarning("Couldn't make creature walk anim state for creature ID \"" +
			std::to_string(creatureID) + "\".");
	}

	outAnimDef->addState(std::move(idleDefState));
	outAnimDef->addState(std::move(lookDefState));
	outAnimDef->addState(std::move(walkDefState));
	outAnimInst->addState(std::move(idleInstState));
	outAnimInst->addState(std::move(lookInstState));
	outAnimInst->addState(std::move(walkInstState));

	// Attack state.
	EntityAnimationDefinition::State attackDefState;
	EntityAnimationInstance::State attackInstState;
	if (!ArenaAnimUtils::tryMakeDynamicEntityCreatureAttackAnimState(creatureID, exeData,
		textureManager, &attackDefState, &attackInstState))
	{
		DebugLogWarning("Couldn't make creature attack anim for creature ID \"" +
			std::to_string(creatureID) + "\".");
		return false;
	}

	outAnimDef->addState(std::move(attackDefState));
	outAnimInst->addState(std::move(attackInstState));

	// Death state.
	EntityAnimationDefinition::State deathDefState;
	EntityAnimationInstance::State deathInstState;
	if (!ArenaAnimUtils::tryMakeDynamicEntityCreatureDeathAnimState(creatureID, exeData,
		textureManager, &deathDefState, &deathInstState))
	{
		DebugLogWarning("Couldn't make creature death anim for creature ID \"" +
			std::to_string(creatureID) + "\".");
		return false;
	}

	outAnimDef->addState(std::move(deathDefState));
	outAnimInst->addState(std::move(deathInstState));
	return true;
}

bool ArenaAnimUtils::tryMakeDynamicEntityHumanAnims(int charClassIndex, bool isMale,
	const CharacterClassLibrary &charClassLibrary, const INFFile &inf,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
	EntityAnimationDefinition *outAnimDef, EntityAnimationInstance *outAnimInst)
{
	// Basic states are idle and walk. Human enemies don't have look animations.
	EntityAnimationDefinition::State idleDefState;
	EntityAnimationInstance::State idleInstState;
	if (!ArenaAnimUtils::tryMakeDynamicEntityHumanBasicAnimState(charClassIndex, isMale,
		EntityAnimationUtils::STATE_IDLE.c_str(), HumanIdleSecondsPerFrame, HumanIdleLoop,
		HumanIdleIndices, charClassLibrary, binaryAssetLibrary, textureManager, &idleDefState,
		&idleInstState))
	{
		DebugLogWarning("Couldn't make human idle anim state for character class \"" +
			std::to_string(charClassIndex) + "\".");
		return false;
	}

	EntityAnimationDefinition::State walkDefState;
	EntityAnimationInstance::State walkInstState;
	if (!ArenaAnimUtils::tryMakeDynamicEntityHumanBasicAnimState(charClassIndex, isMale,
		EntityAnimationUtils::STATE_WALK.c_str(), HumanWalkSecondsPerFrame, HumanWalkLoop,
		HumanWalkIndices, charClassLibrary, binaryAssetLibrary, textureManager, &walkDefState,
		&walkInstState))
	{
		DebugLogWarning("Couldn't make human walk anim state for character class \"" +
			std::to_string(charClassIndex) + "\".");
		return false;
	}

	outAnimDef->addState(std::move(idleDefState));
	outAnimDef->addState(std::move(walkDefState));
	outAnimInst->addState(std::move(idleInstState));
	outAnimInst->addState(std::move(walkInstState));

	// Attack state.
	EntityAnimationDefinition::State attackDefState;
	EntityAnimationInstance::State attackInstState;
	if (!ArenaAnimUtils::tryMakeDynamicEntityHumanAttackAnimState(charClassIndex, isMale,
		charClassLibrary, binaryAssetLibrary, textureManager, &attackDefState, &attackInstState))
	{
		DebugLogWarning("Couldn't make human attack anim for character class \"" +
			std::to_string(charClassIndex) + "\".");
		return false;
	}

	outAnimDef->addState(std::move(attackDefState));
	outAnimInst->addState(std::move(attackInstState));

	// Death state.
	EntityAnimationDefinition::State deathDefState;
	EntityAnimationInstance::State deathInstState;
	if (!ArenaAnimUtils::tryMakeDynamicEntityHumanDeathAnimState(
		inf, textureManager, &deathDefState, &deathInstState))
	{
		DebugLogWarning("Couldn't make human death anim for character class \"" +
			std::to_string(charClassIndex) + "\".");
		return false;
	}

	outAnimDef->addState(std::move(deathDefState));
	outAnimInst->addState(std::move(deathInstState));
	return true;
}

bool ArenaAnimUtils::tryMakeDynamicEntityAnims(ArenaTypes::FlatIndex flatIndex,
	const std::optional<bool> &isMale, const INFFile &inf, const CharacterClassLibrary &charClassLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
	EntityAnimationDefinition *outAnimDef, EntityAnimationInstance *outAnimInst)
{
	DebugAssert(outAnimDef != nullptr);
	DebugAssert(outAnimInst != nullptr);

	const auto &exeData = binaryAssetLibrary.getExeData();
	const INFFile::FlatData &flatData = inf.getFlat(flatIndex);
	const std::optional<ArenaTypes::ItemIndex> &optItemIndex = flatData.itemIndex;
	if (!optItemIndex.has_value())
	{
		DebugLogWarning("Can't make dynamic entity anim states for flat \"" +
			std::to_string(flatIndex) + "\" without *ITEM index.");
		return false;
	}

	const ArenaTypes::ItemIndex itemIndex = *optItemIndex;

	bool isFinalBoss;
	const bool isCreature = ArenaAnimUtils::isCreatureIndex(itemIndex, &isFinalBoss);
	const bool isHuman = ArenaAnimUtils::isHumanEnemyIndex(itemIndex);

	if (isCreature)
	{
		const int creatureID = ArenaAnimUtils::getCreatureIDFromItemIndex(itemIndex);
		return ArenaAnimUtils::tryMakeDynamicEntityCreatureAnims(creatureID, exeData,
			textureManager, outAnimDef, outAnimInst);
	}
	else if (isHuman)
	{
		DebugAssert(isMale.has_value());
		const int charClassIndex = ArenaAnimUtils::getCharacterClassIndexFromItemIndex(itemIndex);
		return ArenaAnimUtils::tryMakeDynamicEntityHumanAnims(charClassIndex, *isMale,
			charClassLibrary, inf, binaryAssetLibrary, textureManager, outAnimDef, outAnimInst);
	}
	else
	{
		DebugCrash("Unrecognized *ITEM index \"" + std::to_string(itemIndex) + "\".");
		return false;
	}
}

bool ArenaAnimUtils::tryMakeCitizenAnims(ClimateType climateType, bool isMale,
	const ExeData &exeData, TextureManager &textureManager,
	EntityAnimationDefinition *outAnimDef, EntityAnimationInstance *outAnimInst)
{
	// Index into citizen animation filenames, depends on the climate and gender.
	const int citizenIndex = [climateType, isMale]()
	{
		if (isMale)
		{
			switch (climateType)
			{
			case ClimateType::Temperate:
				return 2;
			case ClimateType::Desert:
				return 1;
			case ClimateType::Mountain:
				return 0;
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

	EntityAnimationDefinition::State idleDefState;
	EntityAnimationInstance::State idleInstState;
	if (!ArenaAnimUtils::tryMakeDynamicEntityCitizenBasicAnimState(
		EntityAnimationUtils::STATE_IDLE.c_str(), CitizenIdleSecondsPerFrame, CitizenIdleLoop,
		citizenIndex, isMale, CitizenIdleIndices, exeData, textureManager, &idleDefState,
		&idleInstState))
	{
		DebugLogWarning("Couldn't make citizen idle anim state for citizen ID \"" +
			std::to_string(citizenIndex) + "\".");
	}

	EntityAnimationDefinition::State walkDefState;
	EntityAnimationInstance::State walkInstState;
	if (!ArenaAnimUtils::tryMakeDynamicEntityCitizenBasicAnimState(
		EntityAnimationUtils::STATE_WALK.c_str(), CitizenWalkSecondsPerFrame, CitizenWalkLoop,
		citizenIndex, isMale, CitizenWalkIndices, exeData, textureManager, &walkDefState,
		&walkInstState))
	{
		DebugLogWarning("Couldn't make citizen walk anim state for citizen ID \"" +
			std::to_string(citizenIndex) + "\".");
	}

	outAnimDef->addState(std::move(idleDefState));
	outAnimDef->addState(std::move(walkDefState));
	outAnimInst->addState(std::move(idleInstState));
	outAnimInst->addState(std::move(walkInstState));
	return true;
}

Palette ArenaAnimUtils::transformCitizenColors(int raceIndex, uint16_t seed, const Palette &palette,
	const ExeData &exeData)
{
	const std::array<uint8_t, 16> &colorBase = exeData.entities.citizenColorBase;

	// Clothes transformation.
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
				DebugAssertIndex(newPalette, oldIndex);
				DebugAssertIndex(palette, newIndex);
				newPalette[oldIndex] = palette[newIndex];
			}
		}
	}

	const std::array<uint8_t, 10> &skinColors = exeData.entities.citizenSkinColors;

	// Skin transformation, only if the given race should have its colors transformed.
	constexpr std::array<int, 9> RaceOffsets = { -1, 148, -1, 52, 192, -1, -1, 116, 148 };
	DebugAssertIndex(RaceOffsets, raceIndex);
	const int raceOffset = RaceOffsets[raceIndex];
	const bool hasTransformation = raceOffset != -1;
	if (hasTransformation)
	{
		for (int i = 0; i < 10; i++)
		{
			const int oldIndex = raceOffset + i;
			const int newIndex = skinColors[i];
			DebugAssertIndex(palette, oldIndex);
			DebugAssertIndex(newPalette, newIndex);
			newPalette[newIndex] = palette[oldIndex];
		}
	}

	return newPalette;
}
