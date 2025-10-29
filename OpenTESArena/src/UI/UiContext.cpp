#include "UiContext.h"
#include "UiManager.h"

void UiContextElements::free(UiManager &uiManager)
{
	for (const UiElementInstanceID instID : this->imageElementInstIDs)
	{
		uiManager.freeImage(instID);
	}

	this->imageElementInstIDs.clear();

	for (const UiElementInstanceID instID : this->textBoxElementInstIDs)
	{
		uiManager.freeTextBox(instID);
	}

	this->textBoxElementInstIDs.clear();

	for (const UiElementInstanceID instID : this->buttonElementInstIDs)
	{
		uiManager.freeButton(instID);
	}

	this->buttonElementInstIDs.clear();
}
