#include "ChooseClassCreationUiState.h"

ChooseClassCreationUiState::ChooseClassCreationUiState()
{
	this->game = nullptr;
}

void ChooseClassCreationUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseClassCreationUI::create(Game &game)
{
	DebugNotImplemented();
}

void ChooseClassCreationUI::destroy()
{
	DebugNotImplemented();
}

void ChooseClassCreationUI::update(double dt)
{
	DebugNotImplemented();
}

void ChooseClassCreationUI::onGenerateButtonSelected(MouseButtonType mouseButtonType)
{
	DebugNotImplemented();
}

void ChooseClassCreationUI::onSelectButtonSelected(MouseButtonType mouseButtonType)
{
	DebugNotImplemented();
}
