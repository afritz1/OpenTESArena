#include "CharacterRaceLibrary.h"
#include "../Assets/ArenaPortraitUtils.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/ExeData.h"
#include "../Player/Player.h"

#include "components/debug/Debug.h"

void CharacterRaceLibrary::init(const ExeData &exeData)
{
	constexpr int playableRaceCount = CityDataFile::PROVINCE_COUNT - 1;
	for (int i = 0; i < playableRaceCount; i++)
	{
		const int provinceID = i;
		const int originalRaceIndex = i;
		DebugAssertIndex(exeData.races.singularNames, originalRaceIndex);
		DebugAssertIndex(exeData.races.pluralNames, originalRaceIndex);
		const std::string &singularName = exeData.races.singularNames[originalRaceIndex];
		const std::string &pluralName = exeData.races.pluralNames[originalRaceIndex];

		double swimmingMoveSpeed = PlayerConstants::SWIMMING_MOVE_SPEED;
		double swimmingStaminaLossMultiplier = PlayerConstants::SWIMMING_STAMINA_LOSS_MULTIPLIER;
		if (originalRaceIndex == 7)
		{
			swimmingMoveSpeed = PlayerConstants::MOVE_SPEED;
			swimmingStaminaLossMultiplier = 12.0;
		}

		double climbingSpeedScale = 1.0;
		if (originalRaceIndex == 6)
		{
			climbingSpeedScale = 4.0;
		}

		const TextureAsset maleCharSheetBodyTextureAsset(ArenaPortraitUtils::getBody(true, originalRaceIndex));
		const std::string maleCharSheetHeadsFilename = ArenaPortraitUtils::getHeads(true, originalRaceIndex, false);
		const std::string maleGameUiHeadsFilename = ArenaPortraitUtils::getHeads(true, originalRaceIndex, true);
		const TextureAsset femaleCharSheetBodyTextureAsset(ArenaPortraitUtils::getBody(false, originalRaceIndex));
		const std::string femaleCharSheetHeadsFilename = ArenaPortraitUtils::getHeads(false, originalRaceIndex, false);
		const std::string femaleGameUiHeadsFilename = ArenaPortraitUtils::getHeads(false, originalRaceIndex, true);

		CharacterRaceDefinition raceDef;
		raceDef.init(provinceID, originalRaceIndex, singularName.c_str(), pluralName.c_str(), swimmingMoveSpeed, swimmingStaminaLossMultiplier,
			climbingSpeedScale, maleCharSheetBodyTextureAsset, maleCharSheetHeadsFilename, maleGameUiHeadsFilename, femaleCharSheetBodyTextureAsset,
			femaleCharSheetHeadsFilename, femaleGameUiHeadsFilename);
		this->defs.emplace_back(std::move(raceDef));
	}
}

int CharacterRaceLibrary::getDefinitionCount() const
{
	return static_cast<int>(this->defs.size());
}

const CharacterRaceDefinition &CharacterRaceLibrary::getDefinition(CharacterRaceDefinitionID id) const
{
	DebugAssertIndex(this->defs, id);
	return this->defs[id];
}

CharacterRaceDefinitionID CharacterRaceLibrary::findDefinitionIdIf(const CharacterRacePredicate &predicate) const
{
	for (int i = 0; i < static_cast<int>(this->defs.size()); i++)
	{
		const CharacterRaceDefinition &def = this->defs[i];
		if (predicate(def))
		{
			return i;
		}
	}

	return -1;
}

CharacterRaceDefinitionID CharacterRaceLibrary::tryGetDefinitionID(const CharacterRaceDefinition &def) const
{
	for (int i = 0; i < static_cast<int>(this->defs.size()); i++)
	{
		const CharacterRaceDefinition &charClassDef = this->defs[i];
		if (charClassDef.provinceID == def.provinceID)
		{
			return i;
		}
	}

	return -1;
}
