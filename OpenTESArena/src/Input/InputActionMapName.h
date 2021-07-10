#ifndef INPUT_ACTION_MAP_NAME_H
#define INPUT_ACTION_MAP_NAME_H

// Names of input action maps that can be enabled/disabled throughout the game based on UI context.

namespace InputActionMapName
{
	constexpr const char *Common = "Common"; // Accept/cancel, etc..
	constexpr const char *Cinematic = "Cinematic"; // Skip, etc..
	constexpr const char *CharacterCreation = "CharacterCreation"; // Save/reroll attributes.
	constexpr const char *GameWorld = "GameWorld";
	constexpr const char *MainMenu = "MainMenu"; // Load, new game, exit, test.
}

#endif
