#ifndef CHARACTER_CREATION_UI_MODEL_H
#define CHARACTER_CREATION_UI_MODEL_H

#include <optional>
#include <string>
#include <vector>

#include "../Math/Vector2.h"
#include "../Stats/PrimaryAttribute.h"
#include "../UI/TextRenderUtils.h"

class ArenaRandom;
class Game;

struct CharacterClassDefinition;

namespace CharacterCreationUiModel
{
	std::string getPlayerName(Game &game);
	std::string getPlayerRaceName(Game &game);
	std::string getPlayerClassName(Game &game);
	const PrimaryAttributes &getPlayerAttributes(Game &game);
	std::string getPlayerLevel(Game &game);
	std::string getPlayerExperience(Game &game);
}

namespace ChooseClassCreationUiModel
{
	std::string getTitleText(Game &game);
	std::string getGenerateButtonText(Game &game);
	std::string getGenerateButtonTooltipText();
	std::string getSelectButtonText(Game &game);
	std::string getSelectButtonTooltipText();
}

namespace ChooseClassUiModel
{
	std::string getTitleText(Game &game);
	std::string getArmorTooltipText(const CharacterClassDefinition &charClassDef);
	std::string getShieldTooltipText(const CharacterClassDefinition &charClassDef);
	std::string getWeaponTooltipText(const CharacterClassDefinition &charClassDef, Game &game);
	std::string getFullTooltipText(const CharacterClassDefinition &charClassDef, Game &game);
}

namespace ChooseGenderUiModel
{
	std::string getTitleText(Game &game);
	std::string getMaleText(Game &game);
	std::string getFemaleText(Game &game);
}

namespace ChooseNameUiModel
{
	std::string getTitleText(Game &game);
	bool isCharacterAccepted(char c);
}

namespace ChooseRaceUiModel
{
	std::string getTitleText(Game &game);
	std::string getProvinceConfirmTitleText(Game &game);
	std::string getProvinceConfirmYesText(Game &game);
	std::string getProvinceConfirmNoText(Game &game);
	std::string getProvinceConfirmedFirstText(Game &game);
	std::string getProvinceConfirmedSecondText(Game &game);
	std::string getProvinceConfirmedThirdText(Game &game);
	std::string getProvinceConfirmedFourthText(Game &game);
}

namespace ChooseAttributesUiModel
{
	// Based on reversed wiki values
	constexpr int PrimaryAttributeRandomMax = 20;
	constexpr int BonusPointsRandomMax = 25;

	int rollClassic(int n, ArenaRandom &random);

	std::string getPlayerBonusDamage(Game &game);
	std::string getPlayerMaxWeight(Game &game);
	std::string getPlayerMagicDefense(Game &game);
	std::string getPlayerBonusToHit(Game &game);
	std::string getPlayerBonusToDefend(Game &game);
	std::string getPlayerBonusToHealth(Game &game);
	std::string getPlayerHealMod(Game &game);
	std::string getPlayerCharisma(Game &game);
	std::string getPlayerGold(Game &game);

	std::string getInitialText(Game &game);

	std::string getMessageBoxTitleText(Game &game);
	std::string getMessageBoxSaveText(Game &game);
	std::string getMessageBoxRerollText(Game &game);
	std::vector<TextRenderColorOverrideInfoEntry> getMessageBoxSaveColorOverrides(Game &game);
	std::vector<TextRenderColorOverrideInfoEntry> getMessageBoxRerollColorOverrides(Game &game);

	std::string getBonusPointsRemainingText(Game &game);
	std::string getAppearanceText(Game &game);
}

#endif
