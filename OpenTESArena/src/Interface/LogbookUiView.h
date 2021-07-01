#ifndef LOGBOOK_UI_VIEW_H
#define LOGBOOK_UI_VIEW_H

#include "../Assets/TextureAssetReference.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

namespace LogbookUiView
{
	const Int2 TitleTextCenterPoint(
		ArenaRenderUtils::SCREEN_WIDTH / 2,
		ArenaRenderUtils::SCREEN_HEIGHT / 2);
	const std::string TitleFontName = ArenaFontName::A;
	const Color TitleTextColor(255, 207, 12);
	constexpr TextAlignment TitleTextAlignment = TextAlignment::MiddleCenter;

	TextBox::InitInfo getTitleTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary);

	const Int2 BackButtonCenterPoint(
		ArenaRenderUtils::SCREEN_WIDTH - 40,
		ArenaRenderUtils::SCREEN_HEIGHT - 13);
	constexpr int BackButtonWidth = 34;
	constexpr int BackButtonHeight = 14;

	TextureAssetReference getBackgroundPaletteTextureAssetRef();
	TextureAssetReference getBackgroundTextureAssetRef();
}

#endif
