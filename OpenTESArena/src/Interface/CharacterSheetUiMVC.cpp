#include <cmath>
#include <cstring>
#include <optional>

#include "CharacterSheetUiMVC.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaPortraitUtils.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureManager.h"
#include "../Game/Game.h"
#include "../Items/ItemLibrary.h"
#include "../Player/Player.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../Rendering/ArenaRenderUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

CharacterEquipmentPresentationState::CharacterEquipmentPresentationState()
{
	this->bodyTextureID = -1;
	this->shirtTextureID = -1;
	this->pantsTextureID = -1;
	this->headTextureID = -1;
	this->chestArmorTextureID = -1;
	this->handsArmorTextureID = -1;
	this->leftShoulderArmorTextureID = -1;
	this->rightShoulderArmorTextureID = -1;
	this->legsArmorTextureID = -1;
	this->feetArmorTextureID = -1;
	this->shieldTextureID = -1;
	this->weaponTextureID = -1;
	this->isHeadArmorEquipped = false;
	this->isChestArmorEquipped = false;
	this->isHandsArmorEquipped = false;
	this->isLeftShoulderArmorEquipped = false;
	this->isRightShoulderArmorEquipped = false;
	this->isLegsArmorEquipped = false;
	this->isFeetArmorEquipped = false;
	this->isShieldEquipped = false;
	this->isWeaponEquipped = false;
}

std::string CharacterSheetUiModel::getStatusValueCurrentAndMaxString(double currentValue, double maxValue)
{
	const int currentInt = static_cast<int>(std::round(currentValue));
	const int maxInt = static_cast<int>(std::round(maxValue));
	return String::format("%d/%d", currentInt, maxInt);
}

std::string CharacterSheetUiModel::getDerivedAttributeDisplayString(int value)
{
	const char *signString = "";
	if (value >= 0)
	{
		signString = "+";
	}

	return String::format("%s%d", signString, value);
}

std::string CharacterSheetUiModel::getPlayerName(Game &game)
{
	const Player &player = game.player;
	return player.displayName;
}

std::string CharacterSheetUiModel::getPlayerRaceName(Game &game)
{
	const Player &player = game.player;
	const CharacterRaceLibrary &characterRaceLibrary = CharacterRaceLibrary::getInstance();
	const CharacterRaceDefinition &characterRaceDefinition = characterRaceLibrary.getDefinition(player.raceID);
	return characterRaceDefinition.singularName;
}

std::string CharacterSheetUiModel::getPlayerClassName(Game &game)
{
	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const Player &player = game.player;
	const int defID = player.charClassDefID;
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(defID);
	return charClassDef.name;
}

DerivedAttributes CharacterSheetUiModel::getPlayerDerivedAttributes(Game &game)
{
	return ArenaPlayerUtils::calculateTotalDerivedBonuses(game.player.primaryAttributes);
}

std::string CharacterSheetUiModel::getPlayerExperience(Game &game)
{
	const Player &player = game.player;
	return std::to_string(player.experience);
}

std::string CharacterSheetUiModel::getPlayerLevel(Game &game)
{
	const Player &player = game.player;
	return std::to_string(player.level);
}

std::string CharacterSheetUiModel::getPlayerHealth(Game &game)
{
	const Player &player = game.player;
	return CharacterSheetUiModel::getStatusValueCurrentAndMaxString(player.currentHealth, player.maxHealth);
}

std::string CharacterSheetUiModel::getPlayerStamina(Game &game)
{
	const Player &player = game.player;
	return CharacterSheetUiModel::getStatusValueCurrentAndMaxString(player.currentStamina, player.maxStamina);
}

std::string CharacterSheetUiModel::getPlayerSpellPoints(Game &game)
{
	const Player &player = game.player;
	return CharacterSheetUiModel::getStatusValueCurrentAndMaxString(player.currentSpellPoints, player.maxSpellPoints);
}

std::string CharacterSheetUiModel::getPlayerGold(Game &game)
{
	const Player &player = game.player;
	return std::to_string(player.gold);
}

ItemDefinitionID CharacterSheetUiModel::getEquippedArmorItemDefID(ArenaArmorTypeID armorTypeID, Game &game)
{
	const Player &player = game.player;
	const ItemInventory &itemInventory = player.inventory;

	int inventorySlotIndex;
	const bool success = itemInventory.findFirstValidSlotIf(
		[armorTypeID](const ItemInstance &itemInst)
	{
		if (!itemInst.isEquipped)
		{
			return false;
		}

		const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(itemInst.defID);
		if (itemDef.type != ItemType::Armor)
		{
			return false;
		}

		return itemDef.armor.typeID == armorTypeID;
	}, &inventorySlotIndex);

	if (!success)
	{
		return -1;
	}

	return itemInventory.getSlot(inventorySlotIndex).defID;
}

ItemDefinitionID CharacterSheetUiModel::getEquippedHeadArmorItemDefID(Game &game)
{
	constexpr ArenaArmorTypeID headArmorTypeID = 5;
	return CharacterSheetUiModel::getEquippedArmorItemDefID(headArmorTypeID, game);
}

ItemDefinitionID CharacterSheetUiModel::getEquippedChestArmorItemDefID(Game &game)
{
	constexpr ArenaArmorTypeID chestArmorTypeID = 0;
	return CharacterSheetUiModel::getEquippedArmorItemDefID(chestArmorTypeID, game);
}

ItemDefinitionID CharacterSheetUiModel::getEquippedHandsArmorItemDefID(Game &game)
{
	constexpr ArenaArmorTypeID handsArmorTypeID = 1;
	return CharacterSheetUiModel::getEquippedArmorItemDefID(handsArmorTypeID, game);
}

ItemDefinitionID CharacterSheetUiModel::getEquippedLeftShoulderArmorItemDefID(Game &game)
{
	constexpr ArenaArmorTypeID leftShoulderArmorTypeID = 3;
	return CharacterSheetUiModel::getEquippedArmorItemDefID(leftShoulderArmorTypeID, game);
}

ItemDefinitionID CharacterSheetUiModel::getEquippedRightShoulderArmorItemDefID(Game &game)
{
	constexpr ArenaArmorTypeID rightShoulderArmorTypeID = 4;
	return CharacterSheetUiModel::getEquippedArmorItemDefID(rightShoulderArmorTypeID, game);
}

ItemDefinitionID CharacterSheetUiModel::getEquippedLegsArmorItemDefID(Game &game)
{
	constexpr ArenaArmorTypeID legsArmorTypeID = 2;
	return CharacterSheetUiModel::getEquippedArmorItemDefID(legsArmorTypeID, game);
}

ItemDefinitionID CharacterSheetUiModel::getEquippedFeetArmorItemDefID(Game &game)
{
	constexpr ArenaArmorTypeID feetArmorTypeID = 6;
	return CharacterSheetUiModel::getEquippedArmorItemDefID(feetArmorTypeID, game);
}

ItemDefinitionID CharacterSheetUiModel::getEquippedShieldItemDefID(Game &game)
{
	const Player &player = game.player;
	const ItemInventory &itemInventory = player.inventory;

	int inventorySlotIndex;
	const bool success = itemInventory.findFirstValidSlotIf(
		[](const ItemInstance &itemInst)
	{
		if (!itemInst.isEquipped)
		{
			return false;
		}

		const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(itemInst.defID);
		if (itemDef.type != ItemType::Shield)
		{
			return false;			
		}

		const ArenaArmorTypeID armorTypeID = itemDef.shield.armorTypeID;
		return (armorTypeID >= 7) && (armorTypeID <= 11);
	}, &inventorySlotIndex);

	if (!success)
	{
		return -1;
	}

	return itemInventory.getSlot(inventorySlotIndex).defID;
}

ItemDefinitionID CharacterSheetUiModel::getEquippedWeaponItemDefID(Game &game)
{
	const Player &player = game.player;
	return player.getEquippedWeaponItemDefID();
}

Int2 CharacterSheetUiView::getBodyOffset(Game &game)
{
	const TextureAsset textureAsset = CharacterSheetUiView::getBodyTextureAsset(game);

	TextureManager &textureManager = game.textureManager;
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for body \"" + textureAsset.filename + "\".");
	}

	const TextureBuilder &bodyTexture = textureManager.getTextureBuilderHandle(*textureBuilderID);
	return Int2(ArenaRenderUtils::SCREEN_WIDTH - bodyTexture.width, 0);
}

Int2 CharacterSheetUiView::getHeadOffset(Game &game)
{
	const ItemDefinitionID equippedHelmetItemDefID = CharacterSheetUiModel::getEquippedHeadArmorItemDefID(game);
	if (equippedHelmetItemDefID >= 0)
	{
		return CharacterSheetUiView::getHeadArmorOffset(game);
	}

	const TextureAsset headTextureAsset = CharacterSheetUiView::getHeadTextureAsset(game);

	TextureManager &textureManager = game.textureManager;
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(headTextureAsset.filename.c_str());
	if (!metadataID.has_value())
	{
		DebugCrashFormat("Couldn't get texture file metadata for \"%s\".", headTextureAsset.filename.c_str());
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	return textureFileMetadata.getOffset(game.player.portraitID);
}

Int2 CharacterSheetUiView::getShirtOffset(Game &game)
{
	const Player &player = game.player;
	const bool isMale = player.male;

	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const int charClassDefID = player.charClassDefID;
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.castsMagic;

	return ArenaPortraitUtils::getShirtOffset(isMale, isMagic);
}

Int2 CharacterSheetUiView::getPantsOffset(Game &game)
{
	const Player &player = game.player;
	const bool isMale = player.male;
	return ArenaPortraitUtils::getPantsOffset(isMale);
}

Int2 CharacterSheetUiView::getEquipmentOffset(const TextureAsset &textureAsset, TextureManager &textureManager)
{
	if (textureAsset.filename.empty())
	{
		return Int2();
	}

	const std::optional<TextureFileMetadataID> textureFileMetadataID = textureManager.tryGetMetadataID(textureAsset.filename.c_str());
	if (!textureFileMetadataID.has_value())
	{
		return Int2();
	}

	const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*textureFileMetadataID);
	return metadata.getOffset(textureAsset.index);
}

Int2 CharacterSheetUiView::getHeadArmorOffset(Game &game)
{
	const TextureAsset textureAsset = CharacterSheetUiView::getHeadArmorTextureAsset(game);
	return CharacterSheetUiView::getEquipmentOffset(textureAsset, game.textureManager);
}

Int2 CharacterSheetUiView::getChestArmorOffset(Game &game)
{
	const TextureAsset textureAsset = CharacterSheetUiView::getChestArmorTextureAsset(game);
	return CharacterSheetUiView::getEquipmentOffset(textureAsset, game.textureManager);
}

Int2 CharacterSheetUiView::getHandsArmorOffset(Game &game)
{
	const TextureAsset textureAsset = CharacterSheetUiView::getHandsArmorTextureAsset(game);
	return CharacterSheetUiView::getEquipmentOffset(textureAsset, game.textureManager);
}

Int2 CharacterSheetUiView::getLeftShoulderArmorOffset(Game &game)
{
	const TextureAsset textureAsset = CharacterSheetUiView::getLeftShoulderArmorTextureAsset(game);
	return CharacterSheetUiView::getEquipmentOffset(textureAsset, game.textureManager);
}

Int2 CharacterSheetUiView::getRightShoulderArmorOffset(Game &game)
{
	const TextureAsset textureAsset = CharacterSheetUiView::getRightShoulderArmorTextureAsset(game);
	return CharacterSheetUiView::getEquipmentOffset(textureAsset, game.textureManager);
}

Int2 CharacterSheetUiView::getLegsArmorOffset(Game &game)
{
	const TextureAsset textureAsset = CharacterSheetUiView::getLegsArmorTextureAsset(game);
	return CharacterSheetUiView::getEquipmentOffset(textureAsset, game.textureManager);
}

Int2 CharacterSheetUiView::getFeetArmorOffset(Game &game)
{
	const TextureAsset textureAsset = CharacterSheetUiView::getFeetArmorTextureAsset(game);
	return CharacterSheetUiView::getEquipmentOffset(textureAsset, game.textureManager);
}

Int2 CharacterSheetUiView::getShieldOffset(Game &game)
{
	const TextureAsset textureAsset = CharacterSheetUiView::getShieldTextureAsset(game);
	return CharacterSheetUiView::getEquipmentOffset(textureAsset, game.textureManager);
}

Int2 CharacterSheetUiView::getWeaponOffset(Game &game)
{
	const TextureAsset textureAsset = CharacterSheetUiView::getWeaponTextureAsset(game);
	return CharacterSheetUiView::getEquipmentOffset(textureAsset, game.textureManager);
}

TextureAsset CharacterSheetUiView::getPaletteTextureAsset()
{
	return TextureAsset(ArenaPaletteName::CharSheet);
}

TextureAsset CharacterSheetUiView::getBodyTextureAsset(Game &game)
{
	const Player &player = game.player;
	const CharacterRaceLibrary &characterRaceLibrary = CharacterRaceLibrary::getInstance();
	const CharacterRaceDefinition &characterRaceDefinition = characterRaceLibrary.getDefinition(player.raceID);

	if (player.male)
	{
		return characterRaceDefinition.maleCharSheetBodyTextureAsset;
	}
	else
	{
		return characterRaceDefinition.femaleCharSheetBodyTextureAsset;
	}
}

TextureAsset CharacterSheetUiView::getHeadTextureAsset(Game &game)
{
	const ItemDefinitionID equippedHelmetItemDefID = CharacterSheetUiModel::getEquippedHeadArmorItemDefID(game);
	if (equippedHelmetItemDefID >= 0)
	{
		return CharacterSheetUiView::getHeadArmorTextureAsset(game);
	}

	const Player &player = game.player;
	const CharacterRaceLibrary &characterRaceLibrary = CharacterRaceLibrary::getInstance();
	const CharacterRaceDefinition &characterRaceDefinition = characterRaceLibrary.getDefinition(player.raceID);
	const int headIndex = player.portraitID;

	if (player.male)
	{
		return TextureAsset(characterRaceDefinition.maleCharSheetHeadsFilename, headIndex);
	}
	else
	{
		return TextureAsset(characterRaceDefinition.femaleCharSheetHeadsFilename, headIndex);
	}
}

TextureAsset CharacterSheetUiView::getShirtTextureAsset(Game &game)
{
	const Player &player = game.player;
	const bool isMale = player.male;

	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const int charClassDefID = player.charClassDefID;
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.castsMagic;

	const std::string shirtFilename = ArenaPortraitUtils::getShirt(isMale, isMagic);
	return TextureAsset(shirtFilename);
}

TextureAsset CharacterSheetUiView::getPantsTextureAsset(Game &game)
{
	const Player &player = game.player;

	const std::string pantsFilename = ArenaPortraitUtils::getPants(player.male);
	return TextureAsset(pantsFilename);
}

TextureAsset CharacterSheetUiView::getHeadArmorTextureAsset(Game &game)
{
	const ItemDefinitionID itemDefID = CharacterSheetUiModel::getEquippedHeadArmorItemDefID(game);
	if (itemDefID < 0)
	{
		return TextureAsset();
	}

	const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(itemDefID);
	const Player &player = game.player;
	const ArenaArmorTypeID armorTypeID = itemDef.armor.typeID;
	const ArenaArmorMaterialType armorMaterialType = itemDef.armor.materialType;
	return ArenaPortraitUtils::getArmor(armorTypeID, armorMaterialType, player.male);
}

TextureAsset CharacterSheetUiView::getChestArmorTextureAsset(Game &game)
{
	const ItemDefinitionID itemDefID = CharacterSheetUiModel::getEquippedChestArmorItemDefID(game);
	if (itemDefID < 0)
	{
		return TextureAsset();
	}

	const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(itemDefID);
	const Player &player = game.player;
	const ArenaArmorTypeID armorTypeID = itemDef.armor.typeID;
	const ArenaArmorMaterialType armorMaterialType = itemDef.armor.materialType;
	return ArenaPortraitUtils::getArmor(armorTypeID, armorMaterialType, player.male);
}

TextureAsset CharacterSheetUiView::getHandsArmorTextureAsset(Game &game)
{
	const ItemDefinitionID itemDefID = CharacterSheetUiModel::getEquippedHandsArmorItemDefID(game);
	if (itemDefID < 0)
	{
		return TextureAsset();
	}

	const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(itemDefID);
	const Player &player = game.player;
	const ArenaArmorTypeID armorTypeID = itemDef.armor.typeID;
	const ArenaArmorMaterialType armorMaterialType = itemDef.armor.materialType;
	return ArenaPortraitUtils::getArmor(armorTypeID, armorMaterialType, player.male);
}

TextureAsset CharacterSheetUiView::getLeftShoulderArmorTextureAsset(Game &game)
{
	const ItemDefinitionID itemDefID = CharacterSheetUiModel::getEquippedLeftShoulderArmorItemDefID(game);
	if (itemDefID < 0)
	{
		return TextureAsset();
	}

	const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(itemDefID);
	const Player &player = game.player;
	const ArenaArmorTypeID armorTypeID = itemDef.armor.typeID;
	const ArenaArmorMaterialType armorMaterialType = itemDef.armor.materialType;
	return ArenaPortraitUtils::getArmor(armorTypeID, armorMaterialType, player.male);
}

TextureAsset CharacterSheetUiView::getRightShoulderArmorTextureAsset(Game &game)
{
	const ItemDefinitionID itemDefID = CharacterSheetUiModel::getEquippedRightShoulderArmorItemDefID(game);
	if (itemDefID < 0)
	{
		return TextureAsset();
	}

	const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(itemDefID);
	const Player &player = game.player;
	const ArenaArmorTypeID armorTypeID = itemDef.armor.typeID;
	const ArenaArmorMaterialType armorMaterialType = itemDef.armor.materialType;
	return ArenaPortraitUtils::getArmor(armorTypeID, armorMaterialType, player.male);
}

TextureAsset CharacterSheetUiView::getLegsArmorTextureAsset(Game &game)
{
	const ItemDefinitionID itemDefID = CharacterSheetUiModel::getEquippedLegsArmorItemDefID(game);
	if (itemDefID < 0)
	{
		return TextureAsset();
	}

	const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(itemDefID);
	const Player &player = game.player;
	const ArenaArmorTypeID armorTypeID = itemDef.armor.typeID;
	const ArenaArmorMaterialType armorMaterialType = itemDef.armor.materialType;
	return ArenaPortraitUtils::getArmor(armorTypeID, armorMaterialType, player.male);
}

TextureAsset CharacterSheetUiView::getFeetArmorTextureAsset(Game &game)
{
	const ItemDefinitionID itemDefID = CharacterSheetUiModel::getEquippedFeetArmorItemDefID(game);
	if (itemDefID < 0)
	{
		return TextureAsset();
	}

	const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(itemDefID);
	const Player &player = game.player;
	const ArenaArmorTypeID armorTypeID = itemDef.armor.typeID;
	const ArenaArmorMaterialType armorMaterialType = itemDef.armor.materialType;
	return ArenaPortraitUtils::getArmor(armorTypeID, armorMaterialType, player.male);
}

TextureAsset CharacterSheetUiView::getShieldTextureAsset(Game &game)
{
	const ItemDefinitionID itemDefID = CharacterSheetUiModel::getEquippedShieldItemDefID(game);
	if (itemDefID < 0)
	{
		return TextureAsset();
	}

	const ItemDefinition &itemDef = ItemLibrary::getInstance().getDefinition(itemDefID);
	const Player &player = game.player;
	const ArenaArmorTypeID armorTypeID = itemDef.armor.typeID;
	const ArenaArmorMaterialType armorMaterialType = itemDef.armor.materialType;
	return ArenaPortraitUtils::getArmor(armorTypeID, armorMaterialType, player.male);
}

TextureAsset CharacterSheetUiView::getWeaponTextureAsset(Game &game)
{
	const Player &player = game.player;
	const ItemDefinitionID playerEquippedWeaponItemDefID = player.getEquippedWeaponItemDefID();
	if (playerEquippedWeaponItemDefID < 0)
	{
		return TextureAsset();
	}

	const ItemDefinition &playerEquippedWeaponItemDef = ItemLibrary::getInstance().getDefinition(playerEquippedWeaponItemDefID);
	return ArenaPortraitUtils::getWeapon(playerEquippedWeaponItemDef.weapon.typeID, player.male);
}

CharacterEquipmentPresentationState CharacterSheetUiView::getEquipmentPresentationState(Game &game)
{
	UiManager &uiManager = game.uiManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();

	const TextureAsset bodyTextureAsset = CharacterSheetUiView::getBodyTextureAsset(game);
	const TextureAsset shirtTextureAsset = CharacterSheetUiView::getShirtTextureAsset(game);
	const TextureAsset pantsTextureAsset = CharacterSheetUiView::getPantsTextureAsset(game);
	const TextureAsset headTextureAsset = CharacterSheetUiView::getHeadTextureAsset(game);

	const Player &player = game.player;
	const TextureAsset chestArmorTextureAsset = CharacterSheetUiView::getChestArmorTextureAsset(game);
	const TextureAsset handsArmorTextureAsset = CharacterSheetUiView::getHandsArmorTextureAsset(game);
	const TextureAsset leftShoulderArmorTextureAsset = CharacterSheetUiView::getLeftShoulderArmorTextureAsset(game);
	const TextureAsset rightShoulderArmorTextureAsset = CharacterSheetUiView::getRightShoulderArmorTextureAsset(game);
	const TextureAsset legsArmorTextureAsset = CharacterSheetUiView::getLegsArmorTextureAsset(game);
	const TextureAsset feetArmorTextureAsset = CharacterSheetUiView::getFeetArmorTextureAsset(game);
	const TextureAsset weaponTextureAsset = CharacterSheetUiView::getWeaponTextureAsset(game);
	const TextureAsset shieldTextureAsset = CharacterSheetUiView::getShieldTextureAsset(game);

	const ItemDefinitionID headArmorItemDefID = CharacterSheetUiModel::getEquippedHeadArmorItemDefID(game);
	const ItemDefinitionID chestArmorItemDefID = CharacterSheetUiModel::getEquippedChestArmorItemDefID(game);
	const ItemDefinitionID handsArmorItemDefID = CharacterSheetUiModel::getEquippedHandsArmorItemDefID(game);
	const ItemDefinitionID leftShoulderArmorItemDefID = CharacterSheetUiModel::getEquippedLeftShoulderArmorItemDefID(game);
	const ItemDefinitionID rightShoulderArmorItemDefID = CharacterSheetUiModel::getEquippedRightShoulderArmorItemDefID(game);
	const ItemDefinitionID legsArmorItemDefID = CharacterSheetUiModel::getEquippedLegsArmorItemDefID(game);
	const ItemDefinitionID feetArmorItemDefID = CharacterSheetUiModel::getEquippedFeetArmorItemDefID(game);
	const ItemDefinitionID weaponItemDefID = CharacterSheetUiModel::getEquippedWeaponItemDefID(game);
	const ItemDefinitionID shieldItemDefID = CharacterSheetUiModel::getEquippedShieldItemDefID(game);

	auto getOrAddTextureIfEquipped = [&uiManager, &paletteTextureAsset, &textureManager, &renderer](const TextureAsset &textureAsset, ItemDefinitionID itemDefID)
	{
		if (itemDefID < 0)
		{
			return -1;
		}

		DebugAssert(!textureAsset.filename.empty());
		return uiManager.getOrAddTexture(textureAsset, paletteTextureAsset, textureManager, renderer);
	};

	CharacterEquipmentPresentationState presentationState;
	presentationState.bodyTextureID = uiManager.getOrAddTexture(bodyTextureAsset, paletteTextureAsset, textureManager, renderer);
	presentationState.shirtTextureID = uiManager.getOrAddTexture(shirtTextureAsset, paletteTextureAsset, textureManager, renderer);
	presentationState.pantsTextureID = uiManager.getOrAddTexture(pantsTextureAsset, paletteTextureAsset, textureManager, renderer);
	presentationState.headTextureID = uiManager.getOrAddTexture(headTextureAsset, paletteTextureAsset, textureManager, renderer);
	presentationState.chestArmorTextureID = getOrAddTextureIfEquipped(chestArmorTextureAsset, chestArmorItemDefID);
	presentationState.handsArmorTextureID = getOrAddTextureIfEquipped(handsArmorTextureAsset, handsArmorItemDefID);
	presentationState.leftShoulderArmorTextureID = getOrAddTextureIfEquipped(leftShoulderArmorTextureAsset, leftShoulderArmorItemDefID);
	presentationState.rightShoulderArmorTextureID = getOrAddTextureIfEquipped(rightShoulderArmorTextureAsset, rightShoulderArmorItemDefID);
	presentationState.legsArmorTextureID = getOrAddTextureIfEquipped(legsArmorTextureAsset, legsArmorItemDefID);
	presentationState.feetArmorTextureID = getOrAddTextureIfEquipped(feetArmorTextureAsset, feetArmorItemDefID);
	presentationState.weaponTextureID = getOrAddTextureIfEquipped(weaponTextureAsset, weaponItemDefID);
	presentationState.shieldTextureID = getOrAddTextureIfEquipped(shieldTextureAsset, shieldItemDefID);
	presentationState.bodyPosition = CharacterSheetUiView::getBodyOffset(game);
	presentationState.shirtPosition = CharacterSheetUiView::getShirtOffset(game);
	presentationState.pantsPosition = CharacterSheetUiView::getPantsOffset(game);
	presentationState.headPosition = CharacterSheetUiView::getHeadOffset(game);
	presentationState.chestArmorPosition = CharacterSheetUiView::getChestArmorOffset(game);
	presentationState.handsArmorPosition = CharacterSheetUiView::getHandsArmorOffset(game);
	presentationState.leftShoulderArmorPosition = CharacterSheetUiView::getLeftShoulderArmorOffset(game);
	presentationState.rightShoulderArmorPosition = CharacterSheetUiView::getRightShoulderArmorOffset(game);
	presentationState.legsArmorPosition = CharacterSheetUiView::getLegsArmorOffset(game);
	presentationState.feetArmorPosition = CharacterSheetUiView::getFeetArmorOffset(game);
	presentationState.weaponPosition = CharacterSheetUiView::getWeaponOffset(game);
	presentationState.shieldPosition = CharacterSheetUiView::getShieldOffset(game);
	presentationState.isHeadArmorEquipped = headArmorItemDefID >= 0;
	presentationState.isChestArmorEquipped = chestArmorItemDefID >= 0;
	presentationState.isHandsArmorEquipped = handsArmorItemDefID >= 0;
	presentationState.isLeftShoulderArmorEquipped = leftShoulderArmorItemDefID >= 0;
	presentationState.isRightShoulderArmorEquipped = rightShoulderArmorItemDefID >= 0;
	presentationState.isLegsArmorEquipped = legsArmorItemDefID >= 0;
	presentationState.isFeetArmorEquipped = feetArmorItemDefID >= 0;
	presentationState.isWeaponEquipped = weaponItemDefID >= 0;
	presentationState.isShieldEquipped = shieldItemDefID >= 0;
	return presentationState;
}

void CharacterSheetUiView::createOrUpdateEquipmentUiElements(const char *elementPrefix, UiContextInstanceID contextInstID, Game &game)
{
	UiManager &uiManager = game.uiManager;
	Renderer &renderer = game.renderer;

	auto tryCreateElement = [contextInstID, &uiManager, &renderer](const std::string &elementName, Int2 position, int drawOrder, UiTextureID textureID)
	{
		UiElementInstanceID elementInstID = uiManager.getElementByName(elementName.c_str());
		if ((elementInstID < 0) && (textureID >= 0))
		{
			UiElementInitInfo imageElementInitInfo;
			imageElementInitInfo.name = elementName;
			imageElementInitInfo.position = position;
			imageElementInitInfo.drawOrder = drawOrder;
			elementInstID = uiManager.createImage(imageElementInitInfo, textureID, contextInstID, renderer);
		}

		return elementInstID;
	};

	auto updateElementIfValid = [&uiManager](UiElementInstanceID elementInstID, bool isEquipped, Int2 position, int drawOrder, UiTextureID textureID)
	{
		if (elementInstID >= 0)
		{
			uiManager.setElementActive(elementInstID, isEquipped);

			if (isEquipped)
			{
				uiManager.setElementDrawOrder(elementInstID, drawOrder);
				uiManager.setTransformPosition(elementInstID, position);
				uiManager.setImageTexture(elementInstID, textureID);
			}
		}
	};

	const CharacterEquipmentPresentationState equipmentPresentationState = CharacterSheetUiView::getEquipmentPresentationState(game);

	const std::string bodyImageElementName = String::format("%sBodyImage", elementPrefix);
	const std::string headImageElementName = String::format("%sHeadImage", elementPrefix);
	const std::string shirtImageElementName = String::format("%sShirtImage", elementPrefix);
	const std::string pantsImageElementName = String::format("%sPantsImage", elementPrefix);
	const std::string chestArmorImageElementName = String::format("%sChestArmorImage", elementPrefix);
	const std::string handsArmorImageElementName = String::format("%sHandsArmorImage", elementPrefix);
	const std::string leftShoulderArmorImageElementName = String::format("%sLeftShoulderArmorImage", elementPrefix);
	const std::string rightShoulderArmorImageElementName = String::format("%sRightShoulderArmorImage", elementPrefix);
	const std::string legsArmorImageElementName = String::format("%sLegsArmorImage", elementPrefix);
	const std::string feetArmorImageElementName = String::format("%sFeetArmorImage", elementPrefix);
	const std::string weaponImageElementName = String::format("%sWeaponImage", elementPrefix);
	const std::string shieldArmorImageElementName = String::format("%sShieldImage", elementPrefix);

	// Only head draw order can change depending on whether something is equipped.
	const int headDrawOrder = equipmentPresentationState.isHeadArmorEquipped ? CharacterSheetUiView::HeadArmorDrawOrder : CharacterSheetUiView::HeadDrawOrder;

	const UiElementInstanceID bodyImageElementInstID = tryCreateElement(bodyImageElementName, equipmentPresentationState.bodyPosition, CharacterSheetUiView::BodyDrawOrder, equipmentPresentationState.bodyTextureID);
	const UiElementInstanceID headImageElementInstID = tryCreateElement(headImageElementName, equipmentPresentationState.headPosition, headDrawOrder, equipmentPresentationState.headTextureID);
	const UiElementInstanceID shirtImageElementInstID = tryCreateElement(shirtImageElementName, equipmentPresentationState.shirtPosition, CharacterSheetUiView::ShirtDrawOrder, equipmentPresentationState.shirtTextureID);
	const UiElementInstanceID pantsImageElementInstID = tryCreateElement(pantsImageElementName, equipmentPresentationState.pantsPosition, CharacterSheetUiView::PantsDrawOrder, equipmentPresentationState.pantsTextureID);
	const UiElementInstanceID chestArmorImageElementInstID = tryCreateElement(chestArmorImageElementName, equipmentPresentationState.chestArmorPosition, CharacterSheetUiView::ChestArmorDrawOrder, equipmentPresentationState.chestArmorTextureID);
	const UiElementInstanceID handsArmorImageElementInstID = tryCreateElement(handsArmorImageElementName, equipmentPresentationState.handsArmorPosition, CharacterSheetUiView::HandsArmorDrawOrder, equipmentPresentationState.handsArmorTextureID);
	const UiElementInstanceID leftShoulderArmorImageElementInstID = tryCreateElement(leftShoulderArmorImageElementName, equipmentPresentationState.leftShoulderArmorPosition, CharacterSheetUiView::LeftShoulderArmorDrawOrder, equipmentPresentationState.leftShoulderArmorTextureID);
	const UiElementInstanceID rightShoulderArmorImageElementInstID = tryCreateElement(rightShoulderArmorImageElementName, equipmentPresentationState.rightShoulderArmorPosition, CharacterSheetUiView::RightShoulderArmorDrawOrder, equipmentPresentationState.rightShoulderArmorTextureID);
	const UiElementInstanceID legsArmorImageElementInstID = tryCreateElement(legsArmorImageElementName, equipmentPresentationState.legsArmorPosition, CharacterSheetUiView::LegsArmorDrawOrder, equipmentPresentationState.legsArmorTextureID);
	const UiElementInstanceID feetArmorImageElementInstID = tryCreateElement(feetArmorImageElementName, equipmentPresentationState.feetArmorPosition, CharacterSheetUiView::FeetArmorDrawOrder, equipmentPresentationState.feetArmorTextureID);
	const UiElementInstanceID weaponImageElementInstID = tryCreateElement(weaponImageElementName, equipmentPresentationState.weaponPosition, CharacterSheetUiView::WeaponDrawOrder, equipmentPresentationState.weaponTextureID);
	const UiElementInstanceID shieldImageElementInstID = tryCreateElement(shieldArmorImageElementName, equipmentPresentationState.shieldPosition, CharacterSheetUiView::ShieldDrawOrder, equipmentPresentationState.shieldTextureID);

	updateElementIfValid(headImageElementInstID, true, equipmentPresentationState.headPosition, headDrawOrder, equipmentPresentationState.headTextureID);
	updateElementIfValid(chestArmorImageElementInstID, equipmentPresentationState.isChestArmorEquipped, equipmentPresentationState.chestArmorPosition, CharacterSheetUiView::ChestArmorDrawOrder, equipmentPresentationState.chestArmorTextureID);
	updateElementIfValid(handsArmorImageElementInstID, equipmentPresentationState.isHandsArmorEquipped, equipmentPresentationState.handsArmorPosition, CharacterSheetUiView::HandsArmorDrawOrder, equipmentPresentationState.handsArmorTextureID);
	updateElementIfValid(leftShoulderArmorImageElementInstID, equipmentPresentationState.isLeftShoulderArmorEquipped, equipmentPresentationState.leftShoulderArmorPosition, CharacterSheetUiView::LeftShoulderArmorDrawOrder, equipmentPresentationState.leftShoulderArmorTextureID);
	updateElementIfValid(rightShoulderArmorImageElementInstID, equipmentPresentationState.isRightShoulderArmorEquipped, equipmentPresentationState.rightShoulderArmorPosition, CharacterSheetUiView::RightShoulderArmorDrawOrder, equipmentPresentationState.rightShoulderArmorTextureID);
	updateElementIfValid(legsArmorImageElementInstID, equipmentPresentationState.isLegsArmorEquipped, equipmentPresentationState.legsArmorPosition, CharacterSheetUiView::LegsArmorDrawOrder, equipmentPresentationState.legsArmorTextureID);
	updateElementIfValid(feetArmorImageElementInstID, equipmentPresentationState.isFeetArmorEquipped, equipmentPresentationState.feetArmorPosition, CharacterSheetUiView::FeetArmorDrawOrder, equipmentPresentationState.feetArmorTextureID);
	updateElementIfValid(weaponImageElementInstID, equipmentPresentationState.isWeaponEquipped, equipmentPresentationState.weaponPosition, CharacterSheetUiView::WeaponDrawOrder, equipmentPresentationState.weaponTextureID);
	updateElementIfValid(shieldImageElementInstID, equipmentPresentationState.isShieldEquipped, equipmentPresentationState.shieldPosition, CharacterSheetUiView::ShieldDrawOrder, equipmentPresentationState.shieldTextureID);
}
