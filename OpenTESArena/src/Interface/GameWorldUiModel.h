#ifndef GAME_WORLD_UI_MODEL_H
#define GAME_WORLD_UI_MODEL_H

#include <string>

#include "../Math/MathUtils.h"
#include "../World/Coord.h"

#include "components/utilities/BufferView.h"

class Game;
class Rect;

namespace GameWorldUiModel
{
	std::string getPlayerNameText(Game &game);
	std::string getStatusButtonText(Game &game);
	std::string getPlayerPositionText(Game &game);

	// Tooltips.
	std::string getCharacterSheetTooltipText();
	std::string getWeaponTooltipText();
	std::string getMapTooltipText();
	std::string getStealTooltipText();
	std::string getStatusTooltipText();
	std::string getMagicTooltipText();
	std::string getLogbookTooltipText();
	std::string getUseItemTooltipText();
	std::string getCampTooltipText();

	void setFreeLookActive(Game &game, bool active);

	// Converts a screen point to a 3D direction in the world.
	VoxelDouble3 screenToWorldRayDirection(Game &game, const Int2 &windowPoint);

	Radians getCompassAngle(const VoxelDouble2 &direction);

	// Modifies the values in the native cursor regions array so rectangles in
	// the current window correctly represent regions for different arrow cursors.
	void updateNativeCursorRegions(BufferView<Rect> &&nativeCursorRegions, int width, int height);
}

#endif
