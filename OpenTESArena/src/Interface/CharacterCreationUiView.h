#ifndef CHARACTER_CREATION_UI_VIEW_H
#define CHARACTER_CREATION_UI_VIEW_H

#include "../Assets/TextureAssetReference.h"
#include "../Interface/MessageBoxSubPanel.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureUtils.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/ListBox.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

#include "components/utilities/Buffer.h"

class ExeData;
class Game;
class Rect;

namespace CharacterCreationUiView
{
	constexpr int MaxTooltipLineLength = 14;

	TextureAssetReference getNightSkyTextureAssetRef();
}

namespace ChooseClassCreationUiView
{
	constexpr int PopUpTextureWidth = 180;
	constexpr int PopUpTextureHeight = 40;
	constexpr TextureUtils::PatternType PopUpPatternType = TextureUtils::PatternType::Parchment;

	const Int2 TitleCenter((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 80);
	const std::string TitleFontName = ArenaFontName::A;
	const Color TitleColor(48, 12, 12);
	constexpr TextAlignment TitleAlignment = TextAlignment::MiddleCenter;
	constexpr int TitleLineSpacing = 1;

	const Int2 GenerateTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 120);
	const std::string GenerateTextFontName = ArenaFontName::A;
	const Color GenerateTextColor(48, 12, 12);
	constexpr TextAlignment GenerateTextAlignment = TextAlignment::MiddleCenter;

	const Int2 GenerateButtonCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 120);
	constexpr int GenerateButtonWidth = 175;
	constexpr int GenerateButtonHeight = 35;

	const Int2 SelectTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 160);
	const std::string SelectTextFontName = ArenaFontName::A;
	const Color SelectTextColor(48, 12, 12);
	constexpr TextAlignment SelectTextAlignment = TextAlignment::MiddleCenter;

	const Int2 SelectButtonCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 160);
	constexpr int SelectButtonWidth = 175;
	constexpr int SelectButtonHeight = 35;

	int getTitleTextureX(int textureWidth);
	int getTitleTextureY(int textureHeight);

	TextBox::InitInfo getTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getGenerateTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getSelectTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
}

namespace ChooseClassUiView
{
	constexpr int TitleX = 89;
	constexpr int TitleY = 32;
	const std::string TitleFontName = ArenaFontName::C;
	const Color TitleColor(211, 211, 211);
	constexpr TextAlignment TitleAlignment = TextAlignment::TopLeft;

	constexpr int ListTextureX = 55;
	constexpr int ListTextureY = 9;

	Rect getListRect(Game &game);
	Rect getUpButtonRect(Game &game);
	Rect getDownButtonRect(Game &game);

	ListBox::Properties makeListBoxProperties(const FontLibrary &fontLibrary);

	TextureAssetReference getListBoxTextureAssetRef();

	TextBox::InitInfo getTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
}

namespace ChooseGenderUiView
{
	constexpr int TextureWidth = 180;
	constexpr int TextureHeight = 40;
	constexpr TextureUtils::PatternType TexturePatternType = TextureUtils::PatternType::Parchment;

	const Int2 TitleCenterPoint(ArenaRenderUtils::SCREEN_WIDTH / 2, 80);
	const std::string TitleFontName = ArenaFontName::A;
	const Color TitleColor(48, 12, 12);
	constexpr TextAlignment TitleAlignment = TextAlignment::MiddleCenter;

	const Int2 MaleTextBoxCenter(ArenaRenderUtils::SCREEN_WIDTH / 2, 120);
	const std::string MaleFontName = ArenaFontName::A;
	const Color MaleColor(48, 12, 12);
	constexpr TextAlignment MaleAlignment = TextAlignment::MiddleCenter;

	const Int2 MaleButtonCenter(ArenaRenderUtils::SCREEN_WIDTH / 2, 120);
	constexpr int MaleButtonWidth = 175;
	constexpr int MaleButtonHeight = 35;

	const Int2 FemaleTextBoxCenter(ArenaRenderUtils::SCREEN_WIDTH / 2, 160);
	const std::string FemaleFontName = ArenaFontName::A;
	const Color FemaleColor(48, 12, 12);
	constexpr TextAlignment FemaleAlignment = TextAlignment::MiddleCenter;

	const Int2 FemaleButtonCenter(ArenaRenderUtils::SCREEN_WIDTH / 2, 160);
	constexpr int FemaleButtonWidth = 175;
	constexpr int FemaleButtonHeight = 35;

	int getTitleTextureX(int textureWidth);
	int getTitleTextureY(int textureHeight);

	TextBox::InitInfo getTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getMaleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getFemaleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
}

namespace ChooseNameUiView
{
	constexpr int TextureWidth = 300;
	constexpr int TextureHeight = 60;
	constexpr TextureUtils::PatternType TexturePatternType = TextureUtils::PatternType::Parchment;

	constexpr int TitleTextBoxX = 26;
	constexpr int TitleTextBoxY = 82;
	const std::string TitleFontName = ArenaFontName::A;
	const Color TitleColor(48, 12, 12);
	constexpr TextAlignment TitleAlignment = TextAlignment::TopLeft;

	constexpr int EntryTextBoxX = 61;
	constexpr int EntryTextBoxY = 101;
	const std::string EntryFontName = ArenaFontName::A;
	const Color EntryColor(48, 12, 12);
	constexpr TextAlignment EntryAlignment = TextAlignment::TopLeft;

	int getTitleTextureX(int textureWidth);
	int getTitleTextureY(int textureHeight);

	TextBox::InitInfo getTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getEntryTextBoxInitInfo(const FontLibrary &fontLibrary);
}

namespace ChooseRaceUiView
{
	const Int2 InitialPopUpTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string InitialPopUpFontName = ArenaFontName::A;
	const Color InitialPopUpColor(48, 12, 12);
	constexpr TextAlignment InitialPopUpAlignment = TextAlignment::MiddleCenter;
	constexpr int InitialPopUpLineSpacing = 1;
	constexpr TextureUtils::PatternType InitialPopUpPatternType = TextureUtils::PatternType::Parchment;

	constexpr int InitialPopUpTextureWidth = 240;
	constexpr int InitialPopUpTextureHeight = 60;
	const Int2 InitialPopUpTextureCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

	const Int2 ProvinceConfirmTitleCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2),
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 22);
	const std::string ProvinceConfirmTitleFontName = ArenaFontName::A;
	const Color ProvinceConfirmTitleTextColor(52, 24, 8);
	constexpr int ProvinceConfirmTitleLineSpacing = 1;
	constexpr TextureUtils::PatternType ProvinceConfirmTitleTexturePatternType = TextureUtils::PatternType::Parchment;

	const std::string ProvinceConfirmItemFontName = ProvinceConfirmTitleFontName;
	const Color ProvinceConfirmItemTextColor = ProvinceConfirmTitleTextColor;

	const Int2 ProvinceConfirmedFirstTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string ProvinceConfirmedFirstTextFontName = ArenaFontName::Arena;
	const Color ProvinceConfirmedFirstTextColor(48, 12, 12);
	constexpr TextAlignment ProvinceConfirmedFirstTextAlignment = TextAlignment::MiddleCenter;
	constexpr int ProvinceConfirmedFirstTextLineSpacing = 1;
	constexpr TextureUtils::PatternType ProvinceConfirmedFirstTextPatternType = TextureUtils::PatternType::Parchment;

	const Int2 ProvinceConfirmedSecondTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string ProvinceConfirmedSecondTextFontName = ArenaFontName::Arena;
	const Color ProvinceConfirmedSecondTextColor(48, 12, 12);
	constexpr TextAlignment ProvinceConfirmedSecondTextAlignment = TextAlignment::MiddleCenter;
	constexpr int ProvinceConfirmedSecondTextLineSpacing = 1;
	constexpr TextureUtils::PatternType ProvinceConfirmedSecondTextPatternType = TextureUtils::PatternType::Parchment;

	const Int2 ProvinceConfirmedThirdTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string ProvinceConfirmedThirdTextFontName = ArenaFontName::Arena;
	const Color ProvinceConfirmedThirdTextColor(48, 12, 12);
	constexpr TextAlignment ProvinceConfirmedThirdTextAlignment = TextAlignment::MiddleCenter;
	constexpr int ProvinceConfirmedThirdTextLineSpacing = 1;
	constexpr TextureUtils::PatternType ProvinceConfirmedThirdTextPatternType = TextureUtils::PatternType::Parchment;

	const Int2 ProvinceConfirmedFourthTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string ProvinceConfirmedFourthTextFontName = ArenaFontName::Arena;
	const Color ProvinceConfirmedFourthTextColor(48, 12, 12);
	constexpr TextAlignment ProvinceConfirmedFourthTextAlignment = TextAlignment::MiddleCenter;
	constexpr int ProvinceConfirmedFourthTextLineSpacing = 1;
	constexpr TextureUtils::PatternType ProvinceConfirmedFourthTextPatternType = TextureUtils::PatternType::Parchment;

	TextureAssetReference getBackgroundTextureAssetRef();
	TextureAssetReference getNoExitTextureAssetRef(); // Covers up the exit button since character creation doesn't use it.

	int getNoExitTextureX(int textureWidth);
	int getNoExitTextureY(int textureHeight);

	Rect getProvinceConfirmTitleTextBoxRect(const std::string_view &text, const FontLibrary &fontLibrary);

	MessageBoxSubPanel::BackgroundProperties getProvinceConfirmMessageBoxBackgroundProperties();
	MessageBoxSubPanel::TitleProperties getProvinceConfirmMessageBoxTitleProperties(const std::string_view &text,
		const FontLibrary &fontLibrary);
	MessageBoxSubPanel::ItemsProperties getProvinceConfirmMessageBoxItemsProperties(const FontLibrary &fontLibrary);

	Rect getProvinceConfirmedFirstTextureRect(int textWidth, int textHeight);
	Rect getProvinceConfirmedSecondTextureRect(int textWidth, int textHeight);
	Rect getProvinceConfirmedThirdTextureRect(int textWidth, int textHeight);
	Rect getProvinceConfirmedFourthTextureRect(int textWidth, int textHeight);

	TextBox::InitInfo getProvinceConfirmedFirstTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getProvinceConfirmedSecondTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getProvinceConfirmedThirdTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
	TextBox::InitInfo getProvinceConfirmedFourthTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);
}

namespace ChooseAttributesUiView
{
	const Int2 InitialTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 2);
	const std::string InitialTextFontName = ArenaFontName::Arena;
	const Color InitialTextColor(199, 199, 199);
	constexpr TextAlignment InitialTextAlignment = TextAlignment::MiddleCenter;
	constexpr int InitialTextLineSpacing = 1;
	constexpr TextureUtils::PatternType InitialTextPatternType = TextureUtils::PatternType::Dark;

	const Int2 InitialTextureCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

	const Int2 MessageBoxTitleCenterPoint(
		ArenaRenderUtils::SCREEN_WIDTH / 2,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 22);
	const std::string MessageBoxTitleFontName = ArenaFontName::A;
	const Color MessageBoxTitleColor(199, 199, 199);
	constexpr TextureUtils::PatternType MessageBoxPatternType = TextureUtils::PatternType::Dark;

	const std::string MessageBoxItemFontName = ArenaFontName::A;
	const Color MessageBoxItemTextColor(190, 113, 0);

	const Int2 AppearanceTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	const std::string AppearanceTextFontName = ArenaFontName::Arena;
	const Color AppearanceTextColor(199, 199, 199);
	constexpr TextAlignment AppearanceTextAlignment = TextAlignment::MiddleCenter;
	constexpr int AppearanceTextLineSpacing = 1;
	constexpr TextureUtils::PatternType AppearanceTextPatternType = TextureUtils::PatternType::Dark;

	const Int2 PortraitButtonCenterPoint(ArenaRenderUtils::SCREEN_WIDTH - 72, 25);
	constexpr int PortraitButtonWidth = 60;
	constexpr int PortraitButtonHeight = 42;

	int getInitialTextureWidth();
	int getInitialTextureHeight();

	Rect getMessageBoxTitleTextBoxRect(const std::string_view &text, const FontLibrary &fontLibrary);

	MessageBoxSubPanel::BackgroundProperties getMessageBoxBackgroundProperties();
	MessageBoxSubPanel::TitleProperties getMessageBoxTitleProperties(const std::string_view &text,
		const FontLibrary &fontLibrary);
	MessageBoxSubPanel::ItemsProperties getMessageBoxItemsProperties(const FontLibrary &fontLibrary);

	int getAppearanceTextBoxTextureWidth(int textWidth);
	int getAppearanceTextBoxTextureHeight(int textHeight);

	Int2 getBodyOffset(Game &game);
	Int2 getHeadOffset(Game &game);
	Int2 getShirtOffset(Game &game);
	Int2 getPantsOffset(Game &game);

	TextureAssetReference getBodyTextureAssetRef(Game &game);
	Buffer<TextureAssetReference> getHeadTextureAssetRefs(Game &game);
	TextureAssetReference getShirtTextureAssetRef(Game &game);
	TextureAssetReference getPantsTextureAssetRef(Game &game);

	PaletteID getPaletteID(TextureManager &textureManager);

	UiTextureID allocBodyTexture(Game &game);
	UiTextureID allocShirtTexture(Game &game);
	UiTextureID allocPantsTexture(Game &game);
	UiTextureID allocHeadTexture(const TextureAssetReference &textureAssetRef,
		TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocStatsBgTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocCursorTexture(TextureManager &textureManager, Renderer &renderer);
}

#endif
