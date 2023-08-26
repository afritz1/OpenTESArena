#ifndef PLAYER_LOGIC_CONTROLLER_H
#define PLAYER_LOGIC_CONTROLLER_H

#include "../Math/Vector2.h"

#include "components/utilities/BufferView.h"

class Game;
class Rect;
class TextBox;

namespace PlayerLogicController
{
	// Determines how much to turn the player by, given user input and delta time.
	// @todo: make these be Radians instead of "units".
	Double2 makeTurningAngularValues(Game &game, double dt, BufferView<const Rect> nativeCursorRegions);

	// Turns the player by some angle values (note: the units are not yet formalized to be degrees/radians).
	// @todo: this should take like delta angles or something, not sure.
	void turnPlayer(Game &game, double dx, double dy);

	// Handles input for player movement in the game world.
	void handlePlayerMovement(Game &game, double dt, BufferView<const Rect> nativeCursorRegions);

	// Handles input for the player's attack. Takes the change in mouse position since the previous frame.
	void handlePlayerAttack(Game &game, const Int2 &mouseDelta);

	// Handles the behavior of the player clicking in the game world. "primaryInteraction" is true for left clicks,
	// false for right clicks.
	void handleScreenToWorldInteraction(Game &game, const Int2 &nativePoint, bool primaryInteraction,
		bool debugFadeVoxel, TextBox &actionTextBox);
}

#endif
