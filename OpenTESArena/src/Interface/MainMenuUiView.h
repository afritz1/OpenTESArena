#ifndef MAIN_MENU_UI_VIEW_H
#define MAIN_MENU_UI_VIEW_H

#include <string>

#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureAssetReference.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/FontName.h"
#include "../UI/TextAlignment.h"

namespace MainMenuUiView
{
	const Int2 LoadButtonCenterPoint(168, 58);
	constexpr int LoadButtonWidth = 150;
	constexpr int LoadButtonHeight = 20;

	const Int2 NewGameButtonCenterPoint(168, 112);
	constexpr int NewGameButtonWidth = 150;
	constexpr int NewGameButtonHeight = 20;

	const Int2 ExitButtonCenterPoint(168, 158);
	constexpr int ExitButtonWidth = 45;
	constexpr int ExitButtonHeight = 20;

	// Test values.
	const Rect TestButtonRect(135, ArenaRenderUtils::SCREEN_HEIGHT - 17, 30, 14);
	const Int2 TestButtonTextBoxPoint(
		TestButtonRect.getLeft() + (TestButtonRect.getWidth() / 2),
		TestButtonRect.getTop() + (TestButtonRect.getHeight() / 2));
	constexpr TextureUtils::PatternType TestButtonPatternType = TextureUtils::PatternType::Custom1;
	constexpr FontName TestButtonFontName = FontName::Arena;
	constexpr TextAlignment TestButtonTextAlignment = TextAlignment::Center;

	// Can't rely on Color statics being initialized first.
	// @todo: change Color constructors to constexpr? Or is there still an ordering problem?
	Color getTestButtonTextColor()
	{
		return Color::White;
	}

	constexpr int TestTypeUpButtonX = 312;
	constexpr int TestTypeUpButtonY = 164;
	constexpr int TestTypeUpButtonWidth = 8;
	constexpr int TestTypeUpButtonHeight = 8;

	constexpr int TestTypeDownButtonX = TestTypeUpButtonX;
	constexpr int TestTypeDownButtonY = TestTypeUpButtonY + TestTypeUpButtonHeight;
	constexpr int TestTypeDownButtonWidth = TestTypeUpButtonWidth;
	constexpr int TestTypeDownButtonHeight = TestTypeUpButtonHeight;

	constexpr int TestIndexUpButtonX = TestTypeUpButtonX - TestTypeUpButtonWidth - 2;
	constexpr int TestIndexUpButtonY = TestTypeUpButtonY + (TestTypeUpButtonHeight * 2) + 2;
	constexpr int TestIndexUpButtonWidth = TestTypeDownButtonWidth;
	constexpr int TestIndexUpButtonHeight = TestTypeDownButtonHeight;

	constexpr int TestIndexDownButtonX = TestIndexUpButtonX;
	constexpr int TestIndexDownButtonY = TestIndexUpButtonY + TestIndexUpButtonHeight;
	constexpr int TestIndexDownButtonWidth = TestIndexUpButtonWidth;
	constexpr int TestIndexDownButtonHeight = TestIndexUpButtonHeight;

	constexpr int TestIndex2UpButtonX = TestIndexUpButtonX + 10;
	constexpr int TestIndex2UpButtonY = TestIndexUpButtonY;
	constexpr int TestIndex2UpButtonWidth = TestIndexUpButtonWidth;
	constexpr int TestIndex2UpButtonHeight = TestIndexUpButtonHeight;

	constexpr int TestIndex2DownButtonX = TestIndex2UpButtonX;
	constexpr int TestIndex2DownButtonY = TestIndex2UpButtonY + TestIndex2UpButtonHeight;
	constexpr int TestIndex2DownButtonWidth = TestIndex2UpButtonWidth;
	constexpr int TestIndex2DownButtonHeight = TestIndex2UpButtonHeight;

	constexpr int TestWeatherUpButtonX = TestTypeUpButtonX;
	constexpr int TestWeatherUpButtonY = TestTypeUpButtonY - 2 - (2 * TestTypeUpButtonHeight);
	constexpr int TestWeatherUpButtonWidth = TestTypeUpButtonWidth;
	constexpr int TestWeatherUpButtonHeight = TestTypeUpButtonHeight;

	constexpr int TestWeatherDownButtonX = TestWeatherUpButtonX;
	constexpr int TestWeatherDownButtonY = TestWeatherUpButtonY + TestWeatherUpButtonHeight;
	constexpr int TestWeatherDownButtonWidth = TestWeatherUpButtonWidth;
	constexpr int TestWeatherDownButtonHeight = TestWeatherUpButtonHeight;

	TextureAssetReference getBackgroundTextureAssetRef()
	{
		return TextureAssetReference(std::string(ArenaTextureName::MainMenu));
	}

	TextureAssetReference getPaletteTextureAssetRef()
	{
		return MainMenuUiView::getBackgroundTextureAssetRef();
	}

	TextureAssetReference getTestArrowsTextureAssetRef()
	{
		return TextureAssetReference(std::string(ArenaTextureName::UpDown));
	}

	TextureAssetReference getTestArrowsPaletteTextureAssetRef()
	{
		return TextureAssetReference(std::string(ArenaPaletteName::CharSheet));
	}
}

#endif
