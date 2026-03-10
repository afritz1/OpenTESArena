#ifndef LOAD_SAVE_UI_MODEL_H
#define LOAD_SAVE_UI_MODEL_H

#include <optional>
#include <string>
#include <vector>

#include "../Math/Vector2.h"

class Game;

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

#endif
