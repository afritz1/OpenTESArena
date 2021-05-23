#ifndef CHARACTER_CREATION_UI_VIEW_H
#define CHARACTER_CREATION_UI_VIEW_H

#include "../Assets/TextureAssetReference.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/FontName.h"
#include "../UI/TextAlignment.h"

class Game;

namespace CharacterCreationUiView
{
	const Int2 ChooseAttributesTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 2);
	constexpr FontName ChooseAttributesTextFontName = FontName::Arena;
	const Color ChooseAttributesTextColor(199, 199, 199);
	constexpr TextAlignment ChooseAttributesTextAlignment = TextAlignment::Center;
	constexpr int ChooseAttributesTextLineSpacing = 1;
	constexpr TextureUtils::PatternType ChooseAttributesTextPatternType = TextureUtils::PatternType::Dark;

	const Int2 ChooseAttributesTextureCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

	const Int2 AttributesMessageBoxCenterPoint(
		ArenaRenderUtils::SCREEN_WIDTH / 2,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 22);
	constexpr FontName AttributesMessageBoxTitleFontName = FontName::A;
	const Color AttributesMessageBoxTitleColor(199, 199, 199);
	constexpr TextAlignment AttributesMessageBoxTitleAlignment = TextAlignment::Center;
	constexpr TextureUtils::PatternType AttributesMessageBoxPatternType = TextureUtils::PatternType::Dark;

	// @todo: various properties of the message box buttons will likely be combined in the future by MessageBoxSubPanel.
	const Int2 AttributesMessageBoxSaveCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) + 2);
	constexpr FontName AttributesMessageBoxSaveFontName = FontName::A;
	const Color AttributesMessageBoxSaveColor(190, 113, 0);
	constexpr TextAlignment AttributesMessageBoxSaveAlignment = TextAlignment::Center;

	const Int2 AttributesMessageBoxRerollCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) + 26);
	constexpr FontName AttributesMessageBoxRerollFontName = FontName::A;
	const Color AttributesMessageBoxRerollColor(190, 113, 0);
	constexpr TextAlignment AttributesMessageBoxRerollAlignment = TextAlignment::Center;

	const Int2 AppearanceMessageBoxCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	constexpr FontName AppearanceMessageBoxFontName = FontName::Arena;
	const Color AppearanceMessageBoxColor(199, 199, 199);
	constexpr TextAlignment AppearanceMessageBoxAlignment = TextAlignment::Center;
	constexpr int AppearanceMessageBoxLineSpacing = 1;
	constexpr TextureUtils::PatternType AppearanceMessageBoxPatternType = TextureUtils::PatternType::Dark;

	const Int2 AppearancePortraitButtonCenterPoint(ArenaRenderUtils::SCREEN_WIDTH - 72, 25);
	constexpr int AppearancePortraitButtonWidth = 60;
	constexpr int AppearancePortraitButtonHeight = 42;

	int getChooseAttributesTextureWidth();
	int getChooseAttributesTextureHeight();

	int getAttributesMessageBoxTitleTextureX(int titleTextureWidth);
	int getAttributesMessageBoxTitleTextureY(int titleTextureHeight);
	int getAttributesMessageBoxTitleTextureWidth(int titleTextWidth);
	int getAttributesMessageBoxTitleTextureHeight();

	int getAppearanceMessageBoxTextureWidth(int textWidth);
	int getAppearanceMessageBoxTextureHeight(int textHeight);

	int getBodyOffsetX(Game &game);
	Int2 getHeadOffset(Game &game);
	Int2 getShirtOffset(Game &game);
	Int2 getPantsOffset(Game &game);

	TextureAssetReference getBodyTextureAssetRef(Game &game);
	TextureAssetReference getHeadTextureAssetRef(Game &game);
	TextureAssetReference getShirtTextureAssetRef(Game &game);
	TextureAssetReference getPantsTextureAssetRef(Game &game);
}

#endif
