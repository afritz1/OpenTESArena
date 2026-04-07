#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../Assets/TextureAsset.h"
#include "../Math/Vector2.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../Utilities/Color.h"

class Game;
class Renderer;
class TextureManager;

struct Rect;

struct LoadSaveUiEntry
{
	std::string text;

	void init(const std::string &text);
};

namespace LoadSaveUiModel
{
	constexpr int SlotCount = 10; // @todo: remove this limit

	const std::string ArenaSaveNamesFilename = "NAMES.DAT";

	std::string getSavesPath(Game &game);
	std::vector<LoadSaveUiEntry> getSaveEntries(Game &game);

	// Gets the classic space UI rect of a save slot.
	Rect getSlotRect(int index);

	// Returns the index of a save's clicked area, if any.
	std::optional<int> getClickedIndex(const Int2 &originalPoint);
}

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
