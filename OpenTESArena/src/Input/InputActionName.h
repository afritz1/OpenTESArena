#pragma once

// Pre-defined names for hotkeys that can be performed without clicking a UI button.
namespace InputActionName
{
	// Common.
	constexpr const char *Accept = "Accept";
	constexpr const char *Back = "Back"; // A.k.a. cancel.
	constexpr const char *Skip = "Skip"; // Might be left click, right click, escape, space, enter, keypad enter, etc..
	constexpr const char *Screenshot = "Screenshot";
	constexpr const char *Backspace = "Backspace";

	// Game world.
	constexpr const char *MoveForward = "MoveForward";
	constexpr const char *MoveBackward = "MoveBackward";
	constexpr const char *TurnLeft = "TurnLeft";
	constexpr const char *TurnRight = "TurnRight";
	constexpr const char *StrafeLeft = "StrafeLeft";
	constexpr const char *StrafeRight = "StrafeRight";
	constexpr const char *Jump = "Jump";
	constexpr const char *Activate = "Activate";
	constexpr const char *Inspect = "Inspect";

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
	constexpr const char *ToggleCompass = "ToggleCompass";
	constexpr const char *ToggleWeapon = "ToggleWeapon";
	constexpr const char *UseItem = "UseItem";
	constexpr const char *WorldMap = "WorldMap";

	// Main menu.
	constexpr const char *NewGame = "NewGame";
	constexpr const char *LoadGame = "LoadGame";
	constexpr const char *ExitGame = "ExitGame";
	constexpr const char *TestGame = "TestGame";

	// Character creation.
	constexpr const char *SaveAttributes = "SaveAttributes";
	constexpr const char *RerollAttributes = "RerollAttributes";

	// Camping.
	constexpr const char *CampManualHours = "CampManualHours";
	constexpr const char *CampUntilHealed = "CampUntilHealed";

	// NPC conversation.
	constexpr const char *NpcWhoAreYou = "NpcWhoAreYou";
	constexpr const char *NpcWhereIs = "NpcWhereIs";
	constexpr const char *NpcRumors = "NpcRumors";
	constexpr const char *NpcExit = "NpcExit";
	constexpr const char *NpcRumorsGeneral = "NpcRumorsGeneral";
	constexpr const char *NpcRumorsWork = "NpcRumorsWork";

	constexpr const char *EquipmentStoreBuy = "EquipmentStoreBuy";
	constexpr const char *EquipmentStoreSell = "EquipmentStoreSell";
	constexpr const char *EquipmentStoreRepair = "EquipmentStoreRepair";
	constexpr const char *EquipmentStoreSteal = "EquipmentStoreSteal";
	constexpr const char *EquipmentStoreExit = "EquipmentStoreExit";
	constexpr const char *EquipmentStoreBuyWeapon = "EquipmentStoreBuyWeapon";
	constexpr const char *EquipmentStoreBuyArmor = "EquipmentStoreBuyArmor";

	constexpr const char *MagesGuildBuy = "MagesGuildBuy";
	constexpr const char *MagesGuildDetectMagic = "MagesGuildDetectMagic";
	constexpr const char *MagesGuildSpellmaker = "MagesGuildSpellmaker";
	constexpr const char *MagesGuildSteal = "MagesGuildSteal";
	constexpr const char *MagesGuildExit = "MagesGuildExit";
	constexpr const char *MagesGuildBuyPotions = "MagesGuildBuyPotions";
	constexpr const char *MagesGuildBuyMagicItems = "MagesGuildBuyMagicItems";
	constexpr const char *MagesGuildBuySpells = "MagesGuildBuySpells";
	constexpr const char *MagesGuildStealPotions = "MagesGuildStealPotions";
	constexpr const char *MagesGuildStealMagicItems = "MagesGuildStealMagicItems";

	constexpr const char *TavernBuyDrinks = "TavernBuyDrinks";
	constexpr const char *TavernGetRoom = "TavernGetRoom";
	constexpr const char *TavernSneakIntoRoom = "TavernSneakIntoRoom";
	constexpr const char *TavernRumors = "TavernRumors";
	constexpr const char *TavernExit = "TavernExit";
	constexpr const char *TavernRumorsGeneral = "TavernRumorsGeneral";
	constexpr const char *TavernRumorsWork = "TavernRumorsWork";
	
	constexpr const char *TempleBless = "TempleBless";
	constexpr const char *TempleCure = "TempleCure";
	constexpr const char *TempleHeal = "TempleHeal";
	constexpr const char *TempleExit = "TempleExit";

	// Debug.
	constexpr const char *DebugProfiler = "DebugProfiler";
}
