#ifndef CHARACTER_SHEET_UI_MVC_H
#define CHARACTER_SHEET_UI_MVC_H

#include <string>

#include "../Math/Vector2.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../Stats/PrimaryAttribute.h"

struct TextureAsset;

class Game;
class Renderer;
class TextureManager;

namespace CharacterSheetUiModel
{
	// For UI elements.
	constexpr const char *DerivedAttributeUiNames[] =
	{
		"BonusDamage",
		"MaxWeight",
		"MagicDefense",
		"BonusToHit",
		"BonusToDefend",
		"BonusToHealth",
		"HealMod",
		"BonusToCharisma",
	};

	std::string getStatusValueCurrentAndMaxString(double currentValue, double maxValue);
	std::string getDerivedAttributeDisplayString(int value);

	std::string getPlayerName(Game &game);
	std::string getPlayerRaceName(Game &game);
	std::string getPlayerClassName(Game &game);
	DerivedAttributes getPlayerDerivedAttributes(Game &game);
	std::string getPlayerExperience(Game &game);
	std::string getPlayerLevel(Game &game);
	std::string getPlayerHealth(Game &game);
	std::string getPlayerStamina(Game &game);
	std::string getPlayerSpellPoints(Game &game);
	std::string getPlayerGold(Game &game);
}

namespace CharacterSheetUiView
{
	Int2 getBodyOffset(Game &game);
	Int2 getHeadOffset(Game &game);
	Int2 getShirtOffset(Game &game);
	Int2 getPantsOffset(Game &game);

	TextureAsset getPaletteTextureAsset();
	TextureAsset getBodyTextureAsset(Game &game);
	TextureAsset getHeadTextureAsset(Game &game);
	TextureAsset getShirtTextureAsset(Game &game);
	TextureAsset getPantsTextureAsset(Game &game);

	UiTextureID allocBodyTexture(Game &game);
	UiTextureID allocShirtTexture(Game &game);
	UiTextureID allocPantsTexture(Game &game);
	UiTextureID allocHeadTexture(Game &game);
}

namespace CharacterEquipmentUiView
{
	UiTextureID allocUpDownButtonTexture(TextureManager &textureManager, Renderer &renderer);
}

#endif
