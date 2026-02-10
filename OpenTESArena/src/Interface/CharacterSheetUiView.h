#ifndef CHARACTER_SHEET_UI_VIEW_H
#define CHARACTER_SHEET_UI_VIEW_H

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

#include "components/utilities/Span.h"

class Game;

namespace CharacterSheetUiView
{
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

namespace CharacterEquipmentUiView
{
	constexpr int PlayerLevelTextBoxX = 128;
	constexpr int PlayerLevelTextBoxY = 23;
	const std::string PlayerLevelTextBoxFontName = ArenaFontName::Arena;
	constexpr Color PlayerLevelTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerLevelTextBoxAlignment = TextAlignment::TopLeft;

	TextBoxInitInfo getPlayerLevelTextBoxInitInfo(const FontLibrary &fontLibrary);
}

#endif
