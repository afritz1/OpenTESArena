#ifndef UI_CONTEXT_H
#define UI_CONTEXT_H

#include <vector>

#include "UiElement.h"
#include "../Input/InputManager.h"

class Game;
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

#define DECLARE_UI_CONTEXT(name) \
static constexpr UiContextType ContextType = UiContextType::##name; \
static constexpr const char NamespaceString[] = #name "UI"; \
static name##UiState state; \
void create(Game &game); \
void destroy(); \
void update(double dt)

// For buttons and input actions.
#define DECLARE_UI_FUNC(contextName, functionName) { #functionName, contextName##::functionName }

// Owns UI element handles for a UI context. Copies of these handles can be kept by UI for game logic, activating/deactivating elements, etc..
struct UiContextElements
{
	std::vector<UiElementInstanceID> imageElementInstIDs;
	std::vector<UiElementInstanceID> buttonElementInstIDs;
	std::vector<UiElementInstanceID> textBoxElementInstIDs;

	void free(UiManager &uiManager, Renderer &renderer);
};

struct UiContextInputListeners
{
	std::vector<InputListenerID> inputActionListenerIDs;

	void free(InputManager &inputManager);
};

#endif
