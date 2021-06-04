#ifndef CHARACTER_CREATION_UI_VIEW_H
#define CHARACTER_CREATION_UI_VIEW_H

#include "../Assets/TextureAssetReference.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/FontName.h"
#include "../UI/TextAlignment.h"

class ExeData;
class Game;
class Rect;

namespace CharacterCreationUiView
{
	// -- Common --
	constexpr int MaxTooltipLineLength = 14;

	TextureAssetReference getNightSkyTextureAssetRef();

	// -- Choose class creation --
	constexpr int ChooseClassCreationPopUpTextureWidth = 180;
	constexpr int ChooseClassCreationPopUpTextureHeight = 40;
	constexpr TextureUtils::PatternType ChooseClassCreationPopUpPatternType = TextureUtils::PatternType::Parchment;

	const Int2 ChooseClassCreationTitleCenter((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 80);
	constexpr FontName ChooseClassCreationTitleFontName = FontName::A;
	const Color ChooseClassCreationTitleColor(48, 12, 12);
	constexpr TextAlignment ChooseClassCreationTitleAlignment = TextAlignment::Center;
	constexpr int ChooseClassCreationTitleLineSpacing = 1;

	const Int2 GenerateClassTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 120);
	constexpr FontName GenerateClassTextFontName = FontName::A;
	const Color GenerateClassTextColor(48, 12, 12);
	constexpr TextAlignment GenerateClassTextAlignment = TextAlignment::Center;

	const Int2 GenerateClassButtonCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 120);
	constexpr int GenerateClassButtonWidth = 175;
	constexpr int GenerateClassButtonHeight = 35;

	const Int2 SelectClassTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 160);
	constexpr FontName SelectClassTextFontName = FontName::A;
	const Color SelectClassTextColor(48, 12, 12);
	constexpr TextAlignment SelectClassTextAlignment = TextAlignment::Center;

	const Int2 SelectClassButtonCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 160);
	constexpr int SelectClassButtonWidth = 175;
	constexpr int SelectClassButtonHeight = 35;

	// -- Choose class --
	constexpr int ChooseClassTitleX = 89;
	constexpr int ChooseClassTitleY = 32;
	constexpr FontName ChooseClassTitleFontName = FontName::C;
	const Color ChooseClassTitleColor(211, 211, 211);
	constexpr TextAlignment ChooseClassTitleAlignment = TextAlignment::Left;

	constexpr int ChooseClassListBoxTextureX = 55;
	constexpr int ChooseClassListBoxTextureY = 9;
	const Color ChooseClassListBoxTextColor(85, 44, 20);
	constexpr FontName ChooseClassListBoxFontName = FontName::A;
	constexpr int ChooseClassListBoxMaxDisplayedItems = 6;

	Rect getClassListRect(Game &game);
	Rect getClassListUpButtonRect(Game &game);
	Rect getClassListDownButtonRect(Game &game);
	
	TextureAssetReference getChooseClassListBoxTextureAssetRef();

	// -- Choose gender --
	constexpr int ChooseGenderTextureWidth = 180;
	constexpr int ChooseGenderTextureHeight = 40;
	constexpr TextureUtils::PatternType ChooseGenderTexturePatternType = TextureUtils::PatternType::Parchment;

	const Int2 ChooseGenderTitleCenterPoint(ArenaRenderUtils::SCREEN_WIDTH / 2, 80);
	constexpr FontName ChooseGenderTitleFontName = FontName::A;
	const Color ChooseGenderTitleColor(48, 12, 12);
	constexpr TextAlignment ChooseGenderTitleAlignment = TextAlignment::Center;

	const Int2 ChooseGenderMaleTextBoxCenter(ArenaRenderUtils::SCREEN_WIDTH / 2, 120);
	constexpr FontName ChooseGenderMaleFontName = FontName::A;
	const Color ChooseGenderMaleColor(48, 12, 12);
	constexpr TextAlignment ChooseGenderMaleAlignment = TextAlignment::Center;

	const Int2 ChooseGenderMaleButtonCenter(ArenaRenderUtils::SCREEN_WIDTH / 2, 120);
	constexpr int ChooseGenderMaleButtonWidth = 175;
	constexpr int ChooseGenderMaleButtonHeight = 35;

	const Int2 ChooseGenderFemaleTextBoxCenter(ArenaRenderUtils::SCREEN_WIDTH / 2, 160);
	constexpr FontName ChooseGenderFemaleFontName = FontName::A;
	const Color ChooseGenderFemaleColor(48, 12, 12);
	constexpr TextAlignment ChooseGenderFemaleAlignment = TextAlignment::Center;

	const Int2 ChooseGenderFemaleButtonCenter(ArenaRenderUtils::SCREEN_WIDTH / 2, 160);
	constexpr int ChooseGenderFemaleButtonWidth = 175;
	constexpr int ChooseGenderFemaleButtonHeight = 35;

	int getChooseGenderTitleTextureX(int textureWidth);
	int getChooseGenderTitleTextureY(int textureHeight);

	// -- Choose name --
	constexpr int ChooseNameTextureWidth = 300;
	constexpr int ChooseNameTextureHeight = 60;
	constexpr TextureUtils::PatternType ChooseNameTexturePatternType = TextureUtils::PatternType::Parchment;

	constexpr int ChooseNameTitleTextBoxX = 26;
	constexpr int ChooseNameTitleTextBoxY = 82;
	constexpr FontName ChooseNameTitleFontName = FontName::A;
	const Color ChooseNameTitleColor(48, 12, 12);
	constexpr TextAlignment ChooseNameTitleAlignment = TextAlignment::Left;

	constexpr int ChooseNameEntryTextBoxX = 61;
	constexpr int ChooseNameEntryTextBoxY = 101;
	constexpr FontName ChooseNameEntryFontName = FontName::A;
	const Color ChooseNameEntryColor(48, 12, 12);
	constexpr TextAlignment ChooseNameEntryAlignment = TextAlignment::Left;

	int getChooseNameTitleTextureX(int textureWidth);
	int getChooseNameTitleTextureY(int textureHeight);

	// -- Choose race --
	const Int2 ChooseRaceInitialPopUpTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	constexpr FontName ChooseRaceInitialPopUpFontName = FontName::A;
	const Color ChooseRaceInitialPopUpColor(48, 12, 12);
	constexpr TextAlignment ChooseRaceInitialPopUpAlignment = TextAlignment::Center;
	constexpr int ChooseRaceInitialPopUpLineSpacing = 1;
	constexpr TextureUtils::PatternType ChooseRaceInitialPopUpPatternType = TextureUtils::PatternType::Parchment;

	constexpr int ChooseRaceInitialPopUpTextureWidth = 240;
	constexpr int ChooseRaceInitialPopUpTextureHeight = 60;
	const Int2 ChooseRaceInitialPopUpTextureCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

	TextureAssetReference getChooseRaceBackgroundTextureAssetRef();
	TextureAssetReference getChooseRaceNoExitTextureAssetRef(); // Covers up the exit button since character creation doesn't use it.

	int getChooseRaceNoExitTextureX(int textureWidth);
	int getChooseRaceNoExitTextureY(int textureHeight);

	// -- Choose attributes --
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

	int getChooseClassCreationTitleTextureX(int textureWidth);
	int getChooseClassCreationTitleTextureY(int textureHeight);

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
