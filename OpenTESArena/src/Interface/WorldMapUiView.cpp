#include "GameWorldPanel.h"
#include "GameWorldUiView.h"
#include "WorldMapUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"

TextureAssetReference WorldMapUiView::getWorldMapTextureAssetReference()
{
	return TextureAssetReference(std::string(ArenaTextureName::WorldMap));
}

TextureAssetReference WorldMapUiView::getWorldMapPaletteTextureAssetReference()
{
	return TextureAssetReference(std::string(ArenaTextureName::WorldMap));
}

std::string WorldMapUiView::getProvinceNamesFilename()
{
	return ArenaTextureName::ProvinceNames;
}

Int2 WorldMapUiView::getFastTravelAnimationTextureCenter()
{
	return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT / 2);
}

std::string WorldMapUiView::getFastTravelAnimationFilename()
{
	return ArenaTextureName::FastTravel;
}

TextureAssetReference WorldMapUiView::getFastTravelPaletteTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::WorldMap));
}

Int2 WorldMapUiView::getCityArrivalPopUpTextCenterPoint(Game &game)
{
	return GameWorldUiView::getInterfaceCenter(game) - Int2(0, 1);
}

Int2 WorldMapUiView::getCityArrivalPopUpTextureCenterPoint(Game &game)
{
	const Int2 textCenter = WorldMapUiView::getCityArrivalPopUpTextCenterPoint(game);
	return textCenter + Int2(0, 1);
}

int WorldMapUiView::getCityArrivalPopUpTextureWidth(int textWidth)
{
	return textWidth + 10;
}

int WorldMapUiView::getCityArrivalPopUpTextureHeight(int textHeight)
{
	return textHeight + 12;
}
