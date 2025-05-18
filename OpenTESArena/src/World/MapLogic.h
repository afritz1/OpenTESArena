#ifndef MAP_LOGIC_H
#define MAP_LOGIC_H

#include "../World/Coord.h"

class Game;
class TextBox;
class VoxelChunk;

struct RayCastHit;
struct TransitionDefinition;

namespace MapLogic
{
	// Handles changing night-light-related things on and off.
	void handleNightLightChange(Game &game, bool active);

	// Sends an "on voxel enter" message for the given voxel and triggers any lore text, riddles, or sound events.
	void handleTriggersInVoxel(Game &game, const CoordInt3 &coord, TextBox &triggerTextBox);

	// Handles the door open animation and sound.
	void handleDoorOpen(Game &game, VoxelChunk &voxelChunk, const VoxelInt3 &voxel, double ceilingScale, bool isApplyingDoorKeyToLock, int doorKeyID, bool isWeaponBashing);

	// Teleport the player to a random city in their race's province.
	void handleStartDungeonLevelUpVoxelEnter(Game &game);

	// Handles the behavior for when the player activates a map transition block and transitions from one map
	// to another (i.e., from an interior to an exterior). This does not handle level transitions.
	void handleMapTransition(Game &game, const RayCastHit &hit, const TransitionDefinition &transitionDef);

	// Checks the given transition voxel to see if it's a level transition (i.e., level up/down), and changes
	// the current level if it is.
	void handleLevelTransition(Game &game, const CoordInt3 &playerCoord, const CoordInt3 &transitionCoord);
}

#endif
