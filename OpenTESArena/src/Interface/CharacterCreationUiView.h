#ifndef CHARACTER_CREATION_UI_VIEW_H
#define CHARACTER_CREATION_UI_VIEW_H

#include "../Assets/TextureAssetReference.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/ListBox.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

class ExeData;
class Game;
class Rect;

// @todo: might make each of these sections their own namespace in this file so the names aren't so long.

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
	const std::string ChooseClassCreationTitleFontName = ArenaFontName::A;
	const Color ChooseClassCreationTitleColor(48, 12, 12);
	constexpr TextAlignment ChooseClassCreationTitleAlignment = TextAlignment::MiddleCenter;
	constexpr int ChooseClassCreationTitleLineSpacing = 1;

	const Int2 GenerateClassTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 120);
	const std::string GenerateClassTextFontName = ArenaFontName::A;
	const Color GenerateClassTextColor(48, 12, 12);
	constexpr TextAlignment GenerateClassTextAlignment = TextAlignment::MiddleCenter;

	const Int2 GenerateClassButtonCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 120);
	constexpr int GenerateClassButtonWidth = 175;
	constexpr int GenerateClassButtonHeight = 35;

	const Int2 SelectClassTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 160);
	const std::string SelectClassTextFontName = ArenaFontName::A;
	const Color SelectClassTextColor(48, 12, 12);
	constexpr TextAlignment SelectClassTextAlignment = TextAlignment::MiddleCenter;

	const Int2 SelectClassButtonCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 160);
	constexpr int SelectClassButtonWidth = 175;
	constexpr int SelectClassButtonHeight = 35;

	TextBox::InitInfo getChooseClassCreationTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getChooseClassCreationGenerateTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getChooseClassCreationSelectTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);

	// -- Choose class --
	constexpr int ChooseClassTitleX = 89;
	constexpr int ChooseClassTitleY = 32;
	const std::string ChooseClassTitleFontName = ArenaFontName::C;
	const Color ChooseClassTitleColor(211, 211, 211);
	constexpr TextAlignment ChooseClassTitleAlignment = TextAlignment::TopLeft;

	constexpr int ChooseClassListTextureX = 55;
	constexpr int ChooseClassListTextureY = 9;

	Rect getClassListRect(Game &game);
	Rect getClassListUpButtonRect(Game &game);
	Rect getClassListDownButtonRect(Game &game);

	ListBox::Properties makeClassListBoxProperties(const FontLibrary &fontLibrary);
	
	TextureAssetReference getChooseClassListBoxTextureAssetRef();

	TextBox::InitInfo getChooseClassTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);

	// -- Choose gender --
	constexpr int ChooseGenderTextureWidth = 180;
	constexpr int ChooseGenderTextureHeight = 40;
	constexpr TextureUtils::PatternType ChooseGenderTexturePatternType = TextureUtils::PatternType::Parchment;

	const Int2 ChooseGenderTitleCenterPoint(ArenaRenderUtils::SCREEN_WIDTH / 2, 80);
	const std::string ChooseGenderTitleFontName = ArenaFontName::A;
	const Color ChooseGenderTitleColor(48, 12, 12);
	constexpr TextAlignment ChooseGenderTitleAlignment = TextAlignment::MiddleCenter;

	const Int2 ChooseGenderMaleTextBoxCenter(ArenaRenderUtils::SCREEN_WIDTH / 2, 120);
	const std::string ChooseGenderMaleFontName = ArenaFontName::A;
	const Color ChooseGenderMaleColor(48, 12, 12);
	constexpr TextAlignment ChooseGenderMaleAlignment = TextAlignment::MiddleCenter;

	const Int2 ChooseGenderMaleButtonCenter(ArenaRenderUtils::SCREEN_WIDTH / 2, 120);
	constexpr int ChooseGenderMaleButtonWidth = 175;
	constexpr int ChooseGenderMaleButtonHeight = 35;

	const Int2 ChooseGenderFemaleTextBoxCenter(ArenaRenderUtils::SCREEN_WIDTH / 2, 160);
	const std::string ChooseGenderFemaleFontName = ArenaFontName::A;
	const Color ChooseGenderFemaleColor(48, 12, 12);
	constexpr TextAlignment ChooseGenderFemaleAlignment = TextAlignment::MiddleCenter;

	const Int2 ChooseGenderFemaleButtonCenter(ArenaRenderUtils::SCREEN_WIDTH / 2, 160);
	constexpr int ChooseGenderFemaleButtonWidth = 175;
	constexpr int ChooseGenderFemaleButtonHeight = 35;

	int getChooseGenderTitleTextureX(int textureWidth);
	int getChooseGenderTitleTextureY(int textureHeight);

	TextBox::InitInfo getChooseGenderTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getChooseGenderMaleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getChooseGenderFemaleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);

	// -- Choose name --
	constexpr int ChooseNameTextureWidth = 300;
	constexpr int ChooseNameTextureHeight = 60;
	constexpr TextureUtils::PatternType ChooseNameTexturePatternType = TextureUtils::PatternType::Parchment;

	constexpr int ChooseNameTitleTextBoxX = 26;
	constexpr int ChooseNameTitleTextBoxY = 82;
	const std::string ChooseNameTitleFontName = ArenaFontName::A;
	const Color ChooseNameTitleColor(48, 12, 12);
	constexpr TextAlignment ChooseNameTitleAlignment = TextAlignment::TopLeft;

	constexpr int ChooseNameEntryTextBoxX = 61;
	constexpr int ChooseNameEntryTextBoxY = 101;
	const std::string ChooseNameEntryFontName = ArenaFontName::A;
	const Color ChooseNameEntryColor(48, 12, 12);
	constexpr TextAlignment ChooseNameEntryAlignment = TextAlignment::TopLeft;

	int getChooseNameTitleTextureX(int textureWidth);
	int getChooseNameTitleTextureY(int textureHeight);

	TextBox::InitInfo getChooseNameTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getChooseNameEntryTextBoxInitInfo(const FontLibrary &fontLibrary);

	// -- Choose race --
	const Int2 ChooseRaceInitialPopUpTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string ChooseRaceInitialPopUpFontName = ArenaFontName::A;
	const Color ChooseRaceInitialPopUpColor(48, 12, 12);
	constexpr TextAlignment ChooseRaceInitialPopUpAlignment = TextAlignment::MiddleCenter;
	constexpr int ChooseRaceInitialPopUpLineSpacing = 1;
	constexpr TextureUtils::PatternType ChooseRaceInitialPopUpPatternType = TextureUtils::PatternType::Parchment;

	constexpr int ChooseRaceInitialPopUpTextureWidth = 240;
	constexpr int ChooseRaceInitialPopUpTextureHeight = 60;
	const Int2 ChooseRaceInitialPopUpTextureCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

	const Int2 ChooseRaceProvinceConfirmTitleCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2),
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 22);
	const std::string ChooseRaceProvinceConfirmTitleFontName = ArenaFontName::A;
	const Color ChooseRaceProvinceConfirmTitleTextColor(52, 24, 8);
	constexpr TextAlignment ChooseRaceProvinceConfirmTitleAlignment = TextAlignment::MiddleCenter;
	constexpr int ChooseRaceProvinceConfirmTitleLineSpacing = 1;
	constexpr TextureUtils::PatternType ChooseRaceProvinceConfirmTitleTexturePatternType = TextureUtils::PatternType::Parchment;

	const Int2 ChooseRaceProvinceConfirmYesCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) + 28);
	const std::string ChooseRaceProvinceConfirmYesFontName = ChooseRaceProvinceConfirmTitleFontName;
	const Color ChooseRaceProvinceConfirmYesTextColor = ChooseRaceProvinceConfirmTitleTextColor;
	constexpr TextAlignment ChooseRaceProvinceConfirmYesAlignment = ChooseRaceProvinceConfirmTitleAlignment;
	constexpr TextureUtils::PatternType ChooseRaceProvinceConfirmYesTexturePatternType = TextureUtils::PatternType::Parchment;

	const Int2 ChooseRaceProvinceConfirmNoCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) + 68);
	const std::string ChooseRaceProvinceConfirmNoFontName = ChooseRaceProvinceConfirmTitleFontName;
	const Color ChooseRaceProvinceConfirmNoTextColor = ChooseRaceProvinceConfirmTitleTextColor;
	constexpr TextAlignment ChooseRaceProvinceConfirmNoAlignment = ChooseRaceProvinceConfirmTitleAlignment;
	constexpr TextureUtils::PatternType ChooseRaceProvinceConfirmNoTexturePatternType = TextureUtils::PatternType::Parchment;

	const Int2 ChooseRaceProvinceConfirmedFirstTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string ChooseRaceProvinceConfirmedFirstTextFontName = ArenaFontName::Arena;
	const Color ChooseRaceProvinceConfirmedFirstTextColor(48, 12, 12);
	constexpr TextAlignment ChooseRaceProvinceConfirmedFirstTextAlignment = TextAlignment::MiddleCenter;
	constexpr int ChooseRaceProvinceConfirmedFirstTextLineSpacing = 1;
	constexpr TextureUtils::PatternType ChooseRaceProvinceConfirmedFirstTextPatternType = TextureUtils::PatternType::Parchment;

	const Int2 ChooseRaceProvinceConfirmedSecondTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string ChooseRaceProvinceConfirmedSecondTextFontName = ArenaFontName::Arena;
	const Color ChooseRaceProvinceConfirmedSecondTextColor(48, 12, 12);
	constexpr TextAlignment ChooseRaceProvinceConfirmedSecondTextAlignment = TextAlignment::MiddleCenter;
	constexpr int ChooseRaceProvinceConfirmedSecondTextLineSpacing = 1;
	constexpr TextureUtils::PatternType ChooseRaceProvinceConfirmedSecondTextPatternType = TextureUtils::PatternType::Parchment;

	const Int2 ChooseRaceProvinceConfirmedThirdTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string ChooseRaceProvinceConfirmedThirdTextFontName = ArenaFontName::Arena;
	const Color ChooseRaceProvinceConfirmedThirdTextColor(48, 12, 12);
	constexpr TextAlignment ChooseRaceProvinceConfirmedThirdTextAlignment = TextAlignment::MiddleCenter;
	constexpr int ChooseRaceProvinceConfirmedThirdTextLineSpacing = 1;
	constexpr TextureUtils::PatternType ChooseRaceProvinceConfirmedThirdTextPatternType = TextureUtils::PatternType::Parchment;

	const Int2 ChooseRaceProvinceConfirmedFourthTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string ChooseRaceProvinceConfirmedFourthTextFontName = ArenaFontName::Arena;
	const Color ChooseRaceProvinceConfirmedFourthTextColor(48, 12, 12);
	constexpr TextAlignment ChooseRaceProvinceConfirmedFourthTextAlignment = TextAlignment::MiddleCenter;
	constexpr int ChooseRaceProvinceConfirmedFourthTextLineSpacing = 1;
	constexpr TextureUtils::PatternType ChooseRaceProvinceConfirmedFourthTextPatternType = TextureUtils::PatternType::Parchment;

	TextureAssetReference getChooseRaceBackgroundTextureAssetRef();
	TextureAssetReference getChooseRaceNoExitTextureAssetRef(); // Covers up the exit button since character creation doesn't use it.

	int getChooseRaceNoExitTextureX(int textureWidth);
	int getChooseRaceNoExitTextureY(int textureHeight);

	Rect getChooseRaceProvinceConfirmTitleTextureRect(int textWidth, int textHeight);
	Rect getChooseRaceProvinceConfirmYesTextureRect(const Rect &titleTextureRect);
	Rect getChooseRaceProvinceConfirmNoTextureRect(const Rect &yesTextureRect);

	Rect getChooseRaceProvinceConfirmedFirstTextureRect(int textWidth, int textHeight);
	Rect getChooseRaceProvinceConfirmedSecondTextureRect(int textWidth, int textHeight);
	Rect getChooseRaceProvinceConfirmedThirdTextureRect(int textWidth, int textHeight);
	Rect getChooseRaceProvinceConfirmedFourthTextureRect(int textWidth, int textHeight);

	TextBox::InitInfo getChooseRaceProvinceConfirmTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getChooseRaceProvinceConfirmYesTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getChooseRaceProvinceConfirmNoTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);

	TextBox::InitInfo getChooseRaceProvinceConfirmedFirstTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getChooseRaceProvinceConfirmedSecondTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getChooseRaceProvinceConfirmedThirdTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getChooseRaceProvinceConfirmedFourthTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);

	// -- Choose attributes --
	const Int2 ChooseAttributesTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 2);
	const std::string ChooseAttributesTextFontName = ArenaFontName::Arena;
	const Color ChooseAttributesTextColor(199, 199, 199);
	constexpr TextAlignment ChooseAttributesTextAlignment = TextAlignment::MiddleCenter;
	constexpr int ChooseAttributesTextLineSpacing = 1;
	constexpr TextureUtils::PatternType ChooseAttributesTextPatternType = TextureUtils::PatternType::Dark;

	const Int2 ChooseAttributesTextureCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

	const Int2 AttributesMessageBoxTitleCenterPoint(
		ArenaRenderUtils::SCREEN_WIDTH / 2,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 22);
	const std::string AttributesMessageBoxTitleFontName = ArenaFontName::A;
	const Color AttributesMessageBoxTitleColor(199, 199, 199);
	constexpr TextAlignment AttributesMessageBoxTitleAlignment = TextAlignment::MiddleCenter;
	constexpr TextureUtils::PatternType AttributesMessageBoxPatternType = TextureUtils::PatternType::Dark;

	// @todo: various properties of the message box buttons will likely be combined in the future by MessageBoxSubPanel.
	const Int2 AttributesMessageBoxSaveCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) + 2);
	const std::string AttributesMessageBoxSaveFontName = ArenaFontName::A;
	const Color AttributesMessageBoxSaveColor(190, 113, 0);
	constexpr TextAlignment AttributesMessageBoxSaveAlignment = TextAlignment::MiddleCenter;

	const Int2 AttributesMessageBoxRerollCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) + 26);
	const std::string AttributesMessageBoxRerollFontName = ArenaFontName::A;
	const Color AttributesMessageBoxRerollColor(190, 113, 0);
	constexpr TextAlignment AttributesMessageBoxRerollAlignment = TextAlignment::MiddleCenter;

	const Int2 AppearanceMessageBoxCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	const std::string AppearanceMessageBoxFontName = ArenaFontName::Arena;
	const Color AppearanceMessageBoxColor(199, 199, 199);
	constexpr TextAlignment AppearanceMessageBoxAlignment = TextAlignment::MiddleCenter;
	constexpr int AppearanceMessageBoxLineSpacing = 1;
	constexpr TextureUtils::PatternType AppearanceMessageBoxPatternType = TextureUtils::PatternType::Dark;

	const Int2 AppearancePortraitButtonCenterPoint(ArenaRenderUtils::SCREEN_WIDTH - 72, 25);
	constexpr int AppearancePortraitButtonWidth = 60;
	constexpr int AppearancePortraitButtonHeight = 42;

	int getChooseClassCreationTitleTextureX(int textureWidth);
	int getChooseClassCreationTitleTextureY(int textureHeight);

	int getChooseAttributesTextureWidth();
	int getChooseAttributesTextureHeight();

	Rect getChooseAttributesUnsavedDoneTitleTextureRect(int textWidth, int textHeight);
	Rect getChooseAttributesUnsavedDoneSaveTextureRect(const Rect &titleTextureRect);
	Rect getChooseAttributesUnsavedDoneRerollTextureRect(const Rect &saveTextureRect);

	TextBox::InitInfo getChooseAttributesUnsavedDoneTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getChooseAttributesUnsavedDoneSaveTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getChooseAttributesUnsavedDoneRerollTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);

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
