#ifndef CHARACTER_SHEET_UI_VIEW_H
#define CHARACTER_SHEET_UI_VIEW_H

#include <map>

#include "../Assets/TextureAsset.h"
#include "../Assets/TextureUtils.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../Stats/PrimaryAttribute.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../Utilities/Color.h"

#include "components/utilities/BufferView.h"

class Game;

namespace CharacterSheetUiView
{
	constexpr int PlayerNameTextBoxX = 10;
	constexpr int PlayerNameTextBoxY = 8;
	const std::string PlayerNameTextBoxFontName = ArenaFontName::Arena;
	const Color PlayerNameTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerNameTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerRaceTextBoxX = 10;
	constexpr int PlayerRaceTextBoxY = 17;
	const std::string PlayerRaceTextBoxFontName = ArenaFontName::Arena;
	const Color PlayerRaceTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerRaceTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerClassTextBoxX = 10;
	constexpr int PlayerClassTextBoxY = 26;
	const std::string PlayerClassTextBoxFontName = ArenaFontName::Arena;
	const Color PlayerClassTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerClassTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerAttributeTextBoxX = 26;
	constexpr int PlayerAttributeTextBoxesY = 52;
	constexpr int PlayerAttributeTextBoxHeight = 8;
	const std::string PlayerAttributeTextBoxFontName = ArenaFontName::Arena;
	const Color PlayerAttributeTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerAttributeTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerExperienceTextBoxX = 45;
	constexpr int PlayerExperienceTextBoxY = 158;
	const std::string PlayerExperienceTextBoxFontName = ArenaFontName::Arena;
	const Color PlayerExperienceTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerExperienceTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerLevelTextBoxX = 45;
	constexpr int PlayerLevelTextBoxY = 167;
	const std::string PlayerLevelTextBoxFontName = ArenaFontName::Arena;
	const Color PlayerLevelTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerLevelTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerHealthTextBoxX = 45;
	constexpr int PlayerHealthTextBoxY = 127;
	const std::string PlayerHealthTextBoxFontName = ArenaFontName::Arena;
	const Color PlayerHealthTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerHealthTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerStaminaTextBoxX = 45;
	constexpr int PlayerStaminaTextBoxY = 135;
	const std::string PlayerStaminaTextBoxFontName = ArenaFontName::Arena;
	const Color PlayerStaminaTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerStaminaTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerSpellPointsTextBoxX = 86;
	constexpr int PlayerSpellPointsTextBoxY = 60;
	const std::string PlayerSpellPointsTextBoxFontName = ArenaFontName::Arena;
	const Color PlayerSpellPointsTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerSpellPointsTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerDamageTextBoxX = 86;
	constexpr int PlayerDamageTextBoxY = 52;
	const std::string PlayerDamageTextBoxFontName = ArenaFontName::Arena;
	constexpr Color PlayerDamageTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerDamageTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerMagicDefenseTextBoxX = 86;
	constexpr int PlayerMagicDefenseTextBoxY = 68;
	const std::string PlayerMagicDefenseTextBoxFontName = ArenaFontName::Arena;
	constexpr Color PlayerMagicDefenseTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerMagicDefenseTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerBonusToHitTextBoxX = 86;
	constexpr int PlayerBonusToHitTextBoxY = 76;
	const std::string PlayerBonusToHitTextBoxFontName = ArenaFontName::Arena;
	constexpr Color PlayerBonusToHitTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerBonusToHitTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerBonusToDefendTextBoxX = 145;
	constexpr int PlayerBonusToDefendTextBoxY = 76;
	const std::string PlayerBonusToDefendTextBoxFontName = ArenaFontName::Arena;
	constexpr Color PlayerBonusToDefendTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerBonusToDefendTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerBonusToHealthTextBoxX = 86;
	constexpr int PlayerBonusToHealthTextBoxY = 92;
	const std::string PlayerBonusToHealthTextBoxFontName = ArenaFontName::Arena;
	constexpr Color PlayerBonusToHealthTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerBonusToHealthTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerCharismaTextBoxX = 86;
	constexpr int PlayerCharismaTextBoxY = 100;
	const std::string PlayerCharismaTextBoxFontName = ArenaFontName::Arena;
	constexpr Color PlayerCharismaTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerCharismaTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerHealModTextBoxX = 145;
	constexpr int PlayerHealModTextBoxY = 92;
	const std::string PlayerHealModTextBoxFontName = ArenaFontName::Arena;
	constexpr Color PlayerHealModTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerHealModTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerMaxWeightTextBoxX = 145;
	constexpr int PlayerMaxWeightTextBoxY = 52;
	const std::string PlayerMaxWeightTextBoxFontName = ArenaFontName::Arena;
	constexpr Color PlayerMaxWeightTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerMaxWeightTextBoxAlignment = TextAlignment::TopLeft;

	constexpr int PlayerGoldTextBoxX = 45;
	constexpr int PlayerGoldTextBoxY = 143;
	const std::string PlayerGoldTextBoxFontName = ArenaFontName::Arena;
	const Color PlayerGoldTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerGoldTextBoxAlignment = TextAlignment::TopLeft;

	TextBoxInitInfo getPlayerNameTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerRaceTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerClassTextBoxInitInfo(const FontLibrary &fontLibrary);
	Buffer<TextBoxInitInfo> getPlayerAttributeTextBoxInitInfos(const FontLibrary &fontLibrary);
	Buffer<TextBoxInitInfo> getPlayerDerivedAttributeTextBoxInitInfos(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerExperienceTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerLevelTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerHealthTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerStaminaTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerSpellPointsTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerBonusDamageTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerMaxWeightTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerMagicDefenseTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerBonusToHitTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerBonusToDefendTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerBonusToHealthTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerHealModTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerCharismaTextBoxInitInfo(const FontLibrary &fontLibrary);
	TextBoxInitInfo getPlayerGoldTextBoxInitInfo(const FontLibrary &fontLibrary);

	const Int2 DoneButtonCenterPoint(25, ArenaRenderUtils::SCREEN_HEIGHT - 15);
	constexpr int DoneButtonWidth = 21;
	constexpr int DoneButtonHeight = 13;

	constexpr int NextPageButtonX = 108;
	constexpr int NextPageButtonY = 179;
	constexpr int NextPageButtonWidth = 49;
	constexpr int NextPageButtonHeight = 13;

	constexpr int BackToStatsButtonX = 0;
	constexpr int BackToStatsButtonY = 188;
	constexpr int BackToStatsButtonWidth = 47;
	constexpr int BackToStatsButtonHeight = 12;

	constexpr int SpellbookButtonX = 47;
	constexpr int SpellbookButtonY = 188;
	constexpr int SpellbookButtonWidth = 76;
	constexpr int SpellbookButtonHeight = 12;

	constexpr int DropButtonX = 123;
	constexpr int DropButtonY = 188;
	constexpr int DropButtonWidth = 48;
	constexpr int DropButtonHeight = 12;

	const Int2 ScrollDownButtonCenterPoint(16, 131);
	constexpr int ScrollDownButtonWidth = 9;
	constexpr int ScrollDownButtonHeight = 9;

	const Int2 ScrollUpButtonCenterPoint(152, 131);
	constexpr int ScrollUpButtonWidth = 9;
	constexpr int ScrollUpButtonHeight = 9;

	Int2 getBodyOffset(Game &game);
	Int2 getHeadOffset(Game &game);
	Int2 getShirtOffset(Game &game);
	Int2 getPantsOffset(Game &game);

	Int2 getNextPageOffset();

	TextureAsset getPaletteTextureAsset();
	TextureAsset getStatsBackgroundTextureAsset();
	TextureAsset getEquipmentBackgroundTextureAsset();
	TextureAsset getNextPageButtonTextureAsset();
	TextureAsset getBodyTextureAsset(Game &game);
	TextureAsset getHeadTextureAsset(Game &game);
	TextureAsset getShirtTextureAsset(Game &game);
	TextureAsset getPantsTextureAsset(Game &game);

	UiTextureID allocBodyTexture(Game &game);
	UiTextureID allocShirtTexture(Game &game);
	UiTextureID allocPantsTexture(Game &game);
	UiTextureID allocHeadTexture(Game &game);
	UiTextureID allocStatsBgTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocUpDownButtonTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocBonusPointsTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocEquipmentBgTexture(TextureManager &textureManager, Renderer &renderer);
	UiTextureID allocNextPageTexture(TextureManager &textureManager, Renderer &renderer);
}

#endif
