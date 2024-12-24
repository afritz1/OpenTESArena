#include <cstring>
#include <optional>

#include "ArenaWeaponUtils.h"
#include "WeaponAnimationLibrary.h"
#include "../Assets/ExeData.h"
#include "../Assets/TextureManager.h"
#include "../Interface/GameWorldUiView.h"
#include "../Items/ArenaItemUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/String.h"

namespace
{
	void AddAnimationStates(WeaponAnimationDefinition &animDef, const std::string &animFilename, BufferView<const ArenaWeaponUtils::AnimationStateInfo> animStateInfos, TextureManager &textureManager)
	{
		const std::optional<TextureFileMetadataID> textureFileMetadataID = textureManager.tryGetMetadataID(animFilename.c_str());
		if (!textureFileMetadataID.has_value())
		{
			DebugLogError("No texture metadata available for \"" + animFilename + "\".");
			return;
		}

		const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*textureFileMetadataID);

		for (const ArenaWeaponUtils::AnimationStateInfo &animStateInfo : animStateInfos)
		{
			const std::string &stateName = animStateInfo.name;
			const BufferView<const int> frameIndices = animStateInfo.frames;
			const double stateSeconds = (static_cast<double>(frameIndices.getCount()) / ArenaWeaponUtils::FRAMES_PER_SECOND) / animStateInfo.timeScale;
			const int stateIndex = animDef.addState(stateName.c_str(), stateSeconds);
			const WeaponAnimationDefinitionState &state = animDef.states[stateIndex];

			for (const int frameIndex : frameIndices)
			{
				const TextureAsset textureAsset = GameWorldUiView::getWeaponAnimTextureAsset(animFilename, frameIndex);
				const int width = textureFileMetadata.getWidth(frameIndex);
				const int height = textureFileMetadata.getHeight(frameIndex);
				const Int2 offset = textureFileMetadata.getOffset(frameIndex);
				animDef.addFrame(stateIndex, textureAsset, width, height, offset.x, offset.y);
			}
		}
	}

	std::string GetAnimationFilename(int weaponID, const ExeData &exeData)
	{
		int filenameIndex;
		if (weaponID >= 0)
		{
			DebugAssertIndex(ArenaWeaponUtils::FilenameIndices, weaponID);
			filenameIndex = ArenaWeaponUtils::FilenameIndices[weaponID];
		}
		else
		{
			filenameIndex = ArenaWeaponUtils::FistsFilenameIndex;
		}

		const auto &animFilenames = exeData.equipment.weaponAnimationFilenames;
		DebugAssertIndex(animFilenames, filenameIndex);
		const std::string &filename = animFilenames[filenameIndex];
		return String::toUppercase(filename);
	}
}

void WeaponAnimationLibrary::init(const ExeData &exeData, TextureManager &textureManager)
{
	const int fistsWeaponId = ArenaItemUtils::FistsWeaponID;
	const std::string fistsAnimFilename = GetAnimationFilename(fistsWeaponId, exeData);
	WeaponAnimationDefinition fistsAnimDef;
	AddAnimationStates(fistsAnimDef, fistsAnimFilename, ArenaWeaponUtils::FistsAnimationStateInfos, textureManager);
	this->animDefs.emplace(fistsWeaponId, std::move(fistsAnimDef));

	for (int i = 0; i < ArenaWeaponUtils::MeleeWeaponTypeCount; i++)
	{
		const int weaponID = i;
		const std::string weaponAnimFilename = GetAnimationFilename(weaponID, exeData);
		WeaponAnimationDefinition animDef;
		AddAnimationStates(animDef, weaponAnimFilename, ArenaWeaponUtils::MeleeAnimationStateInfos, textureManager);
		this->animDefs.emplace(weaponID, animDef);
	}

	for (int i = 0; i < ArenaWeaponUtils::RangedWeaponTypeCount; i++)
	{
		const int weaponID = ArenaWeaponUtils::MeleeWeaponTypeCount + i;
		const std::string weaponAnimFilename = GetAnimationFilename(weaponID, exeData);
		WeaponAnimationDefinition animDef;
		AddAnimationStates(animDef, weaponAnimFilename, ArenaWeaponUtils::BowAnimationStateInfos, textureManager);
		this->animDefs.emplace(weaponID, animDef);
	}
}

const WeaponAnimationDefinition &WeaponAnimationLibrary::getDefinition(int animDefID) const
{
	const auto iter = this->animDefs.find(animDefID);
	DebugAssert(iter != this->animDefs.end());
	return iter->second;
}
