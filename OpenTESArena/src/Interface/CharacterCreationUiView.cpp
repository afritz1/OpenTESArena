#include <algorithm>
#include <optional>

#include "CharacterCreationUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../Media/PortraitFile.h"
#include "../UI/FontDefinition.h"
#include "../UI/FontLibrary.h"
#include "../UI/FontUtils.h"

TextureAssetReference CharacterCreationUiView::getNightSkyTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::CharacterCreation));
}

TextBox::InitInfo CharacterCreationUiView::getChooseClassCreationTitleTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::ChooseClassCreationTitleCenter,
		CharacterCreationUiView::ChooseClassCreationTitleFontName,
		CharacterCreationUiView::ChooseClassCreationTitleColor,
		CharacterCreationUiView::ChooseClassCreationTitleAlignment,
		std::nullopt,
		CharacterCreationUiView::ChooseClassCreationTitleLineSpacing,
		fontLibrary);
}

TextBox::InitInfo CharacterCreationUiView::getChooseClassCreationGenerateTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::GenerateClassTextCenterPoint,
		CharacterCreationUiView::GenerateClassTextFontName,
		CharacterCreationUiView::GenerateClassTextColor,
		CharacterCreationUiView::GenerateClassTextAlignment,
		fontLibrary);
}

TextBox::InitInfo CharacterCreationUiView::getChooseClassCreationSelectTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::SelectClassTextCenterPoint,
		CharacterCreationUiView::SelectClassTextFontName,
		CharacterCreationUiView::SelectClassTextColor,
		CharacterCreationUiView::SelectClassTextAlignment,
		fontLibrary);
}

Rect CharacterCreationUiView::getClassListRect(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	const auto &chooseClassListUI = exeData.ui.chooseClassList;
	return Rect(
		chooseClassListUI.area.x,
		chooseClassListUI.area.y,
		chooseClassListUI.area.w,
		chooseClassListUI.area.h);
}

Rect CharacterCreationUiView::getClassListUpButtonRect(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	const auto &chooseClassListUI = exeData.ui.chooseClassList;
	return Rect(
		chooseClassListUI.buttonUp.x,
		chooseClassListUI.buttonUp.y,
		chooseClassListUI.buttonUp.w,
		chooseClassListUI.buttonUp.h);
}

Rect CharacterCreationUiView::getClassListDownButtonRect(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	const auto &chooseClassListUI = exeData.ui.chooseClassList;
	return Rect(
		chooseClassListUI.buttonDown.x,
		chooseClassListUI.buttonDown.y,
		chooseClassListUI.buttonDown.w,
		chooseClassListUI.buttonDown.h);
}

ListBox::Properties CharacterCreationUiView::makeClassListBoxProperties(const FontLibrary &fontLibrary)
{
	const char *fontNameStr = FontUtils::fromName(FontName::A);
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontNameStr, &fontDefIndex))
	{
		DebugCrash("Couldn't get class list box font \"" + std::string(fontNameStr) + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const Color itemColor(85, 44, 20);
	constexpr double scrollScale = 1.0;
	constexpr int rowSpacing = 0;
	return ListBox::Properties(fontDefIndex, fontDef.getCharacterHeight(), itemColor, scrollScale, rowSpacing);
}

TextureAssetReference CharacterCreationUiView::getChooseClassListBoxTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::PopUp2));
}

TextBox::InitInfo CharacterCreationUiView::getChooseClassTitleTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithXY(
		text,
		CharacterCreationUiView::ChooseClassTitleX,
		CharacterCreationUiView::ChooseClassTitleY,
		CharacterCreationUiView::ChooseClassTitleFontName,
		CharacterCreationUiView::ChooseClassTitleColor,
		CharacterCreationUiView::ChooseClassTitleAlignment,
		fontLibrary);
}

int CharacterCreationUiView::getChooseGenderTitleTextureX(int textureWidth)
{
	return (ArenaRenderUtils::SCREEN_WIDTH / 2) - (textureWidth / 2);
}

int CharacterCreationUiView::getChooseGenderTitleTextureY(int textureHeight)
{
	return (ArenaRenderUtils::SCREEN_HEIGHT / 2) - (textureHeight / 2);
}

int CharacterCreationUiView::getChooseNameTitleTextureX(int textureWidth)
{
	return (ArenaRenderUtils::SCREEN_WIDTH / 2) - (textureWidth / 2);
}

int CharacterCreationUiView::getChooseNameTitleTextureY(int textureHeight)
{
	return (ArenaRenderUtils::SCREEN_HEIGHT / 2) - (textureHeight / 2);
}

TextureAssetReference CharacterCreationUiView::getChooseRaceBackgroundTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::RaceSelect));
}

TextureAssetReference CharacterCreationUiView::getChooseRaceNoExitTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::NoExit));
}

int CharacterCreationUiView::getChooseRaceNoExitTextureX(int textureWidth)
{
	return ArenaRenderUtils::SCREEN_WIDTH - textureWidth;
}

int CharacterCreationUiView::getChooseRaceNoExitTextureY(int textureHeight)
{
	return ArenaRenderUtils::SCREEN_HEIGHT - textureHeight;
}

Rect CharacterCreationUiView::getChooseRaceProvinceConfirmTitleTextureRect(int textWidth, int textHeight)
{
	const int width = textWidth + 22;
	const int height = 60; // Doesn't need text height.
	return Rect(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - (width / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - (height / 2) - 21,
		width,
		height);
}

Rect CharacterCreationUiView::getChooseRaceProvinceConfirmYesTextureRect(const Rect &titleTextureRect)
{
	return Rect(
		titleTextureRect.getLeft(),
		titleTextureRect.getTop() + titleTextureRect.getHeight(),
		titleTextureRect.getWidth(),
		40);
}

Rect CharacterCreationUiView::getChooseRaceProvinceConfirmNoTextureRect(const Rect &yesTextureRect)
{
	return Rect(
		yesTextureRect.getLeft(),
		yesTextureRect.getTop() + yesTextureRect.getHeight(),
		yesTextureRect.getWidth(),
		yesTextureRect.getHeight());
}

Rect CharacterCreationUiView::getChooseRaceProvinceConfirmedFirstTextureRect(int textWidth, int textHeight)
{
	const Int2 center(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	return Rect(
		center,
		textWidth + 20,
		std::max(textHeight, 40));
}

Rect CharacterCreationUiView::getChooseRaceProvinceConfirmedSecondTextureRect(int textWidth, int textHeight)
{
	const Int2 center(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	return Rect(
		center,
		textWidth + 20,
		std::max(textHeight + 14, 40));
}

Rect CharacterCreationUiView::getChooseRaceProvinceConfirmedThirdTextureRect(int textWidth, int textHeight)
{
	const Int2 center(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	return Rect(
		center,
		textWidth + 20,
		std::max(textHeight + 18, 40));
}

Rect CharacterCreationUiView::getChooseRaceProvinceConfirmedFourthTextureRect(int textWidth, int textHeight)
{
	const Int2 center(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	return Rect(
		center,
		textWidth + 20,
		std::max(textHeight + 8, 40));
}

TextBox::InitInfo CharacterCreationUiView::getChooseRaceProvinceConfirmTitleTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::ChooseRaceProvinceConfirmTitleCenterPoint,
		CharacterCreationUiView::ChooseRaceProvinceConfirmTitleFontName,
		CharacterCreationUiView::ChooseRaceProvinceConfirmTitleTextColor,
		CharacterCreationUiView::ChooseRaceProvinceConfirmTitleAlignment,
		std::nullopt,
		CharacterCreationUiView::ChooseRaceProvinceConfirmTitleLineSpacing,
		fontLibrary);
}

TextBox::InitInfo CharacterCreationUiView::getChooseRaceProvinceConfirmYesTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::ChooseRaceProvinceConfirmYesCenterPoint,
		CharacterCreationUiView::ChooseRaceProvinceConfirmYesFontName,
		CharacterCreationUiView::ChooseRaceProvinceConfirmYesTextColor,
		CharacterCreationUiView::ChooseRaceProvinceConfirmYesAlignment,
		fontLibrary);
}

TextBox::InitInfo CharacterCreationUiView::getChooseRaceProvinceConfirmNoTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::ChooseRaceProvinceConfirmNoCenterPoint,
		CharacterCreationUiView::ChooseRaceProvinceConfirmNoFontName,
		CharacterCreationUiView::ChooseRaceProvinceConfirmNoTextColor,
		CharacterCreationUiView::ChooseRaceProvinceConfirmNoAlignment,
		fontLibrary);
}

TextBox::InitInfo CharacterCreationUiView::getChooseRaceProvinceConfirmedFirstTextBoxInitInfo(
	const std::string_view &text, const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFirstTextCenterPoint,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFirstTextFontName,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFirstTextColor,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFirstTextAlignment,
		std::nullopt,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFirstTextLineSpacing,
		fontLibrary);
}

TextBox::InitInfo CharacterCreationUiView::getChooseRaceProvinceConfirmedSecondTextBoxInitInfo(
	const std::string_view &text, const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedSecondTextCenterPoint,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedSecondTextFontName,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedSecondTextColor,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedSecondTextAlignment,
		std::nullopt,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedSecondTextLineSpacing,
		fontLibrary);
}

TextBox::InitInfo CharacterCreationUiView::getChooseRaceProvinceConfirmedThirdTextBoxInitInfo(
	const std::string_view &text, const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedThirdTextCenterPoint,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedThirdTextFontName,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedThirdTextColor,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedThirdTextAlignment,
		std::nullopt,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedThirdTextLineSpacing,
		fontLibrary);
}

TextBox::InitInfo CharacterCreationUiView::getChooseRaceProvinceConfirmedFourthTextBoxInitInfo(
	const std::string_view &text, const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFourthTextCenterPoint,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFourthTextFontName,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFourthTextColor,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFourthTextAlignment,
		std::nullopt,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFourthTextLineSpacing,
		fontLibrary);
}

int CharacterCreationUiView::getChooseClassCreationTitleTextureX(int textureWidth)
{
	return (ArenaRenderUtils::SCREEN_WIDTH / 2) - (textureWidth / 2) - 1;
}

int CharacterCreationUiView::getChooseClassCreationTitleTextureY(int textureHeight)
{
	return (ArenaRenderUtils::SCREEN_HEIGHT / 2) - (textureHeight / 2) + 1;
}

int CharacterCreationUiView::getChooseAttributesTextureWidth()
{
	return 183;
}

int CharacterCreationUiView::getChooseAttributesTextureHeight()
{
	return 42;
}

Rect CharacterCreationUiView::getChooseAttributesUnsavedDoneTitleTextureRect(int textWidth, int textHeight)
{
	const int textureWidth = textWidth + 12;
	const int textureHeight = 24;
	return Rect(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - (textureWidth / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - (textureHeight / 2) - 21,
		textureWidth,
		textureHeight);
}

Rect CharacterCreationUiView::getChooseAttributesUnsavedDoneSaveTextureRect(const Rect &titleTextureRect)
{
	return Rect(
		titleTextureRect.getLeft(),
		titleTextureRect.getTop() + titleTextureRect.getHeight(),
		titleTextureRect.getWidth(),
		titleTextureRect.getHeight());
}

Rect CharacterCreationUiView::getChooseAttributesUnsavedDoneRerollTextureRect(const Rect &saveTextureRect)
{
	return Rect(
		saveTextureRect.getLeft(),
		saveTextureRect.getTop() + saveTextureRect.getHeight(),
		saveTextureRect.getWidth(),
		saveTextureRect.getHeight());
}

TextBox::InitInfo CharacterCreationUiView::getChooseAttributesUnsavedDoneTitleTextBoxInitInfo(
	const std::string_view &text, const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::AttributesMessageBoxTitleCenterPoint,
		CharacterCreationUiView::AttributesMessageBoxTitleFontName,
		CharacterCreationUiView::AttributesMessageBoxTitleColor,
		CharacterCreationUiView::AttributesMessageBoxTitleAlignment,
		fontLibrary);
}

TextBox::InitInfo CharacterCreationUiView::getChooseAttributesUnsavedDoneSaveTextBoxInitInfo(
	const std::string_view &text, const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::AttributesMessageBoxSaveCenterPoint,
		CharacterCreationUiView::AttributesMessageBoxSaveFontName,
		CharacterCreationUiView::AttributesMessageBoxSaveColor,
		CharacterCreationUiView::AttributesMessageBoxSaveAlignment,
		fontLibrary);
}

TextBox::InitInfo CharacterCreationUiView::getChooseAttributesUnsavedDoneRerollTextBoxInitInfo(
	const std::string_view &text, const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::AttributesMessageBoxRerollCenterPoint,
		CharacterCreationUiView::AttributesMessageBoxRerollFontName,
		CharacterCreationUiView::AttributesMessageBoxRerollColor,
		CharacterCreationUiView::AttributesMessageBoxRerollAlignment,
		fontLibrary);
}

int CharacterCreationUiView::getAppearanceMessageBoxTextureWidth(int textWidth)
{
	return textWidth + 10;
}

int CharacterCreationUiView::getAppearanceMessageBoxTextureHeight(int textHeight)
{
	return textHeight + 12;
}

int CharacterCreationUiView::getBodyOffsetX(Game &game)
{
	const TextureAssetReference textureAssetRef = CharacterCreationUiView::getBodyTextureAssetRef(game);

	TextureManager &textureManager = game.getTextureManager();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for body \"" + textureAssetRef.filename + "\".");
	}

	const TextureBuilder &bodyTexture = textureManager.getTextureBuilderHandle(*textureBuilderID);
	return ArenaRenderUtils::SCREEN_WIDTH - bodyTexture.getWidth();
}

Int2 CharacterCreationUiView::getHeadOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();
	const int raceID = charCreationState.getRaceIndex();

	constexpr bool trimmed = false;
	const std::string &headsFilename = PortraitFile::getHeads(isMale, raceID, trimmed);

	auto &textureManager = game.getTextureManager();
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(headsFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugCrash("Couldn't get texture file metadata for \"" + headsFilename + "\".");
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	const int headOffsetIndex = charCreationState.getPortraitIndex();
	return textureFileMetadata.getOffset(headOffsetIndex);
}

Int2 CharacterCreationUiView::getShirtOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();

	const CharacterClassLibrary &charClassLibrary = game.getCharacterClassLibrary();
	const int charClassDefID = charCreationState.getClassDefID();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.canCastMagic();

	return PortraitFile::getShirtOffset(isMale, isMagic);
}

Int2 CharacterCreationUiView::getPantsOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();
	return PortraitFile::getPantsOffset(isMale);
}

TextureAssetReference CharacterCreationUiView::getBodyTextureAssetRef(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();
	const int raceID = charCreationState.getRaceIndex();

	std::string bodyFilename = PortraitFile::getBody(isMale, raceID);
	return TextureAssetReference(std::move(bodyFilename));
}

TextureAssetReference CharacterCreationUiView::getHeadTextureAssetRef(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();
	const int raceID = charCreationState.getRaceIndex();

	constexpr bool trimmed = false;
	std::string headsFilename = PortraitFile::getHeads(isMale, raceID, trimmed);
	const int headIndex = charCreationState.getPortraitIndex();
	return TextureAssetReference(std::move(headsFilename), headIndex);
}

TextureAssetReference CharacterCreationUiView::getShirtTextureAssetRef(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();

	const CharacterClassLibrary &charClassLibrary = game.getCharacterClassLibrary();
	const int charClassDefID = charCreationState.getClassDefID();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.canCastMagic();

	std::string shirtFilename = PortraitFile::getShirt(isMale, isMagic);
	return TextureAssetReference(std::move(shirtFilename));
}

TextureAssetReference CharacterCreationUiView::getPantsTextureAssetRef(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();

	std::string pantsFilename = PortraitFile::getPants(isMale);
	return TextureAssetReference(std::move(pantsFilename));
}
