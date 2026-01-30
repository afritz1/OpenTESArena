#include "UiContext.h"

UiContextInitInfo::UiContextInitInfo()
{
	this->drawOrder = 0;
}

UiContext::UiContext()
{
	this->drawOrder = 0;
	this->active = false;
}
