#include "GameWorldPanel.h"
#include "WorldMapUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"

int WorldMapUiView::getFastTravelAnimationTextureX(int textureWidth)
{
	return (ArenaRenderUtils::SCREEN_WIDTH / 2) - (textureWidth / 2);
}

int WorldMapUiView::getFastTravelAnimationTextureY(int textureHeight)
{
	return (ArenaRenderUtils::SCREEN_HEIGHT / 2) - (textureHeight / 2);
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
	const bool modernInterface = game.getOptions().getGraphics_ModernInterface();
	auto &textureManager = game.getTextureManager();
	return GameWorldPanel::getInterfaceCenter(modernInterface, textureManager) - Int2(0, 1);
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
