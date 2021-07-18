#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "ApplicationEvents.h"
#include "InputActionEvents.h"
#include "InputActionMap.h"
#include "PointerEvents.h"
#include "../Math/Vector2.h"

// Handles active input action maps, input listeners, and pointer input events.

union SDL_Event;

class InputManager
{
public:
	using ListenerID = uint32_t;
private:
	struct InputActionListenerEntry
	{
		ListenerID id;
		std::string actionName;
		InputActionCallback callback;

		void init(ListenerID id, const std::string_view &actionName, const InputActionCallback &callback);
	};

	struct MouseButtonChangedListenerEntry
	{
		ListenerID id;
		MouseButtonChangedCallback callback;

		void init(ListenerID id, const MouseButtonChangedCallback &callback);
	};

	struct MouseButtonHeldListenerEntry
	{
		ListenerID id;
		MouseButtonHeldCallback callback;

		void init(ListenerID id, const MouseButtonHeldCallback &callback);
	};

	struct MouseScrollChangedListenerEntry
	{
		ListenerID id;
		MouseScrollChangedCallback callback;

		void init(ListenerID id, const MouseScrollChangedCallback &callback);
	};

	struct MouseMotionListenerEntry
	{
		ListenerID id;
		MouseMotionCallback callback;

		void init(ListenerID id, const MouseMotionCallback &callback);
	};

	struct ApplicationExitListenerEntry
	{
		ListenerID id;
		ApplicationExitCallback callback;

		void init(ListenerID id, const ApplicationExitCallback &callback);
	};

	struct WindowResizedListenerEntry
	{
		ListenerID id;
		WindowResizedCallback callback;

		void init(ListenerID id, const WindowResizedCallback &callback);
	};

	std::vector<InputActionMap> inputActionMaps;
	std::vector<InputActionListenerEntry> inputActionListeners;
	std::vector<MouseButtonChangedListenerEntry> mouseButtonChangedListeners;
	std::vector<MouseButtonHeldListenerEntry> mouseButtonHeldListeners;
	std::vector<MouseScrollChangedListenerEntry> mouseScrollChangedListeners;
	std::vector<MouseMotionListenerEntry> mouseMotionListeners;
	std::vector<ApplicationExitListenerEntry> applicationExitListeners;
	std::vector<WindowResizedListenerEntry> windowResizedListeners;
	Int2 mouseDelta;
	ListenerID nextID;

	std::optional<int> getInputActionListenerEntryIndex(ListenerID id, const std::string_view &actionName) const;

	template <typename EntryType>
	static std::optional<int> getListenerEntryIndex(ListenerID id, const std::vector<EntryType> &listeners);
	template <typename EntryType, typename CallbackType>
	static void addListenerInternal(ListenerID id, CallbackType &&callback, std::vector<EntryType> &listeners);
	template <typename EntryType>
	static void removeListenerInternal(ListenerID id, std::vector<EntryType> &listeners);
public:
	InputManager();

	void init();

	ListenerID nextListenerID();

	bool keyPressed(const SDL_Event &e, SDL_Keycode keycode) const;
	bool keyReleased(const SDL_Event &e, SDL_Keycode keycode) const;
	bool keyIsDown(SDL_Scancode scancode) const;
	bool keyIsUp(SDL_Scancode scancode) const;
	bool mouseButtonPressed(const SDL_Event &e, uint8_t button) const;
	bool mouseButtonReleased(const SDL_Event &e, uint8_t button) const;
	bool mouseButtonIsDown(uint8_t button) const;
	bool mouseButtonIsUp(uint8_t button) const;
	bool mouseWheeledUp(const SDL_Event &e) const;
	bool mouseWheeledDown(const SDL_Event &e) const;
	bool windowResized(const SDL_Event &e) const;
	bool applicationExit(const SDL_Event &e) const;
	Int2 getMousePosition() const;
	Int2 getMouseDelta() const;

	bool setInputActionMapActive(const std::string &name, bool active);

	void addInputActionListener(ListenerID id, const std::string_view &actionName, const InputActionCallback &callback);
	void addMouseButtonChangedListener(ListenerID id, const MouseButtonChangedCallback &callback);
	void addMouseButtonHeldListener(ListenerID id, const MouseButtonHeldCallback &callback);
	void addMouseScrollChangedListener(ListenerID id, const MouseScrollChangedCallback &callback);
	void addMouseMotionListener(ListenerID id, const MouseMotionCallback &callback);
	void addApplicationExitListener(ListenerID id, const ApplicationExitCallback &callback);
	void addWindowResizedListener(ListenerID id, const WindowResizedCallback &callback);

	void removeInputActionListener(ListenerID id, const std::string_view &actionName);
	void removeMouseButtonChangedListener(ListenerID id);
	void removeMouseButtonHeldListener(ListenerID id);
	void removeMouseScrollChangedListener(ListenerID id);
	void removeMouseMotionListener(ListenerID id);
	void removeApplicationExitListener(ListenerID id);
	void removeWindowResizedListener(ListenerID id);

	// Sets whether the mouse should move during motion events (for player camera).
	void setRelativeMouseMode(bool active);

	// Updates input values whose associated SDL functions should only be called once 
	// per frame.
	void update();
};

#endif
