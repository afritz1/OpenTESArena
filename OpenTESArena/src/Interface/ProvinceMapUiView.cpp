#include <algorithm>

#include "ProvinceMapUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../UI/FontDefinition.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

Int2 ProvinceMapUiView::getLocationCenterPoint(Game &game, int provinceID, int locationID)
{
	const auto &gameState = game.gameState;
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
	const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceID);
	const LocationDefinition &locationDef = provinceDef.getLocationDef(locationID);
	return Int2(locationDef.getScreenX(), locationDef.getScreenY());
}

Int2 ProvinceMapUiView::getLocationTextClampedCenter(const Rect &unclampedRect)
{
	const Int2 unclampedTopLeft = unclampedRect.getTopLeft();
	const Int2 clampedTopLeft(
		std::clamp(unclampedTopLeft.x, 2, ArenaRenderUtils::SCREEN_WIDTH - unclampedRect.width - 2),
		std::clamp(unclampedTopLeft.y, 2, ArenaRenderUtils::SCREEN_HEIGHT - unclampedRect.height - 2));
	return clampedTopLeft + Int2(unclampedRect.width / 2, unclampedRect.height / 2);
}

TextBoxInitInfo ProvinceMapUiView::getHoveredLocationTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	const std::string dummyText(24, TextRenderUtils::LARGEST_CHAR);

	TextRenderShadowInfo shadowInfo;
	shadowInfo.init(ProvinceMapUiView::LocationTextShadowOffsetX, ProvinceMapUiView::LocationTextShadowOffsetY,
		ProvinceMapUiView::LocationTextShadowColor);
	constexpr int lineSpacing = 0;

	return TextBoxInitInfo::makeWithCenter(
		dummyText,
		Int2::Zero,
		ProvinceMapUiView::LocationFontName,
		ProvinceMapUiView::LocationTextColor,
		ProvinceMapUiView::LocationTextAlignment,
		shadowInfo,
		lineSpacing,
		fontLibrary);
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

bool ProvinceMapUiView::provinceHasStaffDungeonIcon(int provinceID)
{
	return provinceID != ArenaLocationUtils::CENTER_PROVINCE_ID;
}

TextureAsset ProvinceMapUiView::getBackgroundTextureAsset(int provinceID, const BinaryAssetLibrary &binaryAssetLibrary)
{
	const auto &exeData = binaryAssetLibrary.getExeData();
	const auto &provinceImgFilenames = exeData.locations.provinceImgFilenames;
	DebugAssertIndex(provinceImgFilenames, provinceID);
	const std::string &filename = provinceImgFilenames[provinceID];
	return TextureAsset(String::toUppercase(filename));
}

TextureAsset ProvinceMapUiView::getBackgroundPaletteTextureAsset(int provinceID, const BinaryAssetLibrary &binaryAssetLibrary)
{
	return TextureAsset(ProvinceMapUiView::getBackgroundTextureAsset(provinceID, binaryAssetLibrary));
}

TextureAsset ProvinceMapUiView::getCityStateIconTextureAsset(HighlightType highlightType)
{
	if (highlightType == HighlightType::None)
	{
		return TextureAsset(std::string(ArenaTextureName::CityStateIcon));
	}
	else if (highlightType == HighlightType::PlayerLocation)
	{
		return TextureAsset(std::string(ArenaTextureName::MapIconOutlines), ProvinceMapUiView::CityStateIconHighlightIndex);
	}
	else if (highlightType == HighlightType::TravelDestination)
	{
		return TextureAsset(std::string(ArenaTextureName::MapIconOutlinesBlinking), ProvinceMapUiView::CityStateIconHighlightIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(TextureAsset, std::to_string(static_cast<int>(highlightType)));
	}
}

TextureAsset ProvinceMapUiView::getTownIconTextureAsset(HighlightType highlightType)
{
	if (highlightType == HighlightType::None)
	{
		return TextureAsset(std::string(ArenaTextureName::TownIcon));
	}
	else if (highlightType == HighlightType::PlayerLocation)
	{
		return TextureAsset(std::string(ArenaTextureName::MapIconOutlines), ProvinceMapUiView::TownIconHighlightIndex);
	}
	else if (highlightType == HighlightType::TravelDestination)
	{
		return TextureAsset(std::string(ArenaTextureName::MapIconOutlinesBlinking), ProvinceMapUiView::TownIconHighlightIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(TextureAsset, std::to_string(static_cast<int>(highlightType)));
	}
}

TextureAsset ProvinceMapUiView::getVillageIconTextureAsset(HighlightType highlightType)
{
	if (highlightType == HighlightType::None)
	{
		return TextureAsset(std::string(ArenaTextureName::VillageIcon));
	}
	else if (highlightType == HighlightType::PlayerLocation)
	{
		return TextureAsset(std::string(ArenaTextureName::MapIconOutlines), ProvinceMapUiView::VillageIconHighlightIndex);
	}
	else if (highlightType == HighlightType::TravelDestination)
	{
		return TextureAsset(std::string(ArenaTextureName::MapIconOutlinesBlinking), ProvinceMapUiView::VillageIconHighlightIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(TextureAsset, std::to_string(static_cast<int>(highlightType)));
	}
}

TextureAsset ProvinceMapUiView::getDungeonIconTextureAsset(HighlightType highlightType)
{
	if (highlightType == HighlightType::None)
	{
		return TextureAsset(std::string(ArenaTextureName::DungeonIcon));
	}
	else if (highlightType == HighlightType::PlayerLocation)
	{
		return TextureAsset(std::string(ArenaTextureName::MapIconOutlines), ProvinceMapUiView::DungeonIconHighlightIndex);
	}
	else if (highlightType == HighlightType::TravelDestination)
	{
		return TextureAsset(std::string(ArenaTextureName::MapIconOutlinesBlinking), ProvinceMapUiView::DungeonIconHighlightIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(TextureAsset, std::to_string(static_cast<int>(highlightType)));
	}
}

TextureAsset ProvinceMapUiView::getStaffDungeonIconTextureAsset(int provinceID)
{
	return TextureAsset(std::string(ArenaTextureName::StaffDungeonIcons), provinceID);
}

UiTextureID ProvinceMapUiView::allocBackgroundTexture(int provinceID, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = ProvinceMapUiView::getBackgroundTextureAsset(provinceID, binaryAssetLibrary);
	const TextureAsset paletteTextureAsset = ProvinceMapUiView::getBackgroundPaletteTextureAsset(provinceID, binaryAssetLibrary);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate province \"" + std::to_string(provinceID) + "\" background texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocCityStateIconTexture(HighlightType highlightType, const TextureAsset &paletteTextureAsset,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = ProvinceMapUiView::getCityStateIconTextureAsset(highlightType);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate city state icon texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocTownIconTexture(HighlightType highlightType, const TextureAsset &paletteTextureAsset,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = ProvinceMapUiView::getTownIconTextureAsset(highlightType);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate town icon texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocVillageIconTexture(HighlightType highlightType, const TextureAsset &paletteTextureAsset,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = ProvinceMapUiView::getVillageIconTextureAsset(highlightType);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate village icon texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocDungeonIconTexture(HighlightType highlightType, const TextureAsset &paletteTextureAsset,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = ProvinceMapUiView::getDungeonIconTextureAsset(highlightType);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate dungeon icon texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocStaffDungeonIconTexture(int provinceID, HighlightType highlightType,
	const TextureAsset &paletteTextureAsset, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssert(ProvinceMapUiView::provinceHasStaffDungeonIcon(provinceID));

	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteTextureAsset);
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get staff dungeon palette ID for \"" + paletteTextureAsset.filename + "\".");
	}

	const TextureAsset textureAsset = ProvinceMapUiView::getStaffDungeonIconTextureAsset(provinceID);
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get staff dungeon texture builder ID for \"" + textureAsset.filename + "\".");
	}

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, *paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create staff dungeon texture for \"" + textureAsset.filename + "\".");
	}

	if (highlightType == HighlightType::None)
	{
		return textureID;
	}

	// Modify icon background texels based on the highlight type.
	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
	DebugAssert(textureBuilder.type == TextureBuilderType::Paletted);

	const TextureBuilderPalettedTexture &srcTexture = textureBuilder.paletteTexture;
	const uint8_t *srcTexels = srcTexture.texels.begin();
	uint32_t *dstTexels = renderer.lockUiTexture(textureID);
	if (dstTexels == nullptr)
	{
		DebugCrash("Couldn't lock staff dungeon icon texels for highlight modification.");
	}

	const uint8_t highlightColorIndex = (highlightType == HighlightType::PlayerLocation) ?
		ProvinceMapUiView::YellowPaletteIndex : ProvinceMapUiView::RedPaletteIndex;
	const uint32_t highlightColor = palette[highlightColorIndex].toARGB();

	const int texelCount = textureBuilder.getWidth() * textureBuilder.getHeight();
	for (int i = 0; i < texelCount; i++)
	{
		const uint8_t srcTexel = srcTexels[i];
		if (srcTexel == ProvinceMapUiView::BackgroundPaletteIndex)
		{
			dstTexels[i] = highlightColor;
		}
	}

	renderer.unlockUiTexture(textureID);
	return textureID;
}

UiTextureID ProvinceMapUiView::allocTextPopUpTexture(int textWidth, int textHeight,
	TextureManager &textureManager, Renderer &renderer)
{
	const Surface surface = TextureUtils::generate(
		ProvinceMapUiView::TextPopUpTexturePatternType,
		ProvinceMapUiView::getTextPopUpTextureWidth(textWidth),
		ProvinceMapUiView::getTextPopUpTextureHeight(textHeight),
		textureManager,
		renderer);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create text pop-up texture.");
	}

	return textureID;
}

TextBoxInitInfo ProvinceSearchUiView::getTitleTextBoxInitInfo(const std::string_view text,
	const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		text,
		ProvinceSearchUiView::TitleTextBoxX,
		ProvinceSearchUiView::TitleTextBoxY,
		ProvinceSearchUiView::TitleFontName,
		ProvinceSearchUiView::TitleColor,
		ProvinceSearchUiView::TitleTextAlignment,
		fontLibrary);
}

TextBoxInitInfo ProvinceSearchUiView::getTextEntryTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	const std::string dummyText(ProvinceSearchUiModel::MaxNameLength, TextRenderUtils::LARGEST_CHAR);
	const Int2 &origin = ProvinceSearchUiView::DefaultTextCursorPosition;
	return TextBoxInitInfo::makeWithXY(
		dummyText,
		origin.x,
		origin.y,
		ProvinceSearchUiView::TextEntryFontName,
		ProvinceSearchUiView::TextEntryColor,
		ProvinceSearchUiView::TextEntryTextAlignment,
		fontLibrary);
}

ListBoxProperties ProvinceSearchUiView::makeListBoxProperties(const FontLibrary &fontLibrary)
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
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	const Color itemColor(52, 24, 8);
	constexpr double scrollScale = 1.0;
	return ListBoxProperties(fontDefIndex, textureGenInfo, fontDef.getCharacterHeight(), itemColor, scrollScale);
}

TextureAsset ProvinceSearchUiView::getListTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::PopUp8));
}

TextureAsset ProvinceSearchUiView::getListPaletteTextureAsset(const BinaryAssetLibrary &binaryAssetLibrary, int provinceID)
{
	const auto &exeData = binaryAssetLibrary.getExeData();
	const auto &provinceImgFilenames = exeData.locations.provinceImgFilenames;
	DebugAssertIndex(provinceImgFilenames, provinceID);
	const std::string &filename = provinceImgFilenames[provinceID];

	// Set all characters to uppercase because the texture manager expects 
	// extensions to be uppercase, and most filenames in A.EXE are lowercase.
	return String::toUppercase(filename);
}

UiTextureID ProvinceSearchUiView::allocParchmentTexture(TextureManager &textureManager, Renderer &renderer)
{
	const int width = ProvinceSearchUiView::TextureWidth;
	const int height = ProvinceSearchUiView::TextureHeight;
	const Surface surface = TextureUtils::generate(ProvinceSearchUiView::TexturePattern, width, height, textureManager, renderer);
	
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(width, height, &textureID))
	{
		DebugCrash("Couldn't create parchment texture (dims: " + std::to_string(width) + "x" + std::to_string(height) + ").");
	}

	uint32_t *dstTexels = renderer.lockUiTexture(textureID);
	if (dstTexels == nullptr)
	{
		DebugCrash("Couldn't lock parchment texels for writing.");
	}

	const int texelCount = width * height;
	const uint32_t *srcTexels = static_cast<const uint32_t*>(surface.getPixels());
	std::copy(srcTexels, srcTexels + texelCount, dstTexels);
	renderer.unlockUiTexture(textureID);

	return textureID;
}

UiTextureID ProvinceSearchUiView::allocListBackgroundTexture(int provinceID, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = ProvinceSearchUiView::getListTextureAsset();
	const TextureAsset paletteTextureAsset = ProvinceSearchUiView::getListPaletteTextureAsset(binaryAssetLibrary, provinceID);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create list background texture.");
	}

	return textureID;
}
