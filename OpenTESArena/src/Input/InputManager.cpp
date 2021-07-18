#include <algorithm>

#include "SDL_events.h"

#include "InputManager.h"

#include "components/debug/Debug.h"

void InputManager::InputActionListenerEntry::init(ListenerID id, const std::string_view &actionName,
	const InputActionCallback &callback)
{
	this->id = id;
	this->actionName = std::string(actionName);
	this->callback = callback;
}

void InputManager::MouseButtonChangedListenerEntry::init(ListenerID id, const MouseButtonChangedCallback &callback)
{
	this->id = id;
	this->callback = callback;
}

void InputManager::MouseButtonHeldListenerEntry::init(ListenerID id, const MouseButtonHeldCallback &callback)
{
	this->id = id;
	this->callback = callback;
}

void InputManager::MouseScrollChangedListenerEntry::init(ListenerID id, const MouseScrollChangedCallback &callback)
{
	this->id = id;
	this->callback = callback;
}

void InputManager::MouseMotionListenerEntry::init(ListenerID id, const MouseMotionCallback &callback)
{
	this->id = id;
	this->callback = callback;
}

void InputManager::ApplicationExitListenerEntry::init(ListenerID id, const ApplicationExitCallback &callback)
{
	this->id = id;
	this->callback = callback;
}

void InputManager::WindowResizedListenerEntry::init(ListenerID id, const WindowResizedCallback &callback)
{
	this->id = id;
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
	this->inputActionMaps = InputActionMap::loadDefaultMaps();
}

InputManager::ListenerID InputManager::nextListenerID()
{
	const InputManager::ListenerID id = this->nextID;
	this->nextID++;
	return id;
}

std::optional<int> InputManager::getInputActionListenerEntryIndex(ListenerID id, const std::string_view &actionName) const
{
	const auto iter = std::find_if(this->inputActionListeners.begin(), this->inputActionListeners.end(),
		[id, &actionName](const InputActionListenerEntry &entry)
	{
		return (entry.id == id) && (entry.actionName == actionName);
	});

	if (iter != this->inputActionListeners.end())
	{
		return static_cast<int>(std::distance(this->inputActionListeners.begin(), iter));
	}
	else
	{
		return std::nullopt;
	}
}

template <typename EntryType>
std::optional<int> InputManager::getListenerEntryIndex(ListenerID id, const std::vector<EntryType> &listeners)
{
	const auto iter = std::find_if(listeners.begin(), listeners.end(),
		[id](const EntryType &entry)
	{
		return entry.id == id;
	});

	if (iter != listeners.end())
	{
		return static_cast<int>(std::distance(listeners.begin(), iter));
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
	const auto iter = std::find_if(this->inputActionMaps.begin(), this->inputActionMaps.end(),
		[&name](const InputActionMap &map)
	{
		return map.name == name;
	});

	if (iter != this->inputActionMaps.end())
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

template <typename EntryType, typename CallbackType>
void InputManager::addListenerInternal(ListenerID id, CallbackType &&callback, std::vector<EntryType> &listeners)
{
	const std::optional<int> existingIndex = InputManager::getListenerEntryIndex(id, listeners);
	if (existingIndex.has_value())
	{
		DebugLogError("Already registered \"" + std::string(typeid(EntryType).name()) + "\" for listener " + std::to_string(id) + ".");
		return;
	}

	EntryType entry;
	entry.init(id, callback);
	listeners.emplace_back(std::move(entry));
}

void InputManager::addInputActionListener(ListenerID id, const std::string_view &actionName, const InputActionCallback &callback)
{
	const std::optional<int> existingIndex = this->getInputActionListenerEntryIndex(id, actionName);
	if (existingIndex.has_value())
	{
		DebugLogError("Already registered \"" + std::string(actionName) + "\" input action for listener " + std::to_string(id) + ".");
		return;
	}

	InputActionListenerEntry entry;
	entry.init(id, actionName, callback);
	this->inputActionListeners.emplace_back(std::move(entry));
}

void InputManager::addMouseButtonChangedListener(ListenerID id, const MouseButtonChangedCallback &callback)
{
	this->addListenerInternal(id, callback, this->mouseButtonChangedListeners);
}

void InputManager::addMouseButtonHeldListener(ListenerID id, const MouseButtonHeldCallback &callback)
{
	this->addListenerInternal(id, callback, this->mouseButtonHeldListeners);
}

void InputManager::addMouseScrollChangedListener(ListenerID id, const MouseScrollChangedCallback &callback)
{
	this->addListenerInternal(id, callback, this->mouseScrollChangedListeners);
}

void InputManager::addMouseMotionListener(ListenerID id, const MouseMotionCallback &callback)
{
	this->addListenerInternal(id, callback, this->mouseMotionListeners);
}

void InputManager::addApplicationExitListener(ListenerID id, const ApplicationExitCallback &callback)
{
	this->addListenerInternal(id, callback, this->applicationExitListeners);
}

void InputManager::addWindowResizedListener(ListenerID id, const WindowResizedCallback &callback)
{
	this->addListenerInternal(id, callback, this->windowResizedListeners);
}

template <typename EntryType>
void InputManager::removeListenerInternal(ListenerID id, std::vector<EntryType> &listeners)
{
	const std::optional<int> entryIndex = InputManager::getListenerEntryIndex(id, listeners);
	if (entryIndex.has_value())
	{
		listeners.erase(listeners.begin() + *entryIndex);
	}
	else
	{
		DebugLogWarning("No \"" + std::string(typeid(EntryType).name()) + "\" entry to remove for listener " + std::to_string(id) + ".");
	}
}

void InputManager::removeInputActionListener(ListenerID id, const std::string_view &actionName)
{
	const std::optional<int> entryIndex = this->getInputActionListenerEntryIndex(id, actionName);
	if (entryIndex.has_value())
	{
		this->inputActionListeners.erase(this->inputActionListeners.begin() + *entryIndex);
	}
	else
	{
		DebugLogWarning("No \"" + std::string(actionName) + "\" input action entry to remove for listener " + std::to_string(id) + ".");
	}
}

void InputManager::removeMouseButtonChangedListener(ListenerID id)
{
	this->removeListenerInternal(id, this->mouseButtonChangedListeners);
}

void InputManager::removeMouseButtonHeldListener(ListenerID id)
{
	this->removeListenerInternal(id, this->mouseButtonHeldListeners);
}

void InputManager::removeMouseScrollChangedListener(ListenerID id)
{
	this->removeListenerInternal(id, this->mouseScrollChangedListeners);
}

void InputManager::removeMouseMotionListener(ListenerID id)
{
	this->removeListenerInternal(id, this->mouseMotionListeners);
}

void InputManager::removeApplicationExitListener(ListenerID id)
{
	this->removeListenerInternal(id, this->applicationExitListeners);
}

void InputManager::removeWindowResizedListener(ListenerID id)
{
	this->removeListenerInternal(id, this->windowResizedListeners);
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
