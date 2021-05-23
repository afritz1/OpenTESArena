#include "CharacterCreationUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/CIFFile.h"
#include "../Game/Game.h"
#include "../Media/PortraitFile.h"

int CharacterCreationUiView::getChooseClassCreationTitleTextureX(int textureWidth)
{
	return (ArenaRenderUtils::SCREEN_WIDTH / 2) - (textureWidth / 2) - 1;
}

int CharacterCreationUiView::getChooseClassCreationTitleTextureY(int textureHeight)
{
	return (ArenaRenderUtils::SCREEN_HEIGHT / 2) - (textureHeight / 2) + 1;
}

int CharacterCreationUiView::getChooseAttributesTextureWidth()
{
	return 183;
}

int CharacterCreationUiView::getChooseAttributesTextureHeight()
{
	return 42;
}

int CharacterCreationUiView::getAttributesMessageBoxTitleTextureX(int titleTextureWidth)
{
	return (ArenaRenderUtils::SCREEN_WIDTH / 2) - (titleTextureWidth / 2) - 1;
}

int CharacterCreationUiView::getAttributesMessageBoxTitleTextureY(int titleTextureHeight)
{
	return (ArenaRenderUtils::SCREEN_HEIGHT / 2) - (titleTextureHeight / 2) - 21;
}

int CharacterCreationUiView::getAttributesMessageBoxTitleTextureWidth(int titleTextWidth)
{
	return titleTextWidth + 12;
}

int CharacterCreationUiView::getAttributesMessageBoxTitleTextureHeight()
{
	return 24;
}

int CharacterCreationUiView::getAppearanceMessageBoxTextureWidth(int textWidth)
{
	return textWidth + 10;
}

int CharacterCreationUiView::getAppearanceMessageBoxTextureHeight(int textHeight)
{
	return textHeight + 12;
}

int CharacterCreationUiView::getBodyOffsetX(Game &game)
{
	const TextureAssetReference textureAssetRef = CharacterCreationUiView::getBodyTextureAssetRef(game);

	TextureManager &textureManager = game.getTextureManager();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for body \"" + textureAssetRef.filename + "\".");
	}

	const TextureBuilder &bodyTexture = textureManager.getTextureBuilderHandle(*textureBuilderID);
	return ArenaRenderUtils::SCREEN_WIDTH - bodyTexture.getWidth();
}

Int2 CharacterCreationUiView::getHeadOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();
	const int raceID = charCreationState.getRaceIndex();

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

	const int headOffsetIndex = charCreationState.getPortraitIndex();
	return headOffsets.get(headOffsetIndex);
}

Int2 CharacterCreationUiView::getShirtOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();

	const CharacterClassLibrary &charClassLibrary = game.getCharacterClassLibrary();
	const int charClassDefID = charCreationState.getClassDefID();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.canCastMagic();

	return PortraitFile::getShirtOffset(isMale, isMagic);
}

Int2 CharacterCreationUiView::getPantsOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();
	return PortraitFile::getPantsOffset(isMale);
}

TextureAssetReference CharacterCreationUiView::getBodyTextureAssetRef(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();
	const int raceID = charCreationState.getRaceIndex();

	std::string bodyFilename = PortraitFile::getBody(isMale, raceID);
	return TextureAssetReference(std::move(bodyFilename));
}

TextureAssetReference CharacterCreationUiView::getHeadTextureAssetRef(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();
	const int raceID = charCreationState.getRaceIndex();

	constexpr bool trimmed = false;
	std::string headsFilename = PortraitFile::getHeads(isMale, raceID, trimmed);
	const int headIndex = charCreationState.getPortraitIndex();
	return TextureAssetReference(std::move(headsFilename), headIndex);
}

TextureAssetReference CharacterCreationUiView::getShirtTextureAssetRef(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();

	const CharacterClassLibrary &charClassLibrary = game.getCharacterClassLibrary();
	const int charClassDefID = charCreationState.getClassDefID();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.canCastMagic();

	std::string shirtFilename = PortraitFile::getShirt(isMale, isMagic);
	return TextureAssetReference(std::move(shirtFilename));
}

TextureAssetReference CharacterCreationUiView::getPantsTextureAssetRef(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();

	std::string pantsFilename = PortraitFile::getPants(isMale);
	return TextureAssetReference(std::move(pantsFilename));
}

TextureAssetReference CharacterCreationUiView::getNightSkyTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::CharacterCreation));
}
