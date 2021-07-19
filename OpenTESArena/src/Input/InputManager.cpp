#include <algorithm>

#include "SDL_events.h"

#include "InputManager.h"

#include "components/debug/Debug.h"

void InputManager::InputActionListenerEntry::init(const std::string_view &actionName, const InputActionCallback &callback)
{
	this->actionName = std::string(actionName);
	this->callback = callback;
}

void InputManager::MouseButtonChangedListenerEntry::init(const MouseButtonChangedCallback &callback)
{
	this->callback = callback;
}

void InputManager::MouseButtonHeldListenerEntry::init(const MouseButtonHeldCallback &callback)
{
	this->callback = callback;
}

void InputManager::MouseScrollChangedListenerEntry::init(const MouseScrollChangedCallback &callback)
{
	this->callback = callback;
}

void InputManager::MouseMotionListenerEntry::init(const MouseMotionCallback &callback)
{
	this->callback = callback;
}

void InputManager::ApplicationExitListenerEntry::init(const ApplicationExitCallback &callback)
{
	this->callback = callback;
}

void InputManager::WindowResizedListenerEntry::init(const WindowResizedCallback &callback)
{
	this->callback = callback;
}

InputManager::InputManager()
	: mouseDelta(0, 0)
{
	this->nextListenerID = 0;
}

void InputManager::init()
{
	// Add input action maps to be enabled/disabled as needed.
	this->inputActionMaps = InputActionMap::loadDefaultMaps();
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

InputManager::ListenerID InputManager::getNextListenerID()
{
	ListenerID listenerID;
	if (!this->freedListenerIDs.empty())
	{
		listenerID = this->freedListenerIDs.back();
		this->freedListenerIDs.pop_back();
	}
	else
	{
		listenerID = this->nextListenerID;
		this->nextListenerID++;
	}

	return listenerID;
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
InputManager::ListenerID InputManager::addListenerInternal(CallbackType &&callback, std::vector<EntryType> &listeners,
	std::vector<int> &freedListenerIndices)
{
	int insertIndex;
	if (!freedListenerIndices.empty())
	{
		insertIndex = freedListenerIndices.back();
		freedListenerIndices.pop_back();
	}
	else
	{
		insertIndex = static_cast<int>(listeners.size());
		listeners.emplace_back(EntryType());
	}

	DebugAssertIndex(listeners, insertIndex);
	EntryType &listenerEntry = listeners[insertIndex];
	listenerEntry.init(callback);

	const ListenerID listenerID = this->getNextListenerID();
	this->listenerIndices.emplace(listenerID, insertIndex);

	return listenerID;
}

InputManager::ListenerID InputManager::addInputActionListener(const std::string_view &actionName,
	const InputActionCallback &callback)
{
	int insertIndex;
	if (!this->freedInputActionListenerIndices.empty())
	{
		insertIndex = this->freedInputActionListenerIndices.back();
		this->freedInputActionListenerIndices.pop_back();
	}
	else
	{
		insertIndex = static_cast<int>(this->inputActionListeners.size());
		this->inputActionListeners.emplace_back(InputActionListenerEntry());
	}

	DebugAssertIndex(this->inputActionListeners, insertIndex);
	InputActionListenerEntry &listenerEntry = this->inputActionListeners[insertIndex];
	listenerEntry.init(actionName, callback);

	const ListenerID listenerID = this->getNextListenerID();
	this->listenerIndices.emplace(listenerID, insertIndex);

	return listenerID;
}

InputManager::ListenerID InputManager::addMouseButtonChangedListener(const MouseButtonChangedCallback &callback)
{
	return this->addListenerInternal(callback, this->mouseButtonChangedListeners, this->freedMouseButtonChangedListenerIndices);
}

InputManager::ListenerID InputManager::addMouseButtonHeldListener(const MouseButtonHeldCallback &callback)
{
	return this->addListenerInternal(callback, this->mouseButtonHeldListeners, this->freedMouseButtonHeldListenerIndices);
}

InputManager::ListenerID InputManager::addMouseScrollChangedListener(const MouseScrollChangedCallback &callback)
{
	return this->addListenerInternal(callback, this->mouseScrollChangedListeners, this->freedMouseScrollChangedListenerIndices);
}

InputManager::ListenerID InputManager::addMouseMotionListener(const MouseMotionCallback &callback)
{
	return this->addListenerInternal(callback, this->mouseMotionListeners, this->freedMouseMotionListenerIndices);
}

InputManager::ListenerID InputManager::addApplicationExitListener(const ApplicationExitCallback &callback)
{
	return this->addListenerInternal(callback, this->applicationExitListeners, this->freedApplicationExitListenerIndices);
}

InputManager::ListenerID InputManager::addWindowResizedListener(const WindowResizedCallback &callback)
{
	return this->addListenerInternal(callback, this->windowResizedListeners, this->freedWindowResizedListenerIndices);
}

template <typename EntryType>
void InputManager::removeListenerInternal(ListenerID id, std::vector<EntryType> &listeners,
	std::vector<int> &freedListenerIndices)
{
	const auto iter = this->listenerIndices.find(id);
	if (iter != this->listenerIndices.end())
	{
		// Remove the means of looking up this entry. Don't need to actually erase the entry itself.
		const int removeIndex = iter->second;
		this->listenerIndices.erase(iter);

		DebugAssertIndex(listeners, removeIndex); // Double-check that the entry exists.
		freedListenerIndices.emplace_back(removeIndex);
		this->freedListenerIDs.emplace_back(id);
	}
	else
	{
		DebugLogWarning("No entry to remove for listener " + std::to_string(id) + ".");
	}
}

void InputManager::removeInputActionListener(ListenerID id)
{
	this->removeListenerInternal(id, this->inputActionListeners, this->freedInputActionListenerIndices);
}

void InputManager::removeMouseButtonChangedListener(ListenerID id)
{
	this->removeListenerInternal(id, this->mouseButtonChangedListeners, this->freedMouseButtonChangedListenerIndices);
}

void InputManager::removeMouseButtonHeldListener(ListenerID id)
{
	this->removeListenerInternal(id, this->mouseButtonHeldListeners, this->freedMouseButtonHeldListenerIndices);
}

void InputManager::removeMouseScrollChangedListener(ListenerID id)
{
	this->removeListenerInternal(id, this->mouseScrollChangedListeners, this->freedMouseScrollChangedListenerIndices);
}

void InputManager::removeMouseMotionListener(ListenerID id)
{
	this->removeListenerInternal(id, this->mouseMotionListeners, this->freedMouseMotionListenerIndices);
}

void InputManager::removeApplicationExitListener(ListenerID id)
{
	this->removeListenerInternal(id, this->applicationExitListeners, this->freedApplicationExitListenerIndices);
}

void InputManager::removeWindowResizedListener(ListenerID id)
{
	this->removeListenerInternal(id, this->windowResizedListeners, this->freedWindowResizedListenerIndices);
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
