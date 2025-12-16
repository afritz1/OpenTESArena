#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include <string>

#include "UiElement.h"
#include "../Input/PointerTypes.h"

#include <functional>

using UiButtonCallback = std::function<void(MouseButtonType mouseButtonType)>;

struct UiButtonInitInfo
{
	MouseButtonTypeFlags mouseButtonFlags;
	UiButtonCallback callback;
	std::string contentElementName;

	UiButtonInitInfo();
};

struct UiButton
{
	MouseButtonTypeFlags mouseButtonFlags; // Buttons allowed to trigger callback. Defaults to left mouse button only.
	UiButtonCallback callback;
	std::string contentElementName; // Optionally points to child image/text box/etc. for transform size.

	UiButton();

	void init(MouseButtonTypeFlags mouseButtonTypeFlags, const UiButtonCallback &callback, const std::string &contentElementName);
};

#endif
