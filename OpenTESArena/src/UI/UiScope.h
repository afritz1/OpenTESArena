#ifndef UI_SCOPE_H
#define UI_SCOPE_H

enum class UiScope
{
	Global,
	MainMenu,
	CharacterCreation,
	LoadedSession,
	PauseMenu
};

static constexpr int UI_SCOPE_COUNT = static_cast<int>(UiScope::PauseMenu) + 1;

#endif
