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

void AutomapUiController::onBackToGameInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		AutomapUiController::onBackToGameButtonSelected(values.game);
	}
}

void AutomapUiController::onMouseButtonHeld(Game &game, MouseButtonType buttonType, const Int2 &position,
	double dt, Double2 *automapOffset)
{
	// Listen for when the LMB is held on a compass direction.
	if (buttonType == MouseButtonType::Left)
	{
		const Int2 originalPoint = game.renderer.nativeToOriginal(position);
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
