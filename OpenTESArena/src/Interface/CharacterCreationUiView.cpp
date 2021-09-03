#include <algorithm>
#include <optional>

#include "CharacterCreationUiView.h"
#include "CharacterSheetUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../Media/PortraitFile.h"
#include "../UI/FontDefinition.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"

TextureAssetReference CharacterCreationUiView::getNightSkyTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::CharacterCreation));
}

UiTextureID CharacterCreationUiView::allocNightSkyTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAssetReference textureAssetRef = CharacterCreationUiView::getNightSkyTextureAssetRef();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(textureAssetRef);
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + textureAssetRef.filename + "\".");
	}

	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, *paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for background texture.");
	}

	return textureID;
}

Int2 ChooseClassCreationUiView::getTitleTextureCenter()
{
	return Int2(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) + 1 - 20);
}

Int2 ChooseClassCreationUiView::getGenerateTextureCenter()
{
	return ChooseClassCreationUiView::getTitleTextureCenter() + Int2(0, 40);
}

Int2 ChooseClassCreationUiView::getSelectTextureCenter()
{
	return ChooseClassCreationUiView::getGenerateTextureCenter() + Int2(0, 40);
}

TextBox::InitInfo ChooseClassCreationUiView::getTitleTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		ChooseClassCreationUiView::TitleCenter,
		ChooseClassCreationUiView::TitleFontName,
		ChooseClassCreationUiView::TitleColor,
		ChooseClassCreationUiView::TitleAlignment,
		std::nullopt,
		ChooseClassCreationUiView::TitleLineSpacing,
		fontLibrary);
}

TextBox::InitInfo ChooseClassCreationUiView::getGenerateTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		ChooseClassCreationUiView::GenerateTextCenterPoint,
		ChooseClassCreationUiView::GenerateTextFontName,
		ChooseClassCreationUiView::GenerateTextColor,
		ChooseClassCreationUiView::GenerateTextAlignment,
		fontLibrary);
}

TextBox::InitInfo ChooseClassCreationUiView::getSelectTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		ChooseClassCreationUiView::SelectTextCenterPoint,
		ChooseClassCreationUiView::SelectTextFontName,
		ChooseClassCreationUiView::SelectTextColor,
		ChooseClassCreationUiView::SelectTextAlignment,
		fontLibrary);
}

UiTextureID ChooseClassCreationUiView::allocParchmentTexture(TextureManager &textureManager, Renderer &renderer)
{
	const Surface surface = TextureUtils::generate(
		ChooseClassCreationUiView::PopUpPatternType,
		ChooseClassCreationUiView::PopUpTextureWidth,
		ChooseClassCreationUiView::PopUpTextureHeight,
		textureManager,
		renderer);

	const BufferView2D<const uint32_t> texelsView(reinterpret_cast<const uint32_t*>(surface.getPixels()),
		surface.getWidth(), surface.getHeight());

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(texelsView, &textureID))
	{
		DebugCrash("Couldn't create UI texture for parchment.");
	}

	return textureID;
}

Rect ChooseClassUiView::getListRect(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	const auto &chooseClassListUI = exeData.ui.chooseClassList;
	return Rect(
		chooseClassListUI.area.x,
		chooseClassListUI.area.y,
		chooseClassListUI.area.w,
		chooseClassListUI.area.h);
}

Rect ChooseClassUiView::getUpButtonRect(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	const auto &chooseClassListUI = exeData.ui.chooseClassList;
	return Rect(
		chooseClassListUI.buttonUp.x,
		chooseClassListUI.buttonUp.y,
		chooseClassListUI.buttonUp.w,
		chooseClassListUI.buttonUp.h);
}

Rect ChooseClassUiView::getDownButtonRect(Game &game)
{
	const auto &exeData = game.getBinaryAssetLibrary().getExeData();
	const auto &chooseClassListUI = exeData.ui.chooseClassList;
	return Rect(
		chooseClassListUI.buttonDown.x,
		chooseClassListUI.buttonDown.y,
		chooseClassListUI.buttonDown.w,
		chooseClassListUI.buttonDown.h);
}

ListBox::Properties ChooseClassUiView::makeListBoxProperties(const FontLibrary &fontLibrary)
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
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	const Color itemColor(85, 44, 20);
	constexpr double scrollScale = 1.0;
	return ListBox::Properties(fontDefIndex, &fontLibrary, textureGenInfo, fontDef.getCharacterHeight(),
		itemColor, scrollScale);
}

TextureAssetReference ChooseClassUiView::getListBoxTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::PopUp2));
}

TextBox::InitInfo ChooseClassUiView::getTitleTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithXY(
		text,
		ChooseClassUiView::TitleX,
		ChooseClassUiView::TitleY,
		ChooseClassUiView::TitleFontName,
		ChooseClassUiView::TitleColor,
		ChooseClassUiView::TitleAlignment,
		fontLibrary);
}

TextBox::InitInfo ChooseClassUiView::getClassDescriptionTextBoxInitInfo(const FontLibrary &fontLibrary)
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

	TextRenderUtils::TextShadowInfo shadowInfo;
	shadowInfo.init(1, 0, Color::Black);

	return TextBox::InitInfo::makeWithCenter(
		dummyText,
		Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT - 32),
		ArenaFontName::D,
		Color::White,
		TextAlignment::TopCenter,
		shadowInfo,
		0,
		fontLibrary);
}

UiTextureID ChooseClassUiView::allocPopUpTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAssetReference paletteTextureAssetRef = CharacterCreationUiView::getNightSkyTextureAssetRef();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteTextureAssetRef);
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + paletteTextureAssetRef.filename + "\".");
	}

	const TextureAssetReference textureAssetRef = ChooseClassUiView::getListBoxTextureAssetRef();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get list pop-up texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, *paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for class pop-up.");
	}

	return textureID;
}

Int2 ChooseGenderUiView::getTitleTextureCenter()
{
	return Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, (ArenaRenderUtils::SCREEN_HEIGHT / 2) - 20);
}

Int2 ChooseGenderUiView::getMaleTextureCenter()
{
	return ChooseGenderUiView::getTitleTextureCenter() + Int2(0, 40);
}

Int2 ChooseGenderUiView::getFemaleTextureCenter()
{
	return ChooseGenderUiView::getMaleTextureCenter() + Int2(0, 40);
}

TextBox::InitInfo ChooseGenderUiView::getTitleTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		ChooseGenderUiView::TitleCenterPoint,
		ChooseGenderUiView::TitleFontName,
		ChooseGenderUiView::TitleColor,
		ChooseGenderUiView::TitleAlignment,
		fontLibrary);
}

TextBox::InitInfo ChooseGenderUiView::getMaleTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		ChooseGenderUiView::MaleTextBoxCenter,
		ChooseGenderUiView::MaleFontName,
		ChooseGenderUiView::MaleColor,
		ChooseGenderUiView::MaleAlignment,
		fontLibrary);
}

TextBox::InitInfo ChooseGenderUiView::getFemaleTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		ChooseGenderUiView::FemaleTextBoxCenter,
		ChooseGenderUiView::FemaleFontName,
		ChooseGenderUiView::FemaleColor,
		ChooseGenderUiView::FemaleAlignment,
		fontLibrary);
}

UiTextureID ChooseGenderUiView::allocParchmentTexture(TextureManager &textureManager, Renderer &renderer)
{
	const Surface surface = TextureUtils::generate(ChooseGenderUiView::TexturePatternType,
		ChooseGenderUiView::TextureWidth, ChooseGenderUiView::TextureHeight, textureManager, renderer);
	const BufferView2D<const uint32_t> texelsView(reinterpret_cast<const uint32_t*>(surface.getPixels()),
		surface.getWidth(), surface.getHeight());

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(texelsView, &textureID))
	{
		DebugCrash("Couldn't create UI texture for parchment.");
	}

	return textureID;
}

int ChooseNameUiView::getTitleTextureX(int textureWidth)
{
	return (ArenaRenderUtils::SCREEN_WIDTH / 2) - (textureWidth / 2);
}

int ChooseNameUiView::getTitleTextureY(int textureHeight)
{
	return (ArenaRenderUtils::SCREEN_HEIGHT / 2) - (textureHeight / 2);
}

TextBox::InitInfo ChooseNameUiView::getTitleTextBoxInitInfo(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithXY(
		text,
		ChooseNameUiView::TitleTextBoxX,
		ChooseNameUiView::TitleTextBoxY,
		ChooseNameUiView::TitleFontName,
		ChooseNameUiView::TitleColor,
		ChooseNameUiView::TitleAlignment,
		fontLibrary);
}

TextBox::InitInfo ChooseNameUiView::getEntryTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	const std::string dummyText(CharacterCreationState::MAX_NAME_LENGTH, TextRenderUtils::LARGEST_CHAR);

	return TextBox::InitInfo::makeWithXY(
		dummyText,
		ChooseNameUiView::EntryTextBoxX,
		ChooseNameUiView::EntryTextBoxY,
		ChooseNameUiView::EntryFontName,
		ChooseNameUiView::EntryColor,
		ChooseNameUiView::EntryAlignment,
		fontLibrary);
}

TextureAssetReference ChooseRaceUiView::getBackgroundTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::RaceSelect));
}

TextureAssetReference ChooseRaceUiView::getNoExitTextureAssetRef()
{
	return TextureAssetReference(std::string(ArenaTextureName::NoExit));
}

int ChooseRaceUiView::getNoExitTextureX(int textureWidth)
{
	return ArenaRenderUtils::SCREEN_WIDTH - textureWidth;
}

int ChooseRaceUiView::getNoExitTextureY(int textureHeight)
{
	return ArenaRenderUtils::SCREEN_HEIGHT - textureHeight;
}

Rect ChooseRaceUiView::getProvinceConfirmTitleTextBoxRect(const std::string_view &text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = ChooseRaceUiView::ProvinceConfirmTitleFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(
		text, fontDef, std::nullopt, ChooseRaceUiView::ProvinceConfirmTitleLineSpacing);
	return Rect(
		ChooseRaceUiView::ProvinceConfirmTitleCenterPoint,
		textureGenInfo.width,
		textureGenInfo.height);
}

MessageBoxSubPanel::BackgroundProperties ChooseRaceUiView::getProvinceConfirmMessageBoxBackgroundProperties()
{
	return MessageBoxSubPanel::BackgroundProperties(
		ChooseRaceUiView::ProvinceConfirmTitleTexturePatternType,
		22,
		0,
		std::nullopt,
		60,
		40);
}

MessageBoxSubPanel::TitleProperties ChooseRaceUiView::getProvinceConfirmMessageBoxTitleProperties(
	const std::string_view &text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = ChooseRaceUiView::ProvinceConfirmTitleFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(
		text, fontDef, std::nullopt, ChooseRaceUiView::ProvinceConfirmTitleLineSpacing);
	return MessageBoxSubPanel::TitleProperties(
		fontName,
		textureGenInfo,
		ChooseRaceUiView::ProvinceConfirmTitleTextColor,
		ChooseRaceUiView::ProvinceConfirmTitleLineSpacing);
}

MessageBoxSubPanel::ItemsProperties ChooseRaceUiView::getProvinceConfirmMessageBoxItemsProperties(
	const FontLibrary &fontLibrary)
{
	const std::string dummyText(5, TextRenderUtils::LARGEST_CHAR);
	const std::string &fontName = ChooseRaceUiView::ProvinceConfirmItemFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	constexpr int itemCount = 2;
	return MessageBoxSubPanel::ItemsProperties(
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

TextBox::InitInfo ChooseRaceUiView::getProvinceConfirmedFirstTextBoxInitInfo(
	const std::string_view &text, const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		ChooseRaceUiView::ProvinceConfirmedFirstTextCenterPoint,
		ChooseRaceUiView::ProvinceConfirmedFirstTextFontName,
		ChooseRaceUiView::ProvinceConfirmedFirstTextColor,
		ChooseRaceUiView::ProvinceConfirmedFirstTextAlignment,
		std::nullopt,
		ChooseRaceUiView::ProvinceConfirmedFirstTextLineSpacing,
		fontLibrary);
}

TextBox::InitInfo ChooseRaceUiView::getProvinceConfirmedSecondTextBoxInitInfo(
	const std::string_view &text, const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		ChooseRaceUiView::ProvinceConfirmedSecondTextCenterPoint,
		ChooseRaceUiView::ProvinceConfirmedSecondTextFontName,
		ChooseRaceUiView::ProvinceConfirmedSecondTextColor,
		ChooseRaceUiView::ProvinceConfirmedSecondTextAlignment,
		std::nullopt,
		ChooseRaceUiView::ProvinceConfirmedSecondTextLineSpacing,
		fontLibrary);
}

TextBox::InitInfo ChooseRaceUiView::getProvinceConfirmedThirdTextBoxInitInfo(
	const std::string_view &text, const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		ChooseRaceUiView::ProvinceConfirmedThirdTextCenterPoint,
		ChooseRaceUiView::ProvinceConfirmedThirdTextFontName,
		ChooseRaceUiView::ProvinceConfirmedThirdTextColor,
		ChooseRaceUiView::ProvinceConfirmedThirdTextAlignment,
		std::nullopt,
		ChooseRaceUiView::ProvinceConfirmedThirdTextLineSpacing,
		fontLibrary);
}

TextBox::InitInfo ChooseRaceUiView::getProvinceConfirmedFourthTextBoxInitInfo(
	const std::string_view &text, const FontLibrary &fontLibrary)
{
	return TextBox::InitInfo::makeWithCenter(
		text,
		ChooseRaceUiView::ProvinceConfirmedFourthTextCenterPoint,
		ChooseRaceUiView::ProvinceConfirmedFourthTextFontName,
		ChooseRaceUiView::ProvinceConfirmedFourthTextColor,
		ChooseRaceUiView::ProvinceConfirmedFourthTextAlignment,
		std::nullopt,
		ChooseRaceUiView::ProvinceConfirmedFourthTextLineSpacing,
		fontLibrary);
}

int ChooseAttributesUiView::getInitialTextureWidth()
{
	return 183;
}

int ChooseAttributesUiView::getInitialTextureHeight()
{
	return 42;
}

Rect ChooseAttributesUiView::getMessageBoxTitleTextBoxRect(const std::string_view &text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = ChooseAttributesUiView::MessageBoxTitleFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(text, fontDef);
	return Rect(
		ChooseAttributesUiView::MessageBoxTitleCenterPoint,
		textureGenInfo.width,
		textureGenInfo.height);
}

MessageBoxSubPanel::BackgroundProperties ChooseAttributesUiView::getMessageBoxBackgroundProperties()
{
	return MessageBoxSubPanel::BackgroundProperties(
		ChooseAttributesUiView::MessageBoxPatternType,
		12,
		0,
		std::nullopt,
		24,
		24);
}

MessageBoxSubPanel::TitleProperties ChooseAttributesUiView::getMessageBoxTitleProperties(const std::string_view &text,
	const FontLibrary &fontLibrary)
{
	const std::string &fontName = ChooseAttributesUiView::MessageBoxTitleFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(text, fontDef);
	return MessageBoxSubPanel::TitleProperties(fontName, textureGenInfo, ChooseAttributesUiView::MessageBoxTitleColor);
}

MessageBoxSubPanel::ItemsProperties ChooseAttributesUiView::getMessageBoxItemsProperties(const FontLibrary &fontLibrary)
{
	const std::string dummyText(10, TextRenderUtils::LARGEST_CHAR);
	const std::string &fontName = ChooseAttributesUiView::MessageBoxItemFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(dummyText, fontDef);

	constexpr int itemCount = 2;
	return MessageBoxSubPanel::ItemsProperties(
		itemCount,
		fontName,
		textureGenInfo,
		ChooseAttributesUiView::MessageBoxItemTextColor);
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
	const TextureAssetReference textureAssetRef = ChooseAttributesUiView::getBodyTextureAssetRef(game);

	TextureManager &textureManager = game.getTextureManager();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for body \"" + textureAssetRef.filename + "\".");
	}

	const TextureBuilder &bodyTexture = textureManager.getTextureBuilderHandle(*textureBuilderID);
	return Int2(ArenaRenderUtils::SCREEN_WIDTH - bodyTexture.getWidth(), 0);
}

Int2 ChooseAttributesUiView::getHeadOffset(Game &game)
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

Int2 ChooseAttributesUiView::getShirtOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();

	const CharacterClassLibrary &charClassLibrary = game.getCharacterClassLibrary();
	const int charClassDefID = charCreationState.getClassDefID();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.canCastMagic();

	return PortraitFile::getShirtOffset(isMale, isMagic);
}

Int2 ChooseAttributesUiView::getPantsOffset(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();
	return PortraitFile::getPantsOffset(isMale);
}

TextureAssetReference ChooseAttributesUiView::getBodyTextureAssetRef(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();
	const int raceID = charCreationState.getRaceIndex();

	std::string bodyFilename = PortraitFile::getBody(isMale, raceID);
	return TextureAssetReference(std::move(bodyFilename));
}

Buffer<TextureAssetReference> ChooseAttributesUiView::getHeadTextureAssetRefs(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();
	const int raceID = charCreationState.getRaceIndex();

	constexpr bool trimmed = false;
	std::string headsFilename = PortraitFile::getHeads(isMale, raceID, trimmed);

	auto &textureManager = game.getTextureManager();
	return TextureUtils::makeTextureAssetRefs(headsFilename, textureManager);
}

TextureAssetReference ChooseAttributesUiView::getShirtTextureAssetRef(Game &game)
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

TextureAssetReference ChooseAttributesUiView::getPantsTextureAssetRef(Game &game)
{
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const bool isMale = charCreationState.isMale();

	std::string pantsFilename = PortraitFile::getPants(isMale);
	return TextureAssetReference(std::move(pantsFilename));
}

PaletteID ChooseAttributesUiView::getPaletteID(TextureManager &textureManager)
{
	return CharacterSheetUiView::getPaletteID(textureManager);
}

UiTextureID ChooseAttributesUiView::allocBodyTexture(Game &game)
{
	auto &textureManager = game.getTextureManager();
	const PaletteID paletteID = ChooseAttributesUiView::getPaletteID(textureManager);

	const TextureAssetReference textureAssetRef = ChooseAttributesUiView::getBodyTextureAssetRef(game);
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	auto &renderer = game.getRenderer();
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character body.");
	}

	return textureID;
}

UiTextureID ChooseAttributesUiView::allocShirtTexture(Game &game)
{
	auto &textureManager = game.getTextureManager();
	const PaletteID paletteID = ChooseAttributesUiView::getPaletteID(textureManager);

	const TextureAssetReference textureAssetRef = ChooseAttributesUiView::getShirtTextureAssetRef(game);
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	auto &renderer = game.getRenderer();
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character shirt.");
	}

	return textureID;
}

UiTextureID ChooseAttributesUiView::allocPantsTexture(Game &game)
{
	auto &textureManager = game.getTextureManager();
	const PaletteID paletteID = ChooseAttributesUiView::getPaletteID(textureManager);

	const TextureAssetReference textureAssetRef = ChooseAttributesUiView::getPantsTextureAssetRef(game);
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	auto &renderer = game.getRenderer();
	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character pants.");
	}

	return textureID;
}

UiTextureID ChooseAttributesUiView::allocHeadTexture(const TextureAssetReference &textureAssetRef,
	TextureManager &textureManager, Renderer &renderer)
{
	const PaletteID paletteID = ChooseAttributesUiView::getPaletteID(textureManager);
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
	}

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character head \"" + textureAssetRef.filename + "\".");
	}

	return textureID;
}

UiTextureID ChooseAttributesUiView::allocStatsBgTexture(TextureManager &textureManager, Renderer &renderer)
{
	return CharacterSheetUiView::allocStatsBgTexture(textureManager, renderer);
}
