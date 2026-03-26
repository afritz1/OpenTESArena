#include "UiButton.h"

UiButtonInitInfo::UiButtonInitInfo()
{
	this->mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Left);
	this->callback = [](MouseButtonType) { };
}

UiButton::UiButton()
{
	this->mouseButtonFlags = MouseButtonTypeFlags(MouseButtonType::Left);
	this->callback = [](MouseButtonType) { };
}

void UiButton::init(MouseButtonTypeFlags mouseButtonTypeFlags, const UiButtonCallback &callback, const std::string &contentElementName)
{
	this->mouseButtonFlags = mouseButtonTypeFlags;
	this->callback = callback;
	this->contentElementName = contentElementName;
}
