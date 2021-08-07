#ifndef INPUT_ACTION_MAP_NAME_H
#define INPUT_ACTION_MAP_NAME_H

// Names of input action maps that can be enabled/disabled throughout the game based on UI context.

namespace InputActionMapName
{
	constexpr const char *Common = "Common"; // Accept/cancel, etc.. Globally available to all UI.

	constexpr const char *Automap = "Automap";
	constexpr const char *CharacterCreation = "CharacterCreation"; // Save/reroll attributes.
	constexpr const char *Cinematic = "Cinematic"; // Skip, etc..
	constexpr const char *GameWorld = "GameWorld";
	constexpr const char *Logbook = "Logbook";
	constexpr const char *MainMenu = "MainMenu"; // Load, new game, exit, test.

	constexpr const char *Names[] =
	{
		Common,

		Automap,
		CharacterCreation,
		Cinematic,
		GameWorld,
		Logbook,
		MainMenu
	};
}

#endif
