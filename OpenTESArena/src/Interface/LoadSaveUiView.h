#ifndef LOAD_SAVE_UI_VIEW_H
#define LOAD_SAVE_UI_VIEW_H

#include "../Assets/TextureAssetReference.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"

namespace LoadSaveUiView
{
	const std::string EntryFontName = ArenaFontName::Arena;
	const Color EntryTextColor = Color::White;
	constexpr TextAlignment EntryTextAlignment = TextAlignment::MiddleCenter;

	Int2 getEntryCenterPoint(int index);

	TextureAssetReference getPaletteTextureAssetRef();
	TextureAssetReference getLoadSaveTextureAssetRef();
}

#endif
