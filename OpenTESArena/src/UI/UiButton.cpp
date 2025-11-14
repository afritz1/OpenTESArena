#include "UiButton.h"

UiButtonInitInfo::UiButtonInitInfo()
{
	this->mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Left);
	this->callback = [](MouseButtonType) { };
	this->contentElementInstID = -1;
}

UiButton::UiButton()
{
	this->mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Left);
	this->callback = [](MouseButtonType) { };
	this->contentElementInstID = -1;
}

void UiButton::init(MouseButtonTypeFlags mouseButtonTypeFlags, const UiButtonCallback &callback, UiElementInstanceID contentElementInstID)
{
	this->mouseButtonFlags = mouseButtonTypeFlags;
	this->callback = callback;
	this->contentElementInstID = contentElementInstID;
}
