#ifndef GAME_WORLD_UI_CONTROLLER_H
#define GAME_WORLD_UI_CONTROLLER_H

#include "../Math/Vector2.h"

class Game;
class GameWorldPanel;
class Rect;
class TextBox;

enum class MouseButtonType;

struct InputActionCallbackValues;
struct Player;

namespace GameWorldUiController
{
	void onActivate(Game &game, const Int2 &screenPoint, TextBox &actionText);
	void onActivateInputAction(const InputActionCallbackValues &values, TextBox &actionText);
	void onInspect(Game &game, const Int2 &screenPoint, TextBox &actionText);
	void onInspectInputAction(const InputActionCallbackValues &values, TextBox &actionText);
	void onMouseButtonChanged(Game &game, MouseButtonType type, const Int2 &position, bool pressed,
		const Rect &centerCursorRegion, TextBox &actionText);
	void onMouseButtonHeld(Game &game, MouseButtonType type, const Int2 &position, double dt,
		const Rect &centerCursorRegion);
	void onCharacterSheetButtonSelected(Game &game);
	void onWeaponButtonSelected(Player &player);
	void onStealButtonSelected();
	void onStatusButtonSelected(Game &game);
	void onStatusPopUpSelected(Game &game);
	void onMagicButtonSelected();
	void onMapButtonSelected(Game &game, bool goToAutomap);
	void onLogbookButtonSelected(Game &game);
	void onUseItemButtonSelected();
	void onCampButtonSelected();
	void onScrollUpButtonSelected(GameWorldPanel &panel);
	void onScrollDownButtonSelected(GameWorldPanel &panel);
	void onToggleCompassInputAction(const InputActionCallbackValues &values);
	void onPlayerPositionInputAction(const InputActionCallbackValues &values, TextBox &actionText);
	void onPauseInputAction(const InputActionCallbackValues &values);

	void onKeyPickedUp(Game &game, int keyID, const ExeData &exeData);
	void onDoorUnlockedWithKey(Game &game, int keyID, const std::string &soundFilename, const WorldDouble3 &soundPosition, const ExeData &exeData);
}

#endif
