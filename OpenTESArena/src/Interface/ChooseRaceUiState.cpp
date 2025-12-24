#include "ChooseRaceUiState.h"

ChooseRaceUiState::ChooseRaceUiState()
{
	this->game = nullptr;
}

void ChooseRaceUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseRaceUI::create(Game &game)
{
	DebugNotImplemented();
}

void ChooseRaceUI::destroy()
{
	DebugNotImplemented();
}

void ChooseRaceUI::update(double dt)
{
	DebugNotImplemented();
}

void ChooseRaceUI::onBackInputAction(const InputActionCallbackValues &values)
{
	DebugNotImplemented();
}
