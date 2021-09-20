#include "MainMenuUiModel.h"
#include "MainMenuUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../UI/FontLibrary.h"
#include "../Rendering/Renderer.h"
#include "../UI/Surface.h"

#include "components/utilities/BufferView2D.h"

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
	return Color::White;
}

TextBox::InitInfo MainMenuUiView::getTestButtonTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	const Rect rect = MainMenuUiView::getTestButtonRect();
	return TextBox::InitInfo::makeWithCenter(
		text,
		rect.getCenter(),
		MainMenuUiView::TestButtonFontName,
		MainMenuUiView::getTestButtonTextColor(),
		MainMenuUiView::TestButtonTextAlignment,
		fontLibrary);
}

TextBox::InitInfo MainMenuUiView::getTestTypeTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	const std::string &fontName = MainMenuUiView::TestButtonFontName;
	int fontIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontIndex))
	{
		DebugCrash("Couldn't get font definition \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontIndex);
	const std::string dummyText(15, TextRenderUtils::LARGEST_CHAR);
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	const Rect buttonRect = MainMenuUiView::getTestTypeUpButtonRect();
	return TextBox::InitInfo::makeWithXY(
		dummyText,
		buttonRect.getLeft() - 2 - textureGenInfo.width,
		buttonRect.getBottom(),
		fontName,
		MainMenuUiView::getTestButtonTextColor(),
		TextAlignment::MiddleRight,
		fontLibrary);
}

TextBox::InitInfo MainMenuUiView::getTestNameTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	const std::string &fontName = MainMenuUiView::TestButtonFontName;
	int fontIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontIndex))
	{
		DebugCrash("Couldn't get font definition \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontIndex);
	const std::string dummyText(15, TextRenderUtils::LARGEST_CHAR);
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	const Rect buttonRect = MainMenuUiView::getTestIndexUpButtonRect();
	return TextBox::InitInfo::makeWithXY(
		dummyText,
		buttonRect.getLeft() - 2 - textureGenInfo.width,
		buttonRect.getBottom(),
		fontName,
		MainMenuUiView::getTestButtonTextColor(),
		TextAlignment::MiddleRight,
		fontLibrary);
}

TextBox::InitInfo MainMenuUiView::getTestWeatherTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	const std::string &fontName = MainMenuUiView::TestButtonFontName;
	int fontIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontIndex))
	{
		DebugCrash("Couldn't get font definition \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontIndex);
	const std::string dummyText(16, TextRenderUtils::LARGEST_CHAR);
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	const Rect buttonRect = MainMenuUiView::getTestWeatherUpButtonRect();
	return TextBox::InitInfo::makeWithXY(
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
		baseRect.getLeft() - baseRect.getWidth() - 2,
		baseRect.getTop() + (baseRect.getHeight() * 2) + 2,
		baseRect.getWidth(),
		baseRect.getHeight());
}

Rect MainMenuUiView::getTestIndexDownButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestIndexUpButtonRect();
	return Rect(
		baseRect.getLeft(),
		baseRect.getBottom(),
		baseRect.getWidth(),
		baseRect.getHeight());
}

Rect MainMenuUiView::getTestIndex2UpButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestIndexUpButtonRect();
	return Rect(
		baseRect.getLeft() + 10,
		baseRect.getTop(),
		baseRect.getWidth(),
		baseRect.getHeight());
}

Rect MainMenuUiView::getTestIndex2DownButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestIndex2UpButtonRect();
	return Rect(
		baseRect.getLeft(),
		baseRect.getBottom(),
		baseRect.getWidth(),
		baseRect.getHeight());
}

Rect MainMenuUiView::getTestWeatherUpButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestTypeUpButtonRect();
	return Rect(
		baseRect.getLeft(),
		baseRect.getTop() - 2 - (2 * baseRect.getHeight()),
		baseRect.getWidth(),
		baseRect.getHeight());
}

Rect MainMenuUiView::getTestWeatherDownButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestWeatherUpButtonRect();
	return Rect(
		baseRect.getLeft(),
		baseRect.getBottom(),
		baseRect.getWidth(),
		baseRect.getHeight());
}

TextureAssetReference MainMenuUiView::getBackgroundTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::MainMenu));
}

TextureAssetReference MainMenuUiView::getPaletteTextureAssetRef()
{
	return MainMenuUiView::getBackgroundTextureAssetRef();
}

TextureAssetReference MainMenuUiView::getTestArrowsTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::UpDown));
}

TextureAssetReference MainMenuUiView::getTestArrowsPaletteTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaPaletteName::CharSheet));
}

UiTextureID MainMenuUiView::allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAssetReference textureAssetRef = MainMenuUiView::getBackgroundTextureAssetRef();
	const TextureAssetReference paletteTextureAssetRef = MainMenuUiView::getPaletteTextureAssetRef();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for main menu background \"" + textureAssetRef.filename + "\".");
	}

	return textureID;
}

UiTextureID MainMenuUiView::allocTestArrowsTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAssetReference textureAssetRef = MainMenuUiView::getTestArrowsTextureAssetRef();
	const TextureAssetReference paletteTextureAssetRef = MainMenuUiView::getTestArrowsPaletteTextureAssetRef();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAssetRef, paletteTextureAssetRef, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for main menu test arrows \"" + textureAssetRef.filename + "\".");
	}

	return textureID;
}

UiTextureID MainMenuUiView::allocTestButtonTexture(TextureManager &textureManager, Renderer &renderer)
{
	const Rect rect = MainMenuUiView::getTestButtonRect();
	const Surface surface = TextureUtils::generate(MainMenuUiView::TestButtonPatternType,
		rect.getWidth(), rect.getHeight(), textureManager, renderer);
	const BufferView2D<const uint32_t> pixelsView(static_cast<const uint32_t*>(surface.getPixels()),
		surface.getWidth(), surface.getHeight());

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(pixelsView, &textureID))
	{
		DebugCrash("Couldn't create UI texture for test button.");
	}

	return textureID;
}
