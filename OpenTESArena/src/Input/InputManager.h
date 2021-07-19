#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "SDL_events.h"

#include "ApplicationEvents.h"
#include "InputActionEvents.h"
#include "InputActionMap.h"
#include "PointerEvents.h"
#include "../Math/Vector2.h"

// Handles active input action maps, input listeners, and pointer input events.

class InputManager
{
public:
	using ListenerID = uint32_t;
private:
	struct InputActionListenerEntry
	{
		std::string actionName;
		InputActionCallback callback;

		void init(const std::string_view &actionName, const InputActionCallback &callback);
	};

	// Leave these as structs in the event that callback priorities become a thing.
	struct MouseButtonChangedListenerEntry
	{
		MouseButtonChangedCallback callback;

		void init(const MouseButtonChangedCallback &callback);
	};

	struct MouseButtonHeldListenerEntry
	{
		MouseButtonHeldCallback callback;

		void init(const MouseButtonHeldCallback &callback);
	};

	struct MouseScrollChangedListenerEntry
	{
		MouseScrollChangedCallback callback;

		void init(const MouseScrollChangedCallback &callback);
	};

	struct MouseMotionListenerEntry
	{
		MouseMotionCallback callback;

		void init(const MouseMotionCallback &callback);
	};

	struct ApplicationExitListenerEntry
	{
		ApplicationExitCallback callback;

		void init(const ApplicationExitCallback &callback);
	};

	struct WindowResizedListenerEntry
	{
		WindowResizedCallback callback;

		void init(const WindowResizedCallback &callback);
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

	// Indices to valid listener entries, shared by all listener containers.
	std::unordered_map<ListenerID, int> listenerIndices;

	// Indices to listener entries that were used but can be reclaimed by a future registration.
	std::vector<int> freedInputActionListenerIndices;
	std::vector<int> freedMouseButtonChangedListenerIndices;
	std::vector<int> freedMouseButtonHeldListenerIndices;
	std::vector<int> freedMouseScrollChangedListenerIndices;
	std::vector<int> freedMouseMotionListenerIndices;
	std::vector<int> freedApplicationExitListenerIndices;
	std::vector<int> freedWindowResizedListenerIndices;

	ListenerID nextListenerID;
	std::vector<ListenerID> freedListenerIDs;

	std::vector<SDL_Event> cachedEvents; // @temp: only for compatibility with old event system until completely moved over.

	Int2 mouseDelta;

	ListenerID getNextListenerID();

	template <typename EntryType, typename CallbackType>
	ListenerID addListenerInternal(CallbackType &&callback, std::vector<EntryType> &listeners,
		std::vector<int> &freedListenerIndices);
	template <typename EntryType>
	void removeListenerInternal(ListenerID id, std::vector<EntryType> &listeners, std::vector<int> &freedListenerIndices);
public:
	InputManager();

	void init();

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

	// @temp until Game::handleEvents() is removed
	int getEventCount() const;
	const SDL_Event &getEvent(int index) const;

	bool setInputActionMapActive(const std::string &name, bool active);

	ListenerID addInputActionListener(const std::string_view &actionName, const InputActionCallback &callback);
	ListenerID addMouseButtonChangedListener(const MouseButtonChangedCallback &callback);
	ListenerID addMouseButtonHeldListener(const MouseButtonHeldCallback &callback);
	ListenerID addMouseScrollChangedListener(const MouseScrollChangedCallback &callback);
	ListenerID addMouseMotionListener(const MouseMotionCallback &callback);
	ListenerID addApplicationExitListener(const ApplicationExitCallback &callback);
	ListenerID addWindowResizedListener(const WindowResizedCallback &callback);

	void removeInputActionListener(ListenerID id);
	void removeMouseButtonChangedListener(ListenerID id);
	void removeMouseButtonHeldListener(ListenerID id);
	void removeMouseScrollChangedListener(ListenerID id);
	void removeMouseMotionListener(ListenerID id);
	void removeApplicationExitListener(ListenerID id);
	void removeWindowResizedListener(ListenerID id);

	// Sets whether the mouse should move during motion events (for player camera).
	void setRelativeMouseMode(bool active);

	// Handle input listener callbacks, etc..
	void update();
};

#endif
