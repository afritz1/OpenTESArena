#include "UiButton.h"

UiButtonInitInfo::UiButtonInitInfo()
{
	this->callback = [](MouseButtonType) { };
	this->contentElementInstID = -1;
}

UiButton::UiButton()
{
	this->callback = [](MouseButtonType) { };
	this->contentElementInstID = -1;
}

void UiButton::init(const UiButtonCallback &callback, UiElementInstanceID contentElementInstID)
{
	this->callback = callback;
	this->contentElementInstID = contentElementInstID;
}
