#ifndef LOAD_SAVE_UI_VIEW_H
#define LOAD_SAVE_UI_VIEW_H

#include "../Assets/TextureAssetReference.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../UI/FontName.h"
#include "../UI/TextAlignment.h"

namespace LoadSaveUiView
{
	constexpr FontName EntryFontName = FontName::Arena;
	const Color EntryTextColor = Color::White;
	constexpr TextAlignment EntryTextAlignment = TextAlignment::Center;

	Int2 getEntryCenterPoint(int index);

	TextureAssetReference getPaletteTextureAssetRef();
	TextureAssetReference getLoadSaveTextureAssetRef();
}

#endif
