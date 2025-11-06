#ifndef UI_CONTEXT_H
#define UI_CONTEXT_H

#include "UiElement.h"

#include <vector>

class Renderer;
class UiManager;

// @todo Pop-up contexts like MessageBox will likely support an array of MessageBoxUiState for extra layers of pop-ups

enum class UiContextType
{
	Global, // Always active.

	Automap,
	CharacterCreation,
	CharacterSheet,
	Cinematic,
	GameWorld,
	Image,
	ImageSequence,
	LoadSave,
	Logbook,
	Loot,
	MainMenu,
	MainQuestSplash,
	MessageBox,
	Options,
	PauseMenu,
	ProvinceMap,
	TextCinematic,
	WorldMap
};

static constexpr int UI_CONTEXT_COUNT = static_cast<int>(UiContextType::WorldMap) + 1;

// Owns UI element handles for a UI context. Copies of these handles can be kept by UI for game logic, activating/deactivating elements, etc..
struct UiContextElements
{
	std::vector<UiElementInstanceID> imageElementInstIDs;
	std::vector<UiElementInstanceID> buttonElementInstIDs;
	std::vector<UiElementInstanceID> textBoxElementInstIDs;

	void free(UiManager &uiManager, Renderer &renderer);
};

#endif
