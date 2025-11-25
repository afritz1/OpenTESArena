#include "UiContext.h"
#include "UiManager.h"

void UiContextElements::free(UiManager &uiManager, Renderer &renderer)
{
	for (const UiElementInstanceID instID : this->imageElementInstIDs)
	{
		uiManager.freeImage(instID);
	}

	this->imageElementInstIDs.clear();

	for (const UiElementInstanceID instID : this->textBoxElementInstIDs)
	{
		uiManager.freeTextBox(instID, renderer);
	}

	this->textBoxElementInstIDs.clear();

	for (const UiElementInstanceID instID : this->buttonElementInstIDs)
	{
		uiManager.freeButton(instID);
	}

	this->buttonElementInstIDs.clear();
}

void UiContextInputListeners::free(InputManager &inputManager)
{
	for (const InputListenerID listenerID : this->inputActionListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	this->inputActionListenerIDs.clear();
}
