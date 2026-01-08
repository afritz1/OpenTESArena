#include "UiContext.h"
#include "UiManager.h"

void UiContextState::free(InputManager &inputManager, UiManager &uiManager, Renderer &renderer)
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

	for (const UiElementInstanceID instID : this->listBoxElementInstIDs)
	{
		uiManager.freeListBox(instID, renderer);
	}

	this->listBoxElementInstIDs.clear();

	for (const UiElementInstanceID instID : this->buttonElementInstIDs)
	{
		uiManager.freeButton(instID);
	}

	this->buttonElementInstIDs.clear();

	for (const InputListenerID listenerID : this->inputActionListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	this->inputActionListenerIDs.clear();

	for (const InputListenerID listenerID : this->mouseButtonChangedListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	this->mouseButtonChangedListenerIDs.clear();

	for (const InputListenerID listenerID : this->mouseButtonHeldListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	this->mouseButtonHeldListenerIDs.clear();

	for (const InputListenerID listenerID : this->mouseScrollChangedListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	this->mouseScrollChangedListenerIDs.clear();

	for (const InputListenerID listenerID : this->mouseMotionListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	this->mouseMotionListenerIDs.clear();

	for (const InputListenerID listenerID : this->applicationExitListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	this->applicationExitListenerIDs.clear();

	for (const InputListenerID listenerID : this->windowResizedListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	this->windowResizedListenerIDs.clear();

	for (const InputListenerID listenerID : this->renderTargetsResetListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	this->renderTargetsResetListenerIDs.clear();

	for (const InputListenerID listenerID : this->textInputListenerIDs)
	{
		inputManager.removeListener(listenerID);
	}

	this->textInputListenerIDs.clear();
}
