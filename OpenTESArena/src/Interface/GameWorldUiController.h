#ifndef GAME_WORLD_UI_CONTROLLER_H
#define GAME_WORLD_UI_CONTROLLER_H

class Game;
class GameWorldPanel;
class Player;

namespace GameWorldUiController
{
	void onCharacterSheetButtonSelected(Game &game);
	void onWeaponButtonSelected(Player &player);
	void onStealButtonSelected();
	void onStatusButtonSelected(Game &game);
	void onStatusPopUpSelected(Game &game);
	void onMagicButtonSelected();
	void onLogbookButtonSelected(Game &game);
	void onUseItemButtonSelected();
	void onCampButtonSelected();
	void onScrollUpButtonSelected(GameWorldPanel &panel);
	void onScrollDownButtonSelected(GameWorldPanel &panel);
	void onPauseButtonSelected(Game &game);
	void onMapButtonSelected(Game &game, bool goToAutomap);
}

#endif
