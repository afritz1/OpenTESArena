#ifndef TEXT_CINEMATIC_UI_VIEW_H
#define TEXT_CINEMATIC_UI_VIEW_H

#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"

namespace TextCinematicUiView
{
	const Int2 SubtitleTextBoxCenterPoint(
		ArenaRenderUtils::SCREEN_WIDTH / 2,
		ArenaRenderUtils::SCREEN_HEIGHT - 11);
	const std::string &SubtitleTextBoxFontName = ArenaFontName::Arena;
	constexpr TextAlignment SubtitleTextBoxTextAlignment = TextAlignment::MiddleCenter;
	constexpr int SubtitleTextBoxLineSpacing = 1;
}

#endif
