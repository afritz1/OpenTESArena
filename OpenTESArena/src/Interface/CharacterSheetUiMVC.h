#pragma once

#include <string>

#include "../Items/ItemDefinition.h"
#include "../Math/Vector2.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../Stats/PrimaryAttribute.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/UiContext.h"
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
	UiTextureID chestArmorTextureID;
	UiTextureID handsArmorTextureID;
	UiTextureID leftShoulderArmorTextureID;
	UiTextureID rightShoulderArmorTextureID;
	UiTextureID legsArmorTextureID;
	UiTextureID feetArmorTextureID;
	UiTextureID shieldTextureID;
	UiTextureID weaponTextureID;

	Int2 bodyPosition;
	Int2 shirtPosition;
	Int2 pantsPosition;
	Int2 headPosition;
	Int2 chestArmorPosition;
	Int2 handsArmorPosition;
	Int2 leftShoulderArmorPosition;
	Int2 rightShoulderArmorPosition;
	Int2 legsArmorPosition;
	Int2 feetArmorPosition;
	Int2 shieldPosition;
	Int2 weaponPosition;

	bool isHeadArmorEquipped;
	bool isChestArmorEquipped;
	bool isHandsArmorEquipped;
	bool isLeftShoulderArmorEquipped;
	bool isRightShoulderArmorEquipped;
	bool isLegsArmorEquipped;
	bool isFeetArmorEquipped;
	bool isShieldEquipped;
	bool isWeaponEquipped;

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

	ItemDefinitionID getEquippedArmorItemDefID(ArenaArmorTypeID armorTypeID, Game &game);
	ItemDefinitionID getEquippedHeadArmorItemDefID(Game &game);
	ItemDefinitionID getEquippedChestArmorItemDefID(Game &game);
	ItemDefinitionID getEquippedHandsArmorItemDefID(Game &game);
	ItemDefinitionID getEquippedLeftShoulderArmorItemDefID(Game &game);
	ItemDefinitionID getEquippedRightShoulderArmorItemDefID(Game &game);
	ItemDefinitionID getEquippedLegsArmorItemDefID(Game &game);
	ItemDefinitionID getEquippedFeetArmorItemDefID(Game &game);
	ItemDefinitionID getEquippedShieldItemDefID(Game &game);
	ItemDefinitionID getEquippedWeaponItemDefID(Game &game);
}

namespace CharacterSheetUiView
{
	static constexpr int BodyDrawOrder = 1;
	static constexpr int HeadDrawOrder = 4;
	static constexpr int ShirtDrawOrder = 5;
	static constexpr int PantsDrawOrder = 2;
	static constexpr int HeadArmorDrawOrder = 7;
	static constexpr int ChestArmorDrawOrder = 6;
	static constexpr int HandsArmorDrawOrder = 8;
	static constexpr int LeftShoulderArmorDrawOrder = 10;
	static constexpr int RightShoulderArmorDrawOrder = 9;
	static constexpr int LegsArmorDrawOrder = 3;
	static constexpr int FeetArmorDrawOrder = 11;
	static constexpr int WeaponDrawOrder = 12;
	static constexpr int ShieldDrawOrder = 13;

	Int2 getBodyOffset(Game &game);
	Int2 getHeadOffset(Game &game);
	Int2 getShirtOffset(Game &game);
	Int2 getPantsOffset(Game &game);

	Int2 getEquipmentOffset(const TextureAsset &textureAsset, TextureManager &textureManager);
	Int2 getHeadArmorOffset(Game &game);
	Int2 getChestArmorOffset(Game &game);
	Int2 getHandsArmorOffset(Game &game);
	Int2 getLeftShoulderArmorOffset(Game &game);
	Int2 getRightShoulderArmorOffset(Game &game);
	Int2 getLegsArmorOffset(Game &game);
	Int2 getFeetArmorOffset(Game &game);
	Int2 getShieldOffset(Game &game);
	Int2 getWeaponOffset(Game &game);

	TextureAsset getPaletteTextureAsset();
	TextureAsset getBodyTextureAsset(Game &game);
	TextureAsset getHeadTextureAsset(Game &game);
	TextureAsset getShirtTextureAsset(Game &game);
	TextureAsset getPantsTextureAsset(Game &game);
	TextureAsset getHeadArmorTextureAsset(Game &game);
	TextureAsset getChestArmorTextureAsset(Game &game);
	TextureAsset getHandsArmorTextureAsset(Game &game);
	TextureAsset getLeftShoulderArmorTextureAsset(Game &game);
	TextureAsset getRightShoulderArmorTextureAsset(Game &game);
	TextureAsset getLegsArmorTextureAsset(Game &game);
	TextureAsset getFeetArmorTextureAsset(Game &game);
	TextureAsset getShieldTextureAsset(Game &game);
	TextureAsset getWeaponTextureAsset(Game &game);

	CharacterEquipmentPresentationState getEquipmentPresentationState(Game &game);

	// Updates all player body and equipment UI elements, creating if necessary.
	void createOrUpdateEquipmentUiElements(const char *elementPrefix, UiContextInstanceID contextInstID, Game &game);
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
}
