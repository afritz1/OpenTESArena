#include "ChooseClassUiState.h"

ChooseClassUiState::ChooseClassUiState()
{
	this->game = nullptr;
}

void ChooseClassUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseClassUI::create(Game &game)
{
	DebugNotImplemented();
}

void ChooseClassUI::destroy()
{
	DebugNotImplemented();
}

void ChooseClassUI::update(double dt)
{
	DebugNotImplemented();
}

void ChooseClassUI::onBackInputAction(const InputActionCallbackValues &values)
{
	DebugNotImplemented();
}
