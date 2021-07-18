#include <algorithm>

#include "InputManager.h"

#include "components/debug/Debug.h"

void InputManager::ListenerEntry::init(ListenerID id, const std::string_view &actionName,
	const InputActionCallback &callback)
{
	this->id = id;
	this->actionName = std::string(actionName);
	this->callback = callback;
}

InputManager::InputManager()
	: mouseDelta(0, 0)
{
	this->nextID = 0;
}

void InputManager::init()
{
	// Add input action maps to be enabled/disabled as needed.
	this->actionMaps = InputActionMap::loadDefaultMaps();
}

InputManager::ListenerID InputManager::nextListenerID()
{
	const InputManager::ListenerID id = this->nextID;
	this->nextID++;
	return id;
}

std::optional<int> InputManager::getListenerEntryIndex(ListenerID id, const std::string_view &actionName) const
{
	const auto iter = std::find_if(this->listeners.begin(), this->listeners.end(),
		[id, &actionName](const ListenerEntry &entry)
	{
		return (entry.id == id) && (entry.actionName == actionName);
	});

	if (iter != this->listeners.end())
	{
		return static_cast<int>(std::distance(this->listeners.begin(), iter));
	}
	else
	{
		return std::nullopt;
	}
}

bool InputManager::keyPressed(const SDL_Event &e, SDL_Keycode keycode) const
{
	return (e.type == SDL_KEYDOWN) && (e.key.keysym.sym == keycode) && (e.key.repeat == 0);
}

bool InputManager::keyReleased(const SDL_Event &e, SDL_Keycode keycode) const
{
	return (e.type == SDL_KEYUP) && (e.key.keysym.sym == keycode);
}

bool InputManager::keyIsDown(SDL_Scancode scancode) const
{
	const uint8_t *keys = SDL_GetKeyboardState(nullptr);
	return keys[scancode] != 0;
}

bool InputManager::keyIsUp(SDL_Scancode scancode) const
{
	const uint8_t *keys = SDL_GetKeyboardState(nullptr);
	return keys[scancode] == 0;
}

bool InputManager::mouseButtonPressed(const SDL_Event &e, uint8_t button) const
{
	return (e.type == SDL_MOUSEBUTTONDOWN) && (e.button.button == button);
}

bool InputManager::mouseButtonReleased(const SDL_Event &e, uint8_t button) const
{
	return (e.type == SDL_MOUSEBUTTONUP) && (e.button.button == button);
}

bool InputManager::mouseButtonIsDown(uint8_t button) const
{
	const uint32_t mouse = SDL_GetMouseState(nullptr, nullptr);
	return (mouse & SDL_BUTTON(button)) != 0;
}

bool InputManager::mouseButtonIsUp(uint8_t button) const
{
	const uint32_t mouse = SDL_GetMouseState(nullptr, nullptr);
	return (mouse & SDL_BUTTON(button)) == 0;
}

bool InputManager::mouseWheeledUp(const SDL_Event &e) const
{
	return (e.type == SDL_MOUSEWHEEL) && (e.wheel.y > 0);
}

bool InputManager::mouseWheeledDown(const SDL_Event &e) const
{
	return (e.type == SDL_MOUSEWHEEL) && (e.wheel.y < 0);
}

bool InputManager::windowResized(const SDL_Event &e) const
{
	return (e.type == SDL_WINDOWEVENT) && (e.window.event == SDL_WINDOWEVENT_RESIZED);
}

bool InputManager::applicationExit(const SDL_Event &e) const
{
	return e.type == SDL_QUIT;
}

Int2 InputManager::getMousePosition() const
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return Int2(x, y);
}

Int2 InputManager::getMouseDelta() const
{
	return this->mouseDelta;
}

bool InputManager::setInputActionMapActive(const std::string &name, bool active)
{
	const auto iter = std::find_if(this->actionMaps.begin(), this->actionMaps.end(),
		[&name](const InputActionMap &map)
	{
		return map.name == name;
	});

	if (iter != this->actionMaps.end())
	{
		InputActionMap &map = *iter;
		map.active = active;
		return true;
	}
	else
	{
		DebugLogWarning("Couldn't find input action map \"" + name + "\".");
		return false;
	}
}

void InputManager::addListener(ListenerID id, const std::string_view &actionName, const InputActionCallback &callback)
{
	const std::optional<int> existingIndex = this->getListenerEntryIndex(id, actionName);
	if (existingIndex.has_value())
	{
		DebugLogError("Already registered \"" + std::string(actionName) + "\" for listener " + std::to_string(id) + ".");
		return;
	}

	ListenerEntry entry;
	entry.init(id, actionName, callback);
	this->listeners.emplace_back(std::move(entry));
}

void InputManager::removeListener(ListenerID id, const std::string_view &actionName)
{
	const std::optional<int> entryIndex = this->getListenerEntryIndex(id, actionName);
	if (entryIndex.has_value())
	{
		this->listeners.erase(this->listeners.begin() + *entryIndex);
	}
	else
	{
		DebugLogWarning("No \"" + std::string(actionName) + "\" entry to remove for listener " + std::to_string(id) + ".");
	}
}

void InputManager::setRelativeMouseMode(bool active)
{
	SDL_bool enabled = active ? SDL_TRUE : SDL_FALSE;
	SDL_SetRelativeMouseMode(enabled);
}

void InputManager::update()
{
	// Refresh the mouse delta.
	SDL_GetRelativeMouseState(&this->mouseDelta.x, &this->mouseDelta.y);
}
