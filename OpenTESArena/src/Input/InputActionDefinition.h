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

class InputActionDefinition
{
public:
	// Treated like a key; doesn't involve mouse position.
	struct MouseButtonDefinition
	{
		MouseButtonType type;

		MouseButtonDefinition();

		void init(MouseButtonType type);
	};

	struct MouseScrollDefinition
	{
		MouseWheelScrollType type;

		MouseScrollDefinition();

		void init(MouseWheelScrollType type);
	};

	struct KeyDefinition
	{
		// Union of one or more keys (Ctrl, Ctrl + Alt, etc.). All must be pressed when matching key definitions.
		using Keymod = decltype(SDL_Keysym::mod);

		SDL_Keycode keycode;
		Keymod keymod;

		KeyDefinition();

		void init(SDL_Keycode keycode, Keymod keymod);
	};

	std::string name;
	InputActionType type;
	std::optional<InputStateType> stateType; // Optional since some actions like scroll wheel are mono-state inputs.

	union
	{
		MouseButtonDefinition mouseButtonDef;
		MouseScrollDefinition mouseScrollDef;
		KeyDefinition keyDef;
	};
private:
	void init(std::string &&name, InputActionType type, const std::optional<InputStateType> &stateType);
public:
	InputActionDefinition();

	void initMouseButtonDef(const std::string &name, InputStateType stateType, MouseButtonType buttonType);
	void initMouseScrollDef(const std::string &name, MouseWheelScrollType scrollType);
	void initKeyDef(const std::string &name, InputStateType stateType, SDL_Keycode keycode,
		const std::optional<KeyDefinition::Keymod> &keymod = std::nullopt);
};

#endif
