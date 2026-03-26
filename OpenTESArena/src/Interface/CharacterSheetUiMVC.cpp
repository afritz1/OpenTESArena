#include <cmath>
#include <cstring>
#include <optional>

#include "CharacterSheetUiMVC.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaPortraitUtils.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureManager.h"
#include "../Game/Game.h"
#include "../Player/Player.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../Rendering/ArenaRenderUtils.h"

#include "components/debug/Debug.h"

std::string CharacterSheetUiModel::getStatusValueCurrentAndMaxString(double currentValue, double maxValue)
{
	const int currentInt = static_cast<int>(std::round(currentValue));
	const int maxInt = static_cast<int>(std::round(maxValue));

	char buffer[64];
	std::snprintf(buffer, sizeof(buffer), "%d/%d", currentInt, maxInt);
	return std::string(buffer);
}

std::string CharacterSheetUiModel::getDerivedAttributeDisplayString(int value)
{
	const char *signString = "";
	if (value >= 0)
	{
		signString = "+";
	}

	char buffer[64];
	std::snprintf(buffer, sizeof(buffer), "%s%d", signString, value);
	return buffer;
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
	const Player &player = game.player;
	const bool isMale = player.male;
	const int raceID = player.raceID;

	constexpr bool trimmed = false;
	const std::string &headsFilename = ArenaPortraitUtils::getHeads(isMale, raceID, trimmed);

	auto &textureManager = game.textureManager;
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(headsFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugCrash("Couldn't get texture file metadata for \"" + headsFilename + "\".");
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	const int headOffsetIndex = player.portraitID;
	return textureFileMetadata.getOffset(headOffsetIndex);
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

TextureAsset CharacterSheetUiView::getPaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaPaletteName::CharSheet));
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
	const Player &player = game.player;
	const CharacterRaceLibrary &characterRaceLibrary = CharacterRaceLibrary::getInstance();
	const CharacterRaceDefinition &characterRaceDefinition = characterRaceLibrary.getDefinition(player.raceID);
	const int headIndex = player.portraitID;

	if (player.male)
	{
		return TextureAsset(std::string(characterRaceDefinition.maleCharSheetHeadsFilename), headIndex);
	}
	else
	{
		return TextureAsset(std::string(characterRaceDefinition.femaleCharSheetHeadsFilename), headIndex);
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

	std::string shirtFilename = ArenaPortraitUtils::getShirt(isMale, isMagic);
	return TextureAsset(std::move(shirtFilename));
}

TextureAsset CharacterSheetUiView::getPantsTextureAsset(Game &game)
{
	const Player &player = game.player;
	const bool isMale = player.male;

	std::string pantsFilename = ArenaPortraitUtils::getPants(isMale);
	return TextureAsset(std::move(pantsFilename));
}

UiTextureID CharacterSheetUiView::allocBodyTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getBodyTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character body.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocShirtTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getShirtTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character shirt.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocPantsTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getPantsTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character pants.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocHeadTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getHeadTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character head.");
	}

	return textureID;
}

UiTextureID CharacterEquipmentUiView::allocUpDownButtonTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset upDownTextureAsset = TextureAsset(std::string(ArenaTextureName::UpDown));
	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(upDownTextureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugLogError("Couldn't get texture ID for up/down arrows.");
		return -1;
	}

	return textureID;
}
