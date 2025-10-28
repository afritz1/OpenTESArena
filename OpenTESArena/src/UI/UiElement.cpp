#include "PivotType.h"
#include "RenderSpace.h"
#include "UiElement.h"
#include "UiScope.h"

UiElementInitInfo::UiElementInitInfo()
{
	this->pivotType = PivotType::TopLeft;
	this->scope = UiScope::Global;
	this->drawOrder = 0;
	this->renderSpace = RenderSpace::Classic;
}

UiElement::UiElement()
{
	this->scope = static_cast<UiScope>(-1);
	this->drawOrder = -1;
	this->renderSpace = RenderSpace::Classic;
	this->transformInstID = -1;
	this->active = false;
	this->type = static_cast<UiElementType>(-1);
}

void UiElement::initImage(UiScope scope, int drawOrder, RenderSpace renderSpace, UiTransformInstanceID transformInstID, UiImageInstanceID instID)
{
	this->scope = scope;
	this->drawOrder = drawOrder;
	this->renderSpace = renderSpace;
	this->transformInstID = transformInstID;
	this->active = true;

	this->type = UiElementType::Image;
	this->imageInstID = instID;
}

void UiElement::initTextBox(UiScope scope, int drawOrder, RenderSpace renderSpace, UiTransformInstanceID transformInstID, UiTextBoxInstanceID instID)
{
	this->scope = scope;
	this->drawOrder = drawOrder;
	this->renderSpace = renderSpace;
	this->transformInstID = transformInstID;
	this->active = true;

	this->type = UiElementType::TextBox;
	this->textBoxInstID = instID;
}

void UiElement::initButton(UiScope scope, int drawOrder, RenderSpace renderSpace, UiTransformInstanceID transformInstID, UiButtonInstanceID instID)
{
	this->scope = scope;
	this->drawOrder = drawOrder;
	this->renderSpace = renderSpace;
	this->transformInstID = transformInstID;
	this->active = true;

	this->type = UiElementType::Button;
	this->buttonInstID = instID;
}
