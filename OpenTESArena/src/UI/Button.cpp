#include "Button.h"

ButtonProxy::ButtonProxy(MouseButtonType buttonType, const Rect &rect, const std::function<void()> &callback,
	const std::function<bool()> &isActiveFunc)
	: rect(rect), callback(callback), isActiveFunc(isActiveFunc)
{
	this->buttonType = buttonType;
}

ButtonProxy::ButtonProxy()
{
	this->buttonType = static_cast<MouseButtonType>(-1);
}
