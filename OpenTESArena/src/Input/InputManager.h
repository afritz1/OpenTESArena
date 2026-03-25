#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "SDL_events.h"

#include "ApplicationEvents.h"
#include "InputActionEvents.h"
#include "InputActionMap.h"
#include "PointerEvents.h"
#include "TextEvents.h"
#include "../Math/Vector2.h"

#include "components/utilities/Span.h"

struct UiManager;

using InputListenerID = int;

enum class InputListenerType
{
	InputAction,
	MouseButtonChanged,
	MouseButtonHeld,
	MouseScrollChanged,
	MouseMotion,
	ApplicationExit,
	WindowResized,
	RenderTargetsReset,
	TextInput
};

struct InputListenerLookupEntry
{
	InputListenerType type; // The array the index points into.
	int index;

	void init(InputListenerType type, int index);
};

struct InputActionListenerEntry
{
	std::string actionName;
	InputActionCallback callback;
	std::string contextName;
	bool enabled;

	void init(const std::string &actionName, const InputActionCallback &callback, const std::string &contextName);
	void reset();
};

// Leave these as structs in the event that callback priorities become a thing.
struct MouseButtonChangedListenerEntry
{
	MouseButtonChangedCallback callback;
	std::string contextName;
	bool enabled;

	void init(const MouseButtonChangedCallback &callback, const std::string &contextName);
	void reset();
};

struct MouseButtonHeldListenerEntry
{
	MouseButtonHeldCallback callback;
	std::string contextName;
	bool enabled;

	void init(const MouseButtonHeldCallback &callback, const std::string &contextName);
	void reset();
};

struct MouseScrollChangedListenerEntry
{
	MouseScrollChangedCallback callback;
	std::string contextName;
	bool enabled;

	void init(const MouseScrollChangedCallback &callback, const std::string &contextName);
	void reset();
};

struct MouseMotionListenerEntry
{
	MouseMotionCallback callback;
	std::string contextName;
	bool enabled;

	void init(const MouseMotionCallback &callback, const std::string &contextName);
	void reset();
};

struct ApplicationExitListenerEntry
{
	ApplicationExitCallback callback;
	std::string contextName;
	bool enabled;

	void init(const ApplicationExitCallback &callback, const std::string &contextName);
	void reset();
};

struct WindowResizedListenerEntry
{
	WindowResizedCallback callback;
	std::string contextName;
	bool enabled;

	void init(const WindowResizedCallback &callback, const std::string &contextName);
	void reset();
};

struct RenderTargetsResetListenerEntry
{
	RenderTargetsResetCallback callback;
	std::string contextName;
	bool enabled;

	void init(const RenderTargetsResetCallback &callback, const std::string &contextName);
	void reset();
};

struct TextInputListenerEntry
{
	TextInputCallback callback;
	std::string contextName;
	bool enabled;

	void init(const TextInputCallback &callback, const std::string &contextName);
	void reset();
};

// Handles active input action maps, input listeners, and pointer input events.
class InputManager
{
private:
	std::vector<InputActionMap> inputActionMaps;

	// Listener entry containers.
	std::vector<InputActionListenerEntry> inputActionListeners;
	std::vector<MouseButtonChangedListenerEntry> mouseButtonChangedListeners;
	std::vector<MouseButtonHeldListenerEntry> mouseButtonHeldListeners;
	std::vector<MouseScrollChangedListenerEntry> mouseScrollChangedListeners;
	std::vector<MouseMotionListenerEntry> mouseMotionListeners;
	std::vector<ApplicationExitListenerEntry> applicationExitListeners;
	std::vector<WindowResizedListenerEntry> windowResizedListeners;
	std::vector<RenderTargetsResetListenerEntry> renderTargetsResetListeners;
	std::vector<TextInputListenerEntry> textInputListeners;

	// Look-up values for valid listener entries, shared by all listener containers.
	std::unordered_map<InputListenerID, InputListenerLookupEntry> listenerLookupEntries;

	// Indices to listener entries that were used but can be reclaimed by a future registration.
	std::vector<int> freedInputActionListenerIndices;
	std::vector<int> freedMouseButtonChangedListenerIndices;
	std::vector<int> freedMouseButtonHeldListenerIndices;
	std::vector<int> freedMouseScrollChangedListenerIndices;
	std::vector<int> freedMouseMotionListenerIndices;
	std::vector<int> freedApplicationExitListenerIndices;
	std::vector<int> freedWindowResizedListenerIndices;
	std::vector<int> freedRenderTargetsResetListenerIndices;
	std::vector<int> freedTextInputListenerIndices;

	InputListenerID nextListenerID;
	std::vector<InputListenerID> freedListenerIDs;

	double logicalToPixelScale; // For correcting SDL_Window logical point to physical pixel.
	Int2 mouseDelta;
	
	// Frame-rate independent weapon swings.
	Int2 previousCombatMousePosition;
	double secondsSincePreviousCombatMousePosition;

	InputListenerID getNextListenerID();

	bool isInTextEntryMode() const;

	template<typename EntryType, typename CallbackType>
	InputListenerID addListenerInternal(CallbackType &&callback, InputListenerType listenerType, const std::string &contextName,
		std::vector<EntryType> &listeners, std::vector<int> &freedListenerIndices);
	
	void handleHeldInputs(Game &game, Span<const InputActionMap*> activeMaps,
		Span<const InputActionListenerEntry*> enabledInputActionListeners,
		Span<const MouseButtonHeldListenerEntry*> enabledMouseButtonHeldListeners,
		uint32_t mouseState, const Int2 &mousePosition, double dt);
public:
	InputManager();

	void init(double logicalToPixelScale);

	bool isKeyEvent(const SDL_Event &e) const;
	bool keyPressed(const SDL_Event &e, SDL_Keycode keycode) const;
	bool keyReleased(const SDL_Event &e, SDL_Keycode keycode) const;
	bool keyIsDown(SDL_Scancode scancode) const;
	bool keyIsUp(SDL_Scancode scancode) const;
	bool isMouseButtonEvent(const SDL_Event &e) const;
	bool isMouseWheelEvent(const SDL_Event &e) const;
	bool isMouseMotionEvent(const SDL_Event &e) const;
	bool mouseButtonPressed(const SDL_Event &e, uint8_t button) const;
	bool mouseButtonReleased(const SDL_Event &e, uint8_t button) const;
	bool mouseButtonIsDown(uint8_t button) const;
	bool mouseButtonIsUp(uint8_t button) const;
	bool mouseWheeledUp(const SDL_Event &e) const;
	bool mouseWheeledDown(const SDL_Event &e) const;
	bool applicationExit(const SDL_Event &e) const;
	bool windowResized(const SDL_Event &e) const;
	bool renderTargetsReset(const SDL_Event &e) const;
	bool renderDeviceReset(const SDL_Event &e) const;
	bool isTextInput(const SDL_Event &e) const;
	Int2 getMousePosition() const;
	Int2 getMouseDelta() const;
	Int2 getPreviousCombatMousePosition() const;

	bool setInputActionMapActive(const std::string &name, bool active);

	InputListenerID addInputActionListener(const std::string &actionName, const InputActionCallback &callback, const std::string &contextName);
	InputListenerID addMouseButtonChangedListener(const MouseButtonChangedCallback &callback, const std::string &contextName);
	InputListenerID addMouseButtonHeldListener(const MouseButtonHeldCallback &callback, const std::string &contextName);
	InputListenerID addMouseScrollChangedListener(const MouseScrollChangedCallback &callback, const std::string &contextName);
	InputListenerID addMouseMotionListener(const MouseMotionCallback &callback, const std::string &contextName);
	InputListenerID addApplicationExitListener(const ApplicationExitCallback &callback, const std::string &contextName);
	InputListenerID addWindowResizedListener(const WindowResizedCallback &callback, const std::string &contextName);
	InputListenerID addRenderTargetsResetListener(const RenderTargetsResetCallback &callback, const std::string &contextName);
	InputListenerID addTextInputListener(const TextInputCallback &callback, const std::string &contextName);

	void removeListener(InputListenerID id);

	// Sets whether a valid listener can hear input callbacks.
	void setListenerEnabled(InputListenerID id, bool enabled);

	// Sets whether the mouse should move during motion events (for player camera).
	void setRelativeMouseMode(bool active);

	// Sets whether keyboard input is interpreted as text input or hotkeys.
	void setTextInputMode(bool active);

	// Handle input listener callbacks, etc..
	void update(Game &game, double dt, const UiManager &uiManager, const std::function<void()> &onFinishedProcessingEvent);
};

#endif
