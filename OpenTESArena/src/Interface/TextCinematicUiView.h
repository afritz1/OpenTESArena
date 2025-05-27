#ifndef TEXT_CINEMATIC_UI_VIEW_H
#define TEXT_CINEMATIC_UI_VIEW_H

#include <string>

#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"

#include "components/utilities/Buffer.h"

class FontLibrary;

namespace TextCinematicUiView
{
	const Int2 SubtitleTextBoxCenterPoint(
		ArenaRenderUtils::SCREEN_WIDTH / 2,
		ArenaRenderUtils::SCREEN_HEIGHT - 16);
	std::string getSubtitleTextBoxFontName();
	constexpr TextAlignment SubtitleTextBoxTextAlignment = TextAlignment::MiddleCenter;
	constexpr int SubtitleTextBoxLineSpacing = 1;

	TextBoxInitInfo getSubtitlesTextBoxInitInfo(const Color &fontColor, const FontLibrary &fontLibrary);

	Buffer<UiTextureID> allocAnimationTextures(const std::string &animFilename,
		TextureManager &textureManager, Renderer &renderer);
}

#endif
