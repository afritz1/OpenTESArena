#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "SDL.h"

#include "InputActionEvents.h"
#include "InputActionMap.h"
#include "../Math/Vector2.h"

// A simple wrapper class for SDL2 input. 

// This became a necessity after seeing that SDL_GetRelativeMouseState() can only be 
// called once per frame, so its value must be stored somewhere.

class InputManager
{
public:
	using ListenerID = uint32_t;
private:
	struct ListenerEntry
	{
		ListenerID id;
		std::string actionName;
		InputActionCallback callback;

		void init(ListenerID id, const std::string_view &actionName, const InputActionCallback &callback);
	};

	std::vector<InputActionMap> actionMaps;
	std::vector<ListenerEntry> listeners;
	Int2 mouseDelta;
	ListenerID nextID;

	std::optional<int> getListenerEntryIndex(ListenerID id, const std::string_view &actionName) const;
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

	void addListener(ListenerID id, const std::string_view &actionName, const InputActionCallback &callback);
	void removeListener(ListenerID id, const std::string_view &actionName);

	// Sets whether the mouse should move during motion events (for player camera).
	void setRelativeMouseMode(bool active);

	// Updates input values whose associated SDL functions should only be called once 
	// per frame.
	void update();
};

#endif
