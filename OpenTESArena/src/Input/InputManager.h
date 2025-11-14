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

// Handles active input action maps, input listeners, and pointer input events.
class InputManager
{
private:
	enum class ListenerType
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

	struct ListenerLookupEntry
	{
		ListenerType type; // The array the index points into.
		int index;

		void init(ListenerType type, int index);
	};

	struct InputActionListenerEntry
	{
		std::string actionName;
		InputActionCallback callback;
		bool enabled;

		void init(const std::string_view actionName, const InputActionCallback &callback);
		void reset();
	};

	// Leave these as structs in the event that callback priorities become a thing.
	struct MouseButtonChangedListenerEntry
	{
		MouseButtonChangedCallback callback;
		bool enabled;

		void init(const MouseButtonChangedCallback &callback);
		void reset();
	};

	struct MouseButtonHeldListenerEntry
	{
		MouseButtonHeldCallback callback;
		bool enabled;

		void init(const MouseButtonHeldCallback &callback);
		void reset();
	};

	struct MouseScrollChangedListenerEntry
	{
		MouseScrollChangedCallback callback;
		bool enabled;

		void init(const MouseScrollChangedCallback &callback);
		void reset();
	};

	struct MouseMotionListenerEntry
	{
		MouseMotionCallback callback;
		bool enabled;

		void init(const MouseMotionCallback &callback);
		void reset();
	};

	struct ApplicationExitListenerEntry
	{
		ApplicationExitCallback callback;
		bool enabled;

		void init(const ApplicationExitCallback &callback);
		void reset();
	};

	struct WindowResizedListenerEntry
	{
		WindowResizedCallback callback;
		bool enabled;

		void init(const WindowResizedCallback &callback);
		void reset();
	};

	struct RenderTargetsResetListenerEntry
	{
		RenderTargetsResetCallback callback;
		bool enabled;

		void init(const RenderTargetsResetCallback &callback);
		void reset();
	};

	struct TextInputListenerEntry
	{
		TextInputCallback callback;
		bool enabled;

		void init(const TextInputCallback &callback);
		void reset();
	};

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
	std::unordered_map<InputListenerID, ListenerLookupEntry> listenerLookupEntries;

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
	InputListenerID addListenerInternal(CallbackType &&callback, ListenerType listenerType, std::vector<EntryType> &listeners,
		std::vector<int> &freedListenerIndices);
	
	void handleHeldInputs(Game &game, Span<const InputActionMap*> activeMaps,
		Span<const InputActionListenerEntry*> enabledInputActionListeners, uint32_t mouseState,
		const Int2 &mousePosition, double dt);
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

	InputListenerID addInputActionListener(const std::string_view actionName, const InputActionCallback &callback);
	InputListenerID addMouseButtonChangedListener(const MouseButtonChangedCallback &callback);
	InputListenerID addMouseButtonHeldListener(const MouseButtonHeldCallback &callback);
	InputListenerID addMouseScrollChangedListener(const MouseScrollChangedCallback &callback);
	InputListenerID addMouseMotionListener(const MouseMotionCallback &callback);
	InputListenerID addApplicationExitListener(const ApplicationExitCallback &callback);
	InputListenerID addWindowResizedListener(const WindowResizedCallback &callback);
	InputListenerID addRenderTargetsResetListener(const RenderTargetsResetCallback &callback);
	InputListenerID addTextInputListener(const TextInputCallback &callback);

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
