#ifndef CHARACTER_CREATION_UI_MODEL_H
#define CHARACTER_CREATION_UI_MODEL_H

#include <optional>
#include <string>
#include <vector>

#include "../Math/Vector2.h"
#include "../UI/TextRenderUtils.h"

class CharacterClassDefinition;
class Game;

namespace CharacterCreationUiModel
{
	std::string getPlayerName(Game &game);
	std::string getPlayerRaceName(Game &game);
	std::string getPlayerClassName(Game &game);
	std::string getPlayerStrengthText(Game &game);
	std::string getPlayerIntelligenceText(Game &game);
	std::string getPlayerWillpowerText(Game &game);
	std::string getPlayerAgilityText(Game &game);
	std::string getPlayerSpeedText(Game &game);
	std::string getPlayerEnduranceText(Game &game);
	std::string getPlayerPersonalityText(Game &game);
	std::string getPlayerLuckText(Game &game);
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
	std::string getProvinceTooltipText(Game &game, int provinceID);
	std::string getProvinceConfirmedFirstText(Game &game);
	std::string getProvinceConfirmedSecondText(Game &game);
	std::string getProvinceConfirmedThirdText(Game &game);
	std::string getProvinceConfirmedFourthText(Game &game);
}

namespace ChooseAttributesUiModel
{
	std::string getInitialText(Game &game);

	std::string getMessageBoxTitleText(Game &game);
	std::string getMessageBoxSaveText(Game &game);
	std::string getMessageBoxRerollText(Game &game);
	std::vector<TextRenderUtils::ColorOverrideInfo::Entry> getMessageBoxSaveColorOverrides(Game &game);
	std::vector<TextRenderUtils::ColorOverrideInfo::Entry> getMessageBoxRerollColorOverrides(Game &game);

	std::string getAppearanceText(Game &game);
}

#endif
