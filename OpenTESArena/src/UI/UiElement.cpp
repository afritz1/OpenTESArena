#include "PivotType.h"
#include "UiContext.h"
#include "UiElement.h"
#include "UiManager.h"
#include "UiRenderSpace.h"

UiElementInitInfo::UiElementInitInfo()
{
	this->pivotType = PivotType::TopLeft;
	this->contextType = static_cast<UiContextType>(-1);
	this->drawOrder = 0;
	this->renderSpace = UiRenderSpace::Classic;
}

UiElement::UiElement()
{
	this->contextType = static_cast<UiContextType>(-1);
	this->drawOrder = -1;
	this->renderSpace = UiRenderSpace::Classic;
	this->transformInstID = -1;
	this->active = false;
	this->type = static_cast<UiElementType>(-1);
}

void UiElement::initImage(UiContextType contextType, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiImageInstanceID instID)
{
	this->contextType = contextType;
	this->drawOrder = drawOrder;
	this->renderSpace = renderSpace;
	this->transformInstID = transformInstID;
	this->active = true;

	this->type = UiElementType::Image;
	this->imageInstID = instID;
}

void UiElement::initTextBox(UiContextType contextType, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiTextBoxInstanceID instID)
{
	this->contextType = contextType;
	this->drawOrder = drawOrder;
	this->renderSpace = renderSpace;
	this->transformInstID = transformInstID;
	this->active = true;

	this->type = UiElementType::TextBox;
	this->textBoxInstID = instID;
}

void UiElement::initButton(UiContextType contextType, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiButtonInstanceID instID)
{
	this->contextType = contextType;
	this->drawOrder = drawOrder;
	this->renderSpace = renderSpace;
	this->transformInstID = transformInstID;
	this->active = true;

	this->type = UiElementType::Button;
	this->buttonInstID = instID;
}
