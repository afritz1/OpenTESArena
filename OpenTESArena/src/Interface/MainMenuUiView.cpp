#include "MainMenuUiModel.h"
#include "MainMenuUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../UI/FontLibrary.h"
#include "../Rendering/Renderer.h"
#include "../UI/Surface.h"

#include "components/utilities/Span2D.h"

Rect MainMenuUiView::getLoadButtonRect()
{
	return Rect(Int2(168, 58), 150, 20);
}

Rect MainMenuUiView::getNewGameButtonRect()
{
	return Rect(Int2(168, 112), 150, 20);
}

Rect MainMenuUiView::getExitButtonRect()
{
	return Rect(Int2(168, 158), 45, 20);
}

Rect MainMenuUiView::getTestButtonRect()
{
	return Rect(135, ArenaRenderUtils::SCREEN_HEIGHT - 17, 30, 14);
}

Color MainMenuUiView::getTestButtonTextColor()
{
	return Colors::White;
}

TextBoxInitInfo MainMenuUiView::getTestButtonTextBoxInitInfo(const std::string_view text,
	const FontLibrary &fontLibrary)
{
	const Rect rect = MainMenuUiView::getTestButtonRect();
	return TextBoxInitInfo::makeWithCenter(
		text,
		rect.getCenter(),
		MainMenuUiView::TestButtonFontName,
		MainMenuUiView::getTestButtonTextColor(),
		MainMenuUiView::TestButtonTextAlignment,
		fontLibrary);
}

TextBoxInitInfo MainMenuUiView::getTestTypeTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	const std::string &fontName = MainMenuUiView::TestButtonFontName;
	int fontIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontIndex))
	{
		DebugCrash("Couldn't get font definition \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontIndex);
	const std::string dummyText(15, TextRenderUtils::LARGEST_CHAR);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	const Rect buttonRect = MainMenuUiView::getTestTypeUpButtonRect();
	return TextBoxInitInfo::makeWithXY(
		dummyText,
		buttonRect.getLeft() - 2 - textureGenInfo.width,
		buttonRect.getBottom(),
		fontName,
		MainMenuUiView::getTestButtonTextColor(),
		TextAlignment::MiddleRight,
		fontLibrary);
}

TextBoxInitInfo MainMenuUiView::getTestNameTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	const std::string &fontName = MainMenuUiView::TestButtonFontName;
	int fontIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontIndex))
	{
		DebugCrash("Couldn't get font definition \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontIndex);
	const std::string dummyText(15, TextRenderUtils::LARGEST_CHAR);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	const Rect buttonRect = MainMenuUiView::getTestIndexUpButtonRect();
	return TextBoxInitInfo::makeWithXY(
		dummyText,
		buttonRect.getLeft() - 2 - textureGenInfo.width,
		buttonRect.getBottom(),
		fontName,
		MainMenuUiView::getTestButtonTextColor(),
		TextAlignment::MiddleRight,
		fontLibrary);
}

TextBoxInitInfo MainMenuUiView::getTestWeatherTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	const std::string &fontName = MainMenuUiView::TestButtonFontName;
	int fontIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontIndex))
	{
		DebugCrash("Couldn't get font definition \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontIndex);
	const std::string dummyText(16, TextRenderUtils::LARGEST_CHAR);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	const Rect buttonRect = MainMenuUiView::getTestWeatherUpButtonRect();
	return TextBoxInitInfo::makeWithXY(
		dummyText,
		buttonRect.getLeft() - 2 - textureGenInfo.width,
		buttonRect.getBottom(),
		fontName,
		MainMenuUiView::getTestButtonTextColor(),
		TextAlignment::MiddleRight,
		fontLibrary);
}

Rect MainMenuUiView::getTestTypeUpButtonRect()
{
	return Rect(312, 164, 8, 8);
}

Rect MainMenuUiView::getTestTypeDownButtonRect()
{
	return Rect(312, 172, 8, 8);
}

Rect MainMenuUiView::getTestIndexUpButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestTypeUpButtonRect();
	return Rect(
		baseRect.getLeft() - baseRect.width - 2,
		baseRect.getTop() + (baseRect.height * 2) + 2,
		baseRect.width,
		baseRect.height);
}

Rect MainMenuUiView::getTestIndexDownButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestIndexUpButtonRect();
	return Rect(
		baseRect.getLeft(),
		baseRect.getBottom(),
		baseRect.width,
		baseRect.height);
}

Rect MainMenuUiView::getTestIndex2UpButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestIndexUpButtonRect();
	return Rect(
		baseRect.getLeft() + 10,
		baseRect.getTop(),
		baseRect.width,
		baseRect.height);
}

Rect MainMenuUiView::getTestIndex2DownButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestIndex2UpButtonRect();
	return Rect(
		baseRect.getLeft(),
		baseRect.getBottom(),
		baseRect.width,
		baseRect.height);
}

Rect MainMenuUiView::getTestWeatherUpButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestTypeUpButtonRect();
	return Rect(
		baseRect.getLeft(),
		baseRect.getTop() - 2 - (2 * baseRect.height),
		baseRect.width,
		baseRect.height);
}

Rect MainMenuUiView::getTestWeatherDownButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestWeatherUpButtonRect();
	return Rect(
		baseRect.getLeft(),
		baseRect.getBottom(),
		baseRect.width,
		baseRect.height);
}

TextureAsset MainMenuUiView::getBackgroundTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::MainMenu));
}

TextureAsset MainMenuUiView::getPaletteTextureAsset()
{
	return MainMenuUiView::getBackgroundTextureAsset();
}

TextureAsset MainMenuUiView::getTestArrowsTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::UpDown));
}

TextureAsset MainMenuUiView::getTestArrowsPaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaPaletteName::CharSheet));
}

UiTextureID MainMenuUiView::allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = MainMenuUiView::getBackgroundTextureAsset();
	const TextureAsset paletteTextureAsset = MainMenuUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for main menu background \"" + textureAsset.filename + "\".");
	}

	return textureID;
}

UiTextureID MainMenuUiView::allocTestArrowsTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = MainMenuUiView::getTestArrowsTextureAsset();
	const TextureAsset paletteTextureAsset = MainMenuUiView::getTestArrowsPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for main menu test arrows \"" + textureAsset.filename + "\".");
	}

	return textureID;
}

UiTextureID MainMenuUiView::allocTestButtonTexture(TextureManager &textureManager, Renderer &renderer)
{
	const Rect rect = MainMenuUiView::getTestButtonRect();
	const Surface surface = TextureUtils::generate(MainMenuUiView::TestButtonPatternType, rect.width, rect.height, textureManager, renderer);
	const Span2D<const uint32_t> pixelsView(static_cast<const uint32_t*>(surface.getPixels()), surface.getWidth(), surface.getHeight());

	const UiTextureID textureID = renderer.createUiTexture(pixelsView);
	if (textureID < 0)
	{
		DebugCrash("Couldn't create UI texture for test button.");
	}

	return textureID;
}
