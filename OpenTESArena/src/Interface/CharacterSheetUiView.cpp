#include <map>
#include <optional>

#include "CharacterSheetUiView.h"
#include "CommonUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaPortraitUtils.h"
#include "../Assets/ArenaTextureName.h"
#include "../Entities/PrimaryAttribute.h"
#include "../Entities/PrimaryAttributeName.h"
#include "../Game/Game.h"

TextBox::InitInfo CharacterSheetUiView::getPlayerNameTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithXY(
		text,
		CharacterSheetUiView::PlayerNameTextBoxX,
		CharacterSheetUiView::PlayerNameTextBoxY,
		CharacterSheetUiView::PlayerNameTextBoxFontName,
		CharacterSheetUiView::PlayerNameTextBoxColor,
		CharacterSheetUiView::PlayerNameTextBoxAlignment,
		fontLibrary);
}

TextBox::InitInfo CharacterSheetUiView::getPlayerRaceTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithXY(
		text,
		CharacterSheetUiView::PlayerRaceTextBoxX,
		CharacterSheetUiView::PlayerRaceTextBoxY,
		CharacterSheetUiView::PlayerRaceTextBoxFontName,
		CharacterSheetUiView::PlayerRaceTextBoxColor,
		CharacterSheetUiView::PlayerRaceTextBoxAlignment,
		fontLibrary);
}

TextBox::InitInfo CharacterSheetUiView::getPlayerClassTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithXY(
		text,
		CharacterSheetUiView::PlayerClassTextBoxX,
		CharacterSheetUiView::PlayerClassTextBoxY,
		CharacterSheetUiView::PlayerClassTextBoxFontName,
		CharacterSheetUiView::PlayerClassTextBoxColor,
		CharacterSheetUiView::PlayerClassTextBoxAlignment,
		fontLibrary);
}

std::map<PrimaryAttributeName, TextBox::InitInfo> CharacterSheetUiView::getPlayerAttributeTextBoxInitInfoMap(
	const std::vector<PrimaryAttribute> &attributes, const FontLibrary &fontLibrary)
{
	std::map<PrimaryAttributeName, TextBox::InitInfo> textBoxInitInfoMap;
	
	for (int i = 0; i < static_cast<int>(attributes.size()); i++)
	{
		const PrimaryAttribute &attribute = attributes[i];
		const PrimaryAttributeName attributeName = attribute.getAttributeName();
		const int attributeValue = attribute.get();
		const std::string attributeValueStr = std::to_string(attributeValue);
		TextBox::InitInfo initInfo = TextBox::InitInfo::makeWithXY(
			attributeValueStr,
			CharacterSheetUiView::PlayerAttributeTextBoxX,
			CharacterSheetUiView::PlayerAttributeTextBoxesY + i * CharacterSheetUiView::PlayerAttributeTextBoxHeight,
			CharacterSheetUiView::PlayerAttributeTextBoxFontName,
			CharacterSheetUiView::PlayerAttributeTextBoxColor,
			CharacterSheetUiView::PlayerAttributeTextBoxAlignment,
			fontLibrary);

		textBoxInitInfoMap.emplace(attributeName, std::move(initInfo));
	}

	return textBoxInitInfoMap;
}

Int2 CharacterSheetUiView::getBodyOffset(Game &game)
{
	const TextureAsset textureAsset = CharacterSheetUiView::getBodyTextureAsset(game);

	TextureManager &textureManager = game.getTextureManager();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for body \"" + textureAsset.filename + "\".");
	}

	const TextureBuilder &bodyTexture = textureManager.getTextureBuilderHandle(*textureBuilderID);
	return Int2(ArenaRenderUtils::SCREEN_WIDTH - bodyTexture.getWidth(), 0);
}

Int2 CharacterSheetUiView::getHeadOffset(Game &game)
{
	const Player &player = game.getPlayer();
	const bool isMale = player.isMale();
	const int raceID = player.getRaceID();

	constexpr bool trimmed = false;
	const std::string &headsFilename = ArenaPortraitUtils::getHeads(isMale, raceID, trimmed);

	auto &textureManager = game.getTextureManager();
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(headsFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugCrash("Couldn't get texture file metadata for \"" + headsFilename + "\".");
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	const int headOffsetIndex = player.getPortraitID();
	return textureFileMetadata.getOffset(headOffsetIndex);
}

Int2 CharacterSheetUiView::getShirtOffset(Game &game)
{
	const Player &player = game.getPlayer();
	const bool isMale = player.isMale();

	const CharacterClassLibrary &charClassLibrary = game.getCharacterClassLibrary();
	const int charClassDefID = player.getCharacterClassDefID();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.canCastMagic();

	return ArenaPortraitUtils::getShirtOffset(isMale, isMagic);
}

Int2 CharacterSheetUiView::getPantsOffset(Game &game)
{
	const Player &player = game.getPlayer();
	const bool isMale = player.isMale();
	return ArenaPortraitUtils::getPantsOffset(isMale);
}

Int2 CharacterSheetUiView::getNextPageOffset()
{
	return Int2(108, 179);
}

TextureAsset CharacterSheetUiView::getPaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaPaletteName::CharSheet));
}

TextureAsset CharacterSheetUiView::getStatsBackgroundTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::CharacterStats));
}

TextureAsset CharacterSheetUiView::getEquipmentBackgroundTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::CharacterEquipment));
}

TextureAsset CharacterSheetUiView::getNextPageButtonTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::NextPage));
}

TextureAsset CharacterSheetUiView::getBodyTextureAsset(Game &game)
{
	const Player &player = game.getPlayer();
	const bool isMale = player.isMale();
	const int raceID = player.getRaceID();

	std::string bodyFilename = ArenaPortraitUtils::getBody(isMale, raceID);
	return TextureAsset(std::move(bodyFilename));
}

TextureAsset CharacterSheetUiView::getHeadTextureAsset(Game &game)
{
	const Player &player = game.getPlayer();
	const bool isMale = player.isMale();
	const int raceID = player.getRaceID();

	constexpr bool trimmed = false;
	std::string headsFilename = ArenaPortraitUtils::getHeads(isMale, raceID, trimmed);
	const int headIndex = player.getPortraitID();
	return TextureAsset(std::move(headsFilename), headIndex);
}

TextureAsset CharacterSheetUiView::getShirtTextureAsset(Game &game)
{
	const Player &player = game.getPlayer();
	const bool isMale = player.isMale();

	const CharacterClassLibrary &charClassLibrary = game.getCharacterClassLibrary();
	const int charClassDefID = player.getCharacterClassDefID();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.canCastMagic();

	std::string shirtFilename = ArenaPortraitUtils::getShirt(isMale, isMagic);
	return TextureAsset(std::move(shirtFilename));
}

TextureAsset CharacterSheetUiView::getPantsTextureAsset(Game &game)
{
	const Player &player = game.getPlayer();
	const bool isMale = player.isMale();

	std::string pantsFilename = ArenaPortraitUtils::getPants(isMale);
	return TextureAsset(std::move(pantsFilename));
}

UiTextureID CharacterSheetUiView::allocBodyTexture(Game &game)
{
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();

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
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();

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
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();

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
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getHeadTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character head.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocStatsBgTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getStatsBackgroundTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for stats background.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocEquipmentBgTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getEquipmentBackgroundTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for equipment background.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocNextPageTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getNextPageButtonTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for next page button.");
	}

	return textureID;
}
