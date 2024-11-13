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


	// @todo: can't there just be a MakeNextVelocity() convenience function that 
	// determines where the player is headed next? or a GetAccelerationFromInput()?

	// @todo: change these void functions to ones that return a value (acceleration vector, immediate acceleration)

	// Handles input for player movement in the game world.
	void handlePlayerMovement(Game &game, double dt, BufferView<const Rect> nativeCursorRegions);


	// @todo: change this to be framed like a physics ray cast: it generates a result of what was tested against,
	// then afterwards we decide to play a hit sound or miss sound, etc.

	// Handles input for the player's attack. Takes the change in mouse position since the previous frame.
	void handlePlayerAttack(Game &game, const Int2 &mouseDelta);

	// Handles the behavior of the player clicking in the game world. "primaryInteraction" is true for left clicks,
	// false for right clicks.
	void handleScreenToWorldInteraction(Game &game, const Int2 &nativePoint, bool primaryInteraction,
		bool debugFadeVoxel, TextBox &actionTextBox);
}

#endif
