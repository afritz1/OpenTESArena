#include "InputActionMap.h"
#include "InputActionMapName.h"
#include "InputActionName.h"
#include "InputStateType.h"
#include "PointerTypes.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

namespace
{
	InputActionMap MakeInputActionMapFromDefault(const char *mapName)
	{
		DebugAssert(!String::isNullOrEmpty(mapName));

		auto makeMouseButtonDef = [](const std::string &defName, InputStateType stateType,
			MouseButtonType buttonType)
		{
			InputActionDefinition def;
			def.initMouseButtonDef(defName, stateType, buttonType);
			return def;
		};

		auto makeMouseScrollDef = [](const std::string &defName, MouseWheelScrollType scrollType)
		{
			InputActionDefinition def;
			def.initMouseScrollDef(defName, scrollType);
			return def;
		};

		auto makeKeyDef = [](const std::string &defName, InputStateType stateType, SDL_Keycode keycode,
			const std::optional<InputActionDefinition::KeyDefinition::Keymod> &keymod = std::nullopt)
		{
			InputActionDefinition def;
			def.initKeyDef(defName, stateType, keycode, keymod);
			return def;
		};

		InputActionMap map;

		// Common map is always active.
		const bool allowedDuringTextEntry = StringView::equals(mapName, InputActionMapName::Common);
		const bool active = allowedDuringTextEntry;
		map.init(mapName, allowedDuringTextEntry, active);

		std::vector<InputActionDefinition> &defs = map.defs;
		if (StringView::equals(mapName, InputActionMapName::Common))
		{
			defs.emplace_back(makeKeyDef(
				InputActionName::Accept,
				InputStateType::BeginPerform,
				SDLK_RETURN));
			defs.emplace_back(makeKeyDef(
				InputActionName::Back,
				InputStateType::BeginPerform,
				SDLK_ESCAPE));
			defs.emplace_back(makeKeyDef(
				InputActionName::Screenshot,
				InputStateType::BeginPerform,
				SDLK_PRINTSCREEN));
			defs.emplace_back(makeKeyDef(
				InputActionName::Backspace,
				InputStateType::BeginPerform,
				SDLK_BACKSPACE)); // @todo: or SDLK_KP_BACKSPACE?
			// Going to keep scroll up/down as pointer events since scrollable UI things need the pointer over them.
		}
		else if (StringView::equals(mapName, InputActionMapName::Automap))
		{
			defs.emplace_back(makeKeyDef(
				InputActionName::Automap,
				InputStateType::BeginPerform,
				SDLK_n));
		}
		else if (StringView::equals(mapName, InputActionMapName::CharacterCreation))
		{
			defs.emplace_back(makeKeyDef(
				InputActionName::SaveAttributes,
				InputStateType::BeginPerform,
				SDLK_s));
			defs.emplace_back(makeKeyDef(
				InputActionName::RerollAttributes,
				InputStateType::BeginPerform,
				SDLK_r));
		}
		else if (StringView::equals(mapName, InputActionMapName::Cinematic))
		{
			// @todo: support multiple input buttons like left click, right click, escape, space, enter, keypad enter, etc..
			// The triggering of the Skip input action is the union of those physical inputs.
			defs.emplace_back(makeKeyDef(
				InputActionName::Skip,
				InputStateType::BeginPerform,
				SDLK_ESCAPE));
		}
		else if (StringView::equals(mapName, InputActionMapName::GameWorld))
		{
			// Game world interaction.
			// @todo: might want Move{...}Fast variations w/ LeftShift if we want to keep sprint (wasn't in the original game).
			// - might be a good time to remove sprint altogether too.
			defs.emplace_back(makeKeyDef(
				InputActionName::MoveForward,
				InputStateType::Performing,
				SDLK_w));
			defs.emplace_back(makeKeyDef(
				InputActionName::MoveBackward,
				InputStateType::Performing,
				SDLK_s));
			defs.emplace_back(makeKeyDef(
				InputActionName::TurnLeft,
				InputStateType::Performing,
				SDLK_a));
			defs.emplace_back(makeKeyDef(
				InputActionName::TurnRight,
				InputStateType::Performing,
				SDLK_d));
			defs.emplace_back(makeKeyDef(
				InputActionName::StrafeLeft,
				InputStateType::Performing,
				SDLK_a,
				SDL_Keymod::KMOD_LCTRL));
			defs.emplace_back(makeKeyDef(
				InputActionName::StrafeRight,
				InputStateType::Performing,
				SDLK_d,
				SDL_Keymod::KMOD_LCTRL));
			defs.emplace_back(makeKeyDef(
				InputActionName::Jump,
				InputStateType::Performing,
				SDLK_SPACE));
			defs.emplace_back(makeKeyDef(
				InputActionName::Activate,
				InputStateType::BeginPerform,
				SDLK_e));
			defs.emplace_back(makeMouseButtonDef(
				InputActionName::Inspect,
				InputStateType::BeginPerform,
				MouseButtonType::Left));

			// UI interaction.
			defs.emplace_back(makeKeyDef(
				InputActionName::Automap,
				InputStateType::BeginPerform,
				SDLK_n));
			defs.emplace_back(makeKeyDef(
				InputActionName::Camp,
				InputStateType::BeginPerform,
				SDLK_r));
			defs.emplace_back(makeKeyDef(
				InputActionName::Camp,
				InputStateType::EndPerform, // @temp for testing fast forward with hotkey
				SDLK_r));
			defs.emplace_back(makeKeyDef(
				InputActionName::CastMagic,
				InputStateType::BeginPerform,
				SDLK_c));
			defs.emplace_back(makeKeyDef(
				InputActionName::CharacterSheet,
				InputStateType::BeginPerform,
				SDLK_TAB)); // @todo: and F1
			defs.emplace_back(makeKeyDef(
				InputActionName::Logbook,
				InputStateType::BeginPerform,
				SDLK_l));
			defs.emplace_back(makeKeyDef(
				InputActionName::PauseMenu,
				InputStateType::BeginPerform,
				SDLK_ESCAPE));
			defs.emplace_back(makeKeyDef(
				InputActionName::PlayerPosition,
				InputStateType::BeginPerform,
				SDLK_F2));
			defs.emplace_back(makeKeyDef(
				InputActionName::Status,
				InputStateType::BeginPerform,
				SDLK_v));
			defs.emplace_back(makeKeyDef(
				InputActionName::Steal,
				InputStateType::BeginPerform,
				SDLK_p));
			defs.emplace_back(makeKeyDef(
				InputActionName::ToggleCompass,
				InputStateType::BeginPerform,
				SDLK_F8));
			defs.emplace_back(makeKeyDef(
				InputActionName::ToggleWeapon,
				InputStateType::BeginPerform,
				SDLK_f));
			defs.emplace_back(makeKeyDef(
				InputActionName::UseItem,
				InputStateType::BeginPerform,
				SDLK_u));
			defs.emplace_back(makeKeyDef(
				InputActionName::WorldMap,
				InputStateType::BeginPerform,
				SDLK_m));

			// Debug.
			defs.emplace_back(makeKeyDef(
				InputActionName::DebugProfiler,
				InputStateType::BeginPerform,
				SDLK_F4));
		}
		else if (StringView::equals(mapName, InputActionMapName::MainMenu))
		{
			defs.emplace_back(makeKeyDef(
				InputActionName::StartNewGame,
				InputStateType::BeginPerform,
				SDLK_s));
			defs.emplace_back(makeKeyDef(
				InputActionName::LoadGame,
				InputStateType::BeginPerform,
				SDLK_l));
			defs.emplace_back(makeKeyDef(
				InputActionName::ExitGame,
				InputStateType::BeginPerform,
				SDLK_e));
			defs.emplace_back(makeKeyDef(
				InputActionName::TestGame,
				InputStateType::BeginPerform,
				SDLK_f));
		}
		else
		{
			DebugLogError("Unrecognized default map name \"" + std::string(mapName) + "\".");
		}

		return map;
	}
}

InputActionMap::InputActionMap()
{
	this->allowedDuringTextEntry = false;
	this->active = false;
}

void InputActionMap::init(const std::string &name, bool allowedDuringTextEntry, bool active)
{
	this->name = name;
	this->allowedDuringTextEntry = allowedDuringTextEntry;
	this->active = active;
}

std::vector<InputActionMap> InputActionMap::loadDefaultMaps()
{
	std::vector<InputActionMap> maps;
	for (const char *mapName : InputActionMapName::Names)
	{
		InputActionMap map = MakeInputActionMapFromDefault(mapName);
		maps.emplace_back(std::move(map));
	}

	return maps;
}
