#include "UiElement.h"
#include "RenderSpace.h"

UiElement::UiElement()
{
	this->scope = static_cast<UiScope>(-1);
	this->transformInstID = -1;
	this->active = false;
	this->drawOrder = -1;
	this->renderSpace = RenderSpace::Classic;
	this->type = static_cast<UiElementType>(-1);
}

void UiElement::initImage(UiScope scope, UiTransformInstanceID transformInstID, UiImageInstanceID instID)
{
	this->scope = scope;
	this->transformInstID = transformInstID;
	this->active = true;
	this->drawOrder = 0;

	this->type = UiElementType::Image;
	this->imageInstID = instID;
}

void UiElement::initTextBox(UiScope scope, UiTransformInstanceID transformInstID, UiTextBoxInstanceID instID)
{
	this->scope = scope;
	this->transformInstID = transformInstID;
	this->active = true;
	this->drawOrder = 0;

	this->type = UiElementType::TextBox;
	this->textBoxInstID = instID;
}

void UiElement::initButton(UiScope scope, UiTransformInstanceID transformInstID, UiButtonInstanceID instID)
{
	this->scope = scope;
	this->transformInstID = transformInstID;
	this->active = true;
	this->drawOrder = 0;

	this->type = UiElementType::Button;
	this->buttonInstID = instID;
}
