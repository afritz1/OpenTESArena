#include <algorithm>

#include "UiContext.h"
#include "UiElement.h"
#include "UiManager.h"
#include "UiPivotType.h"
#include "UiRenderSpace.h"

UiElementInitInfo::UiElementInitInfo()
{
	this->sizeType = UiTransformSizeType::Content;
	this->pivotType = UiPivotType::TopLeft;
	this->drawOrder = 0;
	this->renderSpace = UiRenderSpace::Classic;
}

UiElement::UiElement()
{
	std::fill(std::begin(this->name), std::end(this->name), '\0');
	this->contextType = static_cast<UiContextType>(-1);
	this->drawOrder = -1;
	this->renderSpace = UiRenderSpace::Classic;
	this->transformInstID = -1;
	this->active = false;
	this->type = static_cast<UiElementType>(-1);
}

void UiElement::initImage(const char *name, UiContextType contextType, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiImageInstanceID instID)
{
	std::snprintf(this->name, sizeof(this->name), "%s", name);
	this->contextType = contextType;
	this->drawOrder = drawOrder;
	this->renderSpace = renderSpace;
	this->transformInstID = transformInstID;
	this->active = true;

	this->type = UiElementType::Image;
	this->imageInstID = instID;
}

void UiElement::initTextBox(const char *name, UiContextType contextType, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiTextBoxInstanceID instID)
{
	std::snprintf(this->name, sizeof(this->name), "%s", name);
	this->contextType = contextType;
	this->drawOrder = drawOrder;
	this->renderSpace = renderSpace;
	this->transformInstID = transformInstID;
	this->active = true;

	this->type = UiElementType::TextBox;
	this->textBoxInstID = instID;
}

void UiElement::initButton(const char *name, UiContextType contextType, int drawOrder, UiRenderSpace renderSpace, UiTransformInstanceID transformInstID, UiButtonInstanceID instID)
{
	std::snprintf(this->name, sizeof(this->name), "%s", name);
	this->contextType = contextType;
	this->drawOrder = drawOrder;
	this->renderSpace = renderSpace;
	this->transformInstID = transformInstID;
	this->active = true;

	this->type = UiElementType::Button;
	this->buttonInstID = instID;
}
