#include <algorithm>

#include "ProvinceMapUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../UI/FontDefinition.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../WorldMap/LocationUtils.h"

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

Int2 ProvinceMapUiView::getLocationTextClampedCenter(const Rect &unclampedRect)
{
	const Int2 unclampedTopLeft = unclampedRect.getTopLeft();
	const Int2 clampedTopLeft(
		std::clamp(unclampedTopLeft.x, 2, ArenaRenderUtils::SCREEN_WIDTH - unclampedRect.getWidth() - 2),
		std::clamp(unclampedTopLeft.y, 2, ArenaRenderUtils::SCREEN_HEIGHT - unclampedRect.getHeight() - 2));
	return clampedTopLeft + Int2(unclampedRect.getWidth() / 2, unclampedRect.getHeight() / 2);
}

TextBox::InitInfo ProvinceMapUiView::getHoveredLocationTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	const std::string dummyText(24, TextRenderUtils::LARGEST_CHAR);

	TextRenderUtils::TextShadowInfo shadowInfo;
	shadowInfo.init(ProvinceMapUiView::LocationTextShadowOffsetX, ProvinceMapUiView::LocationTextShadowOffsetY,
		ProvinceMapUiView::LocationTextShadowColor);
	constexpr int lineSpacing = 0;

	return TextBox::InitInfo::makeWithCenter(
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
	return provinceID != LocationUtils::CENTER_PROVINCE_ID;
}

TextureAssetReference ProvinceMapUiView::getBackgroundTextureAssetRef(int provinceID, const BinaryAssetLibrary &binaryAssetLibrary)
{
	const auto &exeData = binaryAssetLibrary.getExeData();
	const auto &provinceImgFilenames = exeData.locations.provinceImgFilenames;
	DebugAssertIndex(provinceImgFilenames, provinceID);
	const std::string &filename = provinceImgFilenames[provinceID];
	return TextureAssetReference(String::toUppercase(filename));
}

TextureAssetReference ProvinceMapUiView::getBackgroundPaletteTextureAssetRef(int provinceID, const BinaryAssetLibrary &binaryAssetLibrary)
{
	return TextureAssetReference(ProvinceMapUiView::getBackgroundTextureAssetRef(provinceID, binaryAssetLibrary));
}

TextureAssetReference ProvinceMapUiView::getCityStateIconTextureAssetRef(HighlightType highlightType)
{
	if (highlightType == HighlightType::None)
	{
		return TextureAssetReference(std::string(ArenaTextureName::CityStateIcon));
	}
	else if (highlightType == HighlightType::PlayerLocation)
	{
		return TextureAssetReference(std::string(ArenaTextureName::MapIconOutlines), ProvinceMapUiView::CityStateIconHighlightIndex);
	}
	else if (highlightType == HighlightType::TravelDestination)
	{
		return TextureAssetReference(std::string(ArenaTextureName::MapIconOutlinesBlinking), ProvinceMapUiView::CityStateIconHighlightIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(TextureAssetReference, std::to_string(static_cast<int>(highlightType)));
	}
}

TextureAssetReference ProvinceMapUiView::getTownIconTextureAssetRef(HighlightType highlightType)
{
	if (highlightType == HighlightType::None)
	{
		return TextureAssetReference(std::string(ArenaTextureName::TownIcon));
	}
	else if (highlightType == HighlightType::PlayerLocation)
	{
		return TextureAssetReference(std::string(ArenaTextureName::MapIconOutlines), ProvinceMapUiView::TownIconHighlightIndex);
	}
	else if (highlightType == HighlightType::TravelDestination)
	{
		return TextureAssetReference(std::string(ArenaTextureName::MapIconOutlinesBlinking), ProvinceMapUiView::TownIconHighlightIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(TextureAssetReference, std::to_string(static_cast<int>(highlightType)));
	}
}

TextureAssetReference ProvinceMapUiView::getVillageIconTextureAssetRef(HighlightType highlightType)
{
	if (highlightType == HighlightType::None)
	{
		return TextureAssetReference(std::string(ArenaTextureName::VillageIcon));
	}
	else if (highlightType == HighlightType::PlayerLocation)
	{
		return TextureAssetReference(std::string(ArenaTextureName::MapIconOutlines), ProvinceMapUiView::VillageIconHighlightIndex);
	}
	else if (highlightType == HighlightType::TravelDestination)
	{
		return TextureAssetReference(std::string(ArenaTextureName::MapIconOutlinesBlinking), ProvinceMapUiView::VillageIconHighlightIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(TextureAssetReference, std::to_string(static_cast<int>(highlightType)));
	}
}

TextureAssetReference ProvinceMapUiView::getDungeonIconTextureAssetRef(HighlightType highlightType)
{
	if (highlightType == HighlightType::None)
	{
		return TextureAssetReference(std::string(ArenaTextureName::DungeonIcon));
	}
	else if (highlightType == HighlightType::PlayerLocation)
	{
		return TextureAssetReference(std::string(ArenaTextureName::MapIconOutlines), ProvinceMapUiView::DungeonIconHighlightIndex);
	}
	else if (highlightType == HighlightType::TravelDestination)
	{
		return TextureAssetReference(std::string(ArenaTextureName::MapIconOutlinesBlinking), ProvinceMapUiView::DungeonIconHighlightIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(TextureAssetReference, std::to_string(static_cast<int>(highlightType)));
	}
}

TextureAssetReference ProvinceMapUiView::getStaffDungeonIconTextureAssetRef(int provinceID)
{
	return TextureAssetReference(std::string(ArenaTextureName::StaffDungeonIcons), provinceID);
}

UiTextureID ProvinceMapUiView::allocBackgroundTexture(int provinceID, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAssetReference textureAssetRef = ProvinceMapUiView::getBackgroundTextureAssetRef(provinceID, binaryAssetLibrary);
	const TextureAssetReference paletteTextureAssetRef = ProvinceMapUiView::getBackgroundPaletteTextureAssetRef(provinceID, binaryAssetLibrary);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate province \"" + std::to_string(provinceID) + "\" background texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocCityStateIconTexture(HighlightType highlightType, const TextureAssetReference &paletteTextureAssetRef,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAssetReference textureAssetRef = ProvinceMapUiView::getCityStateIconTextureAssetRef(highlightType);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate city state icon texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocTownIconTexture(HighlightType highlightType, const TextureAssetReference &paletteTextureAssetRef,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAssetReference textureAssetRef = ProvinceMapUiView::getTownIconTextureAssetRef(highlightType);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate town icon texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocVillageIconTexture(HighlightType highlightType, const TextureAssetReference &paletteTextureAssetRef,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAssetReference textureAssetRef = ProvinceMapUiView::getVillageIconTextureAssetRef(highlightType);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate village icon texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocDungeonIconTexture(HighlightType highlightType, const TextureAssetReference &paletteTextureAssetRef,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAssetReference textureAssetRef = ProvinceMapUiView::getDungeonIconTextureAssetRef(highlightType);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't allocate dungeon icon texture.");
	}

	return textureID;
}

UiTextureID ProvinceMapUiView::allocStaffDungeonIconTexture(int provinceID, HighlightType highlightType,
	const TextureAssetReference &paletteTextureAssetRef, TextureManager &textureManager, Renderer &renderer)
{
	DebugAssert(ProvinceMapUiView::provinceHasStaffDungeonIcon(provinceID));

	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteTextureAssetRef);
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get staff dungeon palette ID for \"" + paletteTextureAssetRef.filename + "\".");
	}

	const TextureAssetReference textureAssetRef = ProvinceMapUiView::getStaffDungeonIconTextureAssetRef(provinceID);
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get staff dungeon texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, *paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create staff dungeon texture for \"" + textureAssetRef.filename + "\".");
	}

	if (highlightType == HighlightType::None)
	{
		return textureID;
	}

	// Modify icon background texels based on the highlight type.
	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
	DebugAssert(textureBuilder.getType() == TextureBuilder::Type::Paletted);

	const TextureBuilder::PalettedTexture &srcTexture = textureBuilder.getPaletted();
	const uint8_t *srcTexels = srcTexture.texels.get();
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

TextBox::InitInfo ProvinceSearchUiView::getTitleTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithXY(
		text,
		ProvinceSearchUiView::TitleTextBoxX,
		ProvinceSearchUiView::TitleTextBoxY,
		ProvinceSearchUiView::TitleFontName,
		ProvinceSearchUiView::TitleColor,
		ProvinceSearchUiView::TitleTextAlignment,
		fontLibrary);
}

TextBox::InitInfo ProvinceSearchUiView::getTextEntryTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	const std::string dummyText(ProvinceSearchUiModel::MaxNameLength, TextRenderUtils::LARGEST_CHAR);
	const Int2 &origin = ProvinceSearchUiView::DefaultTextCursorPosition;
	return TextBox::InitInfo::makeWithXY(
		dummyText,
		origin.x,
		origin.y,
		ProvinceSearchUiView::TextEntryFontName,
		ProvinceSearchUiView::TextEntryColor,
		ProvinceSearchUiView::TextEntryTextAlignment,
		fontLibrary);
}

ListBox::Properties ProvinceSearchUiView::makeListBoxProperties(const FontLibrary &fontLibrary)
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

TextureAssetReference ProvinceSearchUiView::getListTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::PopUp8));
}

TextureAssetReference ProvinceSearchUiView::getListPaletteTextureAssetRef(const BinaryAssetLibrary &binaryAssetLibrary, int provinceID)
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
	const TextureAssetReference textureAssetRef = ProvinceSearchUiView::getListTextureAssetRef();
	const TextureAssetReference paletteTextureAssetRef = ProvinceSearchUiView::getListPaletteTextureAssetRef(binaryAssetLibrary, provinceID);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create list background texture.");
	}

	return textureID;
}
