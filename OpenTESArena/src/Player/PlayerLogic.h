#ifndef PLAYER_LOGIC_H
#define PLAYER_LOGIC_H

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

#include "components/utilities/Span.h"

class Game;
class Random;
class TextBox;

struct Rect;

enum class CardinalDirectionName;

struct PlayerInputAcceleration
{
	Double3 direction;
	double magnitude;
	bool isInstantJump;
	bool isGhostMode;
	bool shouldResetVelocity;

	PlayerInputAcceleration();
};

namespace PlayerLogic
{
	// Determines how much to turn the player by, given user input and delta time.
	// @todo: make these be Radians instead of "units".
	Double2 makeTurningAngularValues(Game &game, double dt, const Int2 &mouseDelta, Span<const Rect> nativeCursorRegions);

	// Gets movement values based on player input.
	PlayerInputAcceleration getInputAcceleration(Game &game, Span<const Rect> nativeCursorRegions);

	CardinalDirectionName getRandomMeleeSwingDirection(Random &random);

	// Can fail if mouse moves too slowly.
	bool tryGetMeleeSwingDirectionFromMouseDelta(const Int2 &mouseDelta, const Int2 &windowDims, CardinalDirectionName *outDirectionName);

	// Handles input for the player's attack. Takes the change in mouse position since the previous frame.
	void handleAttack(Game &game, const Int2 &mouseDelta);

	// Handles the behavior of the player clicking in the game world. "primaryInteraction" is true for left clicks,
	// false for right clicks.
	void handleScreenToWorldInteraction(Game &game, const Int2 &nativePoint, bool primaryInteraction,
		bool debugFadeVoxel, TextBox &actionTextBox);
}

#endif
