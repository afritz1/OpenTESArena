#include "InputActionDefinition.h"
#include "InputActionType.h"

#include "components/debug/Debug.h"

InputActionDefinition::MouseButtonDefinition::MouseButtonDefinition()
{
	this->type = static_cast<MouseButtonType>(-1);
}

void InputActionDefinition::MouseButtonDefinition::init(MouseButtonType type)
{
	this->type = type;
}

InputActionDefinition::MouseScrollDefinition::MouseScrollDefinition()
{
	this->type = static_cast<MouseWheelScrollType>(-1);
}

void InputActionDefinition::MouseScrollDefinition::init(MouseWheelScrollType type)
{
	this->type = type;
}

InputActionDefinition::KeyDefinition::KeyDefinition()
{
	this->keycode = static_cast<SDL_Keycode>(-1);
	this->keymod = static_cast<SDL_Keymod>(-1);
}

void InputActionDefinition::KeyDefinition::init(SDL_Keycode keycode, SDL_Keymod keymod)
{
	this->keycode = keycode;
	this->keymod = keymod;
}

InputActionDefinition::InputActionDefinition()
{
	this->type = static_cast<InputActionType>(-1);
}

void InputActionDefinition::init(std::string &&name, InputActionType type,
	const std::optional<InputStateType> &stateType)
{
	this->name = std::move(name);
	this->type = type;
	this->stateType = stateType;
}

void InputActionDefinition::initMouseButtonDef(const std::string &name, InputStateType stateType,
	MouseButtonType buttonType)
{
	this->init(std::string(name), InputActionType::MouseButton, stateType);
	this->mouseButtonDef.init(buttonType);
}

void InputActionDefinition::initMouseScrollDef(const std::string &name, MouseWheelScrollType scrollType)
{
	this->init(std::string(name), InputActionType::MouseWheel, std::nullopt);
	this->mouseScrollDef.init(scrollType);
}

void InputActionDefinition::initKeyDef(const std::string &name, InputStateType stateType,
	SDL_Keycode keycode, const std::optional<SDL_Keymod> &keymod)
{
	this->init(std::string(name), InputActionType::Key, stateType);
	this->keyDef.init(keycode, keymod.value_or(SDL_Keymod::KMOD_NONE));
}
