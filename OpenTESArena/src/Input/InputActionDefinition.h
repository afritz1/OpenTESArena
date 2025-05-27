#ifndef INPUT_ACTION_DEFINITION_H
#define INPUT_ACTION_DEFINITION_H

#include <optional>
#include <string>

#include "SDL_keyboard.h"
#include "SDL_keycode.h"

enum class InputActionType;
enum class InputStateType;
enum class MouseButtonType;
enum class MouseWheelScrollType;

// Treated like a key; doesn't involve mouse position.
struct InputActionMouseButtonDefinition
{
	MouseButtonType type;

	InputActionMouseButtonDefinition();

	void init(MouseButtonType type);
};

struct InputActionMouseScrollDefinition
{
	MouseWheelScrollType type;

	InputActionMouseScrollDefinition();

	void init(MouseWheelScrollType type);
};

// Union of one or more keys (Ctrl, Ctrl + Alt, etc.). All must be pressed when matching key definitions.
using KeyDefinitionKeymod = decltype(SDL_Keysym::mod);

struct InputActionKeyDefinition
{
	SDL_Keycode keycode;
	KeyDefinitionKeymod keymod;

	InputActionKeyDefinition();

	void init(SDL_Keycode keycode, KeyDefinitionKeymod keymod);
};

struct InputActionDefinition
{
	std::string name;
	InputActionType type;
	std::optional<InputStateType> stateType; // Optional since some actions like scroll wheel are mono-state inputs.

	union
	{
		InputActionMouseButtonDefinition mouseButtonDef;
		InputActionMouseScrollDefinition mouseScrollDef;
		InputActionKeyDefinition keyDef;
	};

	InputActionDefinition();

	void init(const std::string &name, InputActionType type, const std::optional<InputStateType> &stateType);
	void initMouseButtonDef(const std::string &name, InputStateType stateType, MouseButtonType buttonType);
	void initMouseScrollDef(const std::string &name, MouseWheelScrollType scrollType);
	void initKeyDef(const std::string &name, InputStateType stateType, SDL_Keycode keycode,
		const std::optional<KeyDefinitionKeymod> &keymod = std::nullopt);
};

#endif
