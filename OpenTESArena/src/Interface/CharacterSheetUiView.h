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

	int getBodyOffsetX(Game &game);
	Int2 getHeadOffset(Game &game);
	Int2 getShirtOffset(Game &game);
	Int2 getPantsOffset(Game &game);

	TextureAssetReference getPaletteTextureAssetRef();
	TextureAssetReference getStatsBackgroundTextureAssetRef();
	TextureAssetReference getNextPageButtonTextureAssetRef();
	TextureAssetReference getBodyTextureAssetRef(Game &game);
	TextureAssetReference getHeadTextureAssetRef(Game &game);
	TextureAssetReference getShirtTextureAssetRef(Game &game);
	TextureAssetReference getPantsTextureAssetRef(Game &game);
}

#endif
