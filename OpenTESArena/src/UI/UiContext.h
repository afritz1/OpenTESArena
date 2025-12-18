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

// Owns various handles for a UI context which can be used with game logic for activating/deactivating elements, etc..
struct UiContextState
{
	std::vector<UiElementInstanceID> imageElementInstIDs;
	std::vector<UiElementInstanceID> buttonElementInstIDs;
	std::vector<UiElementInstanceID> textBoxElementInstIDs;

	std::vector<InputListenerID> inputActionListenerIDs;
	std::vector<InputListenerID> mouseButtonChangedListenerIDs;
	std::vector<InputListenerID> mouseButtonHeldListenerIDs;
	std::vector<InputListenerID> mouseScrollChangedListenerIDs;
	std::vector<InputListenerID> mouseMotionListenerIDs;
	std::vector<InputListenerID> applicationExitListenerIDs;
	std::vector<InputListenerID> windowResizedListenerIDs;
	std::vector<InputListenerID> renderTargetsResetListenerIDs;
	std::vector<InputListenerID> textInputListenerIDs;

	void free(InputManager &inputManager, UiManager &uiManager, Renderer &renderer);
};

#endif
