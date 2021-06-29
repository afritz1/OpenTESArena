#include <optional>

#include "CharacterSheetUiView.h"
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

int CharacterSheetUiView::getBodyOffsetX(Game &game)
{
	const TextureAssetReference textureAssetRef = CharacterSheetUiView::getBodyTextureAssetRef(game);

	TextureManager &textureManager = game.getTextureManager();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for body \"" + textureAssetRef.filename + "\".");
	}

	const TextureBuilder &bodyTexture = textureManager.getTextureBuilderHandle(*textureBuilderID);
	return ArenaRenderUtils::SCREEN_WIDTH - bodyTexture.getWidth();
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
