#ifndef COMMON_UI_VIEW_H
#define COMMON_UI_VIEW_H

#include <string>

#include "../Rendering/RenderTextureUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/PivotType.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../Utilities/Color.h"

class FontLibrary;
class Renderer;
class TextureManager;

namespace CommonUiView
{
	constexpr PivotType DefaultCursorPivotType = PivotType::TopLeft;

	UiTextureID allocDefaultCursorTexture(TextureManager &textureManager, Renderer &renderer);

	const std::string DebugInfoFontName = ArenaFontName::Arena;
	Color getDebugInfoTextBoxColor();
	constexpr TextAlignment DebugInfoTextAlignment = TextAlignment::TopLeft;
	Rect getDebugInfoTextBoxRect();

	TextBox::InitInfo getDebugInfoTextBoxInitInfo(const FontLibrary &fontLibrary);
}

#endif
