#pragma once

// Names of input action maps that can be enabled/disabled throughout the game based on UI context.
namespace InputActionMapName
{
	constexpr const char *Common = "Common"; // Accept/cancel, etc.. Globally available to all UI.

	constexpr const char *Automap = "Automap";
	constexpr const char *Camping = "Camping";
	constexpr const char *CharacterCreation = "CharacterCreation"; // Save/reroll attributes.
	constexpr const char *CharacterEquipment = "CharacterEquipment";
	constexpr const char *CharacterSheet = "CharacterSheet";
	constexpr const char *EquipmentStore = "EquipmentStore";
	constexpr const char *EquipmentStoreBuy = "EquipmentStoreBuy";
	constexpr const char *GameWorld = "GameWorld";
	constexpr const char *Logbook = "Logbook";
	constexpr const char *MagesGuild = "MagesGuild";
	constexpr const char *MagesGuildBuy = "MagesGuildBuy";
	constexpr const char *MagesGuildSteal = "MagesGuildSteal";
	constexpr const char *MainMenu = "MainMenu"; // Load, new game, exit, test.
	constexpr const char *NpcGeneral = "NpcGeneral";
	constexpr const char *NpcRumors = "NpcRumors";
	constexpr const char *Tavern = "Tavern";
	constexpr const char *TavernRumors = "TavernRumors";
	constexpr const char *Temple = "Temple";
	constexpr const char *WorldMap = "WorldMap";

	constexpr const char *Names[] =
	{
		Common,

		Automap,
		Camping,
		CharacterCreation,
		CharacterEquipment,
		CharacterSheet,
		EquipmentStore,
		EquipmentStoreBuy,
		GameWorld,
		Logbook,
		MagesGuild,
		MagesGuildBuy,
		MagesGuildSteal,
		MainMenu,		
		NpcGeneral,
		NpcRumors,
		Tavern,
		TavernRumors,
		Temple,
		WorldMap
	};
}
