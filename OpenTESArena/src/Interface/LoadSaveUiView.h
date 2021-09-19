#ifndef LOAD_SAVE_UI_VIEW_H
#define LOAD_SAVE_UI_VIEW_H

#include "../Assets/TextureAssetReference.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"

class Renderer;
class TextureManager;

namespace LoadSaveUiView
{
	const std::string EntryFontName = ArenaFontName::Arena;
	constexpr TextAlignment EntryTextAlignment = TextAlignment::MiddleCenter;
	Color getEntryTextColor();

	Int2 getEntryCenterPoint(int index);

	TextureAssetReference getPaletteTextureAssetRef();
	TextureAssetReference getLoadSaveTextureAssetRef();

	UiTextureID allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer);
}

#endif
