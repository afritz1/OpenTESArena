#include <algorithm>
#include <optional>

#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "CharacterSheetUiView.h"
#include "../Assets/ArenaPortraitUtils.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../UI/FontDefinition.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"

TextureAsset CharacterCreationUiView::getNightSkyTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::CharacterCreation));
}

UiTextureID CharacterCreationUiView::allocNightSkyTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = CharacterCreationUiView::getNightSkyTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, textureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for background texture.");
	}

	return textureID;
}

Rect ChooseClassUiView::getListRect(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const auto &chooseClassListUI = exeData.ui.chooseClassList;
	return Rect(
		chooseClassListUI.area.x,
		chooseClassListUI.area.y,
		chooseClassListUI.area.w,
		chooseClassListUI.area.h);
}

Rect ChooseClassUiView::getUpButtonRect(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const auto &chooseClassListUI = exeData.ui.chooseClassList;
	return Rect(
		chooseClassListUI.buttonUp.x,
		chooseClassListUI.buttonUp.y,
		chooseClassListUI.buttonUp.w,
		chooseClassListUI.buttonUp.h);
}

Rect ChooseClassUiView::getDownButtonRect(Game &game)
{
	const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const auto &chooseClassListUI = exeData.ui.chooseClassList;
	return Rect(
		chooseClassListUI.buttonDown.x,
		chooseClassListUI.buttonDown.y,
		chooseClassListUI.buttonDown.w,
		chooseClassListUI.buttonDown.h);
}

ListBoxProperties ChooseClassUiView::makeListBoxProperties(const FontLibrary &fontLibrary)
{
	const char *fontName = ArenaFontName::A;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName, &fontDefIndex))
	{
		DebugCrash("Couldn't get class list box font \"" + std::string(fontName) + "\".");
	}

	constexpr int maxDisplayedItemCount = 6;
	std::string dummyText;
	for (int i = 0; i < maxDisplayedItemCount; i++)
	{
		if (i > 0)
		{
			dummyText += '\n';
		}

		std::string dummyLine(10, TextRenderUtils::LARGEST_CHAR); // Arbitrary worst-case line size.
		dummyText += dummyLine;
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	const Color itemColor(85, 44, 20);
	constexpr double scrollScale = 1.0;
	return ListBoxProperties(fontDefIndex, textureGenInfo, fontDef.getCharacterHeight(), itemColor, scrollScale);
}

TextureAsset ChooseClassUiView::getListBoxTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::PopUp2));
}

TextBoxInitInfo ChooseClassUiView::getTitleTextBoxInitInfo(const std::string_view text,
	const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		text,
		ChooseClassUiView::TitleX,
		ChooseClassUiView::TitleY,
		ChooseClassUiView::TitleFontName,
		ChooseClassUiView::TitleColor,
		ChooseClassUiView::TitleAlignment,
		fontLibrary);
}

TextBoxInitInfo ChooseClassUiView::getClassDescriptionTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	std::string dummyText;
	for (int i = 0; i < 10; i++)
	{
		if (i > 0)
		{
			dummyText += '\n';
		}

		dummyText += std::string(52, TextRenderUtils::LARGEST_CHAR);
	}

	TextRenderShadowInfo shadowInfo;
	shadowInfo.init(1, 0, Colors::Black);

	return TextBoxInitInfo::makeWithCenter(
		dummyText,
		Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT - 32),
		ArenaFontName::D,
		Colors::White,
		TextAlignment::TopCenter,
		shadowInfo,
		0,
		fontLibrary);
}

UiTextureID ChooseClassUiView::allocPopUpTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = CharacterCreationUiView::getNightSkyTextureAsset();
	const TextureAsset textureAsset = ChooseClassUiView::getListBoxTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for class pop-up.");
	}

	return textureID;
}

TextureAsset ChooseRaceUiView::getBackgroundTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::RaceSelect));
}

TextureAsset ChooseRaceUiView::getNoExitTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::NoExit));
}

Rect ChooseRaceUiView::getProvinceConfirmTitleTextBoxRect(const std::string_view text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = ChooseRaceUiView::ProvinceConfirmTitleFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(
		text, fontDef, std::nullopt, ChooseRaceUiView::ProvinceConfirmTitleLineSpacing);
	return Rect(
		ChooseRaceUiView::ProvinceConfirmTitleCenterPoint,
		textureGenInfo.width,
		textureGenInfo.height);
}

MessageBoxBackgroundProperties ChooseRaceUiView::getProvinceConfirmMessageBoxBackgroundProperties()
{
	return MessageBoxBackgroundProperties(
		ChooseRaceUiView::ProvinceConfirmTitleTexturePatternType,
		22,
		0,
		std::nullopt,
		60,
		40);
}

MessageBoxTitleProperties ChooseRaceUiView::getProvinceConfirmMessageBoxTitleProperties(const std::string_view text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = ChooseRaceUiView::ProvinceConfirmTitleFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(
		text, fontDef, std::nullopt, ChooseRaceUiView::ProvinceConfirmTitleLineSpacing);
	return MessageBoxTitleProperties(
		fontName,
		textureGenInfo,
		ChooseRaceUiView::ProvinceConfirmTitleTextColor,
		ChooseRaceUiView::ProvinceConfirmTitleLineSpacing);
}

MessageBoxItemsProperties ChooseRaceUiView::getProvinceConfirmMessageBoxItemsProperties(const FontLibrary &fontLibrary)
{
	const std::string dummyText(5, TextRenderUtils::LARGEST_CHAR);
	const std::string &fontName = ChooseRaceUiView::ProvinceConfirmItemFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	constexpr int itemCount = 2;
	return MessageBoxItemsProperties(
		itemCount,
		fontName,
		textureGenInfo,
		ChooseRaceUiView::ProvinceConfirmItemTextColor);
}

Rect ChooseRaceUiView::getProvinceConfirmedFirstTextureRect(int textWidth, int textHeight)
{
	const Int2 center(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	return Rect(
		center,
		textWidth + 20,
		std::max(textHeight, 40));
}

Rect ChooseRaceUiView::getProvinceConfirmedSecondTextureRect(int textWidth, int textHeight)
{
	const Int2 center(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	return Rect(
		center,
		textWidth + 20,
		std::max(textHeight + 14, 40));
}

Rect ChooseRaceUiView::getProvinceConfirmedThirdTextureRect(int textWidth, int textHeight)
{
	const Int2 center(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	return Rect(
		center,
		textWidth + 20,
		std::max(textHeight + 18, 40));
}

Rect ChooseRaceUiView::getProvinceConfirmedFourthTextureRect(int textWidth, int textHeight)
{
	const Int2 center(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	return Rect(
		center,
		textWidth + 20,
		std::max(textHeight + 8, 40));
}

Rect ChooseAttributesUiView::getMessageBoxTitleTextBoxRect(const std::string_view text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = ChooseAttributesUiView::MessageBoxTitleFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(text, fontDef);
	return Rect(
		ChooseAttributesUiView::MessageBoxTitleCenterPoint,
		textureGenInfo.width,
		textureGenInfo.height);
}

MessageBoxBackgroundProperties ChooseAttributesUiView::getMessageBoxBackgroundProperties()
{
	return MessageBoxBackgroundProperties(
		ChooseAttributesUiView::MessageBoxPatternType,
		12,
		0,
		std::nullopt,
		24,
		24);
}

MessageBoxTitleProperties ChooseAttributesUiView::getMessageBoxTitleProperties(const std::string_view text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = ChooseAttributesUiView::MessageBoxTitleFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(text, fontDef);
	return MessageBoxTitleProperties(fontName, textureGenInfo, ChooseAttributesUiView::MessageBoxTitleColor);
}

MessageBoxItemsProperties ChooseAttributesUiView::getMessageBoxItemsProperties(const FontLibrary &fontLibrary)
{
	const std::string dummyText(10, TextRenderUtils::LARGEST_CHAR);
	const std::string &fontName = ChooseAttributesUiView::MessageBoxItemFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderTextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	constexpr int itemCount = 2;
	return MessageBoxItemsProperties(
		itemCount,
		fontName,
		textureGenInfo,
		ChooseAttributesUiView::MessageBoxItemTextColor);
}

int ChooseAttributesUiView::getDistributePointsTextBoxTextureWidth(int textWidth)
{
	return textWidth + 12;
}

int ChooseAttributesUiView::getDistributePointsTextBoxTextureHeight(int textHeight)
{
	return textHeight + 12;
}

int ChooseAttributesUiView::getAppearanceTextBoxTextureWidth(int textWidth)
{
	return textWidth + 10;
}

int ChooseAttributesUiView::getAppearanceTextBoxTextureHeight(int textHeight)
{
	return textHeight + 12;
}

Int2 ChooseAttributesUiView::getBodyOffset(Game &game)
{
	const TextureAsset textureAsset = ChooseAttributesUiView::getBodyTextureAsset(game);

	TextureManager &textureManager = game.textureManager;
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for body \"" + textureAsset.filename + "\".");
	}

	const TextureBuilder &bodyTexture = textureManager.getTextureBuilderHandle(*textureBuilderID);
	return Int2(ArenaRenderUtils::SCREEN_WIDTH - bodyTexture.width, 0);
}

Int2 ChooseAttributesUiView::getHeadOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;
	const int raceID = charCreationState.raceIndex;

	constexpr bool trimmed = false;
	const std::string &headsFilename = ArenaPortraitUtils::getHeads(isMale, raceID, trimmed);

	auto &textureManager = game.textureManager;
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(headsFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugCrash("Couldn't get texture file metadata for \"" + headsFilename + "\".");
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	const int headOffsetIndex = charCreationState.portraitIndex;
	return textureFileMetadata.getOffset(headOffsetIndex);
}

Int2 ChooseAttributesUiView::getShirtOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;

	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const int charClassDefID = charCreationState.classDefID;
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.castsMagic;

	return ArenaPortraitUtils::getShirtOffset(isMale, isMagic);
}

Int2 ChooseAttributesUiView::getPantsOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;
	return ArenaPortraitUtils::getPantsOffset(isMale);
}

TextureAsset ChooseAttributesUiView::getBodyTextureAsset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;
	const int raceID = charCreationState.raceIndex;

	std::string bodyFilename = ArenaPortraitUtils::getBody(isMale, raceID);
	return TextureAsset(std::move(bodyFilename));
}

Buffer<TextureAsset> ChooseAttributesUiView::getHeadTextureAssets(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;
	const int raceID = charCreationState.raceIndex;

	constexpr bool trimmed = false;
	std::string headsFilename = ArenaPortraitUtils::getHeads(isMale, raceID, trimmed);

	auto &textureManager = game.textureManager;
	return TextureUtils::makeTextureAssets(headsFilename, textureManager);
}

TextureAsset ChooseAttributesUiView::getShirtTextureAsset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;

	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const int charClassDefID = charCreationState.classDefID;
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.castsMagic;

	std::string shirtFilename = ArenaPortraitUtils::getShirt(isMale, isMagic);
	return TextureAsset(std::move(shirtFilename));
}

TextureAsset ChooseAttributesUiView::getPantsTextureAsset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.male;

	std::string pantsFilename = ArenaPortraitUtils::getPants(isMale);
	return TextureAsset(std::move(pantsFilename));
}

UiTextureID ChooseAttributesUiView::allocBodyTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = ChooseAttributesUiView::getBodyTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character body.");
	}

	return textureID;
}

UiTextureID ChooseAttributesUiView::allocShirtTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = ChooseAttributesUiView::getShirtTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character shirt.");
	}

	return textureID;
}

UiTextureID ChooseAttributesUiView::allocPantsTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = ChooseAttributesUiView::getPantsTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character pants.");
	}

	return textureID;
}

UiTextureID ChooseAttributesUiView::allocHeadTexture(const TextureAsset &textureAsset,
	TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character head \"" + textureAsset.filename + "\".");
	}

	return textureID;
}

UiTextureID ChooseAttributesUiView::allocStatsBgTexture(TextureManager &textureManager, Renderer &renderer)
{
	return CharacterSheetUiView::allocStatsBgTexture(textureManager, renderer);
}

UiTextureID ChooseAttributesUiView::allocUpDownButtonTexture(TextureManager &textureManager, Renderer &renderer)
{
	return CharacterSheetUiView::allocUpDownButtonTexture(textureManager, renderer);
}

UiTextureID ChooseAttributesUiView::allocBonusPointsTexture(TextureManager &textureManager, Renderer &renderer)
{
	return CharacterSheetUiView::allocBonusPointsTexture(textureManager, renderer);
}
