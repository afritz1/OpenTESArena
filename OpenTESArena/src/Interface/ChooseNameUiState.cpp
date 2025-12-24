#include "ChooseNameUiState.h"

ChooseNameUiState::ChooseNameUiState()
{
	this->game = nullptr;
}

void ChooseNameUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseNameUI::create(Game &game)
{
	DebugNotImplemented();
}

void ChooseNameUI::destroy()
{
	DebugNotImplemented();
}

void ChooseNameUI::update(double dt)
{
	DebugNotImplemented();
}

void ChooseNameUI::onBackInputAction(const InputActionCallbackValues &values)
{
	DebugNotImplemented();
}
