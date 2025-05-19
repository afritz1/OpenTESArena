#include <map>
#include <optional>

#include "CharacterSheetUiView.h"
#include "CommonUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaPortraitUtils.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/Game.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Stats/CharacterRaceLibrary.h"
#include "../Stats/PrimaryAttribute.h"

TextBoxInitInfo CharacterSheetUiView::getPlayerNameTextBoxInitInfo(const std::string_view text,
	const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		text,
		CharacterSheetUiView::PlayerNameTextBoxX,
		CharacterSheetUiView::PlayerNameTextBoxY,
		CharacterSheetUiView::PlayerNameTextBoxFontName,
		CharacterSheetUiView::PlayerNameTextBoxColor,
		CharacterSheetUiView::PlayerNameTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerRaceTextBoxInitInfo(const std::string_view text,
	const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		text,
		CharacterSheetUiView::PlayerRaceTextBoxX,
		CharacterSheetUiView::PlayerRaceTextBoxY,
		CharacterSheetUiView::PlayerRaceTextBoxFontName,
		CharacterSheetUiView::PlayerRaceTextBoxColor,
		CharacterSheetUiView::PlayerRaceTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerClassTextBoxInitInfo(const std::string_view text,
	const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		text,
		CharacterSheetUiView::PlayerClassTextBoxX,
		CharacterSheetUiView::PlayerClassTextBoxY,
		CharacterSheetUiView::PlayerClassTextBoxFontName,
		CharacterSheetUiView::PlayerClassTextBoxColor,
		CharacterSheetUiView::PlayerClassTextBoxAlignment,
		fontLibrary);
}

std::vector<TextBoxInitInfo> CharacterSheetUiView::getPlayerAttributeTextBoxInitInfos(BufferView<const PrimaryAttribute> attributes, const FontLibrary &fontLibrary)
{
	std::vector<TextBoxInitInfo> textBoxInitInfos;
	
	for (int i = 0; i < attributes.getCount(); i++)
	{
		const PrimaryAttribute &attribute = attributes[i];
		const std::string worstCaseStr(3, TextRenderUtils::LARGEST_CHAR);
		TextBoxInitInfo initInfo = TextBoxInitInfo::makeWithXY(
			worstCaseStr,
			CharacterSheetUiView::PlayerAttributeTextBoxX,
			CharacterSheetUiView::PlayerAttributeTextBoxesY + i * CharacterSheetUiView::PlayerAttributeTextBoxHeight,
			CharacterSheetUiView::PlayerAttributeTextBoxFontName,
			CharacterSheetUiView::PlayerAttributeTextBoxColor,
			CharacterSheetUiView::PlayerAttributeTextBoxAlignment,
			fontLibrary);

		textBoxInitInfos.emplace_back(std::move(initInfo));
	}

	return textBoxInitInfos;
}

TextBoxInitInfo CharacterSheetUiView::getPlayerExperienceTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		text,
		CharacterSheetUiView::PlayerExperienceTextBoxX,
		CharacterSheetUiView::PlayerExperienceTextBoxY,
		CharacterSheetUiView::PlayerExperienceTextBoxFontName,
		CharacterSheetUiView::PlayerExperienceTextBoxColor,
		CharacterSheetUiView::PlayerExperienceTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerLevelTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		text,
		CharacterSheetUiView::PlayerLevelTextBoxX,
		CharacterSheetUiView::PlayerLevelTextBoxY,
		CharacterSheetUiView::PlayerLevelTextBoxFontName,
		CharacterSheetUiView::PlayerLevelTextBoxColor,
		CharacterSheetUiView::PlayerLevelTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerHealthTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		text,
		CharacterSheetUiView::PlayerHealthTextBoxX,
		CharacterSheetUiView::PlayerHealthTextBoxY,
		CharacterSheetUiView::PlayerHealthTextBoxFontName,
		CharacterSheetUiView::PlayerHealthTextBoxColor,
		CharacterSheetUiView::PlayerHealthTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerStaminaTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		text,
		CharacterSheetUiView::PlayerStaminaTextBoxX,
		CharacterSheetUiView::PlayerStaminaTextBoxY,
		CharacterSheetUiView::PlayerStaminaTextBoxFontName,
		CharacterSheetUiView::PlayerStaminaTextBoxColor,
		CharacterSheetUiView::PlayerStaminaTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerSpellPointsTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		text,
		CharacterSheetUiView::PlayerSpellPointsTextBoxX,
		CharacterSheetUiView::PlayerSpellPointsTextBoxY,
		CharacterSheetUiView::PlayerSpellPointsTextBoxFontName,
		CharacterSheetUiView::PlayerSpellPointsTextBoxColor,
		CharacterSheetUiView::PlayerSpellPointsTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerDamageTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		std::string(5, TextRenderUtils::LARGEST_CHAR),
		CharacterSheetUiView::PlayerDamageTextBoxX,
		CharacterSheetUiView::PlayerDamageTextBoxY,
		CharacterSheetUiView::PlayerDamageTextBoxFontName,
		CharacterSheetUiView::PlayerDamageTextBoxColor,
		CharacterSheetUiView::PlayerDamageTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerMagicDefenseTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		std::string(5, TextRenderUtils::LARGEST_CHAR),
		CharacterSheetUiView::PlayerMagicDefenseTextBoxX,
		CharacterSheetUiView::PlayerMagicDefenseTextBoxY,
		CharacterSheetUiView::PlayerMagicDefenseTextBoxFontName,
		CharacterSheetUiView::PlayerMagicDefenseTextBoxColor,
		CharacterSheetUiView::PlayerMagicDefenseTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerBonusToHitTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		std::string(5, TextRenderUtils::LARGEST_CHAR),
		CharacterSheetUiView::PlayerBonusToHitTextBoxX,
		CharacterSheetUiView::PlayerBonusToHitTextBoxY,
		CharacterSheetUiView::PlayerBonusToHitTextBoxFontName,
		CharacterSheetUiView::PlayerBonusToHitTextBoxColor,
		CharacterSheetUiView::PlayerBonusToHitTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerBonusToDefendTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		std::string(5, TextRenderUtils::LARGEST_CHAR),
		CharacterSheetUiView::PlayerBonusToDefendTextBoxX,
		CharacterSheetUiView::PlayerBonusToDefendTextBoxY,
		CharacterSheetUiView::PlayerBonusToDefendTextBoxFontName,
		CharacterSheetUiView::PlayerBonusToDefendTextBoxColor,
		CharacterSheetUiView::PlayerBonusToDefendTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerBonusToHealthTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		std::string(5, TextRenderUtils::LARGEST_CHAR),
		CharacterSheetUiView::PlayerBonusToHealthTextBoxX,
		CharacterSheetUiView::PlayerBonusToHealthTextBoxY,
		CharacterSheetUiView::PlayerBonusToHealthTextBoxFontName,
		CharacterSheetUiView::PlayerBonusToHealthTextBoxColor,
		CharacterSheetUiView::PlayerBonusToHealthTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerCharismaTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		std::string(5, TextRenderUtils::LARGEST_CHAR),
		CharacterSheetUiView::PlayerCharismaTextBoxX,
		CharacterSheetUiView::PlayerCharismaTextBoxY,
		CharacterSheetUiView::PlayerCharismaTextBoxFontName,
		CharacterSheetUiView::PlayerCharismaTextBoxColor,
		CharacterSheetUiView::PlayerCharismaTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerHealModTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		std::string(5, TextRenderUtils::LARGEST_CHAR),
		CharacterSheetUiView::PlayerHealModTextBoxX,
		CharacterSheetUiView::PlayerHealModTextBoxY,
		CharacterSheetUiView::PlayerHealModTextBoxFontName,
		CharacterSheetUiView::PlayerHealModTextBoxColor,
		CharacterSheetUiView::PlayerHealModTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerMaxWeightTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		std::string(6, TextRenderUtils::LARGEST_CHAR),
		CharacterSheetUiView::PlayerMaxWeightTextBoxX,
		CharacterSheetUiView::PlayerMaxWeightTextBoxY,
		CharacterSheetUiView::PlayerMaxWeightTextBoxFontName,
		CharacterSheetUiView::PlayerMaxWeightTextBoxColor,
		CharacterSheetUiView::PlayerMaxWeightTextBoxAlignment,
		fontLibrary);
}

TextBoxInitInfo CharacterSheetUiView::getPlayerGoldTextBoxInitInfo(const std::string_view text, const FontLibrary &fontLibrary)
{
	return TextBoxInitInfo::makeWithXY(
		text,
		CharacterSheetUiView::PlayerGoldTextBoxX,
		CharacterSheetUiView::PlayerGoldTextBoxY,
		CharacterSheetUiView::PlayerGoldTextBoxFontName,
		CharacterSheetUiView::PlayerGoldTextBoxColor,
		CharacterSheetUiView::PlayerGoldTextBoxAlignment,
		fontLibrary);
}

Int2 CharacterSheetUiView::getBodyOffset(Game &game)
{
	const TextureAsset textureAsset = CharacterSheetUiView::getBodyTextureAsset(game);

	TextureManager &textureManager = game.textureManager;
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for body \"" + textureAsset.filename + "\".");
	}

	const TextureBuilder &bodyTexture = textureManager.getTextureBuilderHandle(*textureBuilderID);
	return Int2(ArenaRenderUtils::SCREEN_WIDTH - bodyTexture.getWidth(), 0);
}

Int2 CharacterSheetUiView::getHeadOffset(Game &game)
{
	const Player &player = game.player;
	const bool isMale = player.male;
	const int raceID = player.raceID;

	constexpr bool trimmed = false;
	const std::string &headsFilename = ArenaPortraitUtils::getHeads(isMale, raceID, trimmed);

	auto &textureManager = game.textureManager;
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(headsFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugCrash("Couldn't get texture file metadata for \"" + headsFilename + "\".");
	}

	const TextureFileMetadata &textureFileMetadata = textureManager.getMetadataHandle(*metadataID);
	const int headOffsetIndex = player.portraitID;
	return textureFileMetadata.getOffset(headOffsetIndex);
}

Int2 CharacterSheetUiView::getShirtOffset(Game &game)
{
	const Player &player = game.player;
	const bool isMale = player.male;

	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const int charClassDefID = player.charClassDefID;
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.castsMagic;

	return ArenaPortraitUtils::getShirtOffset(isMale, isMagic);
}

Int2 CharacterSheetUiView::getPantsOffset(Game &game)
{
	const Player &player = game.player;
	const bool isMale = player.male;
	return ArenaPortraitUtils::getPantsOffset(isMale);
}

Int2 CharacterSheetUiView::getNextPageOffset()
{
	return Int2(108, 179);
}

TextureAsset CharacterSheetUiView::getPaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaPaletteName::CharSheet));
}

TextureAsset CharacterSheetUiView::getStatsBackgroundTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::CharacterStats));
}

TextureAsset CharacterSheetUiView::getEquipmentBackgroundTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::CharacterEquipment));
}

TextureAsset CharacterSheetUiView::getNextPageButtonTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::NextPage));
}

TextureAsset CharacterSheetUiView::getBodyTextureAsset(Game &game)
{
	const Player &player = game.player;
	const CharacterRaceLibrary &characterRaceLibrary = CharacterRaceLibrary::getInstance();
	const CharacterRaceDefinition &characterRaceDefinition = characterRaceLibrary.getDefinition(player.raceID);

	if (player.male)
	{
		return characterRaceDefinition.maleCharSheetBodyTextureAsset;
	}
	else
	{
		return characterRaceDefinition.femaleCharSheetBodyTextureAsset;
	}
}

TextureAsset CharacterSheetUiView::getHeadTextureAsset(Game &game)
{
	const Player &player = game.player;
	const CharacterRaceLibrary &characterRaceLibrary = CharacterRaceLibrary::getInstance();
	const CharacterRaceDefinition &characterRaceDefinition = characterRaceLibrary.getDefinition(player.raceID);
	const int headIndex = player.portraitID;

	if (player.male)
	{
		return TextureAsset(std::string(characterRaceDefinition.maleCharSheetHeadsFilename), headIndex);
	}
	else
	{
		return TextureAsset(std::string(characterRaceDefinition.femaleCharSheetHeadsFilename), headIndex);
	}
}

TextureAsset CharacterSheetUiView::getShirtTextureAsset(Game &game)
{
	const Player &player = game.player;
	const bool isMale = player.male;

	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const int charClassDefID = player.charClassDefID;
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
	const bool isMagic = charClassDef.castsMagic;

	std::string shirtFilename = ArenaPortraitUtils::getShirt(isMale, isMagic);
	return TextureAsset(std::move(shirtFilename));
}

TextureAsset CharacterSheetUiView::getPantsTextureAsset(Game &game)
{
	const Player &player = game.player;
	const bool isMale = player.male;

	std::string pantsFilename = ArenaPortraitUtils::getPants(isMale);
	return TextureAsset(std::move(pantsFilename));
}

UiTextureID CharacterSheetUiView::allocBodyTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getBodyTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character body.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocShirtTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getShirtTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character shirt.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocPantsTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getPantsTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character pants.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocHeadTexture(Game &game)
{
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;

	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getHeadTextureAsset(game);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for character head.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocStatsBgTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getStatsBackgroundTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for stats background.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocUpDownButtonTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset upDownTextureAsset = TextureAsset(std::string(ArenaTextureName::UpDown));
	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();

	UiTextureID upDownTextureID;
	if (!TextureUtils::tryAllocUiTexture(upDownTextureAsset, paletteTextureAsset, textureManager, renderer, &upDownTextureID))
	{
		DebugLogError("Couldn't get texture ID for up/down arrows.");
		return -1;
	}

	return upDownTextureID;
}

UiTextureID CharacterSheetUiView::allocBonusPointsTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset bonusPointsTextureAsset = TextureAsset(std::string(ArenaTextureName::BonusPointsText));
	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();

	UiTextureID bonusPointsTextureID;
	if (!TextureUtils::tryAllocUiTexture(bonusPointsTextureAsset, paletteTextureAsset, textureManager, renderer, &bonusPointsTextureID))
	{
		DebugLogError("Couldn't get texture ID for bonus points.");
		return -1;
	}

	return bonusPointsTextureID;
}

UiTextureID CharacterSheetUiView::allocEquipmentBgTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getEquipmentBackgroundTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for equipment background.");
	}

	return textureID;
}

UiTextureID CharacterSheetUiView::allocNextPageTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const TextureAsset textureAsset = CharacterSheetUiView::getNextPageButtonTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for next page button.");
	}

	return textureID;
}
