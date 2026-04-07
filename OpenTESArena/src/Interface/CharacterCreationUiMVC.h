#pragma once

#include <string>
#include <vector>

#include "../Assets/TextureAsset.h"
#include "../Assets/TextureUtils.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Stats/PrimaryAttribute.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextRenderUtils.h"
#include "../UI/UiListBox.h"
#include "../UI/UiMessageBox.h"
#include "../Utilities/Color.h"

#include "components/utilities/Buffer.h"

class ArenaRandom;
class Game;

struct CharacterClassDefinition;
struct InputActionCallbackValues;

namespace CharacterCreationUiModel
{
	std::string getPlayerName(Game &game);
	std::string getPlayerRaceName(Game &game);
	std::string getPlayerClassName(Game &game);
	const PrimaryAttributes &getPlayerAttributes(Game &game);
	std::string getPlayerLevel(Game &game);
	std::string getPlayerExperience(Game &game);
}

namespace ChooseClassCreationUiModel
{
	std::string getTitleText(Game &game);
	std::string getGenerateButtonText(Game &game);
	std::string getSelectButtonText(Game &game);
}

namespace ChooseClassUiModel
{
	std::string getTitleText(Game &game);
	std::string getArmorTooltipText(const CharacterClassDefinition &charClassDef);
	std::string getShieldTooltipText(const CharacterClassDefinition &charClassDef);
	std::string getWeaponTooltipText(const CharacterClassDefinition &charClassDef, Game &game);
	std::string getFullTooltipText(const CharacterClassDefinition &charClassDef, Game &game);
}

namespace ChooseGenderUiModel
{
	std::string getTitleText(Game &game);
	std::string getMaleText(Game &game);
	std::string getFemaleText(Game &game);
}

namespace ChooseNameUiModel
{
	std::string getTitleText(Game &game);
	bool isCharacterAccepted(char c);
}

namespace ChooseRaceUiModel
{
	std::string getTitleText(Game &game);
	std::string getProvinceConfirmTitleText(Game &game);
	std::string getProvinceConfirmYesText(Game &game);
	std::string getProvinceConfirmNoText(Game &game);
	std::string getProvinceConfirmedFirstText(Game &game);
	std::string getProvinceConfirmedSecondText(Game &game);
	std::string getProvinceConfirmedThirdText(Game &game);
	std::string getProvinceConfirmedFourthText(Game &game);
}

namespace ChooseAttributesUiModel
{
	// Based on reversed wiki values
	constexpr int PrimaryAttributeRandomMax = 20;
	constexpr int BonusPointsRandomMax = 25;

	int rollClassic(int n, ArenaRandom &random);

	std::string getPlayerHealth(Game &game);
	std::string getPlayerStamina(Game &game);
	std::string getPlayerSpellPoints(Game &game);
	std::string getPlayerGold(Game &game);

	std::string getInitialText(Game &game);

	std::string getMessageBoxTitleText(Game &game);
	std::string getMessageBoxSaveText(Game &game);
	std::string getMessageBoxRerollText(Game &game);
	TextRenderColorOverrideInfo getMessageBoxSaveColorOverrideInfo(Game &game);
	TextRenderColorOverrideInfo getMessageBoxRerollColorOverrideInfo(Game &game);

	std::string getBonusPointsRemainingText(Game &game);
	std::string getAppearanceText(Game &game);
}

// ------------------------

namespace CharacterCreationUiView
{
	TextureAsset getNightSkyTextureAsset();

	UiTextureID allocNightSkyTexture(TextureManager &textureManager, Renderer &renderer);
}

namespace ChooseClassUiView
{
	constexpr int MaxTooltipLineLength = 40;

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

	UiListBoxInitInfo makeListBoxProperties(const FontLibrary &fontLibrary);

	TextureAsset getListBoxTextureAsset();

	UiTextureID allocPopUpTexture(TextureManager &textureManager, Renderer &renderer);
}

namespace ChooseRaceUiView
{
	const Int2 InitialPopUpTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string InitialPopUpFontName = ArenaFontName::A;
	const Color InitialPopUpColor(48, 12, 12);
	constexpr TextAlignment InitialPopUpAlignment = TextAlignment::MiddleCenter;
	constexpr int InitialPopUpLineSpacing = 1;
	constexpr UiTexturePatternType InitialPopUpPatternType = UiTexturePatternType::Parchment;

	constexpr int InitialPopUpTextureWidth = 240;
	constexpr int InitialPopUpTextureHeight = 60;
	const Int2 InitialPopUpTextureCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, (ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

	const Int2 ProvinceConfirmTitleCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2), (ArenaRenderUtils::SCREEN_HEIGHT / 2) - 22);
	const std::string ProvinceConfirmTitleFontName = ArenaFontName::A;
	const Color ProvinceConfirmTitleTextColor(52, 24, 8);
	constexpr TextAlignment ProvinceConfirmTitleAlignment = TextAlignment::MiddleCenter;
	constexpr int ProvinceConfirmTitleLineSpacing = 1;
	constexpr UiTexturePatternType ProvinceConfirmTitleTexturePatternType = UiTexturePatternType::Parchment;

	const std::string ProvinceConfirmItemFontName = ProvinceConfirmTitleFontName;
	const Color ProvinceConfirmItemTextColor = ProvinceConfirmTitleTextColor;

	const Int2 ProvinceConfirmedFirstTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string ProvinceConfirmedFirstTextFontName = ArenaFontName::Arena;
	const Color ProvinceConfirmedFirstTextColor(48, 12, 12);
	constexpr TextAlignment ProvinceConfirmedFirstTextAlignment = TextAlignment::MiddleCenter;
	constexpr int ProvinceConfirmedFirstTextLineSpacing = 1;
	constexpr UiTexturePatternType ProvinceConfirmedFirstTextPatternType = UiTexturePatternType::Parchment;

	const Int2 ProvinceConfirmedSecondTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string ProvinceConfirmedSecondTextFontName = ArenaFontName::Arena;
	const Color ProvinceConfirmedSecondTextColor(48, 12, 12);
	constexpr TextAlignment ProvinceConfirmedSecondTextAlignment = TextAlignment::MiddleCenter;
	constexpr int ProvinceConfirmedSecondTextLineSpacing = 1;
	constexpr UiTexturePatternType ProvinceConfirmedSecondTextPatternType = UiTexturePatternType::Parchment;

	const Int2 ProvinceConfirmedThirdTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string ProvinceConfirmedThirdTextFontName = ArenaFontName::Arena;
	const Color ProvinceConfirmedThirdTextColor(48, 12, 12);
	constexpr TextAlignment ProvinceConfirmedThirdTextAlignment = TextAlignment::MiddleCenter;
	constexpr int ProvinceConfirmedThirdTextLineSpacing = 1;
	constexpr UiTexturePatternType ProvinceConfirmedThirdTextPatternType = UiTexturePatternType::Parchment;

	const Int2 ProvinceConfirmedFourthTextCenterPoint((ArenaRenderUtils::SCREEN_WIDTH / 2) - 1, 98);
	const std::string ProvinceConfirmedFourthTextFontName = ArenaFontName::Arena;
	const Color ProvinceConfirmedFourthTextColor(48, 12, 12);
	constexpr TextAlignment ProvinceConfirmedFourthTextAlignment = TextAlignment::MiddleCenter;
	constexpr int ProvinceConfirmedFourthTextLineSpacing = 1;
	constexpr UiTexturePatternType ProvinceConfirmedFourthTextPatternType = UiTexturePatternType::Parchment;

	TextureAsset getBackgroundTextureAsset();
	TextureAsset getNoExitTextureAsset(); // Covers up the exit button since character creation doesn't use it.

	Rect getProvinceConfirmTitleTextBoxRect(const std::string_view text, const FontLibrary &fontLibrary);

	MessageBoxBackgroundProperties getProvinceConfirmMessageBoxBackgroundProperties();
	MessageBoxTitleProperties getProvinceConfirmMessageBoxTitleProperties(const std::string_view text, const FontLibrary &fontLibrary);
	MessageBoxItemsProperties getProvinceConfirmMessageBoxItemsProperties(const FontLibrary &fontLibrary);

	Rect getProvinceConfirmedFirstTextureRect(int textWidth, int textHeight);
	Rect getProvinceConfirmedSecondTextureRect(int textWidth, int textHeight);
	Rect getProvinceConfirmedThirdTextureRect(int textWidth, int textHeight);
	Rect getProvinceConfirmedFourthTextureRect(int textWidth, int textHeight);
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
	constexpr UiTexturePatternType InitialTextPatternType = UiTexturePatternType::Dark;

	const Int2 InitialTextureCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);

	const Int2 MessageBoxTitleCenterPoint(
		ArenaRenderUtils::SCREEN_WIDTH / 2,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 22);
	const std::string MessageBoxTitleFontName = ArenaFontName::A;
	const Color MessageBoxTitleColor(199, 199, 199);
	constexpr UiTexturePatternType MessageBoxPatternType = UiTexturePatternType::Dark;

	const std::string MessageBoxItemFontName = ArenaFontName::A;
	const Color MessageBoxItemTextColor(190, 113, 0);

	const Int2 AppearanceTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	const std::string AppearanceTextFontName = ArenaFontName::Arena;
	const Color AppearanceTextColor(199, 199, 199);
	constexpr TextAlignment AppearanceTextAlignment = TextAlignment::MiddleCenter;
	constexpr int AppearanceTextLineSpacing = 1;
	constexpr UiTexturePatternType AppearanceTextPatternType = UiTexturePatternType::Dark;

	const Int2 PortraitButtonCenterPoint(ArenaRenderUtils::SCREEN_WIDTH - 72, 25);
	constexpr int PortraitButtonWidth = 60;
	constexpr int PortraitButtonHeight = 42;

	constexpr Rect AttributeButtonFirstRect(10, 52, 26, 8);
	constexpr Int2 UpDownButtonFirstTopLeftPosition(38, 48);

	constexpr Int2 BonusPointsTextureTopLeftPosition(45, 109);
	constexpr Int2 BonusPointsTextBoxTopLeftPosition(92, 113);
	const std::string BonusPointsFontName = ArenaFontName::Arena;
	constexpr Color BonusPointsTextColor(199, 199, 199);

	Rect getMessageBoxTitleTextBoxRect(const std::string_view text, const FontLibrary &fontLibrary);

	MessageBoxBackgroundProperties getMessageBoxBackgroundProperties();
	MessageBoxTitleProperties getMessageBoxTitleProperties(const std::string_view text, const FontLibrary &fontLibrary);
	MessageBoxItemsProperties getMessageBoxItemsProperties(const FontLibrary &fontLibrary);

	int getDistributePointsTextBoxTextureWidth(int textWidth);
	int getDistributePointsTextBoxTextureHeight(int textHeight);
	int getAppearanceTextBoxTextureWidth(int textWidth);
	int getAppearanceTextBoxTextureHeight(int textHeight);

	Int2 getBodyOffset(Game &game);
	Int2 getHeadOffset(Game &game);
	Int2 getShirtOffset(Game &game);
	Int2 getPantsOffset(Game &game);

	TextureAsset getBodyTextureAsset(Game &game);
	Buffer<TextureAsset> getHeadTextureAssets(Game &game);
	TextureAsset getShirtTextureAsset(Game &game);
	TextureAsset getPantsTextureAsset(Game &game);

	UiTextureID allocBodyTexture(Game &game);
	UiTextureID allocShirtTexture(Game &game);
	UiTextureID allocPantsTexture(Game &game);
	UiTextureID allocHeadTexture(const TextureAsset &textureAsset, TextureManager &textureManager, Renderer &renderer);
}

// ------------------------

namespace ChooseClassCreationUiController
{
	void onBackToMainMenuInputAction(const InputActionCallbackValues &values);

	void onGenerateButtonSelected(Game &game);
	void onSelectButtonSelected(Game &game);
}

namespace ChooseClassUiController
{
	void onBackToChooseClassCreationInputAction(const InputActionCallbackValues &values);
	void onItemButtonSelected(Game &game, int charClassDefID);
}

namespace ChooseGenderUiController
{
	void onBackToChooseNameInputAction(const InputActionCallbackValues &values);
	void onMaleButtonSelected(Game &game);
	void onFemaleButtonSelected(Game &game);
}

namespace ChooseNameUiController
{
	void onBackToChooseClassInputAction(const InputActionCallbackValues &values);
	void onTextInput(const std::string_view text, std::string &name, bool *outDirty);
	void onBackspaceInputAction(const InputActionCallbackValues &values, std::string &name, bool *outDirty);
	void onAcceptInputAction(const InputActionCallbackValues &values, const std::string &name);
}

namespace ChooseAttributesUiController
{
	// -- Cinematic after character creation --
	void onPostCharacterCreationCinematicFinished(Game &game);
}
