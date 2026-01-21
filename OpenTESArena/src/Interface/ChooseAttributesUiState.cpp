#include "ChooseAttributesUiState.h"

ChooseAttributesUiState::ChooseAttributesUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
}

void ChooseAttributesUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseAttributesUI::create(Game &game)
{
	DebugNotImplemented();
}

void ChooseAttributesUI::destroy()
{
	DebugNotImplemented();
}

void ChooseAttributesUI::update(double dt)
{
	DebugNotImplemented();
}

void ChooseAttributesUI::onBackInputAction(const InputActionCallbackValues &values)
{
	DebugNotImplemented();
}
