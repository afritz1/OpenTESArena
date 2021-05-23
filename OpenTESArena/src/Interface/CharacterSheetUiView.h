#ifndef CHARACTER_SHEET_UI_VIEW_H
#define CHARACTER_SHEET_UI_VIEW_H

#include "../Assets/TextureAssetReference.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/FontName.h"
#include "../UI/TextAlignment.h"

class Game;

namespace CharacterSheetUiView
{
	constexpr int PlayerNameTextBoxX = 10;
	constexpr int PlayerNameTextBoxY = 8;
	constexpr FontName PlayerNameTextBoxFontName = FontName::Arena;
	const Color PlayerNameTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerNameTextBoxAlignment = TextAlignment::Left;

	constexpr int PlayerRaceTextBoxX = 10;
	constexpr int PlayerRaceTextBoxY = 17;
	constexpr FontName PlayerRaceTextBoxFontName = FontName::Arena;
	const Color PlayerRaceTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerRaceTextBoxAlignment = TextAlignment::Left;

	constexpr int PlayerClassTextBoxX = 10;
	constexpr int PlayerClassTextBoxY = 26;
	constexpr FontName PlayerClassTextBoxFontName = FontName::Arena;
	const Color PlayerClassTextBoxColor(199, 199, 199);
	constexpr TextAlignment PlayerClassTextBoxAlignment = TextAlignment::Left;

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

	int getBodyOffsetX(Game &game);
	Int2 getHeadOffset(Game &game);
	Int2 getShirtOffset(Game &game);
	Int2 getPantsOffset(Game &game);

	TextureAssetReference getPaletteTextureAssetRef();
	TextureAssetReference getStatsBackgroundTextureAssetRef();
	TextureAssetReference getEquipmentBackgroundTextureAssetRef();
	TextureAssetReference getNextPageButtonTextureAssetRef();
	TextureAssetReference getBodyTextureAssetRef(Game &game);
	TextureAssetReference getHeadTextureAssetRef(Game &game);
	TextureAssetReference getShirtTextureAssetRef(Game &game);
	TextureAssetReference getPantsTextureAssetRef(Game &game);
}

#endif
