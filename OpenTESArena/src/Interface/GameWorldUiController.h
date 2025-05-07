#ifndef GAME_WORLD_UI_CONTROLLER_H
#define GAME_WORLD_UI_CONTROLLER_H

#include <functional>

#include "../Math/Vector2.h"

class ExeData;
class Game;
class GameWorldPanel;
class TextBox;

enum class MouseButtonType;

struct EntityInstance;
struct InputActionCallbackValues;
struct Player;
struct Rect;

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

	void onKeyPickedUp(Game &game, int keyID, const ExeData &exeData, const std::function<void()> postStatusPopUpCallback);
	void onDoorUnlockedWithKey(Game &game, int keyID, const std::string &soundFilename, const WorldDouble3 &soundPosition, const ExeData &exeData);

	void onCitizenInteracted(Game &game, const EntityInstance &entityInst);

	void onStaminaExhausted(Game &game, bool isSwimming, bool isInterior, bool isNight);
}

#endif
