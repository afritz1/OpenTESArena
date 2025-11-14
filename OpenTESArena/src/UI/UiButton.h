#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "UiElement.h"

#include <functional>

enum class MouseButtonType;

using UiButtonCallback = std::function<void(MouseButtonType mouseButtonType)>;

struct UiButtonInitInfo
{
	UiButtonCallback callback;
	UiElementInstanceID contentElementInstID;

	UiButtonInitInfo();
};

struct UiButton
{
	UiButtonCallback callback;
	UiElementInstanceID contentElementInstID; // Optionally points to child image/text box/etc. for transform size.

	UiButton();

	void init(const UiButtonCallback &callback, UiElementInstanceID contentElementInstID);
};

#endif
