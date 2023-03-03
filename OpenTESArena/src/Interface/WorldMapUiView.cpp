#include "GameWorldPanel.h"
#include "GameWorldUiView.h"
#include "WorldMapUiModel.h"
#include "WorldMapUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../UI/Surface.h"

Int2 WorldMapUiView::getProvinceNameOffset(int provinceID, TextureManager &textureManager)
{
	const std::string provinceNameOffsetFilename = WorldMapUiModel::getProvinceNameOffsetFilename();
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(provinceNameOffsetFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugCrash("Couldn't get texture file metadata for \"" + provinceNameOffsetFilename + "\".");
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	return textureFileMetadata.getOffset(provinceID);
}

TextureAsset WorldMapUiView::getTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::WorldMap));
}

TextureAsset WorldMapUiView::getPaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::WorldMap));
}

std::string WorldMapUiView::getProvinceNamesFilename()
{
	return ArenaTextureName::ProvinceNames;
}

UiTextureID WorldMapUiView::allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = WorldMapUiView::getTextureAsset();
	const TextureAsset paletteTextureAsset = WorldMapUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for world map background.");
	}

	return textureID;
}

UiTextureID WorldMapUiView::allocHighlightedTextTexture(int provinceID, TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = WorldMapUiView::getPaletteTextureAsset();

	const std::string provinceNamesFilename = WorldMapUiView::getProvinceNamesFilename();
	const TextureAsset textureAsset = TextureAsset(std::string(provinceNamesFilename), provinceID);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for highlighted text for province " + std::to_string(provinceID) + ".");
	}

	return textureID;
}

Int2 FastTravelUiView::getAnimationTextureCenter()
{
	return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT / 2);
}

std::string FastTravelUiView::getAnimationFilename()
{
	return ArenaTextureName::FastTravel;
}

TextureAsset FastTravelUiView::getPaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::WorldMap));
}

Int2 FastTravelUiView::getCityArrivalPopUpTextCenterPoint(Game &game)
{
	return GameWorldUiView::getInterfaceCenter(game) - Int2(0, 1);
}

Int2 FastTravelUiView::getCityArrivalPopUpTextureCenterPoint(Game &game)
{
	const Int2 textCenter = FastTravelUiView::getCityArrivalPopUpTextCenterPoint(game);
	return textCenter + Int2(0, 1);
}

int FastTravelUiView::getCityArrivalPopUpTextureWidth(int textWidth)
{
	return textWidth + 10;
}

int FastTravelUiView::getCityArrivalPopUpTextureHeight(int textHeight)
{
	return textHeight + 12;
}

UiTextureID FastTravelUiView::allocCityArrivalPopUpTexture(int textWidth, int textHeight,
	TextureManager &textureManager, Renderer &renderer)
{
	const Surface surface = TextureUtils::generate(
		FastTravelUiView::CityArrivalTexturePatternType,
		FastTravelUiView::getCityArrivalPopUpTextureWidth(textWidth),
		FastTravelUiView::getCityArrivalPopUpTextureHeight(textHeight),
		textureManager,
		renderer);
	
	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create city arrival pop-up texture from surface.");
	}

	return textureID;
}
