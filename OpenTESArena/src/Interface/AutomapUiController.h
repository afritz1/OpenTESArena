#ifndef AUTOMAP_UI_CONTROLLER_H
#define AUTOMAP_UI_CONTROLLER_H

#include <string>

#include "../Math/Vector2.h"

class Game;
class Rect;

struct InputActionCallbackValues;

enum class MouseButtonType;

namespace AutomapUiController
{
	std::string getInputActionName();
	std::string getBackToGameInputActionName();

	void onBackToGameButtonSelected(Game &game);
	void onBackToGameInputAction(const InputActionCallbackValues &values, Game &game);

	// @todo: this might be better handled by providing the input manager all the button + rect pairs on-screen
	// so it can support some kind of "onButtonClicked()" functionality.
	void onMouseButtonChanged(MouseButtonType buttonType, const Int2 &position, bool pressed,
		Game &game, const Rect &exitButtonRect);

	void onMouseButtonHeld(MouseButtonType buttonType, const Int2 &position, double dt,
		Game &game, Double2 *automapOffset);
}

#endif
