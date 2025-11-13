#include "UiButton.h"

UiButtonInitInfo::UiButtonInitInfo()
{
	this->callback = []() { };
	this->contentElementInstID = -1;
}

UiButton::UiButton()
{
	this->callback = []() { };
	this->contentElementInstID = -1;
}

void UiButton::init(const UiButtonCallback &callback, UiElementInstanceID contentElementInstID)
{
	this->callback = callback;
	this->contentElementInstID = contentElementInstID;
}
