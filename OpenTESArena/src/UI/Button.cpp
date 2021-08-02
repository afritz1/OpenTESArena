#include "Button.h"

ButtonProxy::ButtonProxy(MouseButtonType buttonType, const RectFunction &rectFunc,
	const Callback &callback, const ActiveFunction &isActiveFunc)
	: rectFunc(rectFunc), callback(callback), isActiveFunc(isActiveFunc)
{
	this->buttonType = buttonType;
}

ButtonProxy::ButtonProxy()
{
	this->buttonType = static_cast<MouseButtonType>(-1);
}
