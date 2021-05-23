#include "CharacterSheetUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/CIFFile.h"
#include "../Game/Game.h"
#include "../Media/PortraitFile.h"

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

	// @todo: TextureFileMetadata support for CIFFile offsets
	CIFFile cifFile;
	if (!cifFile.init(headsFilename.c_str()))
	{
		DebugCrash("Couldn't init .CIF file \"" + headsFilename + "\".");
	}

	const int imageCount = cifFile.getImageCount();
	Buffer<Int2> headOffsets(imageCount);
	for (int i = 0; i < imageCount; i++)
	{
		headOffsets.set(i, Int2(cifFile.getXOffset(i), cifFile.getYOffset(i)));
	}

	const int headOffsetIndex = player.getPortraitID();
	return headOffsets.get(headOffsetIndex);
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
