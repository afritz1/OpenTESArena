#ifndef PLAYER_LOGIC_CONTROLLER_H
#define PLAYER_LOGIC_CONTROLLER_H

#include "../Math/Vector2.h"

#include "components/utilities/BufferViewReadOnly.h"

class Game;
class Rect;
class TextBox;

namespace PlayerLogicController
{
	// Handles input for the player camera.
	void handlePlayerTurning(Game &game, double dt, const Int2 &mouseDelta,
		BufferViewReadOnly<Rect> &nativeCursorRegions);

	// Handles input for player movement in the game world.
	void handlePlayerMovement(Game &game, double dt, BufferViewReadOnly<Rect> &nativeCursorRegions);

	// Handles input for the player's attack. Takes the change in mouse position since the previous frame.
	void handlePlayerAttack(Game &game, const Int2 &mouseDelta);

	// Handles the behavior of the player clicking in the game world. "primaryClick" is true for left clicks,
	// false for right clicks.
	void handleClickInWorld(Game &game, const Int2 &nativePoint, bool primaryClick, bool debugFadeVoxel,
		TextBox &actionTextBox);
}

#endif
