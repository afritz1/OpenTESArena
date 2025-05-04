#include "InputActionDefinition.h"
#include "InputActionType.h"

#include "components/debug/Debug.h"

InputActionMouseButtonDefinition::InputActionMouseButtonDefinition()
{
	this->type = static_cast<MouseButtonType>(-1);
}

void InputActionMouseButtonDefinition::init(MouseButtonType type)
{
	this->type = type;
}

InputActionMouseScrollDefinition::InputActionMouseScrollDefinition()
{
	this->type = static_cast<MouseWheelScrollType>(-1);
}

void InputActionMouseScrollDefinition::init(MouseWheelScrollType type)
{
	this->type = type;
}

InputActionKeyDefinition::InputActionKeyDefinition()
{
	this->keycode = static_cast<SDL_Keycode>(-1);
	this->keymod = 0;
}

void InputActionKeyDefinition::init(SDL_Keycode keycode, KeyDefinitionKeymod keymod)
{
	this->keycode = keycode;
	this->keymod = keymod;
}

InputActionDefinition::InputActionDefinition()
{
	this->type = static_cast<InputActionType>(-1);
}

void InputActionDefinition::init(const std::string &name, InputActionType type, const std::optional<InputStateType> &stateType)
{
	this->name = name;
	this->type = type;
	this->stateType = stateType;
}

void InputActionDefinition::initMouseButtonDef(const std::string &name, InputStateType stateType, MouseButtonType buttonType)
{
	this->init(name, InputActionType::MouseButton, stateType);
	this->mouseButtonDef.init(buttonType);
}

void InputActionDefinition::initMouseScrollDef(const std::string &name, MouseWheelScrollType scrollType)
{
	this->init(name, InputActionType::MouseWheel, std::nullopt);
	this->mouseScrollDef.init(scrollType);
}

void InputActionDefinition::initKeyDef(const std::string &name, InputStateType stateType,
	SDL_Keycode keycode, const std::optional<KeyDefinitionKeymod> &keymod)
{
	this->init(name, InputActionType::Key, stateType);
	this->keyDef.init(keycode, keymod.value_or(SDL_Keymod::KMOD_NONE));
}
