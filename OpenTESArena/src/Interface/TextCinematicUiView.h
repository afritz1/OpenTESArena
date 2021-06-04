#ifndef TEXT_CINEMATIC_UI_VIEW_H
#define TEXT_CINEMATIC_UI_VIEW_H

#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/FontName.h"
#include "../UI/TextAlignment.h"

namespace TextCinematicUiView
{
	const Int2 SubtitleTextBoxCenterPoint(
		ArenaRenderUtils::SCREEN_WIDTH / 2,
		ArenaRenderUtils::SCREEN_HEIGHT - 11);
	constexpr FontName SubtitleTextBoxFontName = FontName::Arena;
	constexpr TextAlignment SubtitleTextBoxTextAlignment = TextAlignment::Center;
	constexpr int SubtitleTextBoxLineSpacing = 1;
}

#endif
