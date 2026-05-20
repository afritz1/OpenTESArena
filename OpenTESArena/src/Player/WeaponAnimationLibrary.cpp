#include <cstring>
#include <optional>

#include "ArenaWeaponUtils.h"
#include "WeaponAnimationLibrary.h"
#include "../Assets/TextureManager.h"
#include "../Interface/GameWorldUiMVC.h"
#include "../Items/ArenaItemUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/Span.h"
#include "components/utilities/String.h"

namespace
{
	void AddWeaponAnimationStates(WeaponAnimationDefinition &animDef, const std::string &animFilename,
		Span<const ArenaWeaponAnimationStateInfo> animStateInfos, TextureManager &textureManager)
	{
		const std::optional<TextureFileMetadataID> textureFileMetadataID = textureManager.tryGetMetadataID(animFilename.c_str());
		if (!textureFileMetadataID.has_value())
		{
			DebugLogErrorFormat("No texture metadata available for \"%s\".", animFilename.c_str());
			return;
		}

		const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*textureFileMetadataID);

		for (const ArenaWeaponAnimationStateInfo &animStateInfo : animStateInfos)
		{
			const std::string &stateName = animStateInfo.name;
			const Span<const int> frameIndices = animStateInfo.frames;
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
}

void WeaponAnimationLibrary::init(const ExeData &exeData, TextureManager &textureManager)
{
	const int fistsWeaponID = ArenaItemUtils::FistsWeaponID;
	const std::string fistsAnimFilename = ArenaWeaponUtils::getAnimationFilename(fistsWeaponID, exeData);
	WeaponAnimationDefinition fistsAnimDef;
	AddWeaponAnimationStates(fistsAnimDef, fistsAnimFilename, ArenaWeaponUtils::FistsAnimationStateInfos, textureManager);
	this->animDefs.emplace(fistsWeaponID, std::move(fistsAnimDef));

	for (const int meleeWeaponID : ArenaItemUtils::MeleeWeaponIDs)
	{
		const std::string weaponAnimFilename = ArenaWeaponUtils::getAnimationFilename(meleeWeaponID, exeData);
		WeaponAnimationDefinition animDef;
		AddWeaponAnimationStates(animDef, weaponAnimFilename, ArenaWeaponUtils::MeleeAnimationStateInfos, textureManager);
		this->animDefs.emplace(meleeWeaponID, animDef);
	}

	for (const int rangedWeaponID : ArenaItemUtils::RangedWeaponIDs)
	{
		const std::string weaponAnimFilename = ArenaWeaponUtils::getAnimationFilename(rangedWeaponID, exeData);
		WeaponAnimationDefinition animDef;
		AddWeaponAnimationStates(animDef, weaponAnimFilename, ArenaWeaponUtils::BowAnimationStateInfos, textureManager);
		this->animDefs.emplace(rangedWeaponID, animDef);
	}
}

const WeaponAnimationDefinition &WeaponAnimationLibrary::getDefinition(WeaponAnimationDefinitionID weaponAnimDefID) const
{
	const auto iter = this->animDefs.find(weaponAnimDefID);
	DebugAssert(iter != this->animDefs.end());
	return iter->second;
}
