#ifndef PAUSE_MENU_UI_CONTROLLER_H
#define PAUSE_MENU_UI_CONTROLLER_H

class Game;
class PauseMenuPanel;

namespace PauseMenuUiController
{
	void onNewGameButtonSelected(Game &game);
	void onLoadButtonSelected(Game &game);
	void onSaveButtonSelected(Game &game);
	void onExitButtonSelected(Game &game);
	void onResumeButtonSelected(Game &game);
	void onOptionsButtonSelected(Game &game);
	void onSoundUpButtonSelected(Game &game, PauseMenuPanel &panel);
	void onSoundDownButtonSelected(Game &game, PauseMenuPanel &panel);
	void onMusicUpButtonSelected(Game &game, PauseMenuPanel &panel);
	void onMusicDownButtonSelected(Game &game, PauseMenuPanel &panel);
}

#endif
