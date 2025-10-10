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
		const int raceID = i;
		DebugAssertIndex(exeData.races.singularNames, raceID);
		DebugAssertIndex(exeData.races.pluralNames, raceID);
		const std::string &singularName = exeData.races.singularNames[raceID];
		const std::string &pluralName = exeData.races.pluralNames[raceID];

		double swimmingMoveSpeed = PlayerConstants::SWIMMING_MOVE_SPEED;
		if (raceID == 7)
		{
			swimmingMoveSpeed = PlayerConstants::MOVE_SPEED;
		}

		double climbingSpeedScale = 1.0;
		if (raceID == 6)
		{
			climbingSpeedScale = 4.0;
		}

		const TextureAsset maleCharSheetBodyTextureAsset(ArenaPortraitUtils::getBody(true, raceID));
		const std::string maleCharSheetHeadsFilename = ArenaPortraitUtils::getHeads(true, raceID, false);
		const std::string maleGameUiHeadsFilename = ArenaPortraitUtils::getHeads(true, raceID, true);
		const TextureAsset femaleCharSheetBodyTextureAsset(ArenaPortraitUtils::getBody(false, raceID));
		const std::string femaleCharSheetHeadsFilename = ArenaPortraitUtils::getHeads(false, raceID, false);
		const std::string femaleGameUiHeadsFilename = ArenaPortraitUtils::getHeads(false, raceID, true);

		CharacterRaceDefinition raceDef;
		raceDef.init(raceID, singularName.c_str(), pluralName.c_str(), swimmingMoveSpeed, climbingSpeedScale, maleCharSheetBodyTextureAsset,
			maleCharSheetHeadsFilename, maleGameUiHeadsFilename, femaleCharSheetBodyTextureAsset, femaleCharSheetHeadsFilename, femaleGameUiHeadsFilename);
		this->defs.emplace_back(std::move(raceDef));
	}
}

int CharacterRaceLibrary::getDefinitionCount() const
{
	return static_cast<int>(this->defs.size());
}

const CharacterRaceDefinition &CharacterRaceLibrary::getDefinition(int index) const
{
	DebugAssertIndex(this->defs, index);
	return this->defs[index];
}

bool CharacterRaceLibrary::findDefinitionIndexIf(const Predicate &predicate, int *outIndex) const
{
	for (int i = 0; i < static_cast<int>(this->defs.size()); i++)
	{
		const CharacterRaceDefinition &def = this->defs[i];
		if (predicate(def))
		{
			*outIndex = i;
			return true;
		}
	}

	return false;
}

bool CharacterRaceLibrary::tryGetDefinitionIndex(const CharacterRaceDefinition &def, int *outIndex) const
{
	for (int i = 0; i < static_cast<int>(this->defs.size()); i++)
	{
		const CharacterRaceDefinition &charClassDef = this->defs[i];
		if (charClassDef.provinceID == def.provinceID)
		{
			*outIndex = i;
			return true;
		}
	}

	return false;
}
