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
		firstTabRect.getHeight() * 5);

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
	return Color::White;
}

TextBox::InitInfo OptionsUiView::getBackButtonTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
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
	return Color::White;
}

Color OptionsUiView::getOptionTextBoxColor()
{
	return Color::White;
}

Color OptionsUiView::getDescriptionTextColor()
{
	return Color::White;
}

TextBox::InitInfo OptionsUiView::getTabTextBoxInitInfo(int index, const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	const Rect tabRect = OptionsUiView::getTabRect(index);
	return TextBox::InitInfo::makeWithCenter(
		text,
		tabRect.getCenter(),
		OptionsUiView::TabFontName,
		OptionsUiView::getTabTextColor(),
		OptionsUiView::TabTextAlignment,
		fontLibrary);
}

TextBox::InitInfo OptionsUiView::getOptionTextBoxInitInfo(int index, const FontLibrary &fontLibrary)
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
		listRect.getWidth(),
		listRect.getHeight());
	return TextBox::InitInfo::makeWithXY(
		dummyText,
		rect.getLeft(),
		rect.getTop(),
		fontName,
		OptionsUiView::getOptionTextBoxColor(),
		OptionsUiView::OptionTextBoxTextAlignment,
		fontLibrary);
}

TextBox::InitInfo OptionsUiView::getDescriptionTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	std::string dummyText(36, TextRenderUtils::LARGEST_CHAR);
	for (int i = 0; i < 8; i++)
	{
		dummyText += '\n';
	}

	const Int2 origin = OptionsUiView::getDescriptionXY();
	return TextBox::InitInfo::makeWithXY(
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
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(width, height, &textureID))
	{
		DebugCrash("Couldn't create UI texture for options menu background.");
	}

	uint32_t *texels = renderer.lockUiTexture(textureID);
	if (texels == nullptr)
	{
		DebugCrash("Couldn't lock texels for updating options menu background.");
	}

	uint32_t *texelsEnd = texels + (width * height);
	const uint32_t color = OptionsUiView::BackgroundColor.toARGB();
	std::fill(texels, texelsEnd, color);
	renderer.unlockUiTexture(textureID);

	return textureID;
}

UiTextureID OptionsUiView::allocTabTexture(TextureManager &textureManager, Renderer &renderer)
{
	const Rect firstTabRect = OptionsUiView::getTabRect(0);
	Surface surface = TextureUtils::generate(
		OptionsUiView::TabBackgroundPatternType,
		firstTabRect.getWidth(),
		firstTabRect.getHeight(),
		textureManager,
		renderer);

	const int width = surface.getWidth();
	const int height = surface.getHeight();
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(width, height, &textureID))
	{
		DebugCrash("Couldn't create UI texture for options menu tab.");
	}

	uint32_t *dstTexels = renderer.lockUiTexture(textureID);
	if (dstTexels == nullptr)
	{
		DebugCrash("Couldn't lock texels for updating options menu tab.");
	}

	const int texelCount = width * height;
	const uint32_t *srcTexels = reinterpret_cast<const uint32_t*>(surface.getPixels());
	const uint32_t *srcTexelsEnd = srcTexels + texelCount;
	std::copy(srcTexels, srcTexelsEnd, dstTexels);
	renderer.unlockUiTexture(textureID);

	return textureID;
}

UiTextureID OptionsUiView::allocHighlightTexture(Renderer &renderer)
{
	const Rect listRect = OptionsUiView::getListRect();
	const int width = listRect.getWidth();
	const int height = 9;
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(width, height, &textureID))
	{
		DebugCrash("Couldn't create UI texture for highlighted option.");
	}

	uint32_t *texels = renderer.lockUiTexture(textureID);
	if (texels == nullptr)
	{
		DebugCrash("Couldn't lock texels for updating highlighted option.");
	}

	uint32_t *texelsEnd = texels + (width * height);
	const uint32_t color = OptionsUiView::HighlightColor.toARGB();
	std::fill(texels, texelsEnd, color);
	renderer.unlockUiTexture(textureID);

	return textureID;
}

UiTextureID OptionsUiView::allocBackButtonTexture(TextureManager &textureManager, Renderer &renderer)
{
	const Rect backButtonRect = OptionsUiView::getBackButtonRect();
	Surface surface = TextureUtils::generate(
		OptionsUiView::TabBackgroundPatternType,
		backButtonRect.getWidth(),
		backButtonRect.getHeight(),
		textureManager,
		renderer);

	const int width = surface.getWidth();
	const int height = surface.getHeight();
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(width, height, &textureID))
	{
		DebugCrash("Couldn't create UI texture for options menu back button.");
	}

	uint32_t *dstTexels = renderer.lockUiTexture(textureID);
	if (dstTexels == nullptr)
	{
		DebugCrash("Couldn't lock texels for updating options menu back button.");
	}

	const int texelCount = width * height;
	const uint32_t *srcTexels = reinterpret_cast<const uint32_t*>(surface.getPixels());
	const uint32_t *srcTexelsEnd = srcTexels + texelCount;
	std::copy(srcTexels, srcTexelsEnd, dstTexels);
	renderer.unlockUiTexture(textureID);

	return textureID;
}
