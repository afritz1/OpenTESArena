#ifndef LOAD_SAVE_UI_VIEW_H
#define LOAD_SAVE_UI_VIEW_H

#include "../Assets/TextureAsset.h"
#include "../Math/Vector2.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../Utilities/Color.h"

class Renderer;
class TextureManager;

namespace LoadSaveUiView
{
	const std::string EntryFontName = ArenaFontName::Arena;
	constexpr TextAlignment EntryTextAlignment = TextAlignment::MiddleCenter;
	Color getEntryTextColor();

	Int2 getEntryCenterPoint(int index);

	TextureAsset getPaletteTextureAsset();
	TextureAsset getLoadSaveTextureAsset();

	UiTextureID allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer);
}

#endif
