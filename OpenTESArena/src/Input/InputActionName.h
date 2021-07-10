#ifndef INPUT_ACTION_NAME_H
#define INPUT_ACTION_NAME_H

// Pre-defined names for hotkeys that can be performed without clicking a UI button.

namespace InputActionName
{
	// Common.
	constexpr const char *Accept = "Accept";
	constexpr const char *Cancel = "Cancel";
	constexpr const char *Skip = "Skip"; // Might be left click, right click, escape, space, enter, keypad enter, etc..

	// Game world.
	constexpr const char *MoveForward = "MoveForward";
	constexpr const char *MoveBackward = "MoveBackward";
	constexpr const char *TurnLeft = "TurnLeft";
	constexpr const char *TurnRight = "TurnRight";
	constexpr const char *StrafeLeft = "StrafeLeft";
	constexpr const char *StrafeRight = "StrafeRight";
	constexpr const char *Jump = "Jump";
	constexpr const char *Activate = "Activate";

	// Game world interface.
	constexpr const char *Automap = "Automap";
	constexpr const char *Camp = "Camp";
	constexpr const char *CastMagic = "CastMagic";
	constexpr const char *CharacterSheet = "CharacterSheet";
	constexpr const char *Logbook = "Logbook";
	constexpr const char *PauseMenu = "PauseMenu";
	constexpr const char *PlayerPosition = "PlayerPosition";
	constexpr const char *Status = "Status";
	constexpr const char *Steal = "Steal";
	constexpr const char *ToggleWeapon = "ToggleWeapon";
	constexpr const char *UseItem = "UseItem";
	constexpr const char *WorldMap = "WorldMap";

	// Main menu.
	constexpr const char *StartNewGame = "StartNewGame";
	constexpr const char *LoadGame = "LoadGame";
	constexpr const char *ExitGame = "ExitGame";

	// Character creation.
	constexpr const char *SaveAttributes = "SaveAttributes";
	constexpr const char *RerollAttributes = "RerollAttributes";

	// Debug.
	constexpr const char *DebugProfiler = "DebugProfiler";
}

#endif
