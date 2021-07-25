#include "AutomapUiController.h"
#include "AutomapUiView.h"
#include "GameWorldPanel.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"

std::string AutomapUiController::getInputActionName()
{
	return InputActionName::Automap;
}

std::string AutomapUiController::getBackToGameInputActionName()
{
	return InputActionName::Back;
}

void AutomapUiController::onBackToGameButtonSelected(Game &game)
{
	game.setPanel<GameWorldPanel>();
}

void AutomapUiController::onBackToGameInputAction(const InputActionCallbackValues &values, Game &game)
{
	if (values.performed)
	{
		AutomapUiController::onBackToGameButtonSelected(game);
	}
}

void AutomapUiController::onMouseButtonChanged(MouseButtonType buttonType, const Int2 &position, bool pressed,
	Game &game, const Rect &exitButtonRect)
{
	if ((buttonType == MouseButtonType::Left) && pressed)
	{
		const Int2 classicPoint = game.getRenderer().nativeToOriginal(position);
		if (exitButtonRect.contains(classicPoint))
		{
			AutomapUiController::onBackToGameButtonSelected(game);
		}
	}
}

void AutomapUiController::onMouseButtonHeld(MouseButtonType buttonType, const Int2 &position, double dt,
	Game &game, Double2 *automapOffset)
{
	// Listen for when the LMB is held on a compass direction.
	if (buttonType == MouseButtonType::Left)
	{
		const Int2 originalPoint = game.getRenderer().nativeToOriginal(position);
		const double scrollSpeed = AutomapUiView::ScrollSpeed * dt;

		// Modify the automap offset based on input. The directions are reversed because
		// to go right means to push the map left.
		if (AutomapUiView::CompassRightRegion.contains(originalPoint))
		{
			*automapOffset = *automapOffset - (Double2::UnitX * scrollSpeed);
		}
		else if (AutomapUiView::CompassLeftRegion.contains(originalPoint))
		{
			*automapOffset = *automapOffset + (Double2::UnitX * scrollSpeed);
		}
		else if (AutomapUiView::CompassUpRegion.contains(originalPoint))
		{
			*automapOffset = *automapOffset + (Double2::UnitY * scrollSpeed);
		}
		else if (AutomapUiView::CompassDownRegion.contains(originalPoint))
		{
			*automapOffset = *automapOffset - (Double2::UnitY * scrollSpeed);
		}
	}
}
