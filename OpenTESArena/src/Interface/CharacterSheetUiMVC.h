#pragma once

#include <string>

#include "../Math/Vector2.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../Stats/PrimaryAttribute.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/UiPivotType.h"
#include "../Utilities/Color.h"

struct TextureAsset;

class Game;
class Renderer;
class TextureManager;

// Every texture currently required by the player's paper doll. All textures are owned by UiManager.
struct CharacterEquipmentPresentationState
{
	UiTextureID bodyTextureID; // Includes race and gender.
	UiTextureID shirtTextureID;
	UiTextureID pantsTextureID;
	UiTextureID headTextureID;
	Int2 bodyPosition;
	Int2 shirtPosition;
	Int2 pantsPosition;
	Int2 headPosition;

	CharacterEquipmentPresentationState();
};

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

	CharacterEquipmentPresentationState getEquipmentPresentationState(Game &game);
}

namespace CharacterEquipmentUiView
{
	constexpr Int2 ItemDetailCenterPoint(84, 164);
	constexpr UiPivotType ItemDetailPivotType = UiPivotType::Middle;
	constexpr TextAlignment ItemDetailTextAlignment = TextAlignment::MiddleCenter;

	const std::string ItemDetailFontName = ArenaFontName::Arena;
	constexpr Color ItemDetailDefaultTextColor(199, 199, 199);
	constexpr Color ItemDetailErrorTextColor(199, 32, 0);
	constexpr int ItemDetailLineSpacing = 1;

	UiTextureID allocUpDownButtonTexture(TextureManager &textureManager, Renderer &renderer);
}
