#include "OptionsUiModel.h"
#include "OptionsUiView.h"
#include "../UI/FontLibrary.h"
#include "../Rendering/Renderer.h"
#include "../UI/Surface.h"

const Rect OptionsUiView::getTabRect(int index)
{
	const Int2 tabsOrigin(3, 6);
	const Int2 tabsDimensions(54, 16);
	return Rect(
		tabsOrigin.x,
		tabsOrigin.y + (tabsDimensions.y * index),
		tabsDimensions.x,
		tabsDimensions.y);
}

const Rect OptionsUiView::getListRect()
{
	const Rect firstTabRect = OptionsUiView::getTabRect(0);
	const Int2 listOrigin(
		firstTabRect.getRight() + 5,
		firstTabRect.getTop());
	const Int2 listDimensions(
		254,
		firstTabRect.height * 5);

	return Rect(
		listOrigin.x,
		listOrigin.y,
		listDimensions.x,
		listDimensions.y);
}

const Int2 OptionsUiView::getDescriptionXY()
{
	return Int2(5, 122);
}

Color OptionsUiView::getBackButtonTextColor()
{
	return Colors::White;
}

TextBoxInitInfo OptionsUiView::getBackButtonTextBoxInitInfo(const std::string_view text,
	const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithCenter(
		text,
		OptionsUiView::BackButtonTextBoxCenterPoint,
		OptionsUiView::BackButtonFontName,
		OptionsUiView::getBackButtonTextColor(),
		OptionsUiView::BackButtonTextAlignment,
		fontLibrary);
}

Rect OptionsUiView::getBackButtonRect()
{
	return Rect(OptionsUiView::BackButtonTextBoxCenterPoint, 40, 16);
}

Color OptionsUiView::getTabTextColor()
{
	return Colors::White;
}

Color OptionsUiView::getOptionTextBoxColor()
{
	return Colors::White;
}

Color OptionsUiView::getDescriptionTextColor()
{
	return Colors::White;
}

TextBoxInitInfo OptionsUiView::getTabTextBoxInitInfo(int index, const std::string_view text,
	const FontLibrary &fontLibrary)
{
	const Rect tabRect = OptionsUiView::getTabRect(index);
	return TextBoxInitInfo::makeWithCenter(
		text,
		tabRect.getCenter(),
		OptionsUiView::TabFontName,
		OptionsUiView::getTabTextColor(),
		OptionsUiView::TabTextAlignment,
		fontLibrary);
}

TextBoxInitInfo OptionsUiView::getOptionTextBoxInitInfo(int index, const FontLibrary &fontLibrary)
{
	const std::string dummyText(28, TextRenderUtils::LARGEST_CHAR);

	const std::string &fontName = OptionsUiView::OptionTextBoxFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font library index for font \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const Rect listRect = OptionsUiView::getListRect();
	const Rect rect(
		listRect.getLeft(),
		listRect.getTop() + (fontDef.getCharacterHeight() * index),
		listRect.width,
		listRect.height);
	return TextBoxInitInfo::makeWithXY(
		dummyText,
		rect.getLeft(),
		rect.getTop(),
		fontName,
		OptionsUiView::getOptionTextBoxColor(),
		OptionsUiView::OptionTextBoxTextAlignment,
		fontLibrary);
}

TextBoxInitInfo OptionsUiView::getDescriptionTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	std::string dummyText(36, TextRenderUtils::LARGEST_CHAR);
	for (int i = 0; i < 8; i++)
	{
		dummyText += '\n';
	}

	const Int2 origin = OptionsUiView::getDescriptionXY();
	return TextBoxInitInfo::makeWithXY(
		dummyText,
		origin.x,
		origin.y,
		OptionsUiView::DescriptionTextFontName,
		OptionsUiView::getDescriptionTextColor(),
		OptionsUiView::DescriptionTextAlignment,
		fontLibrary);
}

UiTextureID OptionsUiView::allocBackgroundTexture(Renderer &renderer)
{
	constexpr int width = ArenaRenderUtils::SCREEN_WIDTH;
	constexpr int height = ArenaRenderUtils::SCREEN_HEIGHT;
	const UiTextureID textureID = renderer.createUiTexture(width, height);
	if (textureID < 0)
	{
		DebugCrash("Couldn't create UI texture for options menu background.");
	}

	LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
	if (!lockedTexture.isValid())
	{
		DebugCrash("Couldn't lock texels for updating options menu background.");
	}

	Span2D<uint32_t> texels = lockedTexture.getTexels32();
	const uint32_t color = OptionsUiView::BackgroundColor.toRGBA();
	texels.fill(color);
	renderer.unlockUiTexture(textureID);

	return textureID;
}

UiTextureID OptionsUiView::allocTabTexture(TextureManager &textureManager, Renderer &renderer)
{
	const Rect firstTabRect = OptionsUiView::getTabRect(0);
	const Surface surface = TextureUtils::generate(
		OptionsUiView::TabBackgroundPatternType,
		firstTabRect.width,
		firstTabRect.height,
		textureManager,
		renderer);

	const UiTextureID textureID = renderer.createUiTexture(surface.getWidth(), surface.getHeight());
	if (textureID < 0)
	{
		DebugCrash("Couldn't create UI texture for options menu tab.");
	}

	LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
	if (!lockedTexture.isValid())
	{
		DebugCrash("Couldn't lock texels for updating options menu tab.");
	}

	Span2D<const uint32_t> srcTexels = surface.getPixels();
	Span2D<uint32_t> dstTexels = lockedTexture.getTexels32();
	std::copy(srcTexels.begin(), srcTexels.end(), dstTexels.begin());
	renderer.unlockUiTexture(textureID);

	return textureID;
}

UiTextureID OptionsUiView::allocHighlightTexture(Renderer &renderer)
{
	const Rect listRect = OptionsUiView::getListRect();
	const int width = listRect.width;
	const int height = 9;
	const UiTextureID textureID = renderer.createUiTexture(width, height);
	if (textureID < 0)
	{
		DebugCrash("Couldn't create UI texture for highlighted option.");
	}

	LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
	if (!lockedTexture.isValid())
	{
		DebugCrash("Couldn't lock texels for updating highlighted option.");
	}

	Span2D<uint32_t> texels = lockedTexture.getTexels32();
	const uint32_t color = OptionsUiView::HighlightColor.toRGBA();
	texels.fill(color);
	renderer.unlockUiTexture(textureID);

	return textureID;
}

UiTextureID OptionsUiView::allocBackButtonTexture(TextureManager &textureManager, Renderer &renderer)
{
	const Rect backButtonRect = OptionsUiView::getBackButtonRect();
	const Surface surface = TextureUtils::generate(
		OptionsUiView::TabBackgroundPatternType,
		backButtonRect.width,
		backButtonRect.height,
		textureManager,
		renderer);

	const UiTextureID textureID = renderer.createUiTexture(surface.getWidth(), surface.getHeight());
	if (textureID < 0)
	{
		DebugCrash("Couldn't create UI texture for options menu back button.");
	}

	LockedTexture lockedTexture = renderer.lockUiTexture(textureID);
	if (!lockedTexture.isValid())
	{
		DebugCrash("Couldn't lock texels for updating options menu back button.");
	}

	Span2D<const uint32_t> srcTexels = surface.getPixels();
	Span2D<uint32_t> dstTexels = lockedTexture.getTexels32();
	std::copy(srcTexels.begin(), srcTexels.end(), dstTexels.begin());
	renderer.unlockUiTexture(textureID);

	return textureID;
}
