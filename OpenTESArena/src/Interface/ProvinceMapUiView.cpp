#include <algorithm>

#include "ProvinceMapUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../UI/FontDefinition.h"
#include "../UI/FontLibrary.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

Int2 ProvinceMapUiView::getLocationCenterPoint(Game &game, int provinceID, int locationID)
{
	const auto &gameState = game.getGameState();
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceID);
	const LocationDefinition &locationDef = provinceDef.getLocationDef(locationID);
	return Int2(locationDef.getScreenX(), locationDef.getScreenY());
}

Int2 ProvinceMapUiView::getLocationTextClampedPosition(int textX, int textY, int textWidth, int textHeight)
{
	return Int2(
		std::clamp(textX, 2, ArenaRenderUtils::SCREEN_WIDTH - textWidth - 2),
		std::clamp(textY, 2, ArenaRenderUtils::SCREEN_HEIGHT - textHeight - 2));
}

int ProvinceMapUiView::getTextPopUpTextureWidth(int textWidth)
{
	return textWidth + 20;
}

int ProvinceMapUiView::getTextPopUpTextureHeight(int textHeight)
{
	// Parchment minimum height is 40 pixels.
	return std::max(textHeight + 16, 40);
}

TextureAssetReference ProvinceMapUiView::getProvinceBackgroundTextureAssetRef(Game &game, int provinceID)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	const auto &provinceImgFilenames = exeData.locations.provinceImgFilenames;
	DebugAssertIndex(provinceImgFilenames, provinceID);
	const std::string &filename = provinceImgFilenames[provinceID];
	return TextureAssetReference(String::toUppercase(filename));
}

TextureAssetReference ProvinceMapUiView::getProvinceBackgroundPaletteTextureAssetRef(Game &game, int provinceID)
{
	return TextureAssetReference(ProvinceMapUiView::getProvinceBackgroundTextureAssetRef(game, provinceID));
}

TextureAssetReference ProvinceMapUiView::getCityStateIconTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::CityStateIcon));
}

TextureAssetReference ProvinceMapUiView::getTownIconTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::TownIcon));
}

TextureAssetReference ProvinceMapUiView::getVillageIconTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::VillageIcon));
}

TextureAssetReference ProvinceMapUiView::getDungeonIconTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::DungeonIcon));
}

std::string ProvinceMapUiView::getStaffDungeonIconsFilename()
{
	return ArenaTextureName::StaffDungeonIcons;
}

TextureAssetReference ProvinceMapUiView::getStaffDungeonIconTextureAssetRef(int provinceID)
{
	return TextureAssetReference(ProvinceMapUiView::getStaffDungeonIconsFilename(), provinceID);
}

std::string ProvinceMapUiView::getMapIconOutlinesFilename()
{
	return ArenaTextureName::MapIconOutlines;
}

std::string ProvinceMapUiView::getMapIconBlinkingOutlinesFilename()
{
	return ArenaTextureName::MapIconOutlinesBlinking;
}

TextBox::InitInfo ProvinceMapUiView::getSearchSubPanelTitleTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithXY(
		text,
		ProvinceMapUiView::SearchSubPanelTitleTextBoxX,
		ProvinceMapUiView::SearchSubPanelTitleTextBoxY,
		ProvinceMapUiView::SearchSubPanelTitleFontName,
		ProvinceMapUiView::SearchSubPanelTitleColor,
		ProvinceMapUiView::SearchSubPanelTitleTextAlignment,
		fontLibrary);
}

TextBox::InitInfo ProvinceMapUiView::getSearchSubPanelTextEntryTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	const std::string dummyText(ProvinceMapUiModel::SearchSubPanelMaxNameLength, TextRenderUtils::LARGEST_CHAR);
	const Int2 &origin = ProvinceMapUiView::SearchSubPanelDefaultTextCursorPosition;
	return TextBox::InitInfo::makeWithXY(
		dummyText,
		origin.x,
		origin.y,
		ProvinceMapUiView::SearchSubPanelTextEntryFontName,
		ProvinceMapUiView::SearchSubPanelTextEntryColor,
		ProvinceMapUiView::SearchSubPanelTextEntryTextAlignment,
		fontLibrary);
}

int ProvinceMapUiView::getSearchSubPanelTextEntryTextureX(int textureWidth)
{
	return (ArenaRenderUtils::SCREEN_WIDTH / 2) - (textureWidth / 2) - 1;
}

int ProvinceMapUiView::getSearchSubPanelTextEntryTextureY(int textureHeight)
{
	return (ArenaRenderUtils::SCREEN_HEIGHT / 2) - (textureHeight / 2) - 1;
}

ListBox::Properties ProvinceMapUiView::makeSearchSubPanelListBoxProperties(const FontLibrary &fontLibrary)
{
	const char *fontName = ArenaFontName::Arena;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName, &fontDefIndex))
	{
		DebugCrash("Couldn't get search sub-panel list box font \"" + std::string(fontName) + "\".");
	}

	constexpr int maxDisplayedItemCount = 6;
	std::string dummyText;
	for (int i = 0; i < maxDisplayedItemCount; i++)
	{
		if (i > 0)
		{
			dummyText += '\n';
		}

		std::string dummyLine(17, TextRenderUtils::LARGEST_CHAR); // Arbitrary worst-case line size.
		dummyText += dummyLine;
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	const Color itemColor(52, 24, 8);
	constexpr double scrollScale = 1.0;
	return ListBox::Properties(fontDefIndex, &fontLibrary, textureGenInfo, fontDef.getCharacterHeight(),
		itemColor, scrollScale);
}

TextureAssetReference ProvinceMapUiView::getSearchSubPanelListTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::PopUp8));
}

TextureAssetReference ProvinceMapUiView::getSearchSubPanelListPaletteTextureAssetRef(Game &game, int provinceID)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	const auto &provinceImgFilenames = exeData.locations.provinceImgFilenames;
	DebugAssertIndex(provinceImgFilenames, provinceID);
	const std::string &filename = provinceImgFilenames[provinceID];

	// Set all characters to uppercase because the texture manager expects 
	// extensions to be uppercase, and most filenames in A.EXE are lowercase.
	return String::toUppercase(filename);
}
