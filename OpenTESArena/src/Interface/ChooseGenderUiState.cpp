#include "ChooseGenderUiState.h"

ChooseGenderUiState::ChooseGenderUiState()
{
	this->game = nullptr;
}

void ChooseGenderUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseGenderUI::create(Game &game)
{
	DebugNotImplemented();
}

void ChooseGenderUI::destroy()
{
	DebugNotImplemented();
}

void ChooseGenderUI::update(double dt)
{
	DebugNotImplemented();
}

void ChooseGenderUI::onMaleButtonSelected(MouseButtonType mouseButtonType)
{
	DebugNotImplemented();
}

void ChooseGenderUI::onFemaleButtonSelected(MouseButtonType mouseButtonType)
{
	DebugNotImplemented();
}

void ChooseGenderUI::onBackInputAction(const InputActionCallbackValues &values)
{
	DebugNotImplemented();
}
