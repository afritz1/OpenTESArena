#include <optional>

#include "CharacterSheetUiView.h"
#include "CommonUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../Media/PortraitFile.h"

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

Int2 CharacterSheetUiView::getBodyOffset(Game &game)
{
	const TextureAssetReference textureAssetRef = CharacterSheetUiView::getBodyTextureAssetRef(game);

	TextureManager &textureManager = game.getTextureManager();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for body \"" + textureAssetRef.filename + "\".");
	}

	const TextureBuilder &bodyTexture = textureManager.getTextureBuilderHandle(*textureBuilderID);
	return Int2(ArenaRenderUtils::SCREEN_WIDTH - bodyTexture.getWidth(), 0);
}

Int2 CharacterSheetUiView::getHeadOffset(Game &game)
{
	const Player &player = game.getGameState().getPlayer();
	const bool isMale = player.isMale();
	const int raceID = player.getRaceID();

	constexpr bool trimmed = false;
	const std::string &headsFilename = PortraitFile::getHeads(isMale, raceID, trimmed);

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
	const Player &player = game.getGameState().getPlayer();
	const bool isMale = player.isMale();

	const CharacterClassLibrary &charClassLibrary = game.getCharacterClassLibrary();
	const int charClassDefID = player.getCharacterClassDefID();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.canCastMagic();

	return PortraitFile::getShirtOffset(isMale, isMagic);
}

Int2 CharacterSheetUiView::getPantsOffset(Game &game)
{
	const Player &player = game.getGameState().getPlayer();
	const bool isMale = player.isMale();
	return PortraitFile::getPantsOffset(isMale);
}

Int2 CharacterSheetUiView::getNextPageOffset()
{
	return Int2(108, 179);
}

TextureAssetReference CharacterSheetUiView::getPaletteTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaPaletteName::CharSheet));
}

TextureAssetReference CharacterSheetUiView::getStatsBackgroundTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::CharacterStats));
}

TextureAssetReference CharacterSheetUiView::getEquipmentBackgroundTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::CharacterEquipment));
}

TextureAssetReference CharacterSheetUiView::getNextPageButtonTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::NextPage));
}

TextureAssetReference CharacterSheetUiView::getBodyTextureAssetRef(Game &game)
{
	const Player &player = game.getGameState().getPlayer();
	const bool isMale = player.isMale();
	const int raceID = player.getRaceID();

	std::string bodyFilename = PortraitFile::getBody(isMale, raceID);
	return TextureAssetReference(std::move(bodyFilename));
}

TextureAssetReference CharacterSheetUiView::getHeadTextureAssetRef(Game &game)
{
	const Player &player = game.getGameState().getPlayer();
	const bool isMale = player.isMale();
	const int raceID = player.getRaceID();

	constexpr bool trimmed = false;
	std::string headsFilename = PortraitFile::getHeads(isMale, raceID, trimmed);
	const int headIndex = player.getPortraitID();
	return TextureAssetReference(std::move(headsFilename), headIndex);
}

TextureAssetReference CharacterSheetUiView::getShirtTextureAssetRef(Game &game)
{
	const Player &player = game.getGameState().getPlayer();
	const bool isMale = player.isMale();

	const CharacterClassLibrary &charClassLibrary = game.getCharacterClassLibrary();
	const int charClassDefID = player.getCharacterClassDefID();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.canCastMagic();

	std::string shirtFilename = PortraitFile::getShirt(isMale, isMagic);
	return TextureAssetReference(std::move(shirtFilename));
}

TextureAssetReference CharacterSheetUiView::getPantsTextureAssetRef(Game &game)
{
	const Player &player = game.getGameState().getPlayer();
	const bool isMale = player.isMale();

	std::string pantsFilename = PortraitFile::getPants(isMale);
	return TextureAssetReference(std::move(pantsFilename));
}

PaletteID CharacterSheetUiView::getPaletteID(TextureManager &textureManager)
{
	const TextureAssetReference paletteTextureAssetRef = CharacterSheetUiView::getPaletteTextureAssetRef();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteTextureAssetRef);
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + paletteTextureAssetRef.filename + "\".");
	}

	return *paletteID;
}

UiTextureID CharacterSheetUiView::allocBodyTexture(Game &game)
{
	auto &textureManager = game.getTextureManager();
	const PaletteID paletteID = CharacterSheetUiView::getPaletteID(textureManager);

	const TextureAssetReference textureAssetRef = CharacterSheetUiView::getBodyTextureAssetRef(game);
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	auto &renderer = game.getRenderer();
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character body.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocShirtTexture(Game &game)
{
	auto &textureManager = game.getTextureManager();
	const PaletteID paletteID = CharacterSheetUiView::getPaletteID(textureManager);

	const TextureAssetReference textureAssetRef = CharacterSheetUiView::getShirtTextureAssetRef(game);
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	auto &renderer = game.getRenderer();
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character shirt.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocPantsTexture(Game &game)
{
	auto &textureManager = game.getTextureManager();
	const PaletteID paletteID = CharacterSheetUiView::getPaletteID(textureManager);

	const TextureAssetReference textureAssetRef = CharacterSheetUiView::getPantsTextureAssetRef(game);
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	auto &renderer = game.getRenderer();
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character pants.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocHeadTexture(Game &game)
{
	auto &textureManager = game.getTextureManager();
	const PaletteID paletteID = CharacterSheetUiView::getPaletteID(textureManager);

	const TextureAssetReference textureAssetRef = CharacterSheetUiView::getHeadTextureAssetRef(game);
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	auto &renderer = game.getRenderer();
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character head.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocStatsBgTexture(TextureManager &textureManager, Renderer &renderer)
{
	const PaletteID paletteID = CharacterSheetUiView::getPaletteID(textureManager);
	const TextureAssetReference textureAssetRef = CharacterSheetUiView::getStatsBackgroundTextureAssetRef();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for stats background.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocEquipmentBgTexture(TextureManager &textureManager, Renderer &renderer)
{
	const PaletteID paletteID = CharacterSheetUiView::getPaletteID(textureManager);
	const TextureAssetReference textureAssetRef = CharacterSheetUiView::getEquipmentBackgroundTextureAssetRef();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for equipment background.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocNextPageTexture(TextureManager &textureManager, Renderer &renderer)
{
	const PaletteID paletteID = CharacterSheetUiView::getPaletteID(textureManager);
	const TextureAssetReference textureAssetRef = CharacterSheetUiView::getNextPageButtonTextureAssetRef();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for next page button.");
	}

	return textureID;
}
