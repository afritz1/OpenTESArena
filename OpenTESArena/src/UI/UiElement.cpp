#include "UiElement.h"

UiElement::UiElement()
{
	this->scope = static_cast<UiScope>(-1);
	this->transformInstID = -1;
	this->type = static_cast<UiElementType>(-1);
}

void UiElement::initImage(UiScope scope, UiTransformInstanceID transformInstID, UiImageInstanceID instID)
{
	this->scope = scope;
	this->transformInstID = transformInstID;
	this->type = UiElementType::Image;
	this->imageInstID = instID;
}

void UiElement::initTextBox(UiScope scope, UiTransformInstanceID transformInstID, UiTextBoxInstanceID instID)
{
	this->scope = scope;
	this->transformInstID = transformInstID;
	this->type = UiElementType::TextBox;
	this->textBoxInstID = instID;
}

void UiElement::initButton(UiScope scope, UiTransformInstanceID transformInstID, UiButtonInstanceID instID)
{
	this->scope = scope;
	this->transformInstID = transformInstID;
	this->type = UiElementType::Button;
	this->buttonInstID = instID;
}
